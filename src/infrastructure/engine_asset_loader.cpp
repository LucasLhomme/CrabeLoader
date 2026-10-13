/*
** CrabeLoader
** File description:
** Serves loose mod textures to the engine's own file loader, below the Win32 file calls.
** Substitutes a loose .tbody for the archive member the engine would otherwise read.
** Doubles as a measurement probe while the marker file crabe_probe_asset_loader.txt exists.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_asset_loader.hpp"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

#include "domain/game_profile.hpp"
#include "infrastructure/vfs_hook.hpp"
#include "infrastructure/vfs_override_manager.hpp"
#include "shared/logger.hpp"

namespace {

constexpr const char* kProbeMarker = "crabe_probe_asset_loader.txt";
constexpr const char* kDisableMarker = "crabe_disable_asset_override.txt";
constexpr const char* kLoadFileSymbol = "Asset_LoadFile";
constexpr const char* kAllocSymbol = "Engine_Alloc";
constexpr const char* kOverridableExtension = ".tbody";
constexpr std::uint32_t kMaxTextureLines = 4000;
constexpr std::uint32_t kMaxOtherLines = 300;
constexpr std::uint32_t kMaxServedLines = 500;
constexpr std::size_t kPathCapacity = 260;

struct CallSample {
    char path[kPathCapacity];
    std::uint32_t size;
};

using LoadFileFn = void*(__cdecl*)(const char*, std::uint32_t*,
                                   std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
using AllocFn = void*(__cdecl*)(std::uint32_t);

// Copies the requested path and the size out-parameter, tolerating an unreadable pointer.
bool readCallSample(const char* path, const std::uint32_t* sizeOut, CallSample& out) noexcept
{
    if (path == nullptr)
        return false;

    __try {
        std::size_t i = 0;
        while (i + 1 < kPathCapacity && path[i] != '\0') {
            out.path[i] = path[i];
            ++i;
        }
        out.path[i] = '\0';
        out.size = sizeOut != nullptr ? *sizeOut : 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Reads the byte just past the returned buffer, tolerating an unreadable page.
int readByteAfter(const void* buffer, std::uint32_t size) noexcept
{
    if (buffer == nullptr)
        return -1;

    __try {
        return static_cast<int>(static_cast<const std::uint8_t*>(buffer)[size]);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -2;
    }
}

// Allocates through the engine allocator and copies the bytes; null when either step faults.
void* allocateAndCopy(std::uintptr_t allocator, const std::uint8_t* data, std::size_t size) noexcept
{
    __try {
        void* buffer = reinterpret_cast<AllocFn>(allocator)(static_cast<std::uint32_t>(size));
        if (buffer == nullptr)
            return nullptr;
        std::memcpy(buffer, data, size);
        return buffer;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool writeSize(std::uint32_t* sizeOut, std::size_t size) noexcept
{
    if (sizeOut == nullptr)
        return true;

    __try {
        *sizeOut = static_cast<std::uint32_t>(size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool hasOverridableExtension(const char* path) noexcept
{
    const std::size_t length = std::strlen(path);
    const std::size_t extension = std::strlen(kOverridableExtension);
    return length > extension && _stricmp(path + length - extension, kOverridableExtension) == 0;
}

bool isTexturePath(const std::string& lowered)
{
    return lowered.find(".tbody") != std::string::npos
        || lowered.find("textures/") != std::string::npos;
}

} // namespace

namespace crabe::infrastructure {

EngineAssetLoader& EngineAssetLoader::get()
{
    static EngineAssetLoader instance;
    return instance;
}

void EngineAssetLoader::initialize()
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    if (_loadFileHook.isInstalled())
        return;

    std::error_code ec;
    const bool probing = std::filesystem::exists(kProbeMarker, ec);
    bool overrideEnabled = !std::filesystem::exists(kDisableMarker, ec);
    if (!probing && !overrideEnabled) {
        logger.info("EngineAssetLoader: loose texture override disabled by '{}'.", kDisableMarker);
        return;
    }

    const HMODULE hGame = GetModuleHandleA(nullptr);
    const auto* profile = crabe::domain::activeProfile();
    if (hGame == nullptr || profile == nullptr) {
        logger.warning("EngineAssetLoader: no game profile is active; loose textures stay unreachable.");
        return;
    }

    const std::uint32_t rvaLoad = profile->engineSymbolRva(kLoadFileSymbol);
    if (rvaLoad == crabe::domain::kUnmeasured) {
        logger.warning("EngineAssetLoader: profile '{}' has no '{}' symbol; hook not installed.",
                       profile->id, kLoadFileSymbol);
        return;
    }

    const std::uint32_t rvaAlloc = profile->engineSymbolRva(kAllocSymbol);
    if (overrideEnabled && rvaAlloc == crabe::domain::kUnmeasured) {
        logger.warning("EngineAssetLoader: profile '{}' has no '{}' symbol; override disabled.",
                       profile->id, kAllocSymbol);
        overrideEnabled = false;
        if (!probing)
            return;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(hGame);
    const std::uintptr_t target = base + rvaLoad;
    _allocator = rvaAlloc == crabe::domain::kUnmeasured ? 0 : base + rvaAlloc;
    _overrideEnabled.store(overrideEnabled, std::memory_order_relaxed);
    _probing.store(probing, std::memory_order_relaxed);

    if (!_loadFileHook.install(reinterpret_cast<void*>(target),
                               reinterpret_cast<void*>(&EngineAssetLoader::hkLoadFile),
                               "EngineAssetLoader::LoadFile")) {
        _overrideEnabled.store(false, std::memory_order_relaxed);
        _probing.store(false, std::memory_order_relaxed);
        logger.warning("EngineAssetLoader: failed to install the file loader hook at 0x{:X}.", target);
        return;
    }

    if (probing)
        VfsHook::setTraceTextures(true);

    logger.info("EngineAssetLoader: hook installed at 0x{:X} (loose texture override {}, probe {}).",
                target, overrideEnabled ? "on" : "off", probing ? "on" : "off");
}

void EngineAssetLoader::uninitialize()
{
    if (!_loadFileHook.isInstalled())
        return;

    _overrideEnabled.store(false, std::memory_order_relaxed);
    VfsHook::setTraceTextures(false);
    _loadFileHook.remove();
    crabe::shared::Logger::getInstance().info(
        "EngineAssetLoader: hook removed; served={} serveFailures={} (probe: calls={} textureCalls={} textureMisses={}).",
        _served.load(), _serveFailures.load(), _totalCalls.load(), _textureCalls.load(), _textureMisses.load());
}

bool EngineAssetLoader::isHooked() const noexcept
{
    return _loadFileHook.isInstalled();
}

std::uint32_t EngineAssetLoader::servedCount() const noexcept
{
    return _served.load(std::memory_order_relaxed);
}

void* __cdecl EngineAssetLoader::hkLoadFile(const char* path, std::uint32_t* sizeOut,
                                            std::uintptr_t flag0, std::uintptr_t flag1,
                                            std::uintptr_t flag2, std::uintptr_t flag3)
{
    EngineAssetLoader& self = get();
    auto original = reinterpret_cast<LoadFileFn>(self._loadFileHook.getOriginal());
    if (original == nullptr)
        return nullptr;

    const bool probing = self._probing.load(std::memory_order_relaxed);

    if (self._overrideEnabled.load(std::memory_order_relaxed)) {
        void* served = self.serveOverride(path, sizeOut);
        if (served != nullptr) {
            if (probing)
                self.record(path, sizeOut, served, true, flag0, flag1, flag2, flag3);
            return served;
        }
    }

    void* result = original(path, sizeOut, flag0, flag1, flag2, flag3);
    if (probing)
        self.record(path, sizeOut, result, false, flag0, flag1, flag2, flag3);
    return result;
}

void* EngineAssetLoader::serveOverride(const char* path, std::uint32_t* sizeOut) noexcept
{
    CallSample sample{};
    if (!readCallSample(path, nullptr, sample) || !hasOverridableExtension(sample.path))
        return nullptr;

    const auto bytes = VfsOverrideManager::get().readOverrideBytes(sample.path);
    if (!bytes.has_value())
        return nullptr;

    void* buffer = allocateAndCopy(_allocator, bytes->data(), bytes->size());
    if (buffer == nullptr || !writeSize(sizeOut, bytes->size())) {
        _serveFailures.fetch_add(1, std::memory_order_relaxed);
        crabe::shared::Logger::getInstance().warning(
            "EngineAssetLoader: could not serve loose override for '{}' ({} bytes); falling back to the archive.",
            sample.path, bytes->size());
        return nullptr;
    }

    _served.fetch_add(1, std::memory_order_relaxed);
    if (_servedLogged.fetch_add(1, std::memory_order_relaxed) < kMaxServedLines) {
        crabe::shared::Logger::getInstance().info(
            "EngineAssetLoader: served loose override '{}' ({} bytes).", sample.path, bytes->size());
    }
    return buffer;
}

void EngineAssetLoader::record(const char* path, const std::uint32_t* sizeOut, const void* result,
                               bool overridden,
                               std::uintptr_t flag0, std::uintptr_t flag1,
                               std::uintptr_t flag2, std::uintptr_t flag3) noexcept
{
    CallSample sample{};
    if (!readCallSample(path, sizeOut, sample))
        return;

    _totalCalls.fetch_add(1, std::memory_order_relaxed);

    std::string lowered(sample.path);
    std::ranges::transform(lowered, lowered.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const bool texture = isTexturePath(lowered);
    if (texture) {
        _textureCalls.fetch_add(1, std::memory_order_relaxed);
        if (result == nullptr)
            _textureMisses.fetch_add(1, std::memory_order_relaxed);
    }

    std::atomic<std::uint32_t>& counter = texture ? _textureLogged : _otherLogged;
    const std::uint32_t limit = texture ? kMaxTextureLines : kMaxOtherLines;
    if (counter.fetch_add(1, std::memory_order_relaxed) >= limit)
        return;

    std::filesystem::path indexed;
    const bool vfsIndexed = VfsOverrideManager::get().resolve(lowered, indexed);

    crabe::shared::Logger::getInstance().info(
        "EngineAssetLoader: thread={} {} path='{}' result={} size={} vfsIndexed={} overridden={} "
        "flags={:X},{:X},{:X},{:X} after={}",
        GetCurrentThreadId(), texture ? "TEX" : "OTH", sample.path,
        result != nullptr ? "buffer" : "null", sample.size, vfsIndexed, overridden,
        flag0, flag1, flag2, flag3, readByteAfter(result, sample.size));
}

} // namespace crabe::infrastructure
