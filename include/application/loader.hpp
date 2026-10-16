/*
** CrabeLoader
** File description:
** Owns the loader lifecycle: Lua state adoption, keybind registration, and the mod work queue.
** Everything here runs on the game script thread; the render thread never enters this header.
** Discovers no mod and orders none -- that is domain/mod_manager.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef LOADER_HPP_
#define LOADER_HPP_
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace crabe::application {

class Loader {
    public:
        static Loader& get();

        bool initialize();
        void uninitialize();
        std::mutex _stateMutex;
        void onLuaState(void *L);
        void onLoadmods();
        void registerKeybind(int virtualKey, std::function<void()> onPress);
        // Handles key events; isRepeat indicates a held key from Windows.
        void onKeyEvent(int virtualKey, bool isDown, bool isRepeat = false);

        // Sets virtual keys swallowed by the window procedure.
        void setCapturedKeys(std::vector<int> virtualKeys);
        [[nodiscard]] bool isKeyCaptured(int virtualKey) const;
        [[nodiscard]] void* getLuaState() const noexcept { return _runtimeReady.load() ? _luaState : nullptr; }
        [[nodiscard]] bool isRuntimeReady() const noexcept { return _runtimeReady.load(); }
        void queueLuaCall(const std::string& luaFunctionName);
        void drainPendingKeybindCalls(void* L);

        // Emits key presses recorded by the window thread.
        void drainPendingKeyEvents(void* L);
        void queueConsoleSnippet(const std::string& code);
        void drainPendingSnippets(void* L);

        // Queues a Lua snippet sent by a native plugin through the plugin API.
        void queuePluginSnippet(const std::string& owner, std::string code);
        void drainPluginSnippets(void* L);
        void drainLuaOutput(void* L);

        // Polls crabe_remote_cmd.txt for external commands.
        void drainRemoteCommandFile(void* L);

        // Polls quarantine report and updates active configuration.
        void drainQuarantineReport(void* L);
        void ensureRuntimeReady(void* L);
        void runTicks(void* L);
        bool isGameState(void* L) const;

        struct ChunkRule {
            std::string key;
            std::string source;
            std::string label;
        };

        // Replaces content-matched loadbuffer chunk before compilation.
        void registerLoadOverride(std::string matchSubstring, std::string replacement,
                                  std::string label = {});
        void clearLoadOverrides();
        const std::string* findLoadOverride(const char* buff, size_t size) const;

        // Registers patch source to run after matching chunk returns.
        void registerChunkPatch(std::string matchSubstring, std::string patchSource,
                                std::string label = {});
        // Arms chunk patch matched by content and call depth.
        void armPatchIfMatched(const char* buff, size_t size, int depth);
        // Arms chunk patch matched by name and call depth.
        void armPatchIfNameMatched(const char* name, int depth);

        // Returns patch rule armed for the given call depth.
        std::optional<ChunkRule> takePatchForDepth(int depth);

        // Registers chunk patch keyed on exact loadbuffer name.
        void registerNamedPatch(std::string exactName, std::string patchSource,
                                std::string label = {});

    protected:
    private:
        Loader() = default;
        ~Loader() = default;
        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
        bool isInjected();
        void registerDefaultKeybinds();
        void registerLuaKeybind(int virtualKey, const std::string& luaFunctionName);

        // Reads skilltrees/*.lua and patch files from disk.
        void loadOverridesFromDisk();

        // Reads character definitions and generates gateway registry patch.
        void loadCharactersFromDisk();

        struct Keybind {
            std::function<void()> onPress;
            bool wasDown = false;
        };

        void* _luaState = nullptr;
        std::atomic<bool> _modsLoaded{false};
        std::unordered_map<int, Keybind> _keybinds;
        std::mutex _keybindsMutex;
        std::vector<std::string> _pendingLuaCalls;
        std::mutex _luaCallQueueMutex;
        std::vector<std::string> _pendingSnippets;
        std::mutex _snippetQueueMutex;
        std::vector<std::pair<std::string, std::string>> _pendingPluginSnippets;
        std::mutex _pluginSnippetQueueMutex;
        static constexpr std::size_t kMaxPendingKeyEvents = 64;
        struct PendingKeyEvent {
            int virtualKey;
            bool isRepeat;
        };
        std::vector<PendingKeyEvent> _pendingKeyEvents;
        std::mutex _keyEventQueueMutex;
        std::unordered_set<int> _capturedKeys;
        mutable std::mutex _capturedKeysMutex;
        std::chrono::steady_clock::time_point _lastOutputDrain{};
        std::chrono::steady_clock::time_point _lastReadyProbe{};
        std::chrono::steady_clock::time_point _lastTick{};
        std::chrono::steady_clock::time_point _lastRemoteCommandProbe{};
        std::chrono::steady_clock::time_point _lastQuarantinePoll{};
        std::atomic<bool> _runtimeReady{false};
        bool _sawForeignState = false;
        std::unordered_set<void*> _initializedStates;
        std::unordered_set<void*> _rejectedStates;

        std::vector<ChunkRule> _loadOverrides;
        std::vector<ChunkRule> _chunkPatches;
        std::vector<ChunkRule> _namedPatches;
        std::string _characterInjectionScript;

        struct ArmedPatch {
            ChunkRule rule;
            int depth{0};
        };
        std::vector<ArmedPatch> _armedPatches;
};


} // namespace crabe::application

#endif /* !LOADER_HPP_ */
