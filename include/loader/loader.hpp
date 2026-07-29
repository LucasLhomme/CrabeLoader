/*
** CrabeLoader
** File description:
** loader
*/

#ifndef LOADER_HPP_
#define LOADER_HPP_
#include <atomic>
#include <mutex>

class Loader {
    public:
        static Loader& get();

        bool initialize();
        void uninitialize();
        std::mutex _StateMutex;
        void onLuaState(void *L);
        void onLoadmods();

    protected:
    private:
        Loader() = default;
        ~Loader() = default;
        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
        bool _isInjected();

        void* _luaState = nullptr;
        std::atomic<bool> _modsLoaded{false};
};


#endif /* !LOADER_HPP_ */
