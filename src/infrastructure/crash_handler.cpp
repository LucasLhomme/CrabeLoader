#include "infrastructure/crash_handler.hpp"
#include "shared/logger.hpp"

#include <windows.h>

namespace {

struct ExceptionDetails {
    DWORD code = 0;
    void* address = nullptr;
};

/// Safely extracts exception code and instruction address from exception pointers.
/// Safely extracts exception code and instruction address.
int exceptionFilter(struct _EXCEPTION_POINTERS* ep, ExceptionDetails* details)
{
    if (ep && ep->ExceptionRecord && details) {
        details->code = ep->ExceptionRecord->ExceptionCode;
        details->address = ep->ExceptionRecord->ExceptionAddress;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

/// Invokes the callable inside an MSVC SEH block, trapping hardware exceptions.
/// Invokes callable inside MSVC SEH block, trapping hardware faults.
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
bool CrashHandler::runGuarded(const std::function<void()>& action, const char* contextLabel)
{
    if (!action) {
        return false;
    }
    ExceptionDetails details{};
    auto invoker = [](void* ctx) {
        const auto* fn = static_cast<const std::function<void()>*>(ctx);
        (*fn)();
    };
    if (!executeGuardedSeh(invoker, const_cast<void*>(static_cast<const void*>(&action)), &details)) {
        const char* label = contextLabel ? contextLabel : "Unknown";
        Logger::getInstance().error(
            "CrashHandler: [{}] Exception 0x{:08X} at address {}",
            label,
            details.code,
            details.address);
        return false;
    }
    return true;
}
