/*
** CrabeLoader
** File description:
** Implements the raw C functions the game Lua can call: logging, storage, window and input.
** No native touches window state directly; a window change is posted to the render thread.
** Exposes no ImGui here -- the widget bindings are src/presentation/imgui_bindings.cpp.
**
** Authors: @LucasLhomme
*/

#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

#include "shared/version.hpp"
#include "presentation/imgui_bindings.hpp"
#include "presentation/input_hook.hpp"
#include "application/loader.hpp"
#include "application/lua_runtime.hpp"
#include "infrastructure/lua_call.hpp"
#include "infrastructure/memory.hpp"
#include "infrastructure/code_cave.hpp"
#include "infrastructure/engine_free_camera.hpp"
#include "infrastructure/message_hook.hpp"
#include "infrastructure/vfs_override_manager.hpp"
#include "application/multiplayer/multiplayer_natives.hpp"
#include "presentation/render_hook.hpp"
#include "shared/logger.hpp"

namespace {

    // No native exposes window/fullscreen state to Lua; window calls must
    // happen on the render thread, not here, hence just posting a request.
    int __cdecl nativeSetWindowMode(void* L)
    {
        const char* mode = crabe::infrastructure::LuaCall::get().argToString(L, 1);
        bool borderless = mode && std::strcmp(mode, "borderless") == 0;

        crabe::presentation::RenderHook::get().requestWindowMode(borderless ? crabe::presentation::WindowMode::BorderlessWindowed : crabe::presentation::WindowMode::Windowed);
        return 0;
    }

    // Returns the active window mode as a string: borderless or windowed.
    int __cdecl nativeGetWindowMode(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        bool borderless = (crabe::presentation::RenderHook::get().getCurrentWindowMode() == crabe::presentation::WindowMode::BorderlessWindowed);
        lua.pushString(L, borderless ? "borderless" : "windowed");
        return 1;
    }

    // Crabe._setCapturedKeys(vk, ...) -> swallows those keys before the game
    // sees them. Called with no arguments, it releases every captured key.
    int __cdecl nativeSetCapturedKeys(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        std::vector<int> keys;

        int top = lua.getTop(L);
        for (int index = 1; index <= top; ++index) {
            int virtualKey = static_cast<int>(lua.argToNumber(L, index, 0.0));
            if (virtualKey > 0 && virtualKey < 256)
                keys.push_back(virtualKey);
        }

        crabe::application::Loader::get().setCapturedKeys(std::move(keys));
        return 0;
    }

    // Crabe._findGameNative(name) -> address, or nil.
    int __cdecl nativeFindGameNative(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const char* name = lua.argToString(L, 1);
        if (!name) return 0;

        uintptr_t address = crabe::memory::findRegisteredFunction(name);
        if (!address) return 0;

        lua.pushNumber(L, static_cast<double>(address));
        return 1;
    }

