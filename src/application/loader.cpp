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
#include <utility>
#include <windows.h>

#include "application/loader.hpp"
#include "application/lua_runtime.hpp"
#include "application/update_launcher.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/crash_reporter.hpp"
#include "infrastructure/engine_asset_loader.hpp"
#include "infrastructure/engine_free_camera.hpp"
#include "infrastructure/engine_gateway.hpp"
#include "infrastructure/engine_meridian.hpp"
#include "infrastructure/engine_resolution.hpp"
#include "infrastructure/engine_shaders.hpp"
#include "infrastructure/http_redirect.hpp"
#include "infrastructure/lua_symbols.hpp"
#include "infrastructure/lua_call.hpp"
#include "infrastructure/patch/patch_borderless.hpp"
#include "infrastructure/patch/patch_keybind.hpp"
#include "infrastructure/patch/patch_resolution.hpp"
#include "presentation/dinput_hook.hpp"
#include "presentation/input_hook.hpp"
#include "presentation/input_remap.hpp"
#include "infrastructure/message_hook.hpp"
#include "infrastructure/vfs_override_manager.hpp"
#include "infrastructure/vfs_hook.hpp"
#if CRABELOADER_WITH_MULTIPLAYER
#include "application/multiplayer/multiplayer_manager.hpp"
#endif
#include "infrastructure/portal/portal_service.hpp"
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

    // Resolves a config-file key name to a virtual key, falling back on error.
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

    // Sets logger level from configuration string.
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

    if (crabe::domain::ModManager::get().needsReload(L)) {
        crabe::infrastructure::CrashReporter::pushBreadcrumb("Loader: hot reload requested");
        crabe::infrastructure::VfsOverrideManager::get().scanModsDirectory(std::filesystem::current_path() / "mods");
        crabe::domain::ModManager::get().reloadAllMods(L);
        crabe::infrastructure::patch::PatchResolution::apply(L);
        crabe::infrastructure::patch::PatchBorderless::apply(L);
        crabe::infrastructure::patch::PatchKeybind::apply(L);
    }

    constexpr auto kInterval = std::chrono::milliseconds(16);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - _lastTick;
    if (elapsed < kInterval)
        return;

    _lastTick = now;
    double dt = std::chrono::duration<double>(elapsed).count();

    {
        const crabe::infrastructure::ScopedHook scopedHook("Loader::callTick");
        crabe::infrastructure::LuaCall::get().callTick(L, dt);
    }

    drainPendingKeyEvents(L);
    drainPendingKeybindCalls(L);
    drainRemoteCommandFile(L);
    drainPendingSnippets(L);
    drainPluginSnippets(L);
    drainLuaOutput(L);
    drainQuarantineReport(L);

    crabe::infrastructure::portal::PortalService::Instance().Update();

    crabe::infrastructure::CrashHandler::runGuarded([L]() {
        crabe::domain::ModManager::get().dispatchDraw(L);
    }, "Loader::dispatchDraw");
}

// Emits every key press collected by the window thread since the last tick.
void Loader::drainPendingKeyEvents(void* L)
{
    std::vector<PendingKeyEvent> pending;
    {
        std::lock_guard<std::mutex> lock(_keyEventQueueMutex);
        if (_pendingKeyEvents.empty())
            return;
        pending.swap(_pendingKeyEvents);
    }

    for (const PendingKeyEvent& event : pending) {
        crabe::infrastructure::LuaCall::get().runSnippet(L, std::format(
            "if Crabe and Crabe.Events and Crabe.Events.emit then Crabe.Events.emit('keyDown', {}, {}) end",
            event.virtualKey, event.isRepeat ? "true" : "false"));
    }
}

// Queues Lua from a native plugin; nothing runs until the next tick on the script thread.
void Loader::queuePluginSnippet(const std::string& owner, std::string code)
{
    std::lock_guard<std::mutex> lock(_pluginSnippetQueueMutex);
    _pendingPluginSnippets.emplace_back(owner, std::move(code));
}

