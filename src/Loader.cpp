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
    };

    constexpr LuaSymbol kLoadfile   { "luaL_loadfile",   "loadfile",   2 };
    constexpr LuaSymbol kLoadbuffer { "luaL_loadbuffer", "loadstring", 3 };
    constexpr LuaSymbol kPcall      { "lua_pcall",       "xpcall",     4 };

    std::string firstBytes(uintptr_t addr, size_t count)
    {
        if (!Memory::IsReadable(addr, count)) return "<unreadable>";

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

        uintptr_t wrapper = Memory::FindRegisteredFunction(symbol.stdlibName);
        if (!wrapper) {
            logger.error("Loader: {}: no '{}' entry in the Lua stdlib table.",
                        symbol.name, symbol.stdlibName);
            return 0;
        }

        uintptr_t addr = Memory::FindNthCall(wrapper, symbol.callIndex);
        if (!addr) {
            logger.error("Loader: {}: call #{} not found in the '{}' wrapper (0x{:X}).",
                        symbol.name, symbol.callIndex, symbol.stdlibName, wrapper);
            return 0;
        }

        logger.info("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                    symbol.name, addr, addr - base, firstBytes(addr, 8));
        return addr;
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

    std::lock_guard<std::mutex> lock(_StateMutex);
    if (_modsLoaded)
        return; // another thread handled it while we waited for the lock

    _luaState = L;
    _modsLoaded = true;
    Logger::getInstance().info("Loader: Lua state INJECTED.");

    onLoadmods();
}

bool Loader::_isInjected()
{
    return _luaState != nullptr;
}

void Loader::onLoadmods()
{
    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";

    if (!Loader::get()._isInjected()) {
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

void Loader::HandleKeybind()
{
    // Edge-detected: GetAsyncKeyState is polled every tick, so without this
    // the toggle would flip back and forth for as long as the key stays down.
    static bool f5WasDown = false;

    char BUFFER[256];
    auto writesignal = [&](const char* signal) {
        std::snprintf(BUFFER, sizeof(BUFFER), "Loader: keybind signal '%s'.", signal);
        Logger::getInstance().info("{}", BUFFER);
    };

    bool f5IsDown = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
    if (f5IsDown && !f5WasDown) {
        _menuOpen = !_menuOpen;
        writesignal(_menuOpen ? "menu_open" : "menu_close");
    }
    f5WasDown = f5IsDown;
}

void Loader::inputLoop()
{
    while (true) {
        HandleKeybind();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 Hz
    }
}

bool Loader::initialize()
{
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    uintptr_t loadfile = resolveLuaFunction(kLoadfile, base);
    uintptr_t loadbuffer = resolveLuaFunction(kLoadbuffer, base);
    uintptr_t pcall = resolveLuaFunction(kPcall, base);

    if (!LuaCall::get().initialize(loadfile, loadbuffer, pcall)) {
        Logger::getInstance().error("Loader: failed to initialize LuaCall.");
        return false;
    }

    Logger::getInstance().info("Loader: initialized.");
    onLuaState(nullptr);
    onLoadmods();

    std::thread(&Loader::inputLoop, this).detach();
    return true;
}

void Loader::uninitialize()
{
    LuaCall::get().uninitialize();
}
