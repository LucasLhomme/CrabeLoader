/*
** CrabeLoader
** File description:
** loader
*/

#ifndef LOADER_HPP_
#define LOADER_HPP_
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Loader {
    public:
        static Loader& get();

        bool initialize();
        void uninitialize();
        std::mutex _StateMutex;
        void onLuaState(void *L);
        void onLoadmods();
        void RegisterKeybind(int virtualKey, std::function<void()> onPress);
        void QueueLuaCall(const std::string& luaFunctionName);
        void DrainPendingKeybindCalls(void* L);

    protected:
    private:
        Loader() = default;
        ~Loader() = default;
        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
        bool _isInjected();
        void registerDefaultKeybinds();
        void RegisterLuaKeybind(int virtualKey, const std::string& luaFunctionName);
        void HandleKeybind();
        void inputLoop();

        struct Keybind {
            std::function<void()> onPress;
            bool wasDown = false;
        };

        void* _luaState = nullptr;
        std::atomic<bool> _modsLoaded{false};
        std::atomic<bool> _menuOpen{false};
        std::unordered_map<int, Keybind> _keybinds;
        std::mutex _keybindsMutex; // guards _keybinds: RegisterKeybind() may be called after inputLoop() has started
        std::vector<std::string> _pendingLuaCalls;
        std::mutex _luaCallQueueMutex;
};


#endif /* !LOADER_HPP_ */
