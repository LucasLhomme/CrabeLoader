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

#include "application/loader.hpp"
#include "application/lua_runtime.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/lua_symbols.hpp"
#include "infrastructure/lua_call.hpp"
#include "infrastructure/memory.hpp"
#include "presentation/input_hook.hpp"
#include "infrastructure/message_hook.hpp"
#include "application/multiplayer/multiplayer_manager.hpp"
#include "presentation/render_hook.hpp"
#include "domain/mod_manager.hpp"
#include "shared/logger.hpp"

namespace crabe::application {

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
        crabe::shared::Logger::getInstance().error("Loader: Lua state UNINJECTED.");
        return;
    }

    if (_modsLoaded)
        return;

    std::lock_guard<std::mutex> lock(_stateMutex);
    if (_modsLoaded)
        return;

    _luaState = L;
    _modsLoaded = true;
    crabe::shared::Logger::getInstance().debug("Loader: Lua state INJECTED.");

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
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";

    if (!isInjected()) {
        logger.error("Loader: Lua state not injected, skipping mod loading.");
        return;
    }

    crabe::domain::ModManager::get().discoverAndLoadMods(_luaState, modsFolder);
}

void Loader::registerKeybind(int virtualKey, std::function<void()> onPress)
{
    std::lock_guard<std::mutex> lock(_keybindsMutex);
    _keybinds[virtualKey] = Keybind{ std::move(onPress), false };
    crabe::shared::Logger::getInstance().debug("Loader: registered keybind for virtual key 0x{:X}.", virtualKey);
}

// Replaces the captured key set. An empty list restores normal forwarding.
void Loader::setCapturedKeys(std::vector<int> virtualKeys)
{
    std::lock_guard<std::mutex> lock(_capturedKeysMutex);
    _capturedKeys.clear();
    for (int virtualKey : virtualKeys)
        _capturedKeys.insert(virtualKey);
}

bool Loader::isKeyCaptured(int virtualKey) const
{
    std::lock_guard<std::mutex> lock(_capturedKeysMutex);
    return _capturedKeys.count(virtualKey) != 0;
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
        if (!crabe::infrastructure::LuaCall::get().runGlobalIfExists(L, luaFunctionName)) {
            crabe::shared::Logger::getInstance().warning("Loader: keybind call '{}()' raised a Lua error.", luaFunctionName);
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

    crabe::shared::Logger::getInstance().info("> {}", input);

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
        crabe::shared::Logger::getInstance().warning("Loader: could not consume {} ({}); skipping it to avoid a crash loop.",
            kPath.filename().string(), removeError.message());
        return;
    }

    if (content.find_first_not_of(" \t\r\n") == std::string::npos)
        return;

    queueConsoleSnippet(content);
}

// Drives periodic game ticks, executes pending menus, and handles mod reload.
void Loader::runTicks(void* L)
{
    if (!_runtimeReady)
        return;

    if (crabe::domain::ModManager::get().isHotReloadRequested())
        crabe::domain::ModManager::get().reloadAllMods(L);

    constexpr auto kInterval = std::chrono::milliseconds(16);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - _lastTick;
    if (elapsed < kInterval)
        return;

    _lastTick = now;
    double dt = std::chrono::duration<double>(elapsed).count();
    crabe::infrastructure::LuaCall::get().callTick(L, dt);

    drainPendingKeyEvents(L);
    drainPendingKeybindCalls(L);
    drainRemoteCommandFile(L);
    drainPendingSnippets(L);
    drainLuaOutput(L);

    crabe::infrastructure::CrashHandler::runGuarded([L]() {
        crabe::domain::ModManager::get().dispatchDraw(L);
    }, "Loader::dispatchDraw");
}

