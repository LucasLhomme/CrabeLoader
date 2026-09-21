/*
** CrabeLoader
** File description:
** Implements the SEH guard, extracting the exception code and faulting address safely.
** The __try body is its own function because MSVC refuses C++ unwinding in the same frame.
** Writes no report file; that is src/infrastructure/crash_reporter.cpp.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/crash_handler.hpp"
#include "infrastructure/crash_reporter.hpp"
#include "shared/logger.hpp"

#include <windows.h>

namespace crabe::infrastructure {

namespace {

struct ExceptionDetails {
    DWORD code = 0;
    void* address = nullptr;
};

/// Safely extracts exception code and instruction address from exception pointers.
int exceptionFilter(struct _EXCEPTION_POINTERS* ep, ExceptionDetails* details)
{
    if (ep && ep->ExceptionRecord && details) {
        details->code = ep->ExceptionRecord->ExceptionCode;
        details->address = ep->ExceptionRecord->ExceptionAddress;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

/// Invokes the callable inside an MSVC SEH block, trapping hardware exceptions.
bool executeGuardedSeh(void (*callable)(void*), void* context, ExceptionDetails* details)
{
    __try {
        callable(context);
        return true;
    }
    __except (exceptionFilter(GetExceptionInformation(), details)) {
        return false;
    }
}

}

/// Traps hardware faults and logs exception details on failure.
///
/// The context label is not logged twice: it is also what the crash reporter
/// prints as "Active hook", fed from here rather than from each call site, so
/// a guarded region is named exactly once -- at the point that already had to
/// name it. The ScopedHook lives in this function and not in
/// executeGuardedSeh() because MSVC refuses (C2712) to unwind a C++ object out
/// of a function containing __try/__except.
bool CrashHandler::runGuarded(const std::function<void()>& action, const char* contextLabel)
{
    if (!action) {
        return false;
    }
    const ScopedHook scopedHook(contextLabel ? contextLabel : "Unknown");
    ExceptionDetails details{};
    auto invoker = [](void* ctx) {
        const auto* fn = static_cast<const std::function<void()>*>(ctx);
        (*fn)();
    };
    if (!executeGuardedSeh(invoker, const_cast<void*>(static_cast<const void*>(&action)), &details)) {
        const char* label = contextLabel ? contextLabel : "Unknown";
        crabe::shared::Logger::getInstance().error(
            "CrashHandler: [{}] Exception 0x{:08X} at address {}",
            label,
            details.code,
            details.address);
        return false;
    }
    return true;
}

} // namespace crabe::infrastructure

