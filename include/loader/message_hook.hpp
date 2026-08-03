/*
** CrabeLoader
** File description:
** message_hook
*/

#ifndef MESSAGE_HOOK_HPP_
#define MESSAGE_HOOK_HPP_

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "loader/hook.hpp"

// Traces the engine's named-message dispatcher (the bus System_StartButtonPushed
// posts to, alongside "DropInBlocked"). Observer only: nothing is recorded
// unless it matches a watch prefix.
class MessageHook {
    public:
        static MessageHook& get();

        bool initialize();
        void uninitialize();

        // Only messages whose name contains one of these substrings are kept.
        void watch(const std::string& substring);
        void clear();

        // Recorded messages, oldest first, as "name(arg1,arg2)".
        std::string report() const;

    private:
        MessageHook() = default;
        ~MessageHook() = default;
        MessageHook(const MessageHook&) = delete;
        MessageHook& operator=(const MessageHook&) = delete;

        // thiscall: `this` arrives in ecx, so __fastcall with a dummy edx is
        // the only way to express it. Argument order comes from the call site
        // in System_StartButtonPushed (pushed right to left: 0, id, name).
        typedef void(__fastcall* t_dispatch)(void* self, void* edx, const char* name, int arg1, int arg2);
        static void __fastcall hkDispatch(void* self, void* edx, const char* name, int arg1, int arg2);

        // Resolves the dispatcher by following System_StartButtonPushed's 4th
        // call, rather than hardcoding an RVA that a patch would move.
        static uintptr_t resolveDispatcher();

        Hook _hook;
        mutable std::mutex _mutex;
        std::vector<std::string> _watched;
        std::vector<std::string> _recorded;
};

#endif /* !MESSAGE_HOOK_HPP_ */
