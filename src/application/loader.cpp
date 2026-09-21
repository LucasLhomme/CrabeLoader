/*
** CrabeLoader
** File description:
** Brings the loader up: adopts the game Lua state, gates multiplayer, arms the keybinds.
** Multiplayer needs two gates open at once, the game profile and the config; both are reported.
** Loads no mod file -- discovery and ordering live in src/domain/mod_manager.cpp.
**
** Authors: @LucasLhomme
*/

#include <algorithm>
#include <cctype>
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
#include "infrastructure/crash_reporter.hpp"
#include "infrastructure/lua_symbols.hpp"
#include "infrastructure/lua_call.hpp"
#include "infrastructure/memory.hpp"
#include "presentation/input_hook.hpp"
#include "infrastructure/message_hook.hpp"
#include "application/multiplayer/multiplayer_manager.hpp"
#include "presentation/render_hook.hpp"
#include "domain/config.hpp"
#include "domain/game_profile.hpp"
#include "domain/mod_manager.hpp"
#include "shared/keybind_names.hpp"
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

    // Resolves a config-file key name to a virtual key, falling back to
    // `fallback` (the current hardcoded default) both when [keybinds]
    // carries no override at all and when it carries one this loader does
    // not recognise -- either way the user sees the behaviour they had
    // before crabe.toml existed, not an unbound key.
    int resolveKeybind(const std::string& configuredName, int fallback, const char* which)
    {
        if (configuredName.empty())
            return fallback;

        if (const auto virtualKey = crabe::shared::parseVirtualKeyName(configuredName))
            return *virtualKey;

        crabe::shared::Logger::getInstance().warning(
            "Loader: crabe.toml [keybinds].{} = \"{}\" is not a recognised key name; keeping the default.",
            which, configuredName);
        return fallback;
    }

    // debug/info/warning/error, case-insensitive; anything else is reported
    // and the level is left unchanged rather than guessed at.
    void applyConfiguredLogLevel(const std::string& configuredLevel)
    {
        std::string lower = configuredLevel;
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
        if (lower == "debug") logger.setLogLevel(crabe::shared::LogLevel::DEBUG);
        else if (lower == "info") logger.setLogLevel(crabe::shared::LogLevel::INFO);
        else if (lower == "warning" || lower == "warn") logger.setLogLevel(crabe::shared::LogLevel::WARNING);
        else if (lower == "error") logger.setLogLevel(crabe::shared::LogLevel::ERR);
        else
            logger.warning("Loader: crabe.toml [general].logLevel = \"{}\" is not recognised "
                           "(expected debug, info, warning or error); leaving the level unchanged.",
                           configuredLevel);
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

    crabe::infrastructure::CrashReporter::pushBreadcrumb("Loader: console snippet queued");

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

    // Per state, not once globally: this runs inside L's own pcall, so L is the
    // one state it is safe to reload from here -- and every other live state
    // asks the same question on its own thread. See domain/reload_generation.hpp.
    if (crabe::domain::ModManager::get().needsReload(L)) {
        crabe::infrastructure::CrashReporter::pushBreadcrumb("Loader: hot reload requested");
        crabe::domain::ModManager::get().reloadAllMods(L);
    }

    constexpr auto kInterval = std::chrono::milliseconds(16);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - _lastTick;
    if (elapsed < kInterval)
        return;

    _lastTick = now;
    double dt = std::chrono::duration<double>(elapsed).count();

    // Named, not breadcrumbed. This runs 60 times a second: a crumb per tick
    // would fill the 64-slot ring with one second of "tick" and push out the
    // events that actually say what led to the crash. setActiveHook costs a
    // bounded memcpy into this thread's slot and is overwritten rather than
    // accumulated, which is exactly what a hot path wants.
    {
        const crabe::infrastructure::ScopedHook scopedHook("Loader::callTick");
        crabe::infrastructure::LuaCall::get().callTick(L, dt);
    }

    drainPendingKeyEvents(L);
    drainPendingKeybindCalls(L);
    drainRemoteCommandFile(L);
    drainPendingSnippets(L);
    drainLuaOutput(L);
    drainQuarantineReport(L);

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

// Polls Crabe.Quarantine.report() (src/api/02c_quarantine.lua) at a slow,
// fixed interval -- this is diagnostic bookkeeping, not per-frame state, so
// it does not need drainLuaOutput's 100ms cadence. Runs on the script thread
// inside runTicks, same as every other Lua call here (Invariant I3): the
// counting itself already happened in Lua, at the actual dispatch sites, the
// only place with per-callback granularity (see 02c_quarantine.lua's own
// header comment for why that split is where it is). This is purely readout.
void Loader::drainQuarantineReport(void* L)
{
    constexpr auto kInterval = std::chrono::milliseconds(1000);

    auto now = std::chrono::steady_clock::now();
    if (now - _lastQuarantinePoll < kInterval)
        return;
    _lastQuarantinePoll = now;

    std::string report;
    if (!crabe::infrastructure::LuaCall::get().runSnippet(
            L, "return Crabe.Quarantine and Crabe.Quarantine.report() or ''", report))
        return;

    // Derived once, not on every poll: current_path() is a syscall (see
    // drainRemoteCommandFile above for the same reasoning at a faster cadence).
    static const std::filesystem::path kGameRoot = std::filesystem::current_path();

    crabe::domain::QuarantineSnapshot snapshot = crabe::domain::QuarantineSnapshot::parseReport(report);
    crabe::domain::Config::active().updateQuarantineSnapshot(kGameRoot, snapshot);
}

// Binds the two keys the loader has always had, reading their virtual-key
// codes from crabe.toml's [keybinds] (Config::active()) with VK_F4 / VK_INSERT
// as the defaults -- so a user with no config, or one that predates T11,
// sees no change at all.
void Loader::registerDefaultKeybinds()
{
    const crabe::domain::Config& config = crabe::domain::Config::active();

    const int hotReloadKey = resolveKeybind(config.hotReloadKeybind(), VK_F4, "hotReload");
    registerKeybind(hotReloadKey, []() {
        crabe::domain::ModManager::get().requestHotReload();
    });

    const int devOverlayKey = resolveKeybind(config.devOverlayKeybind(), VK_INSERT, "devOverlay");
    registerKeybind(devOverlayKey, []() {
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
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

    // Loaded first: registerDefaultKeybinds (below) reads [keybinds] from
    // it, and everything else in this function that can be config-driven
    // (log level, multiplayer) reads it too. current_path() is the game
    // root -- the same base discoverAndLoadMods() resolves "mods" against.
    crabe::domain::Config::initializeActive(std::filesystem::current_path());
    const crabe::domain::Config& config = crabe::domain::Config::active();

    if (const auto& diagnostic = config.diagnostic()) {
        logger.error("Loader: {}; using defaults. The file on disk was left untouched.",
                     diagnostic->what());
    }
    if (!config.getIgnoredKeys().empty()) {
        for (const std::string& key : config.getIgnoredKeys())
            logger.debug("Loader: crabe.toml: key '{}' is not recognised and was ignored.", key);
    }
    if (!config.activeProfileFound()) {
        logger.warning("Loader: crabe.toml: [general].profile = \"{}\" names no [profiles.{}] table; "
                       "loading every mod.", config.activeProfileName(), config.activeProfileName());
    }
    applyConfiguredLogLevel(config.logLevel());

    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    // Which build is this? Everything below turns on the answer. The
    // detection itself is cached, so MemoryPatcher -- which runs from
    // DllMain, before this thread exists -- and this call see one answer.
    const crabe::domain::GameProfile* profile = crabe::domain::activeProfile();

    loadOverridesFromDisk();
    loadCharactersFromDisk();
    const crabe::lua_symbols::Resolution resolution = crabe::lua_symbols::resolveAll(base, profile);

    // Three outcomes, and only three: the build is one we measured, or it is
    // not but every address was found anyway, or it is not and one was missed.
    const crabe::domain::LoadDecision decision =
        crabe::domain::decideLoad(profile != nullptr, resolution.allResolved());

    if (decision == crabe::domain::LoadDecision::Refuse) {
        const crabe::domain::PeIdentity identity = crabe::domain::runningGameIdentity();

        logger.error("========================================================================");
        logger.error("Loader: REFUSING TO MODIFY THIS GAME.");
        logger.error("Loader: no profile matches this executable "
                     "(TimeDateStamp 0x{:08X}, SizeOfImage 0x{:X}, CheckSum 0x{:08X}),",
                     identity.timeDateStamp, identity.sizeOfImage, identity.checkSum);
        logger.error("Loader: and {}/{} Lua symbols could not be found by scanning either.",
                     resolution.total - resolution.resolved, resolution.total);
        logger.error("Loader: nothing has been hooked and no byte has been patched. The game "
                     "will start, unmodded.");
        logger.error("Loader: add a profile for TimeDateStamp 0x{:08X} to "
                     "src/domain/game_profile.cpp to support this build.",
                     identity.timeDateStamp);
        logger.error("========================================================================");
        return false;
    }

    if (decision == crabe::domain::LoadDecision::Degraded) {
        const crabe::domain::PeIdentity identity = crabe::domain::runningGameIdentity();

        logger.warning("========================================================================");
        logger.warning("Loader: DEGRADED MODE -- unrecognised game build.");
        logger.warning("Loader: no profile matches this executable "
                       "(TimeDateStamp 0x{:08X}, SizeOfImage 0x{:X}).",
                       identity.timeDateStamp, identity.sizeOfImage);
        logger.warning("Loader: all {} Lua symbols were found by scanning, so mods will load, "
                       "but no address was confirmed against a measured RVA.", resolution.total);
        logger.warning("Loader: multiplayer is DISABLED -- its patches are known by address "
                       "only, and an address from the wrong build corrupts code.");
        logger.warning("========================================================================");
    }

    if (!crabe::infrastructure::LuaCall::get().initialize(resolution.addresses)) {
        logger.error("Loader: failed to initialize LuaCall.");
        return false;
    }

    if (profile)
        logger.info("Loader: initialized against profile '{}'.", profile->id);
    else
        logger.info("Loader: initialized (degraded, no profile).");
    registerDefaultKeybinds();

    if (!crabe::presentation::RenderHook::get().initialize()) {
        logger.warning("Loader: failed to initialize the render hook (overlay disabled).");
    }

    crabe::presentation::InputHook::get().initialize();
    crabe::infrastructure::MessageHook::get().initialize();

    // Two independent gates, and both must open. The config gate is the
    // player's stated preference; the profile gate is a safety property --
    // the multiplayer patches are known by address only, so applying them to
    // a build we did not measure corrupts code. Report whichever one closed,
    // so the log says something actionable rather than just "disabled".
    if (!config.multiplayerEnabled()) {
        logger.info("Loader: multiplayer disabled by crabe.toml ([multiplayer].enabled = false).");
    } else if (decision != crabe::domain::LoadDecision::Supported) {
        logger.warning("Loader: multiplayer left uninitialised ({}); its patches are known by "
                       "address only and this build is not a measured one.",
                       crabe::domain::describe(decision));
    } else {
        crabe::multiplayer::application::MultiplayerManager::getInstance().initialize();
    }

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

