/*
** CrabeLoader
** File description:
** Declares what the loader writes down when the game dies: module+RVA frames plus context.
** Installs as a vectored handler and allocates nothing at fault time; buffers are preallocated.
** Cannot name a mod faulting inside draw: Lua picks the mod there, so C++ never learns which.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_CRASH_REPORTER_HPP_
#define CRABELOADER_INFRASTRUCTURE_CRASH_REPORTER_HPP_

// What the loader writes down when the game dies under it.
//
// The old handler logged an exception code and an absolute address and then
// terminated. Neither number survives contact with reality: the loader is a
// relocatable DLL, so its addresses differ every launch, and a bare code says
// nothing about which mod, which hook or which thread was running. This file
// replaces both halves of that -- where the handler is installed, and what it
// is able to say.
//
// Two rules shape everything below.
//
//  * The handler runs inside a broken process. The heap may be corrupt, the
//    loader lock may be held by the faulting thread, and another thread may be
//    halfway through a std::vector resize. So nothing here allocates: every
//    buffer, every path and every table is sized at compile time and filled at
//    install time. That is why the context API takes std::string_view and
//    copies into fixed storage instead of keeping std::string, and why the
//    module table is a PEB walk rather than a Toolhelp snapshot.
//
//  * The handler must not be the loudest thing in the process. A vectored
//    handler at priority 1 sees *every* exception raised anywhere in the game,
//    including the ones the game's own code raises and handles as a matter of
//    course. classifyException() below is the whole policy, stated once, pure,
//    and unit-tested -- because a filter that is wrong in the permissive
//    direction turns this file into a performance sink that writes minidumps
//    during normal play.
//
// The split mirrors domain/game_profile.hpp: classifyException(),
// exceptionName() and BreadcrumbRing are pure and reachable from a test with
// no process to crash, while CrashReporter is the Win32 half.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace crabe::infrastructure {

    // Which of the loader's threads is running.
    //
    // The loader has three that matter and they are not interchangeable:
    // invariant I2 says Lua is only ever touched from the script thread, I3
    // says Direct3D is only ever touched from the render thread. A crash
    // report that does not name the thread cannot tell a genuine fault from a
    // violated invariant, so a thread says which it is (declareThreadRole)
    // rather than the reporter guessing from a thread id.
    enum class ThreadRole : std::uint8_t {
        // No declareThreadRole() call was ever made on this thread. Every
        // thread the game itself created is Unknown, and that is correct:
        // those are not ours.
        Unknown = 0,
        Script,  // the detached thread started from DllMain; runs Lua
        Render,  // inside Present; the only thread allowed to touch D3D
        Window,  // inside the hooked WndProc
        Worker,  // the multiplayer patch worker and anything else of ours
    };

    // A short label for a report line, e.g. "Script".
    [[nodiscard]] const char* describe(ThreadRole role) noexcept;

    // What the vectored handler does with one exception.
    enum class FilterDecision : std::uint8_t {
        // Write a crash report, then let the exception carry on being handled
        // exactly as it would have been. Reporting never swallows.
        Report,

        // Not ours. Return EXCEPTION_CONTINUE_SEARCH without touching a thing.
        PassThrough,
    };

    // The filter policy, in full.
    //
    // PassThrough for:
    //
    //  * every code below 0x80000000. The top two bits of an NTSTATUS are its
    //    severity; below 0x80000000 means Success or Informational, which is
    //    not a fault. This one rule already covers the whole debugger channel:
    //    DBG_PRINTEXCEPTION_C (0x40010006), its wide sibling (0x4001000A),
    //    DBG_CONTROL_C (0x40010005) and the MS_VC_EXCEPTION thread-naming
    //    exception (0x406D1388) that every profiler and half the middleware in
    //    a game raises constantly.
    //
    //  * C++ exceptions (0xE06D7363). These are a language feature with their
    //    own handler already on the stack. A game that throws one per frame is
    //    doing nothing wrong, and reporting them would be both wrong and
    //    ruinous.
    //
    //  * CLR exceptions (0xE0434352), for the same reason and no other: it is
    //    0xE06D7363's exact analogue for managed code, raised and caught
    //    entirely inside a runtime that is not ours. In-process .NET is
    //    unlikely in this game, but an overlay or an injected tool brings it,
    //    and a managed `throw` is not a crash.
    //
    //  * EXCEPTION_BREAKPOINT (0x80000003) and EXCEPTION_SINGLE_STEP
    //    (0x80000004) *when a debugger is attached*, and only then. Under a
    //    debugger these are the debugger talking, several per second; without
    //    one, an INT3 that reaches a vectored handler is a real abort -- an
    //    assertion or a __debugbreak() -- and is worth a report.
    //
    // Everything else at severity Warning or Error is reported. Note what is
    // deliberately *not* on the pass-through list: no attempt is made to guess
    // whether an access violation is "one the game handles". Some code does
    // fault on purpose and recover, and if this game turns out to, the report
    // budget (CrashReporter::kMaxReports) is what bounds the damage -- not a
    // heuristic that would also swallow the crash we are here for.
    [[nodiscard]] FilterDecision classifyException(std::uint32_t code,
                                                   bool debuggerPresent) noexcept;

    // "EXCEPTION_ACCESS_VIOLATION" and friends; "UNKNOWN" for a code with no
    // name here. Never null, never allocates -- these are string literals.
    [[nodiscard]] const char* exceptionName(std::uint32_t code) noexcept;

    // The last kCapacity things the loader said it was doing.
    //
    // Fixed slots of fixed-length text, written in place. push() costs one
    // relaxed fetch_add and one bounded memcpy: no allocation, no lock, and
    // nothing for a crashing process to have to unwind. Text longer than
    // kTextCapacity - 1 is truncated rather than split or dropped, because a
    // truncated crumb still says which one it was.
    //
    // Concurrent push() from several threads can interleave -- two threads can
    // land in the same slot and one overwrite the other. That is accepted: the
    // alternative is a lock on a path that runs every frame, and a crash
    // report is evidence, not an audit log.
    class BreadcrumbRing final {
    public:
        // A power of two, which is what makes the index arithmetic below stay
        // correct when the 32-bit counter wraps: 2^32 is a whole number of
        // laps around the ring.
        static constexpr std::size_t kCapacity = 64;
        static constexpr std::size_t kTextCapacity = 96; // bytes, NUL included

        void push(std::uint32_t threadId, std::string_view text) noexcept;

        // Two-part form. The parts are concatenated straight into the slot, so
        // a caller can tag a crumb with a name it already holds -- a mod id, a
        // file name -- without building a std::string to do it.
        void push(std::uint32_t threadId, std::string_view text,
                  std::string_view detail) noexcept;

        // How many crumbs are still held: min(pushed(), kCapacity).
        [[nodiscard]] std::size_t recorded() const noexcept;

        // How many were ever pushed, so a report can say how many it lost.
        [[nodiscard]] std::uint32_t pushed() const noexcept;

        // Oldest first: index 0 is the oldest crumb still held. Out of range
        // returns "" and 0 rather than reading past the end -- the caller is
        // a crash handler and must not be given a way to fault.
        [[nodiscard]] const char* text(std::size_t index) const noexcept;
        [[nodiscard]] std::uint32_t threadId(std::size_t index) const noexcept;

        void clear() noexcept;

    private:
        struct Slot {
            std::uint32_t threadId{0};
            char text[kTextCapacity]{};
        };

        Slot _slots[kCapacity]{};
        std::atomic<std::uint32_t> _pushed{0};
    };

    // The installed handler, and the context it will print.
    class CrashReporter final {
    public:
        // Fixed storage sizes. A label longer than this is truncated.
        static constexpr std::size_t kLabelCapacity = 64;

        // How many reports one process will ever write. A vectored handler at
        // priority 1 sits in front of CrashHandler::runGuarded, which exists
        // precisely to survive a faulting mod -- so a mod that faults every
        // frame would otherwise have the loader writing a minidump every
        // frame. Three is enough to see a pattern and few enough to be free.
        static constexpr std::size_t kMaxReports = 3;

        // At most this many mods are named in the report.
        static constexpr std::size_t kMaxRegisteredMods = 64;

        CrashReporter() = delete;

        // Installs the vectored handler at priority 1 and, behind it,
        // SetUnhandledExceptionFilter as a secondary net. Priority 1 runs
        // before anything the game installs later and, unlike the unhandled
        // filter, cannot be displaced by the game overwriting it.
        //
        // `reportDirectory` is created if missing and is where crash-<stamp>
        // .txt and .dmp are written. Safe to call from DllMain: it touches no
        // DbgHelp, loads nothing, and walks the module list under the loader
        // lock DllMain already holds.
        static bool install(const char* reportDirectory) noexcept;

        // The half of installation that must NOT run from DllMain:
        // SymInitialize loads and enumerates, which under the loader lock
        // deadlocks. Call it once from the script thread. Without it the
        // report still resolves every frame to module+RVA -- only the function
        // names and line numbers are missing.
        static void completeInstall() noexcept;

        static void uninstall() noexcept;

        // Which build is running, copied into fixed storage so the handler
        // never calls back into domain. Passed in rather than read here so
        // that this file stays free of any dependency on the domain layer and
        // its detection cache, which allocates on first use.
        // An empty `id` means no profile matched, which the report says.
        static void setGameProfile(std::string_view id, std::string_view displayName,
                                   std::uint32_t timeDateStamp, std::uint32_t sizeOfImage,
                                   std::uint32_t checkSum) noexcept;

        // ---- The context API. All three are allocation-free. ----

        // The mod whose code is running on the calling thread, "" for none.
        static void setActiveMod(std::string_view modId) noexcept;

        // The hook or guarded region the calling thread is inside, "" for
        // none. CrashHandler::runGuarded drives this from its own context
        // label, so a guarded call site labels itself exactly once.
        static void setActiveHook(std::string_view hookLabel) noexcept;

        // Appends to the shared ring, tagged with the calling thread.
        static void pushBreadcrumb(std::string_view text) noexcept;
        static void pushBreadcrumb(std::string_view text, std::string_view detail) noexcept;

        // Records what the calling thread is for. Also claims the thread's
        // slot, so the first setActiveMod/setActiveHook on that thread does
        // not have to. The first declaration on a thread wins: Present and the
        // message pump can share a thread, and a report should not depend on
        // which of the two happened to run first that launch.
        static void declareThreadRole(ThreadRole role) noexcept;

        // Copies the calling thread's current hook label into `out`, NUL
        // terminated and truncated to `capacity`. For ScopedHook's restore.
        static void currentHook(char* out, std::size_t capacity) noexcept;

        // The mod list the report prints. Fed at load time rather than read
        // from ModManager at crash time, for two reasons: domain::Mod carries
        // no version at all, and walking a std::vector<Mod> of std::string and
        // std::filesystem::path inside a handler would read a container that
        // another thread may be resizing.
        static void registerMod(std::string_view id, std::string_view version,
                                bool loaded) noexcept;
        static void forgetMods() noexcept;

        // ---- Introspection, for tests and for the log line after a crash. ----

        [[nodiscard]] static BreadcrumbRing& breadcrumbs() noexcept;
        [[nodiscard]] static std::size_t reportsWritten() noexcept;

        // The last report written, or "" if none. Points at fixed storage.
        [[nodiscard]] static const char* lastReportPath() noexcept;
        [[nodiscard]] static const char* lastDumpPath() noexcept;
    };

    // Sets the calling thread's hook label for a scope and puts back whatever
    // was there before, so nested guards do not each have to remember.
    //
    // Cannot be used in a function containing __try/__except (MSVC C2712), which
    // is why CrashHandler::runGuarded keeps its SEH in a separate function.
    class ScopedHook final {
    public:
        explicit ScopedHook(std::string_view label) noexcept;
        ~ScopedHook();

        ScopedHook(const ScopedHook&) = delete;
        ScopedHook& operator=(const ScopedHook&) = delete;
        ScopedHook(ScopedHook&&) = delete;
        ScopedHook& operator=(ScopedHook&&) = delete;

    private:
        char _previous[CrashReporter::kLabelCapacity]{};
    };

    // The same, for the mod currently being run -- except that it clears the
    // label on the way out rather than restoring a previous one. Mod execution
    // does not nest: a mod never runs another mod, so there is never a
    // previous one to go back to, and "cleared" is the honest answer between
    // two mods rather than "still the last one".
    class ScopedMod final {
    public:
        explicit ScopedMod(std::string_view modId) noexcept;
        ~ScopedMod();

        ScopedMod(const ScopedMod&) = delete;
        ScopedMod& operator=(const ScopedMod&) = delete;
        ScopedMod(ScopedMod&&) = delete;
        ScopedMod& operator=(ScopedMod&&) = delete;
    };

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_CRASH_REPORTER_HPP_ */
