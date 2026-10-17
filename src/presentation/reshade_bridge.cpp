/*
** CrabeLoader
** File description:
** Hosts optional ReShade effects on the game's existing D3D11 swap chain.
** Manual runtime calls preserve native COM identities to avoid proxy-related crashes.
** Add-on initialization belongs to the companion runtime built by tools/reshade/build.ps1.
**
** Authors: @lightningcal
*/
#include "presentation/reshade_bridge.hpp"
#include <d3d11.h>
#include <psapi.h>
#include <filesystem>
#include <mutex>
#include <string>
#include "infrastructure/crash_handler.hpp"
#include "shared/logger.hpp"

namespace crabe::presentation {
namespace {
    // ReShade 6.x C exports: cdecl; device_api::d3d11 = 0xb000.
    // The opaque effect_runtime pointer is never dereferenced by this host.
    using CreateRuntime = bool(__cdecl*)(int, void*, void*, void*, const char*, void**);
    using DestroyRuntime = void(__cdecl*)(void*);
    using PresentRuntime = void(__cdecl*)(void*);
    std::recursive_mutex bridgeMutex;
    bool checked = false;
    bool failed = false;
    HMODULE module = nullptr; // Kept loaded: input hooks may still be executing.
    CreateRuntime createRuntime = nullptr;
    DestroyRuntime destroyRuntime = nullptr;
    PresentRuntime presentRuntime = nullptr;
    void* runtime = nullptr;
    IDXGISwapChain* owner = nullptr; // Borrowed. Reset before resizing.
    std::filesystem::path gameRoot;

    // Changes only this process during LoadLibrary, outside DllMain; restores
    // the previous values immediately afterward. No persistent environment edits.
    class EnvironmentOverride {
    public:
        EnvironmentOverride(const wchar_t* name, const wchar_t* value) : name_(name) {
            const DWORD size = GetEnvironmentVariableW(name_, nullptr, 0);
            if (size) {
                old_.resize(size);
                old_.resize(GetEnvironmentVariableW(name_, old_.data(), size));
                existed_ = true;
            }
            ok_ = SetEnvironmentVariableW(name_, value) != FALSE;
        }
        ~EnvironmentOverride() { if (ok_) SetEnvironmentVariableW(name_, existed_ ? old_.c_str() : nullptr); }
        bool ok() const { return ok_; }
    private:
        const wchar_t* name_;
        std::wstring old_;
        bool existed_ = false;
        bool ok_ = false;
    };

    bool guarded(const std::function<void()>& action, const char* label) {
        const bool ok = crabe::infrastructure::CrashHandler::runGuarded(action, label);
        if (!ok) failed = true;
        return ok;
    }

