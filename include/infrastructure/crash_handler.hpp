#ifndef CRASH_HANDLER_HPP_
#define CRASH_HANDLER_HPP_

#include <functional>

class CrashHandler final {
public:
    CrashHandler() = delete;

    /// Runs an action inside an SEH try-except guard to catch hardware faults.
    static bool runGuarded(const std::function<void()>& action, const char* contextLabel);
};

#endif
