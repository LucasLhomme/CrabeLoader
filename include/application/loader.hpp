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
        void onKeyEvent(int virtualKey, bool isDown);

        // Virtual keys the window procedure must swallow instead of forwarding
        // to the game. Which keys those are is the caller's business, so a mod
        // navigating a list can stop it from also steering the player.
        void setCapturedKeys(std::vector<int> virtualKeys);
        [[nodiscard]] bool isKeyCaptured(int virtualKey) const;
        [[nodiscard]] void* getLuaState() const noexcept { return _runtimeReady.load() ? _luaState : nullptr; }
        [[nodiscard]] bool isRuntimeReady() const noexcept { return _runtimeReady.load(); }
        void queueLuaCall(const std::string& luaFunctionName);
        void drainPendingKeybindCalls(void* L);

        // Emits key presses recorded by the window thread. Script thread only.
        void drainPendingKeyEvents(void* L);
        void queueConsoleSnippet(const std::string& code);
        void drainPendingSnippets(void* L);
        void drainLuaOutput(void* L);

        // Polls crabe_remote_cmd.txt for a new command and queues it as if
        // typed into the overlay console -- lets an external process drive it.
        void drainRemoteCommandFile(void* L);

        // Polls Crabe.Quarantine.report() (src/api/02c_quarantine.lua) the
        // same way drainLuaOutput polls Crabe.flush(), and folds any change
        // into domain::Config::active()'s [quarantine] section -- the
        // overlay reads that same snapshot. Script thread only.
        void drainQuarantineReport(void* L);
        void ensureRuntimeReady(void* L);
        void runTicks(void* L);
        bool isGameState(void* L) const;

        // One rule against one chunk. `label` names the rule in the log --
        // "skilltrees/hulk", "characters/", "figure registry" -- since `key`
        // may be binary and tells a reader nothing.
        struct ChunkRule {
            std::string key;
            std::string source;
            std::string label;
        };

        // Replaces the next content-matched loadbuffer chunk outright, before
        // compilation. See skilltrees/README.md.
        void registerLoadOverride(std::string matchSubstring, std::string replacement,
                                  std::string label = {});
        void clearLoadOverrides();
        const std::string* findLoadOverride(const char* buff, size_t size) const;

        // Runs `patchSource` against the next content-matched chunk right
        // after that chunk's own call returns. Not limited to skill trees --
        // the figure registry is injected this way too. See skilltrees/README.md.
        void registerChunkPatch(std::string matchSubstring, std::string patchSource,
                                std::string label = {});
        // Arming and consuming both carry the Lua call depth, because chunks
        // nest and a single armed slot cannot tell whose call just returned.
        //
        // Presentation/SettingsVideo.lua loads Presentation/SettingsBase.lua
        // while it is running. With one slot consumed by the next call to
        // return, SettingsVideo's patch fired after SettingsBase -- before
        // SettingsVideo.lua had finished, so before the table it builds was
        // assigned to its global. Every mod trying to reach that screen found
        // nothing, silently, and the patch was gone by the time the chunk
        // really returned.
        //
        // A chunk armed at depth D runs under a call that takes the depth to
        // D+1, so its patch belongs to the return that brings it back to D.
        void armPatchIfMatched(const char* buff, size_t size, int depth);
        void armPatchIfNameMatched(const char* name, int depth);

        // The patch owed to a call returning to `depth`, or nothing. Anything
        // armed deeper can no longer fire -- its chunk never ran -- and is
        // dropped here rather than left to accumulate.
        std::optional<ChunkRule> takePatchForDepth(int depth);

        // Same as registerChunkPatch/armPatchIfMatched, keyed on the chunk's
        // exact loadbuffer name instead of its content.
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

        // Reads <gameDir>/skilltrees/*.{lua,patch} into overrides/patches,
        // before LuaCall installs the loadbuffer hook. See skilltrees/README.md.
        void loadOverridesFromDisk();

        // Reads <gameDir>/characters/*.lua and mods/<name>/characters/*.lua into
        // the catalog patch and the figure registry slots the game needs to
        // accept those characters. Same timing constraint as the above: it must
        // run before LuaCall installs the loadbuffer hook. See characters/README.md.
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
        // Bounded: a key held down while the script thread is stalled must not
        // grow this queue without limit.
        static constexpr std::size_t kMaxPendingKeyEvents = 64;
        std::vector<int> _pendingKeyEvents;
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
        // States that answered the probe and are not the game's. Their answer
        // cannot change, so they are never probed again.
        std::unordered_set<void*> _rejectedStates;

        std::vector<ChunkRule> _loadOverrides;
        std::vector<ChunkRule> _chunkPatches;
        std::vector<ChunkRule> _namedPatches;

        // Armed patches that have not fired yet, innermost last. Bounded by
        // the nesting depth of the game's own chunk loading, which is two.
        struct ArmedPatch {
            ChunkRule rule;
            int depth{0};
        };
        std::vector<ArmedPatch> _armedPatches;
};


} // namespace crabe::application

#endif /* !LOADER_HPP_ */
