#ifndef CRASH_HANDLER_HPP_
#define CRASH_HANDLER_HPP_

#include <functional>

namespace crabe::infrastructure {

class CrashHandler final {
public:
    CrashHandler() = delete;

    /// Runs an action inside an SEH try-except guard to catch hardware faults.
    static bool runGuarded(const std::function<void()>& action, const char* contextLabel);
};

} // namespace crabe::infrastructure

#endif