// Emits every key press collected by the window thread since the last tick.
void Loader::drainPendingKeyEvents(void* L)
{
    std::vector<int> pending;
    {
        std::lock_guard<std::mutex> lock(_keyEventQueueMutex);
        if (_pendingKeyEvents.empty())
            return;
        pending.swap(_pendingKeyEvents);
    }

    for (int virtualKey : pending) {
        crabe::infrastructure::LuaCall::get().runSnippet(L, std::format(
            "if Crabe and Crabe.Events and Crabe.Events.emit then Crabe.Events.emit('keyDown', {}) end",
            virtualKey));
    }
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
        bool ok = crabe::infrastructure::LuaCall::get().runSnippet(L, code, result);

        if (!ok) {
            crabe::shared::Logger::getInstance().error("! {}", result);
        } else {
            crabe::shared::Logger::getInstance().info("= {}", result.empty() ? "(empty string)" : result);
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

    crabe::lua_runtime::StateKind kind = crabe::lua_runtime::classifyState(L);
    if (kind != crabe::lua_runtime::StateKind::Game) {
        if (kind == crabe::lua_runtime::StateKind::NotTheGame) {
            // It answered, and it has no game natives -- that verdict is final,
            // so stop re-probing this state four times a second forever.
            _rejectedStates.insert(L);

            if (!_sawForeignState) {
                _sawForeignState = true;
                crabe::shared::Logger::getInstance().debug(
                    "Loader: skipping a Lua state without the game's natives (shader compiler); still waiting.");
            }
        }
        return;
    }

    _initializedStates.insert(L);
    _luaState = L;

    crabe::shared::Logger::getInstance().info("Loader: game Lua state 0x{:X} ready, injecting the API ({} state(s) so far).",
                            reinterpret_cast<uintptr_t>(L), _initializedStates.size());

    crabe::lua_runtime::injectAll(L);
    crabe::lua_runtime::registerNatives(L);
    onLoadmods();
    _runtimeReady = true;
}

void Loader::drainLuaOutput(void* L)
{
    constexpr auto kInterval = std::chrono::milliseconds(100);

    auto now = std::chrono::steady_clock::now();
    if (now - _lastOutputDrain < kInterval)
        return;
    _lastOutputDrain = now;

    std::string output;
    if (!crabe::infrastructure::LuaCall::get().runSnippet(L, "return Crabe and Crabe.flush() or ''", output))
        return;

    size_t start = 0;
    while (start < output.size()) {
        size_t end = output.find('\n', start);
        if (end == std::string::npos)
            end = output.size();

        if (end > start) crabe::shared::Logger::getInstance().debug("{}", output.substr(start, end - start));
        start = end + 1;
    }
}

void Loader::registerDefaultKeybinds()
{
    registerKeybind(VK_F4, []() {
        crabe::domain::ModManager::get().requestHotReload();
    });

    registerKeybind(VK_INSERT, []() {
        crabe::presentation::RenderHook::get().toggleMenu();
    });
}

// Updates keybind states and executes callbacks on key-down transitions.
void Loader::onKeyEvent(int virtualKey, bool isDown)
{
    std::function<void()> callback;
    {
        std::lock_guard<std::mutex> lock(_keybindsMutex);
        auto it = _keybinds.find(virtualKey);
        if (it != _keybinds.end()) {
            if (isDown && !it->second.wasDown)
                callback = it->second.onPress;
            it->second.wasDown = isDown;
        }
    }

    if (callback) {
        crabe::shared::Logger::getInstance().debug("Loader: keybind pressed: {} (virtual key 0x{:X}).",
                                    virtualKeyName(virtualKey), virtualKey);
        callback();
    }

    if (isDown && _runtimeReady) {
        std::lock_guard<std::mutex> lock(_keyEventQueueMutex);
        if (_pendingKeyEvents.size() < kMaxPendingKeyEvents)
            _pendingKeyEvents.push_back(virtualKey);
    }
}

bool Loader::initialize()
{
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    loadOverridesFromDisk();
    crabe::infrastructure::LuaApiAddresses addresses = crabe::lua_symbols::resolveAll(base);

    if (!crabe::infrastructure::LuaCall::get().initialize(addresses)) {
        crabe::shared::Logger::getInstance().error("Loader: failed to initialize LuaCall.");
        return false;
    }

    crabe::shared::Logger::getInstance().info("Loader: initialized.");
    registerDefaultKeybinds();

    if (!crabe::presentation::RenderHook::get().initialize()) {
        crabe::shared::Logger::getInstance().warning("Loader: failed to initialize the render hook (overlay disabled).");
    }

    crabe::presentation::InputHook::get().initialize();
    crabe::infrastructure::MessageHook::get().initialize();
    crabe::multiplayer::application::MultiplayerManager::getInstance().initialize();

    return true;
}

void Loader::uninitialize()
{
    crabe::multiplayer::application::MultiplayerManager::getInstance().uninitialize();
    crabe::presentation::InputHook::get().uninitialize();
    crabe::infrastructure::MessageHook::get().uninitialize();
    crabe::presentation::RenderHook::get().uninitialize();
    crabe::infrastructure::LuaCall::get().uninitialize();
}

} // namespace crabe::application

