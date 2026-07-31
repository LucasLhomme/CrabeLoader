/*
** CrabeLoader
** File description:
** loader
*/

#ifndef LOADER_HPP_
#define LOADER_HPP_
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Loader {
    public:
        static Loader& get();

        bool initialize();
        void uninitialize();
        std::mutex _stateMutex;
        void onLuaState(void *L);
        void onLoadmods();
        void registerKeybind(int virtualKey, std::function<void()> onPress);
        void queueLuaCall(const std::string& luaFunctionName);
        void drainPendingKeybindCalls(void* L);
        void queueConsoleSnippet(const std::string& code);
        void drainPendingSnippets(void* L);
        void drainLuaOutput(void* L);
        void ensureRuntimeReady(void* L);
        void runTicks(void* L);
        bool isGameState(void* L) const;

    protected:
    private:
        Loader() = default;
        ~Loader() = default;
        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
        bool isInjected();
        void registerDefaultKeybinds();
        void registerLuaKeybind(int virtualKey, const std::string& luaFunctionName);
        void handleKeybind();
        void inputLoop();

        struct Keybind {
            std::function<void()> onPress;
            bool wasDown = false;
        };

        void* _luaState = nullptr;
        std::atomic<bool> _modsLoaded{false};
        std::unordered_map<int, Keybind> _keybinds;
        std::mutex _keybindsMutex; // guards _keybinds: registerKeybind() may be called after inputLoop() has started
        std::vector<std::string> _pendingLuaCalls;
        std::mutex _luaCallQueueMutex;
        std::vector<std::string> _pendingSnippets;
        std::mutex _snippetQueueMutex;
        std::chrono::steady_clock::time_point _lastOutputDrain{};
        std::chrono::steady_clock::time_point _lastReadyProbe{};
        std::chrono::steady_clock::time_point _lastTick{};
        std::atomic<bool> _runtimeReady{false};
        bool _sawForeignState = false;
        std::unordered_set<void*> _initializedStates;
};


#endif /* !LOADER_HPP_ */