    // Crabe._moduleBase() -> base address of the game image
    int __cdecl nativeModuleBase(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushNumber(L, static_cast<double>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))));
        return 1;
    }

    // Crabe._engineFreeCamera(playerId, skipNoControl) -> true/false (now on/off),
    // or nil when this build does not carry the engine free camera.
    int __cdecl nativeEngineFreeCamera(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const int playerId = static_cast<int>(lua.argToNumber(L, 1, 0.0));
        const bool skipNoControl = lua.argToBoolean(L, 2);

        const std::optional<bool> active = crabe::infrastructure::EngineFreeCamera::get().toggle(playerId, skipNoControl);
        if (!lua.hasReturnSupport() || !active) return 0;

        lua.pushBoolean(L, *active);
        return 1;
    }

    // Crabe._vfsGetOverrideCount() -> number of registered file overrides.
    int __cdecl nativeVfsGetOverrideCount(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushNumber(L, static_cast<double>(crabe::infrastructure::VfsOverrideManager::get().getOverrideCount()));
        return 1;
    }

    // Crabe._vfsResolve(virtualPath) -> resolved physical path or nil.
    int __cdecl nativeVfsResolve(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const char* path = lua.argToString(L, 1);
        if (!path || !lua.hasReturnSupport()) return 0;

        std::filesystem::path resolved;
        if (crabe::infrastructure::VfsOverrideManager::get().resolve(path, resolved)) {
            lua.pushString(L, resolved.string());
            return 1;
        }
        return 0;
    }

    // Crabe._vfsGetStats() -> totalOverrides, totalResolutions, totalHits.
    int __cdecl nativeVfsGetStats(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto stats = crabe::infrastructure::VfsOverrideManager::get().getStats();
        lua.pushNumber(L, static_cast<double>(stats.totalOverrides));
        lua.pushNumber(L, static_cast<double>(stats.totalResolutions));
        lua.pushNumber(L, static_cast<double>(stats.totalHits));
        return 3;
    }

    // Crabe._vfsLastRedirected() -> path of most recently redirected asset.
    int __cdecl nativeVfsLastRedirected(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, crabe::infrastructure::VfsOverrideManager::get().getLastRedirectedFile());
        return 1;
    }

    // Crabe._inputReport() -> which XInput slots the game has polled so far.
    int __cdecl nativeInputReport(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, crabe::presentation::InputHook::get().report());
        return 1;
    }

    // Crabe._messageWatch(substring)
    int __cdecl nativeMessageWatch(void* L)
    {
        const char* substring = crabe::infrastructure::LuaCall::get().argToString(L, 1);
        if (substring) crabe::infrastructure::MessageHook::get().watch(substring);
        return 0;
    }

    // Crabe._messageReport() -> "name(a,b) | name(a,b) | ...", oldest first.
    int __cdecl nativeMessageReport(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        lua.pushString(L, crabe::infrastructure::MessageHook::get().report());
        return 1;
    }

    int __cdecl nativeMessageClear(void* L)
    {
        (void)L;
        crabe::infrastructure::MessageHook::get().clear();
        return 0;
    }

    // --- Generic Memory & Patch Primitives ---

    // Crabe._patternScan("55 8B EC ...") -> address, or nil
    int __cdecl nativePatternScan(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const char* pattern = lua.argToString(L, 1);
        if (!pattern) return 0;

        uintptr_t address = crabe::memory::patternScan(pattern);
        if (!address) return 0;

        lua.pushNumber(L, static_cast<double>(address));
        return 1;
    }

    // Crabe._patchBytes(addr, "90 90 ...") -> bool
    int __cdecl nativePatchBytes(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        auto addr = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        const char* hexStr = lua.argToString(L, 2);
        if (!addr || !hexStr) {
            if (lua.hasReturnSupport()) lua.pushBoolean(L, false);
            return 1;
        }

        std::vector<uint8_t> bytes;
        const char* p = hexStr;
        while (*p) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            char* next = nullptr;
            unsigned long b = std::strtoul(p, &next, 16);
            if (next == p) break;
            bytes.push_back(static_cast<uint8_t>(b));
            p = next;
        }

        bool ok = !bytes.empty() && crabe::infrastructure::CodeCave::patchBytes(addr, bytes.data(), bytes.size());
        if (lua.hasReturnSupport()) {
            lua.pushBoolean(L, ok);
            return 1;
        }
        return 0;
    }

    // Crabe._readFloat(addr) -> number or nil
    int __cdecl nativeReadFloat(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto addr = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        if (!addr || !crabe::memory::isReadable(addr, sizeof(float))) return 0;

        float val = *reinterpret_cast<const float*>(addr);
        lua.pushNumber(L, static_cast<double>(val));
        return 1;
    }

    // Crabe._writeFloat(addr, val) -> bool
    int __cdecl nativeWriteFloat(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        auto addr = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto val = static_cast<float>(lua.argToNumber(L, 2));
        if (!addr) {
            if (lua.hasReturnSupport()) lua.pushBoolean(L, false);
            return 1;
        }

        bool ok = crabe::infrastructure::CodeCave::patchBytes(addr, &val, sizeof(float));
        if (lua.hasReturnSupport()) {
            lua.pushBoolean(L, ok);
            return 1;
        }
        return 0;
    }

    // Crabe._readU32(addr) -> number or nil
    int __cdecl nativeReadU32(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        auto addr = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        if (!addr || !crabe::memory::isReadable(addr, sizeof(uint32_t))) return 0;

        uint32_t val = *reinterpret_cast<const uint32_t*>(addr);
        lua.pushNumber(L, static_cast<double>(val));
        return 1;
    }

    // Crabe._writeU32(addr, val) -> bool
    int __cdecl nativeWriteU32(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        auto addr = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        auto val = static_cast<uint32_t>(lua.argToNumber(L, 2));
        if (!addr) {
            if (lua.hasReturnSupport()) lua.pushBoolean(L, false);
            return 1;
        }

        bool ok = crabe::infrastructure::CodeCave::patchBytes(addr, &val, sizeof(uint32_t));
        if (lua.hasReturnSupport()) {
            lua.pushBoolean(L, ok);
            return 1;
        }
        return 0;
    }

    // Crabe._sharedBlock(name, size) -> address of a zeroed block that survives hot reloads, or nil
    int __cdecl nativeSharedBlock(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        if (!lua.hasReturnSupport()) return 0;

        const char* name = lua.argToString(L, 1);
        auto size = static_cast<size_t>(lua.argToNumber(L, 2));
        if (!name) return 0;

        uintptr_t address = crabe::infrastructure::acquireSharedBlock(name, size);
        if (!address) return 0;

        lua.pushNumber(L, static_cast<double>(address));
        return 1;
    }

    // Crabe._installCodeCave(addr, "90 90 ...", [stolenLength = 0]) -> bool
    int __cdecl nativeInstallCodeCave(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        auto addr = static_cast<uintptr_t>(lua.argToNumber(L, 1));
        const char* hexBytes = lua.argToString(L, 2);
        auto stolenLength = static_cast<size_t>(lua.argToNumber(L, 3));
        if (!addr || !hexBytes) {
            if (lua.hasReturnSupport()) lua.pushBoolean(L, false);
            return 1;
        }

        std::vector<uint8_t> body;
        const char* p = hexBytes;
        while (*p) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            char* next = nullptr;
            unsigned long b = std::strtoul(p, &next, 16);
            if (next == p) break;
            body.push_back(static_cast<uint8_t>(b));
            p = next;
        }

        if (body.empty()) {
            if (lua.hasReturnSupport()) lua.pushBoolean(L, false);
            return 1;
        }

        auto cave = std::make_unique<crabe::infrastructure::CodeCave>();
        bool ok = cave->install(addr, body, stolenLength);
        if (ok) {
            cave.release();
        }
        if (lua.hasReturnSupport()) {
            lua.pushBoolean(L, ok);
            return 1;
        }
        return 0;
    }

    // Crabe._registerLoadOverride(matchSubstring, luaSource)
    int __cdecl nativeRegisterLoadOverride(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const char* matchSubstring = lua.argToString(L, 1);
        const char* luaSource = lua.argToString(L, 2);
        if (!matchSubstring || !luaSource) return 0;

        crabe::application::Loader::get().registerLoadOverride(matchSubstring, luaSource);
        return 0;
    }

    // Crabe._clearLoadOverrides()
    int __cdecl nativeClearLoadOverrides(void* L)
    {
        (void)L;
        crabe::application::Loader::get().clearLoadOverrides();
        return 0;
    }

    // Crabe._registerChunkPatch(matchSubstring, luaSource)
    int __cdecl nativeRegisterChunkPatch(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const char* match = lua.argToString(L, 1);
        const char* source = lua.argToString(L, 2);
        if (match && source) {
            crabe::application::Loader::get().registerChunkPatch(match, source);
        }
        return 0;
    }

    // Crabe._registerNamedPatch(exactChunkName, luaSource)
    int __cdecl nativeRegisterNamedPatch(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const char* name = lua.argToString(L, 1);
        const char* source = lua.argToString(L, 2);
        if (name && source) {
            crabe::application::Loader::get().registerNamedPatch(name, source);
        }
        return 0;
    }

    // Saves string content to <GameRoot>/storage/<relPath>.
    int __cdecl nativeStorageSave(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const char* relPath = lua.argToString(L, 1);
        const char* content = lua.argToString(L, 2);
        if (!relPath) {
            lua.pushBoolean(L, false);
            return 1;
        }

        std::filesystem::path storageDir = std::filesystem::current_path() / "storage";
        std::filesystem::path fullPath = storageDir / relPath;
        std::error_code ec;
        std::filesystem::create_directories(fullPath.parent_path(), ec);

        std::ofstream stream(fullPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!stream.is_open()) {
            lua.pushBoolean(L, false);
            return 1;
        }

        if (content) stream << content;
        stream.close();
        lua.pushBoolean(L, true);
        return 1;
    }

    // Reads string content from <GameRoot>/storage/<relPath>.
    int __cdecl nativeStorageLoad(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const char* relPath = lua.argToString(L, 1);
        if (!relPath) {
            lua.pushNil(L);
            return 1;
        }

        std::filesystem::path storageDir = std::filesystem::current_path() / "storage";
        std::filesystem::path fullPath = storageDir / relPath;
        if (!std::filesystem::exists(fullPath)) {
            lua.pushNil(L);
            return 1;
        }

        std::ifstream stream(fullPath, std::ios::in | std::ios::binary);
        if (!stream.is_open()) {
            lua.pushNil(L);
            return 1;
        }

        std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        lua.pushString(L, content);
        return 1;
    }

    // Appends a log entry to <GameRoot>/logs/mods/<modName>.log.
    int __cdecl nativeFileLog(void* L)
    {
        crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
        const char* modName = lua.argToString(L, 1);
        const char* level = lua.argToString(L, 2);
        const char* message = lua.argToString(L, 3);
        if (!modName || !message) return 0;

        std::filesystem::path logDir = std::filesystem::current_path() / "logs" / "mods";
        std::error_code ec;
        std::filesystem::create_directories(logDir, ec);

        std::filesystem::path logFile = logDir / (std::string(modName) + ".log");
        std::ofstream stream(logFile, std::ios::app);
        if (stream.is_open()) {
            stream << std::format("[{}] [{}] {}\n",
                                  crabe::shared::Logger::getInstance().getCurrentTime(),
                                  level ? level : "INFO",
                                  message);
        }
        return 0;
    }

} // namespace