// Runs the Lua chunks plugins queued since the last tick, logging only the ones that fail.
void Loader::drainPluginSnippets(void* L)
{
    std::vector<std::pair<std::string, std::string>> pending;
    {
        std::lock_guard<std::mutex> lock(_pluginSnippetQueueMutex);
        if (_pendingPluginSnippets.empty())
            return;
        pending.swap(_pendingPluginSnippets);
    }

    for (const auto& [owner, code] : pending) {
        std::string result;
        if (!crabe::infrastructure::LuaCall::get().runSnippet(L, code, result))
            crabe::shared::Logger::getInstance().error("PluginApi: Lua from {} failed: {}", owner, result);
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
    if (!L)
        return;

    if (isGameState(L)) {
        if (crabe::infrastructure::LuaCall::get().hasGlobal(L, "Crabe"))
            return;
        _initializedStates.erase(L);
        crabe::shared::Logger::getInstance().info(
            "Loader: Lua state 0x{:X} lost the API -- the game reused a closed state's address; injecting again.",
            reinterpret_cast<uintptr_t>(L));
    } else if (_rejectedStates.count(L) != 0) {
        if (!crabe::infrastructure::LuaCall::get().hasGlobal(L, "UI_GetSparks"))
            return;
        _rejectedStates.erase(L);
    }
    constexpr auto kInterval = std::chrono::milliseconds(250);

    auto now = std::chrono::steady_clock::now();
    if (now - _lastReadyProbe < kInterval)
        return;
    _lastReadyProbe = now;

    crabe::lua_runtime::StateKind kind = crabe::lua_runtime::classifyState(L);
    if (kind != crabe::lua_runtime::StateKind::Game) {
        if (kind == crabe::lua_runtime::StateKind::NotTheGame) {
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

    setCapturedKeys({});
    crabe::presentation::InputHook::get().setCaptured(false);
    crabe::presentation::InputRemap::get().clearAll();

    crabe::shared::Logger::getInstance().info("Loader: game Lua state 0x{:X} ready, injecting the API ({} state(s) so far).",
                            reinterpret_cast<uintptr_t>(L), _initializedStates.size());

    crabe::lua_runtime::injectAll(L);
    crabe::lua_runtime::registerNatives(L);

    crabe::infrastructure::patch::PatchResolution::apply(L);
    crabe::infrastructure::patch::PatchBorderless::apply(L);
    crabe::infrastructure::patch::PatchKeybind::apply(L);

    if (!_characterInjectionScript.empty()) {
        crabe::shared::Logger::getInstance().info("Loader: injecting character definitions into Lua state ({} bytes).",
                                                  _characterInjectionScript.size());
        crabe::infrastructure::LuaCall::get().runSnippet(L, _characterInjectionScript);
    }

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

// Polls quarantine report at a fixed interval on the script thread.
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

    static const std::filesystem::path kGameRoot = std::filesystem::current_path();

    crabe::domain::QuarantineSnapshot snapshot = crabe::domain::QuarantineSnapshot::parseReport(report);
    crabe::domain::Config::active().updateQuarantineSnapshot(kGameRoot, snapshot);
}

// Binds default keybinds from configuration.
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
void Loader::onKeyEvent(int virtualKey, bool isDown, bool isRepeat)
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
            _pendingKeyEvents.push_back({ virtualKey, isRepeat });
    }
}

// Configures HTTP redirect rules and hooks WinHTTP if rules exist.
static void installHttpRedirects(const crabe::domain::Config& config)
{
    auto& logger = crabe::shared::Logger::getInstance();
    crabe::infrastructure::HttpRedirectRules rules;
    for (const auto& [host, url] : config.httpRedirects()) {
        if (auto added = rules.add(host, url); !added)
            logger.error("Loader: crabe.toml [http.redirect] \"{}\": {}; rule ignored.", host, added.error());
        else
            logger.info("Loader: crabe.toml [http.redirect] {} -> {}", host, url);
    }
    if (!rules.empty() && !crabe::infrastructure::HttpRedirect::get().initialize(std::move(rules)))
        logger.error("Loader: [http.redirect] is configured but WinHTTP could not be hooked.");
}

bool Loader::initialize()
{
    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();

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

    crabe::application::startUpdateCheckInBackground(config.updateCheckEnabled());

    auto base =reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    const crabe::domain::GameProfile* profile = crabe::domain::activeProfile();

    loadOverridesFromDisk();
    loadCharactersFromDisk();
    const crabe::lua_symbols::Resolution resolution = crabe::lua_symbols::resolveAll(base, profile);

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

    installHttpRedirects(config);

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
    crabe::presentation::DirectInputHook::get().initialize();
    crabe::infrastructure::MessageHook::get().initialize();
    crabe::infrastructure::EngineFreeCamera::get().initialize();
    crabe::infrastructure::EngineGateway::get().initialize();
    crabe::infrastructure::EngineMeridian::get().initialize();
    crabe::infrastructure::EngineShaders::get().initialize();
    crabe::infrastructure::VfsOverrideManager::get().initialize(std::filesystem::current_path());
    crabe::infrastructure::VfsHook::get().initialize();
    crabe::infrastructure::EngineAssetLoader::get().initialize();
    crabe::infrastructure::portal::PortalService::Instance().Initialize();

#if !CRABELOADER_WITH_MULTIPLAYER
    logger.info("Loader: this build has no multiplayer (its sources were not present at compile time).");
#else
    if (!config.multiplayerEnabled()) {
        logger.info("Loader: multiplayer disabled by crabe.toml ([multiplayer].enabled = false).");
    } else if (decision != crabe::domain::LoadDecision::Supported) {
        logger.warning("Loader: multiplayer left uninitialised ({}); its patches are known by "
                       "address only and this build is not a measured one.",
                       crabe::domain::describe(decision));
    } else {
        crabe::multiplayer::application::MultiplayerManager::getInstance().initialize();
    }
#endif

    return true;
}

void Loader::uninitialize()
{
    crabe::infrastructure::portal::PortalService::Instance().Shutdown();
#if CRABELOADER_WITH_MULTIPLAYER
    crabe::multiplayer::application::MultiplayerManager::getInstance().uninitialize();
#endif
    crabe::infrastructure::HttpRedirect::get().uninitialize();
    crabe::infrastructure::EngineAssetLoader::get().uninitialize();
    crabe::infrastructure::VfsHook::get().uninitialize();
    crabe::infrastructure::VfsOverrideManager::get().clear();
    crabe::presentation::InputHook::get().uninitialize();
    crabe::presentation::DirectInputHook::get().uninitialize();
    crabe::infrastructure::EngineFreeCamera::get().uninitialize();
    crabe::infrastructure::EngineGateway::get().uninitialize();
    crabe::infrastructure::EngineMeridian::get().uninitialize();
    crabe::infrastructure::EngineShaders::get().uninitialize();
    crabe::infrastructure::EngineResolution::get().uninitialize();
    crabe::infrastructure::MessageHook::get().uninitialize();
    crabe::presentation::RenderHook::get().uninitialize();
    crabe::infrastructure::LuaCall::get().uninitialize();
}

} // namespace crabe::application

