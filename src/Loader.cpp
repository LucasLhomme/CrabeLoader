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
#include "loader/avatar_relay_hook.hpp"
#include "loader/input_hook.hpp"
#include "loader/message_hook.hpp"
#include "loader/render_hook.hpp"
#include "logger/logger.hpp"

namespace {

    // The game exports no Lua symbol; each stdlib name is bound to a small
    // wrapper that delegates to the real C API function, so the real address
    // is the n-th `call` inside that wrapper.
    struct LuaSymbol {
        const char* name;        // what we are resolving, for the log
        const char* stdlibName;  // Lua stdlib entry holding the wrapper
        int callIndex;           // 1-based `call` inside the wrapper
        uintptr_t expectedRva;   // RVA read off the shipped binary, 0 to skip the check
        size_t scanBytes = 256;  // how far into the wrapper to look
    };

    // Index is a hint, RVA is the contract: a mismatch refuses rather than
    // hooking a wrong address. "type" resolves to io.type (found before
    // luaB_type); "wrap" is coroutine.wrap.
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

        // Some stdlib names are registered more than once ("type" is both
        // luaB_type and io_type); try each binding and keep the one whose
        // call #N lands on the contracted RVA.
        std::vector<uintptr_t> wrappers = Memory::findRegisteredFunctions(symbol.stdlibName);
        if (wrappers.empty()) {
            logger.error("Loader: {}: no '{}' entry in the Lua stdlib table.",
                        symbol.name, symbol.stdlibName);
            return 0;
        }

        auto index = static_cast<size_t>(symbol.callIndex);
        if (symbol.callIndex <= 0) {
            logger.error("Loader: {}: invalid call index {}.", symbol.name, symbol.callIndex);
            return 0;
        }

        uintptr_t fallback = 0; // best guess when the symbol carries no expected RVA

        for (uintptr_t wrapper : wrappers) {
            std::vector<uintptr_t> calls = Memory::findCalls(wrapper, symbol.scanBytes);
            if (index > calls.size()) continue;

            uintptr_t addr = calls[index - 1];
            uintptr_t rva = addr - base;

            if (!symbol.expectedRva) {
                if (!fallback) fallback = addr;
                continue;
            }
            if (rva != symbol.expectedRva) continue;

            logger.debug("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}]",
                        symbol.name, addr, rva, firstBytes(addr, 8));
            return addr;
        }

        if (!symbol.expectedRva && fallback) {
            logger.debug("Loader: {} @ 0x{:X} (RVA 0x{:X}) [{}] (unverified: no expected RVA)",
                        symbol.name, fallback, fallback - base, firstBytes(fallback, 8));
            return fallback;
        }

        // Nothing matched: dump what each candidate did contain, since that is
        // exactly what a game update would change.
        logger.error("Loader: {}: no '{}' binding ({} candidate(s)) whose call #{} lands on RVA 0x{:X}; refusing.",
                    symbol.name, symbol.stdlibName, wrappers.size(), symbol.callIndex, symbol.expectedRva);

        for (uintptr_t wrapper : wrappers) {
            std::vector<uintptr_t> calls = Memory::findCalls(wrapper, symbol.scanBytes);
            logger.debug("Loader: {}: candidate '{}' @ 0x{:X}, {} call(s)",
                        symbol.name, symbol.stdlibName, wrapper, calls.size());

            for (size_t i = 0; i < calls.size(); ++i) {
                logger.debug("Loader: {}:   call #{} -> RVA 0x{:X}",
                            symbol.name, i + 1, calls[i] - base);
            }
        }
        return 0;
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

    // API/mods NOT loaded here: this fires before luaopen_base fills _G, so
    // anything injected now would fail. Deferred to ensureRuntimeReady().
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

void Loader::queueConsoleSnippet(const std::string& rawInput)
{
    std::string input = rawInput;

    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return;
    input.erase(0, start);
    size_t end = input.find_last_not_of(" \t\r\n");
    if (end != std::string::npos)
        input.erase(end + 1);

    Logger::getInstance().info("> {}", input);

    // `=expr` prints a value, REPL-style, via the game's own tostring --
    // lua_tolstring alone returns NULL for anything not already a string/number.
    if (input.front() == '=')
        input = "return tostring(" + input.substr(1) + ")";

    std::lock_guard<std::mutex> lock(_snippetQueueMutex);
    _pendingSnippets.push_back(input);
}

void Loader::drainRemoteCommandFile(void* L)
{
    // Not tied to L: this only decides whether there is a new command to
    // queue. Kept as a parameter for symmetry with the other drain*()
    // functions and in case a future version needs the state directly.
    (void)L;

    constexpr auto kInterval = std::chrono::milliseconds(250);
    auto now = std::chrono::steady_clock::now();
    if (now - _lastRemoteCommandProbe < kInterval)
        return;
    _lastRemoteCommandProbe = now;

    constexpr const char* kRemoteCommandFileName = "crabe_remote_cmd.txt";
    std::filesystem::path path = std::filesystem::current_path() / kRemoteCommandFileName;

    std::ifstream file(path);
    if (!file)
        return; // no file yet: nothing to do, not an error

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Consume before running: a crashing command left in place would boot-loop
    // the game (relaunch, find the same command, die again) since the file
    // outlives the process but "already seen this one" does not.
    std::error_code removeError;
    std::filesystem::remove(path, removeError);
    if (removeError) {
        Logger::getInstance().warning("Loader: could not consume {} ({}); skipping it to avoid a crash loop.",
            kRemoteCommandFileName, removeError.message());
        return;
    }

    if (content.find_first_not_of(" \t\r\n") == std::string::npos)
        return; // cleared/empty: treat as "no command", not a blank submission

    queueConsoleSnippet(content);
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
        } else {
            // Print explicitly for an empty string, or it's indistinguishable
            // from the command not having run at all.
            Logger::getInstance().info("= {}", result.empty() ? "(empty string)" : result);
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
    LuaRuntime::registerNatives(L); // needs Crabe, which injectAll just created
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

    // Purely diagnostic, and optional: it answers "which controller slots does
    // the game actually poll", which nothing else can (the per-controller
    // objects are heap-allocated, so the module scanner cannot count them).
    InputHook::get().initialize();
    MessageHook::get().initialize();

    // Installed disarmed, permanently -- see avatar_relay_hook.hpp. Only
    // acts when Crabe._armAvatarRelay is called from Lua (splitscreen mod).
    AvatarRelayHook::get().initialize();

    std::thread(&Loader::inputLoop, this).detach();
    return true;
}

void Loader::uninitialize()
{
    InputHook::get().uninitialize();
    MessageHook::get().uninitialize();
    AvatarRelayHook::get().uninitialize();
    RenderHook::get().uninitialize();
    LuaCall::get().uninitialize();
}