bool crabe::lua_runtime::registerNatives(void* L)
{
    struct Entry {
        const char* name;
        crabe::infrastructure::LuaCall::t_lua_cfunction fn;
    };

    static constexpr Entry kNatives[] = {
        { "_setWindowModeNative",   &nativeSetWindowMode },
        { "_getWindowModeNative",   &nativeGetWindowMode },
        { "_findGameNative",        &nativeFindGameNative },
        { "_moduleBase",            &nativeModuleBase },
        { "_engineFreeCamera",      &nativeEngineFreeCamera },
        { "_inputReport",           &nativeInputReport },
        { "_keyDown",               &crabe::input_natives::keyDown },
        { "_setCapturedKeys",       &nativeSetCapturedKeys },
        { "_messageWatch",          &nativeMessageWatch },
        { "_messageReport",         &nativeMessageReport },
        { "_messageClear",          &nativeMessageClear },
        { "_patternScan",           &nativePatternScan },
        { "_patchBytes",            &nativePatchBytes },
        { "_readFloat",             &nativeReadFloat },
        { "_writeFloat",            &nativeWriteFloat },
        { "_readU32",               &nativeReadU32 },
        { "_writeU32",              &nativeWriteU32 },
        { "_installCodeCave",       &nativeInstallCodeCave },
        { "_sharedBlock",           &nativeSharedBlock },
        { "_registerLoadOverride",  &nativeRegisterLoadOverride },
        { "_clearLoadOverrides",    &nativeClearLoadOverrides },
        { "_registerChunkPatch",    &nativeRegisterChunkPatch },
        { "_registerNamedPatch",    &nativeRegisterNamedPatch },
        { "_storageSave",           &nativeStorageSave },
        { "_storageLoad",           &nativeStorageLoad },
        { "_fileLog",               &nativeFileLog },
        { "_vfsGetOverrideCount",   &nativeVfsGetOverrideCount },
        { "_vfsResolve",            &nativeVfsResolve },
        { "_vfsGetStats",           &nativeVfsGetStats },
        { "_vfsLastRedirected",     &nativeVfsLastRedirected },
    };

    bool allOk = crabe::multiplayer::natives::registerAll(L);
    crabe::presentation::ImGuiBindings::registerBindings(L);

    crabe::infrastructure::LuaCall::get().runSnippet(L, std::format(
        "Crabe = Crabe or {{}}; Crabe.version = '{}'; Crabe.versionMajor = {}; "
        "Crabe.versionMinor = {}; Crabe.versionPatch = {};",
        crabe::version::String, crabe::version::Major, crabe::version::Minor, crabe::version::Patch));

    for (const auto& entry : kNatives) {
        if (crabe::infrastructure::LuaCall::get().registerNativeFunction(L, "Crabe", entry.name, entry.fn)) continue;

        crabe::shared::Logger::getInstance().error("LuaRuntime: failed to register Crabe.{}.", entry.name);
        allOk = false;
    }

    return allOk;
}
