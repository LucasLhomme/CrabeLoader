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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace crabe::infrastructure {

    enum class ThreadRole : std::uint8_t {
        Unknown = 0,
        Script,
        Render,
        Window,
        Worker,
    };

    // Returns a short label describing the thread role.
    [[nodiscard]] const char* describe(ThreadRole role) noexcept;

    enum class FilterDecision : std::uint8_t {
        Report,
        PassThrough,
    };

    // Classifies an exception code to decide whether it should be reported or passed through.
    [[nodiscard]] FilterDecision classifyException(std::uint32_t code,
                                                   bool debuggerPresent) noexcept;

    // Returns the human-readable name of an exception code, or UNKNOWN.
    [[nodiscard]] const char* exceptionName(std::uint32_t code) noexcept;

    class BreadcrumbRing final {
    public:
        static constexpr std::size_t kCapacity = 64;
        static constexpr std::size_t kTextCapacity = 96;

        // Pushes a breadcrumb entry for the specified thread.
        void push(std::uint32_t threadId, std::string_view text) noexcept;

        // Pushes a two-part concatenated breadcrumb entry for the specified thread.
        void push(std::uint32_t threadId, std::string_view text,
                  std::string_view detail) noexcept;

        // Returns the number of breadcrumbs currently held in the ring buffer.
        [[nodiscard]] std::size_t recorded() const noexcept;

        // Returns the total number of breadcrumbs pushed since creation.
        [[nodiscard]] std::uint32_t pushed() const noexcept;

        // Returns the breadcrumb text at the given index.
        [[nodiscard]] const char* text(std::size_t index) const noexcept;

        // Returns the thread ID associated with the breadcrumb at the given index.
        [[nodiscard]] std::uint32_t threadId(std::size_t index) const noexcept;

        // Clears all recorded breadcrumbs.
        void clear() noexcept;

    private:
        struct Slot {
            std::uint32_t threadId{0};
            char text[kTextCapacity]{};
        };

        Slot _slots[kCapacity]{};
        std::atomic<std::uint32_t> _pushed{0};
    };

    class CrashReporter final {
    public:
        static constexpr std::size_t kLabelCapacity = 64;
        static constexpr std::size_t kMaxReports = 3;
        static constexpr std::size_t kMaxRegisteredMods = 64;

        CrashReporter() = delete;

        // Installs the crash reporter handlers and initializes report directory.
        static bool install(const char* reportDirectory) noexcept;

        // Completes crash reporter initialization outside of loader lock.
        static void completeInstall() noexcept;

        // Uninstalls crash reporter handlers.
        static void uninstall() noexcept;

        // Sets the active game profile metadata for crash reporting.
        static void setGameProfile(std::string_view id, std::string_view displayName,
                                   std::uint32_t timeDateStamp, std::uint32_t sizeOfImage,
                                   std::uint32_t checkSum) noexcept;

        // Sets the active mod identifier for the calling thread.
        static void setActiveMod(std::string_view modId) noexcept;

        // Sets the active hook label for the calling thread.
        static void setActiveHook(std::string_view hookLabel) noexcept;

        // Appends a breadcrumb tagged with the calling thread ID.
        static void pushBreadcrumb(std::string_view text) noexcept;
        static void pushBreadcrumb(std::string_view text, std::string_view detail) noexcept;

        // Declares the role of the calling thread.
        static void declareThreadRole(ThreadRole role) noexcept;

        // Copies the active hook label of the calling thread into the output buffer.
        static void currentHook(char* out, std::size_t capacity) noexcept;

        // Registers a mod entry for crash report diagnostics.
        static void registerMod(std::string_view id, std::string_view version,
                                bool loaded) noexcept;

        // Clears registered mods from the crash reporter.
        static void forgetMods() noexcept;

        [[nodiscard]] static BreadcrumbRing& breadcrumbs() noexcept;
        [[nodiscard]] static std::size_t reportsWritten() noexcept;

        // Returns the path of the last generated report, or empty if none.
        [[nodiscard]] static const char* lastReportPath() noexcept;

        // Returns the path of the last generated crash dump, or empty if none.
        [[nodiscard]] static const char* lastDumpPath() noexcept;
    };

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
