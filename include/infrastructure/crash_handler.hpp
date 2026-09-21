/*
** CrabeLoader
** File description:
** Declares the SEH guard that lets a hardware fault be survived instead of ending the game.
** MSVC forbids C++ unwinding in a function holding __try, hence this deliberately thin shell.
** Writes no report; what gets recorded is infrastructure/crash_reporter.hpp.
**
** Authors: @LucasLhomme
*/

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
