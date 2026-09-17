/*
** CrabeLoader
** File description:
** message_hook
*/

#include <cstring>
#include <format>
#include <windows.h>

#include "infrastructure/memory.hpp"
#include "infrastructure/message_hook.hpp"
#include "shared/logger.hpp"

namespace {
    constexpr size_t kMaxRecorded = 200;
}

MessageHook& MessageHook::get()
{
    static MessageHook instance;
    return instance;
}

// Registered by emitted code (mov eax,<fn>; xor ecx,ecx; push <name>), so the
// native's address sits 6 bytes before the push naming it.
uintptr_t MessageHook::resolveDispatcher()
{
    for (uintptr_t nameAddr = Memory::findString("System_StartButtonPushed"); nameAddr;
        nameAddr = Memory::findString("System_StartButtonPushed", nameAddr)) {

        char pattern[32];
        std::snprintf(pattern, sizeof(pattern), "68 %02X %02X %02X %02X",
                    static_cast<unsigned>(nameAddr & 0xFF),
                    static_cast<unsigned>((nameAddr >> 8) & 0xFF),
                    static_cast<unsigned>((nameAddr >> 16) & 0xFF),
                    static_cast<unsigned>((nameAddr >> 24) & 0xFF));

        for (uintptr_t push = Memory::patternScan(pattern); push;
            push = Memory::patternScan(pattern, nullptr, push)) {

            if (!Memory::isReadable(push - 7, 7)) continue;
            if (*reinterpret_cast<const uint8_t*>(push - 7) != 0xB8) continue;

            auto native = *reinterpret_cast<const uintptr_t*>(push - 6);
            if (!Memory::isReadable(native, 64)) continue;

            std::vector<uintptr_t> calls = Memory::findCalls(native, 64);
            if (calls.size() >= 4) return calls[3];
        }
    }
    return 0;
}

bool MessageHook::initialize()
{
    Logger& logger = Logger::getInstance();

    uintptr_t dispatcher = resolveDispatcher();
    if (!dispatcher) {
        logger.warning("MessageHook: could not resolve the message dispatcher; tracing disabled.");
        return false;
    }

    if (!_hook.install(reinterpret_cast<void*>(dispatcher), reinterpret_cast<void*>(&MessageHook::hkDispatch))) {
        logger.error("MessageHook: failed to hook the dispatcher at 0x{:X}.", dispatcher);
        return false;
    }

    logger.info("MessageHook: tracing message dispatch @ 0x{:X} (RVA 0x{:X}).",
                dispatcher, dispatcher - reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)));
    return true;
}

void MessageHook::uninitialize()
{
    _hook.remove();
}

void MessageHook::watch(const std::string& substring)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _watched.push_back(substring);
    _watchCount.store(_watched.size(), std::memory_order_relaxed);
}

void MessageHook::clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _recorded.clear();
}

void __fastcall MessageHook::hkDispatch(void* self, void* edx, const char* name, int arg1, int arg2)
{
    MessageHook& hook = get();
    auto original = reinterpret_cast<t_dispatch>(hook._hook.getOriginal());

    // Nothing watched is the normal case, and this runs on every message the
    // engine dispatches -- return before the isReadable syscall and the lock.
    if (hook._watchCount.load(std::memory_order_relaxed) == 0) {
        if (original) original(self, edx, name, arg1, arg2);
        return;
    }

    if (name && Memory::isReadable(reinterpret_cast<uintptr_t>(name), 1)) {
        std::lock_guard<std::mutex> lock(hook._mutex);

        for (const auto& needle : hook._watched) {
            if (!std::strstr(name, needle.c_str())) continue;
            if (hook._recorded.size() < kMaxRecorded)
                hook._recorded.push_back(std::format("{}({},{})", name, arg1, arg2));
            break;
        }
    }

    if (original) original(self, edx, name, arg1, arg2);
}

std::string MessageHook::report() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::string out;
    for (size_t i = 0; i < _recorded.size(); ++i) {
        if (i) out += " | ";
        out += _recorded[i];
    }
    return out;
}
