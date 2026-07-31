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
#include "loader/lua_runtime.hpp"
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
        uintptr_t expectedRva;   // RVA read off the shipped binary, 0 to skip the check
        size_t scanBytes = 256;  // how far into the wrapper to look
    };

    // Call indexes AND the RVA each one must land on were read off the shipped
    // binary. Hooking a wrong address corrupts the host process on the next
    // call, so a mismatch refuses instead of patching: the index is a hint, the
    // RVA is the contract.
    //
    // The indexes come from cross-referencing a dump of every stdlib wrapper
    //
    // Two of the wrappers are not the obvious ones:
    //  - "wrap" is coroutine.wrap; luaB_cowrap is `cocreate; pushcclosure`.
    //  - "type" resolves to io.type, not luaB_type -- both libraries register
    //    that name, and io's table is found first. Its call sequence matches
    //    io_type (checkany, touserdata, getfield, getmetatable, rawequal, then
    //    the pushliteral branches), which is what makes those indexes valid.
    constexpr LuaSymbol kLoadfile     { "luaL_loadfile",     "loadfile",   2,  0xF0EBF0 };
    constexpr LuaSymbol kLoadbuffer   { "luaL_loadbuffer",   "loadstring", 3,  0xF0EDE0 };
    constexpr LuaSymbol kPcall        { "lua_pcall",         "xpcall",     4,  0xF0DF60 };

    constexpr LuaSymbol kGettop       { "lua_gettop",        "print",      1,  0xF0D0E0 };
    constexpr LuaSymbol kGetfield     { "lua_getfield",      "print",      2,  0xF0DA00 };
    constexpr LuaSymbol kPushvalue    { "lua_pushvalue",     "print",      3,  0xF0D2A0 };
    constexpr LuaSymbol kCall         { "lua_call",          "print",      5,  0xF0DF00 };
    constexpr LuaSymbol kTolstring    { "lua_tolstring",     "print",      6,  0xF0D5A0, 512 };
    constexpr LuaSymbol kSettop       { "lua_settop",        "print",      11, 0xF0D0F0, 512 };

    constexpr LuaSymbol kPushcclosure { "lua_pushcclosure",  "wrap",       2,  0xF0D8E0 };

    constexpr LuaSymbol kChecktype    { "luaL_checktype",    "rawset",     1,  0xF0EFE0 };
    constexpr LuaSymbol kCheckany     { "luaL_checkany",     "rawset",     2,  0xF0F010 };
    constexpr LuaSymbol kRawset       { "lua_rawset",        "rawset",     5,  0xF0DCA0 };
    constexpr LuaSymbol kRawget       { "lua_rawget",        "rawget",     4,  0xF0DA60 };

    constexpr LuaSymbol kChecklstring { "luaL_checklstring", "require",    1,  0xF0F040 };
    constexpr LuaSymbol kToboolean    { "lua_toboolean",     "require",    5,  0xF0D570 };

    constexpr LuaSymbol kIsnumber     { "lua_isnumber",      "tonumber",   3,  0xF0D350 };
    constexpr LuaSymbol kTonumber     { "lua_tonumber",      "tonumber",   4,  0xF0D4F0 };
    constexpr LuaSymbol kPushnumber   { "lua_pushnumber",    "tonumber",   5,  0xF0D7C0 };

    constexpr LuaSymbol kTouserdata   { "lua_touserdata",    "type",       2,  0xF0D6D0 };
    constexpr LuaSymbol kGetmetatable { "lua_getmetatable",  "type",       4,  0xF0DB20 };
    constexpr LuaSymbol kRawequal     { "lua_rawequal",      "type",       5,  0xF0D3F0 };
    constexpr LuaSymbol kPushlstring  { "lua_pushlstring",   "type",       6,  0xF0D800 };
    constexpr LuaSymbol kPushnil      { "lua_pushnil",       "type",       8,  0xF0D7A0 };

    constexpr LuaSymbol kPushboolean  { "lua_pushboolean",   "rawequal",   4,  0xF0D960 };

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

        std::vector<uintptr_t> calls = Memory::findCalls(wrapper, symbol.scanBytes);

        auto logCandidates = [&]() {
            for (size_t i = 0; i < calls.size(); ++i) {
                logger.debug("Loader: {}: '{}' call #{} -> RVA 0x{:X}",
                            symbol.name, symbol.stdlibName, i + 1, calls[i] - base);
            }
        };

        auto index = static_cast<size_t>(symbol.callIndex);
        if (symbol.callIndex <= 0 || index > calls.size()) {
            logger.error("Loader: {}: call #{} not found in the '{}' wrapper (0x{:X}, {} calls).",
                        symbol.name, symbol.callIndex, symbol.stdlibName, wrapper, calls.size());
            logCandidates();
            return 0;
        }

        uintptr_t addr = calls[index - 1];
        uintptr_t rva = addr - base;

        if (symbol.expectedRva && rva != symbol.expectedRva) {
            logger.error("Loader: {}: call #{} of '{}' resolved to RVA 0x{:X}, expected 0x{:X}; refusing.",
                        symbol.name, symbol.callIndex, symbol.stdlibName, rva, symbol.expectedRva);
            logCandidates();
            return 0;
        }

        logger.debug("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                    symbol.name, addr, rva, firstBytes(addr, 8));
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
        Logger::getInstance().error("Loader: Lua state UNINJECTED.");
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
    Logger::getInstance().debug("Loader: Lua state INJECTED.");

    // The API and the mods are NOT loaded here. This runs on the game's very
    // first loadbuffer, before luaopen_base has filled _G: `type`, `rawget`
    // and the rest are still nil, so anything injected now fails. Loading is
    // deferred to ensureRuntimeReady(), retried from the hooked pcall.
}

