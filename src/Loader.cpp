/*
** CrabeLoader
** File description:
** Loader
*/

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <windows.h>

#include "loader/loader.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "loader/render_hook.hpp"
#include "logger/logger.hpp"

namespace {

    // The game exports no Lua symbol, and byte signatures proved unreliable on
    // this build. What does work is the standard library's own registration
    // table: each stdlib name is bound to a small wrapper, which delegates to the
    // C API function we actually want. So: find the wrapper by name, then follow
    // the n-th `call` inside it.
    //
    // Call indexes were read off the shipped binary; resolveLuaFunction() logs the
    // resulting address and its first bytes so a mismatch after a game update is
    // visible in the log instead of crashing the process.
    struct LuaSymbol {
        const char* name;        // what we are resolving, for the log
        const char* stdlibName;  // Lua stdlib entry holding the wrapper
        int callIndex;           // 1-based `call` inside the wrapper
        size_t scanBytes = 256;  // how far into the wrapper to look
    };

    constexpr LuaSymbol kLoadfile   { "luaL_loadfile",   "loadfile",   2 };
    constexpr LuaSymbol kLoadbuffer { "luaL_loadbuffer", "loadstring", 3 };
    constexpr LuaSymbol kPcall      { "lua_pcall",       "xpcall",     4 };
    constexpr LuaSymbol kGetfield   { "lua_getfield",    "print",      2 };
    constexpr LuaSymbol kTolstring  { "lua_tolstring",   "print",      6,  512 };
    constexpr LuaSymbol kSettop     { "lua_settop",      "print",      11, 512 };

    std::string firstBytes(uintptr_t addr, size_t count)
    {
        if (!Memory::isReadable(addr, count)) return "<unreadable>";

        const auto* code = reinterpret_cast<const uint8_t*>(addr);
        std::string out;

        for (size_t i = 0; i < count; ++i) {
            if (i) out += ' ';
            out += std::format("{:02X}", code[i]);
        }
        return out;
    }

    uintptr_t resolveLuaFunction(const LuaSymbol& symbol, uintptr_t base)
    {
        Logger& logger = Logger::getInstance();

        uintptr_t wrapper = Memory::findRegisteredFunction(symbol.stdlibName);
        if (!wrapper) {
            logger.error("Loader: {}: no '{}' entry in the Lua stdlib table.",
                        symbol.name, symbol.stdlibName);
            return 0;
        }

        uintptr_t addr = Memory::findNthCall(wrapper, symbol.callIndex, symbol.scanBytes);
        if (!addr) {
            logger.error("Loader: {}: call #{} not found in the '{}' wrapper (0x{:X}).",
                        symbol.name, symbol.callIndex, symbol.stdlibName, wrapper);
            return 0;
        }

        logger.info("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                    symbol.name, addr, addr - base, firstBytes(addr, 8));
        return addr;
    }

    bool isExtendedKey(int virtualKey)
    {
        switch (virtualKey) {
            case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
            case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
            case VK_INSERT: case VK_DELETE:
            case VK_DIVIDE: case VK_NUMLOCK:
                return true;
            default:
                return false;
        }
    }

    std::string virtualKeyName(int virtualKey)
    {
        LONG scanCode = MapVirtualKeyA(static_cast<UINT>(virtualKey), MAPVK_VK_TO_VSC);
        LONG lParam = scanCode << 16;
        if (isExtendedKey(virtualKey))
            lParam |= 1 << 24;

        char buffer[64];
        int len = GetKeyNameTextA(lParam, buffer, sizeof(buffer));
        if (len <= 0)
            return std::format("VK 0x{:X}", virtualKey);
        return std::string(buffer, len);
    }

} // namespace

Loader& Loader::get()
{
    static Loader instance;
    return instance;
}

void Loader::onLuaState(void *L)
{
    if (!L) {
        Logger::getInstance().info("Loader: Lua state UNINJECTED.");
        return;
    }

    // loadbuffer fires thousands of times per second during startup: bail out
    // before the mutex/log once mods are already loaded, or every single Lua
    // chunk load pays for a lock + a flushed log write.
    if (_modsLoaded)
        return;

    std::lock_guard<std::mutex> lock(_stateMutex);
    if (_modsLoaded)
        return; // another thread handled it while we waited for the lock

    _luaState = L;
    _modsLoaded = true;
    Logger::getInstance().info("Loader: Lua state INJECTED.");

    onLoadmods();
}

bool Loader::isInjected()
{
    return _luaState != nullptr;
}

void Loader::onLoadmods()
{
    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";

    if (!Loader::get().isInjected()) {
        Logger::getInstance().info("Loader: Lua state not injected, skipping mod loading.");
        return;
    } else {
        Logger::getInstance().info("Loader: Reading mods folder...");
        if (!std::filesystem::exists(modsFolder)) {
            Logger::getInstance().info("Loader: Mods folder does not exist, creating...");
            std::filesystem::create_directory(modsFolder);
        } else {
            // here is the part where we iterate over the mods folder and execute any .lua files found
            for (const auto& entry : std::filesystem::directory_iterator(modsFolder)) {
                if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                    std::string filename = entry.path().filename().string();
                    Logger::getInstance().info("Loader: Found mod: {}", filename);

                    if (LuaCall::get().runFile(_luaState, entry.path().string().c_str())) {
                        Logger::getInstance().info("Loader: mod '{}' executed.", filename);
                    } else {
                        Logger::getInstance().warning("Loader: mod '{}' failed to execute.", filename);
                    }
                }
            }
        }
    }
}

