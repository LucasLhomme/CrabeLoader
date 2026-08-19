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
#include <string_view>
#include <thread>
#include <windows.h>

#include "loader/loader.hpp"
#include "loader/gateway.hpp"
#include "loader/lua_runtime.hpp"
#include "loader/lua_symbols.hpp"
#include "loader/luacall.hpp"
#include "loader/memory.hpp"
#include "loader/avatar_relay_hook.hpp"
#include "loader/input_hook.hpp"
#include "loader/message_hook.hpp"
#include "loader/render_hook.hpp"
#include "logger/logger.hpp"

namespace {

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

    if (_modsLoaded)
        return;

    std::lock_guard<std::mutex> lock(_stateMutex);
    if (_modsLoaded)
        return;

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
    Logger& logger = Logger::getInstance();
    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";

    if (!isInjected()) {
        logger.error("Loader: Lua state not injected, skipping mod loading.");
        return;
    }

    logger.debug("Loader: Reading mods folder...");
    if (!std::filesystem::exists(modsFolder)) {
        logger.info("Loader: Mods folder does not exist, creating...");
        std::filesystem::create_directory(modsFolder);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(modsFolder)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua")
            continue;

        std::string filename = entry.path().filename().string();
        logger.info("Loader: Found mod: {}", filename);

        if (LuaCall::get().runFile(_luaState, entry.path().string().c_str()))
            logger.debug("Loader: mod '{}' executed.", filename);
        else
            logger.warning("Loader: mod '{}' failed to execute.", filename);
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

    if (input.front() == '=')
        input = "return tostring(" + input.substr(1) + ")";

    std::lock_guard<std::mutex> lock(_snippetQueueMutex);
    _pendingSnippets.push_back(input);
}

void Loader::drainRemoteCommandFile(void* L)
{
    (void)L;

    constexpr auto kInterval = std::chrono::milliseconds(250);
    auto now = std::chrono::steady_clock::now();
    if (now - _lastRemoteCommandProbe < kInterval)
        return;
    _lastRemoteCommandProbe = now;

    // Derived once: current_path() is a syscall, and this runs 4x/s for the
    // life of the process against a file that usually is not there.
    static const std::filesystem::path kPath =
        std::filesystem::current_path() / "crabe_remote_cmd.txt";

    std::ifstream file(kPath);
    if (!file)
        return;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::error_code removeError;
    std::filesystem::remove(kPath, removeError);
    if (removeError) {
        Logger::getInstance().warning("Loader: could not consume {} ({}); skipping it to avoid a crash loop.",
            kPath.filename().string(), removeError.message());
        return;
    }

    if (content.find_first_not_of(" \t\r\n") == std::string::npos)
        return;

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
            Logger::getInstance().info("= {}", result.empty() ? "(empty string)" : result);
        }
    }

    _lastOutputDrain = {};
    drainLuaOutput(L);
}

void Loader::ensureRuntimeReady(void* L)
{
    if (isGameState(L) || _rejectedStates.count(L) != 0)
        return;
    constexpr auto kInterval = std::chrono::milliseconds(250);

    auto now = std::chrono::steady_clock::now();
    if (now - _lastReadyProbe < kInterval)
        return;
    _lastReadyProbe = now;

    LuaRuntime::StateKind kind = LuaRuntime::classifyState(L);
    if (kind != LuaRuntime::StateKind::Game) {
        if (kind == LuaRuntime::StateKind::NotTheGame) {
            // It answered, and it has no game natives -- that verdict is final,
            // so stop re-probing this state four times a second forever.
            _rejectedStates.insert(L);

            if (!_sawForeignState) {
                _sawForeignState = true;
                Logger::getInstance().debug(
                    "Loader: skipping a Lua state without the game's natives (shader compiler); still waiting.");
            }
        }
        return;
    }

    _initializedStates.insert(L);
    _luaState = L;
    _runtimeReady = true;

    Logger::getInstance().info("Loader: game Lua state 0x{:X} ready, injecting the API ({} state(s) so far).",
                            reinterpret_cast<uintptr_t>(L), _initializedStates.size());

    LuaRuntime::injectAll(L);
    LuaRuntime::registerNatives(L);
    onLoadmods();
}

void Loader::drainLuaOutput(void* L)
{
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

    loadOverridesFromDisk();
    loadCharactersFromDisk();

    LuaApiAddresses addresses = LuaSymbols::resolveAll(base);

    if (!LuaCall::get().initialize(addresses)) {
        Logger::getInstance().error("Loader: failed to initialize LuaCall.");
        return false;
    }

    Logger::getInstance().info("Loader: initialized.");
    registerDefaultKeybinds();

    if (!RenderHook::get().initialize()) {
        Logger::getInstance().warning("Loader: failed to initialize the render hook (overlay disabled).");
    }

    InputHook::get().initialize();
    MessageHook::get().initialize();
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