bool Loader::isInjected()
{
    return _luaState != nullptr;
}

bool Loader::isGameState(void* L) const
{
    return L && _initializedStates.count(L) != 0;
}

void Loader::onLoadmods()
{
    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";

    if (!Loader::get().isInjected()) {
        Logger::getInstance().error("Loader: Lua state not injected, skipping mod loading.");
        return;
    } else {
        Logger::getInstance().debug("Loader: Reading mods folder...");
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
                        Logger::getInstance().debug("Loader: mod '{}' executed.", filename);
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
    Logger::getInstance().debug("Loader: registered keybind for virtual key 0x{:X}.", virtualKey);
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

void Loader::runTicks(void* L)
{
    if (!_runtimeReady)
        return;

    // The game exposes no global Update to wrap, so the per-frame callbacks are
    // driven from here instead. The hooked pcall fires far more often than a
    // frame, hence the interval.
    constexpr auto kInterval = std::chrono::milliseconds(16); // ~60 Hz

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - _lastTick;
    if (elapsed < kInterval)
        return;

    _lastTick = now;
    double dt = std::chrono::duration<double>(elapsed).count();
    LuaCall::get().callTick(L, dt);
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

    _lastOutputDrain = {};
    drainLuaOutput(L);
}

void Loader::ensureRuntimeReady(void* L)
{
    if (isGameState(L))
        return;
    constexpr auto kInterval = std::chrono::milliseconds(250);

    auto now = std::chrono::steady_clock::now();
    if (now - _lastReadyProbe < kInterval)
        return;
    _lastReadyProbe = now;

    LuaRuntime::StateKind kind = LuaRuntime::classifyState(L);
    if (kind != LuaRuntime::StateKind::Game) {
        if (kind == LuaRuntime::StateKind::NotTheGame && !_sawForeignState) {
            _sawForeignState = true;
            Logger::getInstance().debug(
                "Loader: skipping a Lua state without the game's natives (shader compiler); still waiting.");
        }
        return;
    }

    // The game does not keep one script state for its whole run: screen
    // transitions bring up new ones. Each needs the API and the mods of its
    // own, so they are tracked individually rather than latched onto the first.
    _initializedStates.insert(L);
    _luaState = L;
    _runtimeReady = true;

    Logger::getInstance().info("Loader: game Lua state 0x{:X} ready, injecting the API ({} state(s) so far).",
                            reinterpret_cast<uintptr_t>(L), _initializedStates.size());

    LuaRuntime::injectAll(L);
    onLoadmods();
}

void Loader::drainLuaOutput(void* L)
{
    // Compiling and running a chunk is far too expensive to do on every pcall,
    // and console output does not need to be more responsive than this.
    constexpr auto kInterval = std::chrono::milliseconds(100);

    auto now = std::chrono::steady_clock::now();
    if (now - _lastOutputDrain < kInterval)
        return;
    _lastOutputDrain = now;

    std::string output;
    if (!LuaCall::get().runSnippet(L, "return Crabe and Crabe.flush() or ''", output))
        return;

    size_t start = 0;
    while (start < output.size()) {
        size_t end = output.find('\n', start);
        if (end == std::string::npos)
            end = output.size();

        if (end > start) Logger::getInstance().debug("{}", output.substr(start, end - start));
        start = end + 1;
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
    addresses.call = resolveLuaFunction(kCall, base);

    addresses.gettop = resolveLuaFunction(kGettop, base);
    addresses.settop = resolveLuaFunction(kSettop, base);
    addresses.pushvalue = resolveLuaFunction(kPushvalue, base);

    addresses.tolstring = resolveLuaFunction(kTolstring, base);
    addresses.tonumber = resolveLuaFunction(kTonumber, base);
    addresses.toboolean = resolveLuaFunction(kToboolean, base);
    addresses.touserdata = resolveLuaFunction(kTouserdata, base);
    addresses.isnumber = resolveLuaFunction(kIsnumber, base);

    addresses.pushnil = resolveLuaFunction(kPushnil, base);
    addresses.pushnumber = resolveLuaFunction(kPushnumber, base);
    addresses.pushlstring = resolveLuaFunction(kPushlstring, base);
    addresses.pushboolean = resolveLuaFunction(kPushboolean, base);
    addresses.pushcclosure = resolveLuaFunction(kPushcclosure, base);

    addresses.getfield = resolveLuaFunction(kGetfield, base);
    addresses.rawget = resolveLuaFunction(kRawget, base);
    addresses.rawset = resolveLuaFunction(kRawset, base);
    addresses.rawequal = resolveLuaFunction(kRawequal, base);
    addresses.getmetatable = resolveLuaFunction(kGetmetatable, base);

    addresses.checkany = resolveLuaFunction(kCheckany, base);
    addresses.checktype = resolveLuaFunction(kChecktype, base);
    addresses.checklstring = resolveLuaFunction(kChecklstring, base);

    if (!LuaCall::get().initialize(addresses)) {
        Logger::getInstance().error("Loader: failed to initialize LuaCall.");
        return false;
    }

    Logger::getInstance().info("Loader: initialized.");
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