void Loader::registerKeybind(int virtualKey, std::function<void()> onPress)
{
    std::lock_guard<std::mutex> lock(_keybindsMutex);
    _keybinds[virtualKey] = Keybind{ std::move(onPress), false };
    Logger::getInstance().info("Loader: registered keybind for virtual key 0x{:X}.", virtualKey);
}

void Loader::registerLuaKeybind(int virtualKey, const std::string& luaFunctionName)
{
    registerKeybind(virtualKey, [this, luaFunctionName]() {
        queueLuaCall(luaFunctionName);
    });
}

void Loader::queueLuaCall(const std::string& luaFunctionName)
{
    std::lock_guard<std::mutex> lock(_luaCallQueueMutex);
    _pendingLuaCalls.push_back(luaFunctionName);
}

void Loader::drainPendingKeybindCalls(void* L)
{
    std::vector<std::string> pending;
    {
        std::lock_guard<std::mutex> lock(_luaCallQueueMutex);
        if (_pendingLuaCalls.empty())
            return;
        pending.swap(_pendingLuaCalls);
    }

    for (const auto& luaFunctionName : pending) {
        if (!LuaCall::get().runGlobalIfExists(L, luaFunctionName)) {
            Logger::getInstance().warning("Loader: keybind call '{}()' raised a Lua error.", luaFunctionName);
        }
    }
}

void Loader::queueConsoleSnippet(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_snippetQueueMutex);
    _pendingSnippets.push_back(code);
}

void Loader::drainPendingSnippets(void* L)
{
    std::vector<std::string> pending;
    {
        std::lock_guard<std::mutex> lock(_snippetQueueMutex);
        if (_pendingSnippets.empty())
            return;
        pending.swap(_pendingSnippets);
    }

    for (const auto& code : pending) {
        std::string result;
        bool ok = LuaCall::get().runSnippet(L, code, result);

        if (!ok) {
            Logger::getInstance().error("! {}", result);
        } else if (!result.empty()) {
            Logger::getInstance().info("= {}", result);
        }
    }
}

void Loader::registerDefaultKeybinds()
{
    static constexpr std::pair<int, const char*> kLuaKeybinds[] = {
        { VK_F1, "OnKeyF1" }, { VK_F2, "OnKeyF2" }, { VK_F3, "OnKeyF3" }, { VK_F4, "OnKeyF4" }, { VK_F5, "OnKeyF5" },
        { VK_F6, "OnKeyF6" }, { VK_F7, "OnKeyF7" }, { VK_F8, "OnKeyF8" }, { VK_F9, "OnKeyF9" },
        { VK_F10, "OnKeyF10" }, { VK_F11, "OnKeyF11" }, { VK_F12, "OnKeyF12" },
    };
    for (const auto& [virtualKey, luaFunctionName] : kLuaKeybinds) {
        registerLuaKeybind(virtualKey, luaFunctionName);
    }

    registerKeybind(VK_INSERT, []() {
        RenderHook::get().toggleMenu();
    });
}

void Loader::handleKeybind()
{
    std::lock_guard<std::mutex> lock(_keybindsMutex);
    for (auto& [virtualKey, bind] : _keybinds) {
        bool isDown = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
        if (isDown && !bind.wasDown) {
            Logger::getInstance().debug("Loader: keybind pressed: {} (virtual key 0x{:X}).",
                                        virtualKeyName(virtualKey), virtualKey);
            bind.onPress();
        }
        bind.wasDown = isDown;
    }
}

void Loader::inputLoop()
{
    while (true) {
        handleKeybind();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

bool Loader::initialize()
{
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    LuaApiAddresses addresses;
    addresses.loadfile = resolveLuaFunction(kLoadfile, base);
    addresses.loadbuffer = resolveLuaFunction(kLoadbuffer, base);
    addresses.pcall = resolveLuaFunction(kPcall, base);
    addresses.getfield = resolveLuaFunction(kGetfield, base);
    addresses.tolstring = resolveLuaFunction(kTolstring, base);
    addresses.settop = resolveLuaFunction(kSettop, base);

    if (!LuaCall::get().initialize(addresses)) {
        Logger::getInstance().error("Loader: failed to initialize LuaCall.");
        return false;
    }

    Logger::getInstance().info("Loader: initialized.");
    onLuaState(nullptr);
    onLoadmods();
    registerDefaultKeybinds();

    if (!RenderHook::get().initialize()) {
        Logger::getInstance().warning("Loader: failed to initialize the render hook (overlay disabled).");
    }

    std::thread(&Loader::inputLoop, this).detach();
    return true;
}

void Loader::uninitialize()
{
    RenderHook::get().uninitialize();
    LuaCall::get().uninitialize();
}
