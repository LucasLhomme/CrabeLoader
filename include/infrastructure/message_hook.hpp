/*
** CrabeLoader
** File description:
** Declares a trace of the engine named-message dispatcher, filtered by watch prefix.
** Observer only: it records a matching name and always forwards to the real dispatcher.
** Finds the dispatcher by byte pattern, because the game exports nothing useful here.
**
** Authors: @LucasLhomme
*/

#ifndef MESSAGE_HOOK_HPP_
#define MESSAGE_HOOK_HPP_

#include <cstdint>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

class MessageHook {
    public:
        // Returns the singleton instance of MessageHook.
        static MessageHook& get();

        // Initializes and installs the named-message dispatcher hook.
        bool initialize();

        // Uninitializes and removes the dispatcher hook.
        void uninitialize();

        // Adds a substring filter for messages to observe.
        void watch(const std::string& substring);

        // Clears all watched filters and recorded messages.
        void clear();

        // Returns recorded messages in chronological order.
        std::string report() const;

    private:
        MessageHook() = default;
        ~MessageHook() = default;
        MessageHook(const MessageHook&) = delete;
        MessageHook& operator=(const MessageHook&) = delete;

        typedef void(__fastcall* t_dispatch)(void* self, void* edx, const char* name, int arg1, int arg2);

        // Detour for the engine's named-message dispatcher.
        static void __fastcall hkDispatch(void* self, void* edx, const char* name, int arg1, int arg2);

        // Resolves the dispatcher function address by following known engine calls.
        static uintptr_t resolveDispatcher();

        Hook _hook;
        mutable std::mutex _mutex;
        std::vector<std::string> _watched;
        std::atomic<size_t> _watchCount{0};
        std::vector<std::string> _recorded;
};

} // namespace crabe::infrastructure

#endif /* !MESSAGE_HOOK_HPP_ */