    void load() {
        checked = true;
        wchar_t executable[32768]{};
        const DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
        if (!length || length >= 32768) return;
        gameRoot = std::filesystem::path(executable).parent_path();
        const auto path = gameRoot / L"Crabe" / L"ReShade32.dll";
        std::error_code ec;
        if (!std::filesystem::is_regular_file(path, ec)) return;
        auto& logger = crabe::shared::Logger::getInstance();
        // Never mix manual and automatically injected ReShade runtimes.
        HMODULE modules[1024]{};
        DWORD needed = 0;
        if (!K32EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed)
            || needed > sizeof(modules)) {
            logger.error("ReShadeBridge: cannot inspect modules; manual runtime disabled.");
            failed = true;
            return;
        }
        for (DWORD i = 0; i < needed / sizeof(HMODULE); ++i) {
            if (GetProcAddress(modules[i], "ReShadeVersion")) {
                logger.error("ReShadeBridge: ReShade already loaded; remove its root graphics proxy before using Crabe/ReShade32.dll.");
                failed = true;
                return;
            }
        }
        EnvironmentOverride disableGraphics(L"RESHADE_DISABLE_GRAPHICS_HOOK", L"1");
        EnvironmentOverride basePath(L"RESHADE_BASE_PATH_OVERRIDE", gameRoot.c_str());
        if (!disableGraphics.ok() || !basePath.ok()) {
            logger.error("ReShadeBridge: temporary environment setup failed; manual runtime disabled.");
            failed = true;
            return;
        }
        if (!guarded([&] { module = LoadLibraryExW(path.c_str(), nullptr,
                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS); }, "ReShadeBridge::load")) return;
        if (!module) {
            logger.error("ReShadeBridge: cannot load the 32-bit DLL (Windows error {}).", GetLastError());
            failed = true;
            return;
        }
        createRuntime = reinterpret_cast<CreateRuntime>(GetProcAddress(module, "ReShadeCreateEffectRuntime"));
        destroyRuntime = reinterpret_cast<DestroyRuntime>(GetProcAddress(module, "ReShadeDestroyEffectRuntime"));
        presentRuntime = reinterpret_cast<PresentRuntime>(GetProcAddress(module, "ReShadeUpdateAndPresentEffectRuntime"));
        if (!createRuntime || !destroyRuntime || !presentRuntime) {
            logger.error("ReShadeBridge: DLL lacks the ReShade 6.x manual-runtime API; bridge disabled.");
            failed = true;
            return;
        }
        using ManualAddonSupport = unsigned int(__cdecl*)();
        const auto addonSupport = reinterpret_cast<ManualAddonSupport>(
            GetProcAddress(module, "CrabeReShadeManualAddonSupport"));
        unsigned int addonVersion = 0;
        if (addonSupport)
            guarded([&] { addonVersion = addonSupport(); }, "ReShadeBridge::addonSupport");
        if (addonVersion >= 1)
            logger.info("ReShadeBridge: manual add-on lifecycle supported; built-in and external add-ons will initialize with the runtime.");
        else
            logger.warning("ReShadeBridge: stock manual runtime does not initialize add-ons. Build the companion DLL with tools/reshade/build.ps1 for Generic Depth and .addon32 loading.");
        if (addonVersion >= 2)
            logger.info("ReShadeBridge: native game depth event capture supported.");
        logger.info("ReShadeBridge: loaded Crabe/ReShade32.dll with automatic graphics hooks disabled.");
    }

    void reset() {
        void* previous = runtime;
        runtime = nullptr;
        owner = nullptr;
        if (previous && !failed)
            guarded([&] { destroyRuntime(previous); }, "ReShadeBridge::destroy");
    }
}

void ReShadeBridge::present(IDXGISwapChain* swapChain, UINT flags) {
    if (!swapChain || (flags & DXGI_PRESENT_TEST)) return;
    const std::lock_guard lock(bridgeMutex);
    if (failed) return;
    if (!checked) load();
    if (!module || failed) return;
    if (runtime && owner != swapChain) return;
    if (!runtime) {
        ID3D11Device* device = nullptr;
        if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)))) return;
        ID3D11DeviceContext* context = nullptr;
        device->GetImmediateContext(&context);
        const auto configUtf8 = (gameRoot / L"ReShade.ini").u8string();
        bool created = false;
        guarded([&] { created = createRuntime(0xb000, device, context, swapChain,
                    reinterpret_cast<const char*>(configUtf8.c_str()), &runtime); }, "ReShadeBridge::create");
        if (context) context->Release();
        device->Release();
        if (!created || !runtime || failed) {
            crabe::shared::Logger::getInstance().error("ReShadeBridge: manual runtime creation failed; bridge disabled for this session.");
            failed = true;
            return;
        }
        owner = swapChain;
        crabe::shared::Logger::getInstance().info("ReShadeBridge: manual D3D11 effect runtime created on the game's existing swap chain.");
    }
    guarded([&] { presentRuntime(runtime); }, "ReShadeBridge::present");
}

void ReShadeBridge::beforeResize(IDXGISwapChain* swapChain) {
    const std::lock_guard lock(bridgeMutex);
    if (owner == swapChain) reset();
}
void ReShadeBridge::shutdown() {
    const std::lock_guard lock(bridgeMutex);
    reset();
}
}
