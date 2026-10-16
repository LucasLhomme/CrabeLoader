/*
** CrabeLoader
** File description:
** The Win32 half of the reporter: handler, stack walk, module list, report text and minidump.
** The module list comes from a PEB walk, not Toolhelp, which would take the loader lock.
** Classifies nothing on its own; the pure decision is classifyException in the header.
**
** Authors: @LucasLhomme
*/
#include "infrastructure/crash_reporter.hpp"
#include "infrastructure/hook_registry.hpp"
#include "shared/build_info.hpp"

#include <windows.h>
#include <dbghelp.h>
#include <intrin.h>

#include <cstddef>
#include <cstring>
#include <memory>

#if !defined(_M_IX86)
#error "crash_reporter.cpp targets Win32 (x86): CrabeLoader ships as a 32-bit proxy DLL, \
the register dump names EAX/EIP/EBP and the PEB is read from fs:[0x30]."
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "dbghelp.lib")
#endif

namespace crabe::infrastructure {

namespace {

    constexpr std::uint32_t kCppException = 0xE06D7363u;
    constexpr std::uint32_t kClrException = 0xE0434352u;
    constexpr std::uint32_t kFaultSeverityFloor = 0x80000000u;

} // namespace

const char* describe(ThreadRole role) noexcept
{
    switch (role) {
        case ThreadRole::Script: return "Script";
        case ThreadRole::Render: return "Render";
        case ThreadRole::Window: return "Window";
        case ThreadRole::Worker: return "Worker";
        case ThreadRole::Unknown: break;
    }
    return "Unknown";
}

FilterDecision classifyException(std::uint32_t code, bool debuggerPresent) noexcept
{
    if (code < kFaultSeverityFloor)
        return FilterDecision::PassThrough;

    if (code == kCppException || code == kClrException)
        return FilterDecision::PassThrough;

    if (debuggerPresent && (code == static_cast<std::uint32_t>(EXCEPTION_BREAKPOINT) ||
                            code == static_cast<std::uint32_t>(EXCEPTION_SINGLE_STEP)))
        return FilterDecision::PassThrough;

    return FilterDecision::Report;
}

const char* exceptionName(std::uint32_t code) noexcept
{
    switch (code) {
        case 0xC0000005u: return "EXCEPTION_ACCESS_VIOLATION";
        case 0xC000008Cu: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case 0x80000003u: return "EXCEPTION_BREAKPOINT";
        case 0x80000002u: return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case 0xC000008Du: return "EXCEPTION_FLT_DENORMAL_OPERAND";
        case 0xC000008Eu: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case 0xC000008Fu: return "EXCEPTION_FLT_INEXACT_RESULT";
        case 0xC0000090u: return "EXCEPTION_FLT_INVALID_OPERATION";
        case 0xC0000091u: return "EXCEPTION_FLT_OVERFLOW";
        case 0xC0000092u: return "EXCEPTION_FLT_STACK_CHECK";
        case 0xC0000093u: return "EXCEPTION_FLT_UNDERFLOW";
        case 0xC000001Du: return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case 0xC0000006u: return "EXCEPTION_IN_PAGE_ERROR";
        case 0xC0000094u: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case 0xC0000095u: return "EXCEPTION_INT_OVERFLOW";
        case 0xC00000FDu: return "EXCEPTION_STACK_OVERFLOW";
        case 0xC0000025u: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        case 0xC0000026u: return "EXCEPTION_INVALID_DISPOSITION";
        case 0x80000004u: return "EXCEPTION_SINGLE_STEP";
        case 0xC0000008u: return "EXCEPTION_INVALID_HANDLE";
        case 0xC0000017u: return "STATUS_NO_MEMORY";
        case 0xC0000409u: return "STATUS_STACK_BUFFER_OVERRUN";
        case 0xC0000374u: return "STATUS_HEAP_CORRUPTION";
        case 0xC0000602u: return "STATUS_FAIL_FAST_EXCEPTION";
        case kCppException: return "C++ EXCEPTION (msvc)";
        case kClrException: return "CLR EXCEPTION (.NET)";
        default: break;
    }
    return "UNKNOWN";
}

namespace {

    // Copies source into destination, truncating to capacity and guaranteeing NUL termination.
    void copyLabel(char* dst, std::size_t capacity, std::string_view src) noexcept
    {
        if (!dst || capacity == 0)
            return;
        std::size_t length = src.size();
        if (length > capacity - 1)
            length = capacity - 1;
        dst[length] = '\0';
        if (length != 0 && src.data() != nullptr)
            std::memcpy(dst, src.data(), length);
    }

} // namespace

void BreadcrumbRing::push(std::uint32_t threadId, std::string_view text) noexcept
{
    const std::uint32_t ticket = _pushed.fetch_add(1, std::memory_order_relaxed);
    Slot& slot = _slots[ticket & (kCapacity - 1)];
    slot.threadId = threadId;
    copyLabel(slot.text, kTextCapacity, text);
}

void BreadcrumbRing::push(std::uint32_t threadId, std::string_view text,
                          std::string_view detail) noexcept
{
    const std::uint32_t ticket = _pushed.fetch_add(1, std::memory_order_relaxed);
    Slot& slot = _slots[ticket & (kCapacity - 1)];
    slot.threadId = threadId;

    std::size_t at = 0;
    const auto append = [&](std::string_view part) noexcept {
        for (std::size_t i = 0; i < part.size() && at < kTextCapacity - 1; ++i)
            slot.text[at++] = part[i];
    };

    slot.text[0] = '\0';
    append(text);
    append(detail);
    slot.text[at] = '\0';
}

std::size_t BreadcrumbRing::recorded() const noexcept
{
    const std::uint32_t pushedCount = _pushed.load(std::memory_order_relaxed);
    return pushedCount < kCapacity ? static_cast<std::size_t>(pushedCount) : kCapacity;
}

std::uint32_t BreadcrumbRing::pushed() const noexcept
{
    return _pushed.load(std::memory_order_relaxed);
}

namespace {

    // Converts oldest-first index to slot index using unsigned ring arithmetic.
    std::size_t oldestFirstSlot(std::uint32_t pushedCount, std::size_t held,
                                std::size_t index) noexcept
    {
        const std::uint32_t first = pushedCount - static_cast<std::uint32_t>(held);
        return static_cast<std::size_t>((first + static_cast<std::uint32_t>(index)) &
                                        (BreadcrumbRing::kCapacity - 1));
    }

} // namespace

const char* BreadcrumbRing::text(std::size_t index) const noexcept
{
    const std::size_t held = recorded();
    if (index >= held)
        return "";
    return _slots[oldestFirstSlot(pushed(), held, index)].text;
}

std::uint32_t BreadcrumbRing::threadId(std::size_t index) const noexcept
{
    const std::size_t held = recorded();
    if (index >= held)
        return 0;
    return _slots[oldestFirstSlot(pushed(), held, index)].threadId;
}

void BreadcrumbRing::clear() noexcept
{
    for (Slot& slot : _slots) {
        slot.threadId = 0;
        slot.text[0] = '\0';
    }
    _pushed.store(0, std::memory_order_relaxed);
}

namespace {

    constexpr std::size_t kMaxThreadSlots = 16;
    constexpr std::size_t kMaxModules = 256;
    constexpr std::size_t kMaxFrames = 64;
    constexpr std::size_t kModuleNameCapacity = 64;
    constexpr std::size_t kSymbolNameCapacity = 384;
    constexpr std::size_t kVersionCapacity = 32;
    constexpr std::size_t kSinkCapacity = 8192;

    struct ThreadSlot {
        std::atomic<std::uint32_t> threadId{0};
        std::atomic<std::uint8_t> role{static_cast<std::uint8_t>(ThreadRole::Unknown)};
        char activeMod[CrashReporter::kLabelCapacity]{};
        char activeHook[CrashReporter::kLabelCapacity]{};
    };

    struct ModuleEntry {
        std::uintptr_t base{0};
        std::uint32_t size{0};
        char name[kModuleNameCapacity]{};
    };

    struct RegisteredMod {
        char id[CrashReporter::kLabelCapacity]{};
        char version[kVersionCapacity]{};
        bool loaded{false};
    };

    struct UnicodeStringLite {
        USHORT length;
        USHORT maximumLength;
        wchar_t* buffer;
    };

    struct LdrEntry {
        LIST_ENTRY inLoadOrderLinks;
        LIST_ENTRY inMemoryOrderLinks;
        LIST_ENTRY inInitializationOrderLinks;
        void* dllBase;
        void* entryPoint;
        ULONG sizeOfImage;
        UnicodeStringLite fullDllName;
        UnicodeStringLite baseDllName;
    };

    struct LdrData {
        ULONG length;
        BOOLEAN initialized;
        void* ssHandle;
        LIST_ENTRY inLoadOrderModuleList;
        LIST_ENTRY inMemoryOrderModuleList;
        LIST_ENTRY inInitializationOrderModuleList;
    };

    struct PebLite {
        BYTE inheritedAddressSpace;
        BYTE readImageFileExecOptions;
        BYTE beingDebugged;
        BYTE spareBool;
        void* mutant;
        void* imageBaseAddress;
        LdrData* ldr;
    };

    ThreadSlot g_threads[kMaxThreadSlots];
    ModuleEntry g_modules[kMaxModules];
    std::atomic<std::size_t> g_moduleCount{0};

    RegisteredMod g_mods[CrashReporter::kMaxRegisteredMods];
    std::atomic<std::size_t> g_modCount{0};

    HookAttribution g_hookTable[kMaxAttributedHooks];

    BreadcrumbRing g_breadcrumbs;

    char g_reportDirectory[MAX_PATH]{};
    char g_reportPath[MAX_PATH]{};
    char g_dumpPath[MAX_PATH]{};
    char g_processPath[MAX_PATH]{};

    char g_profileId[CrashReporter::kLabelCapacity]{};
    char g_profileName[CrashReporter::kLabelCapacity]{};
    std::uint32_t g_profileTimeDateStamp{0};
    std::uint32_t g_profileSizeOfImage{0};
    std::uint32_t g_profileCheckSum{0};
    std::atomic<bool> g_profileKnown{false};

    std::atomic<bool> g_installed{false};
    std::atomic<bool> g_symbolsReady{false};
    std::atomic<bool> g_inHandler{false};
    std::atomic<std::size_t> g_reportsWritten{0};

    std::atomic<std::uint32_t> g_lastCode{0};
    std::atomic<std::uintptr_t> g_lastAddress{0};
    std::atomic<std::uint32_t> g_lastThread{0};

    void* g_vectoredHandle{nullptr};
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousFilter{nullptr};

    CONTEXT g_walkContext{};
    STACKFRAME64 g_walkFrame{};
    alignas(8) char g_symbolStorage[sizeof(SYMBOL_INFO) + kSymbolNameCapacity]{};
    IMAGEHLP_LINE64 g_lineInfo{};

    class FileSink final {
    public:
        bool open(const char* path) noexcept
        {
            _used = 0;
            _handle = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            return _handle != INVALID_HANDLE_VALUE;
        }

        void put(char c) noexcept
        {
            if (_used == kSinkCapacity)
                flush();
            _buffer[_used++] = c;
        }

        void text(const char* s) noexcept
        {
            if (!s)
                return;
            while (*s != '\0')
                put(*s++);
        }

        void text(const char* s, std::size_t length) noexcept
        {
            for (std::size_t i = 0; i < length && s[i] != '\0'; ++i)
                put(s[i]);
        }

        // Left-justified in width columns to align report labels.
        void column(const char* s, std::size_t width) noexcept
        {
            std::size_t written = 0;
            if (s) {
                while (*s != '\0') {
                    put(*s++);
                    ++written;
                }
            }
            while (written++ < width)
                put(' ');
        }

        void hex(std::uint32_t value, std::size_t digits) noexcept
        {
            static const char kDigits[] = "0123456789ABCDEF";
            for (std::size_t i = digits; i-- > 0;)
                put(kDigits[(value >> (i * 4)) & 0xFu]);
        }

        // Emits hexadecimal value without leading zeros.
        void hexCompact(std::uint32_t value) noexcept
        {
            static const char kDigits[] = "0123456789ABCDEF";
            char scratch[8];
            std::size_t count = 0;
            do {
                scratch[count++] = kDigits[value & 0xFu];
                value >>= 4;
            } while (value != 0 && count < sizeof(scratch));
            while (count-- > 0)
                put(scratch[count]);
        }

        void dec(std::uint32_t value, std::size_t minimumDigits = 1) noexcept
        {
            char scratch[12];
            std::size_t count = 0;
            do {
                scratch[count++] = static_cast<char>('0' + (value % 10u));
                value /= 10u;
            } while (value != 0 && count < sizeof(scratch));
            while (count < minimumDigits && count < sizeof(scratch))
                scratch[count++] = '0';
            while (count-- > 0)
                put(scratch[count]);
        }

        void line() noexcept
        {
            put('\r');
            put('\n');
        }

        void flush() noexcept
        {
            if (_handle == INVALID_HANDLE_VALUE || _used == 0) {
                _used = 0;
                return;
            }
            DWORD written = 0;
            WriteFile(_handle, _buffer, static_cast<DWORD>(_used), &written, nullptr);
            _used = 0;
        }

        void close() noexcept
        {
            flush();
            if (_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(_handle);
                _handle = INVALID_HANDLE_VALUE;
            }
        }

    private:
        HANDLE _handle{INVALID_HANDLE_VALUE};
        std::size_t _used{0};
        char _buffer[kSinkCapacity]{};
    };

    FileSink g_sink;

    ThreadSlot* findSlot(std::uint32_t threadId) noexcept
    {
        for (ThreadSlot& slot : g_threads) {
            if (slot.threadId.load(std::memory_order_acquire) == threadId)
                return &slot;
        }
        return nullptr;
    }

    // Finds or claims a free thread slot for the current thread ID.
    ThreadSlot* claimSlot() noexcept
    {
        const std::uint32_t threadId = GetCurrentThreadId();
        if (ThreadSlot* existing = findSlot(threadId))
            return existing;

        for (ThreadSlot& slot : g_threads) {
            std::uint32_t empty = 0;
            if (slot.threadId.compare_exchange_strong(empty, threadId,
                                                      std::memory_order_acq_rel))
                return &slot;
        }
        return nullptr;
    }

    void narrowName(char* destination, std::size_t capacity,
                    const UnicodeStringLite& source) noexcept
    {
        destination[0] = '\0';
        if (!source.buffer || source.length == 0)
            return;

        std::size_t count = source.length / sizeof(wchar_t);
        if (count > capacity - 1)
            count = capacity - 1;

        destination[count] = '\0';
        for (std::size_t i = 0; i < count; ++i) {
            const wchar_t wide = source.buffer[i];
            destination[i] = (wide > 0 && wide < 0x80) ? static_cast<char>(wide) : '?';
        }
    }

    void snapshotModulesUnguarded() noexcept
    {
        const auto* peb =
            reinterpret_cast<const PebLite*>(static_cast<std::uintptr_t>(__readfsdword(0x30)));
        if (!peb || !peb->ldr)
            return;

        const LIST_ENTRY* head = &peb->ldr->inMemoryOrderModuleList;
        std::size_t count = 0;

        for (const LIST_ENTRY* node = head->Flink;
             node != nullptr && node != head && count < kMaxModules;
             node = node->Flink) {
            const auto* entry = reinterpret_cast<const LdrEntry*>(
                reinterpret_cast<const unsigned char*>(node) -
                offsetof(LdrEntry, inMemoryOrderLinks));

            if (!entry->dllBase || entry->sizeOfImage == 0)
                continue;

            ModuleEntry& module = g_modules[count];
            module.base = reinterpret_cast<std::uintptr_t>(entry->dllBase);
            module.size = entry->sizeOfImage;
            narrowName(module.name, kModuleNameCapacity, entry->baseDllName);
            ++count;
        }

        g_moduleCount.store(count, std::memory_order_release);
    }

    // Safely snapshots modules without taking loader lock.
    void snapshotModules() noexcept
    {
        __try {
            snapshotModulesUnguarded();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    const ModuleEntry* findModule(std::uintptr_t address) noexcept
    {
        const std::size_t count = g_moduleCount.load(std::memory_order_acquire);
        for (std::size_t i = 0; i < count; ++i) {
            const ModuleEntry& module = g_modules[i];
            if (address >= module.base && address < module.base + module.size)
                return &module;
        }
        return nullptr;
    }

    // Writes formatted module-relative or raw address to the sink.
    void writeAddress(FileSink& sink, std::uintptr_t address) noexcept
    {
        const ModuleEntry* module = findModule(address);
        if (module) {
            sink.text(module->name);
            sink.text("+0x");
            sink.hex(static_cast<std::uint32_t>(address - module->base), 8);
            return;
        }
        sink.text("<no-module>+0x");
        sink.hex(static_cast<std::uint32_t>(address), 8);
    }

    void writeStamp(FileSink& sink, const SYSTEMTIME& now) noexcept
    {
        sink.dec(now.wYear, 4);
        sink.put('-');
        sink.dec(now.wMonth, 2);
        sink.put('-');
        sink.dec(now.wDay, 2);
        sink.put(' ');
        sink.dec(now.wHour, 2);
        sink.put(':');
        sink.dec(now.wMinute, 2);
        sink.put(':');
        sink.dec(now.wSecond, 2);
    }

    // Formats a timestamped crash report file path into destination buffer.
    void buildPath(char* destination, const char* extension, const SYSTEMTIME& now,
                   std::size_t ordinal) noexcept
    {
        std::size_t at = 0;
        const auto append = [&](const char* text) noexcept {
            while (*text != '\0' && at < MAX_PATH - 1)
                destination[at++] = *text++;
        };
        const auto appendNumber = [&](std::uint32_t value, std::size_t digits) noexcept {
            char scratch[12];
            std::size_t count = 0;
            do {
                scratch[count++] = static_cast<char>('0' + (value % 10u));
                value /= 10u;
            } while (value != 0 && count < sizeof(scratch));
            while (count < digits && count < sizeof(scratch))
                scratch[count++] = '0';
            while (count-- > 0 && at < MAX_PATH - 1)
                destination[at++] = scratch[count];
        };

        append(g_reportDirectory);
        append("\\crash-");
        appendNumber(now.wYear, 4);
        appendNumber(now.wMonth, 2);
        appendNumber(now.wDay, 2);
        append("-");
        appendNumber(now.wHour, 2);
        appendNumber(now.wMinute, 2);
        appendNumber(now.wSecond, 2);
        if (ordinal != 0) {
            append("-");
            appendNumber(static_cast<std::uint32_t>(ordinal + 1), 1);
        }
        append(extension);
        destination[at] = '\0';
    }

    void writeRegisters(FileSink& sink, const CONTEXT& context) noexcept
    {
        const auto pair = [&](const char* name, DWORD value) noexcept {
            sink.text(name);
            sink.put('=');
            sink.hex(value, 8);
        };
        const auto segment = [&](const char* name, DWORD value) noexcept {
            sink.text(name);
            sink.put('=');
            sink.hex(value, 4);
        };

        sink.text("  ");
        pair("EAX", context.Eax); sink.put(' ');
        pair("EBX", context.Ebx); sink.put(' ');
        pair("ECX", context.Ecx); sink.put(' ');
        pair("EDX", context.Edx);
        sink.line();

        sink.text("  ");
        pair("ESI", context.Esi); sink.put(' ');
        pair("EDI", context.Edi); sink.put(' ');
        pair("EBP", context.Ebp); sink.put(' ');
        pair("ESP", context.Esp);
        sink.line();

        sink.text("  ");
        pair("EIP", context.Eip); sink.put(' ');
        pair("EFL", context.EFlags);
        sink.line();

        sink.text("  ");
        segment("CS", context.SegCs); sink.put(' ');
        segment("SS", context.SegSs); sink.put(' ');
        segment("DS", context.SegDs); sink.put(' ');
        segment("ES", context.SegEs); sink.put(' ');
        segment("FS", context.SegFs); sink.put(' ');
        segment("GS", context.SegGs);
        sink.line();
    }

    void writeSymbol(FileSink& sink, HANDLE process, std::uintptr_t address) noexcept
    {
        if (!g_symbolsReady.load(std::memory_order_acquire))
            return;

        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(g_symbolStorage);
        std::memset(g_symbolStorage, 0, sizeof(g_symbolStorage));
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = static_cast<ULONG>(kSymbolNameCapacity - 1);

        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbol)) {
            sink.text("  ");
            sink.text(symbol->Name);
            if (displacement != 0) {
                sink.text("+0x");
                sink.hexCompact(static_cast<std::uint32_t>(displacement));
            }
        }

        std::memset(&g_lineInfo, 0, sizeof(g_lineInfo));
        g_lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &g_lineInfo)) {
            sink.text("  [");
            sink.text(g_lineInfo.FileName);
            sink.put(':');
            sink.dec(g_lineInfo.LineNumber);
            sink.put(']');
        }
    }

    void writeStack(FileSink& sink, const CONTEXT& context) noexcept
    {
        std::memcpy(&g_walkContext, &context, sizeof(CONTEXT));
        std::memset(&g_walkFrame, 0, sizeof(g_walkFrame));

        g_walkFrame.AddrPC.Offset = g_walkContext.Eip;
        g_walkFrame.AddrPC.Mode = AddrModeFlat;
        g_walkFrame.AddrFrame.Offset = g_walkContext.Ebp;
        g_walkFrame.AddrFrame.Mode = AddrModeFlat;
        g_walkFrame.AddrStack.Offset = g_walkContext.Esp;
        g_walkFrame.AddrStack.Mode = AddrModeFlat;

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        for (std::size_t depth = 0; depth < kMaxFrames; ++depth) {
            if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &g_walkFrame,
                             &g_walkContext, nullptr, SymFunctionTableAccess64,
                             SymGetModuleBase64, nullptr))
                break;

            const auto pc = static_cast<std::uintptr_t>(g_walkFrame.AddrPC.Offset);
            if (pc == 0)
                break;

            sink.text("  #");
            sink.dec(static_cast<std::uint32_t>(depth), 2);
            sink.put(' ');
            writeAddress(sink, pc);
            writeSymbol(sink, process, pc);
            sink.line();
        }
    }

    void writeLoaderContext(FileSink& sink, std::uint32_t faultingThread) noexcept
    {
        const ThreadSlot* slot = findSlot(faultingThread);

        sink.column("Active mod", 14);
        sink.text(": ");
        sink.text((slot && slot->activeMod[0] != '\0') ? slot->activeMod : "(none)");
        sink.line();

        sink.column("Active hook", 14);
        sink.text(": ");
        sink.text((slot && slot->activeHook[0] != '\0') ? slot->activeHook : "(none)");
        sink.line();

        sink.column("Thread role", 14);
        sink.text(": ");
        sink.text(describe(slot ? static_cast<ThreadRole>(slot->role.load(std::memory_order_acquire))
                                : ThreadRole::Unknown));
        sink.line();
    }

    void writeBreadcrumbs(FileSink& sink) noexcept
    {
        const std::size_t held = g_breadcrumbs.recorded();
        const std::uint32_t total = g_breadcrumbs.pushed();

        sink.text("Breadcrumbs   : ");
        sink.dec(static_cast<std::uint32_t>(held));
        sink.text(" held of ");
        sink.dec(total);
        sink.text(" pushed (oldest first)");
        sink.line();

        if (held == 0) {
            sink.text("  (none)");
            sink.line();
            return;
        }

        for (std::size_t i = 0; i < held; ++i) {
            sink.text("  [tid ");
            sink.dec(g_breadcrumbs.threadId(i));
            sink.text("] ");
            sink.text(g_breadcrumbs.text(i), BreadcrumbRing::kTextCapacity);
            sink.line();
        }
    }

    void writeMods(FileSink& sink) noexcept
    {
        const std::size_t count = g_modCount.load(std::memory_order_acquire);
        if (count == 0) {
            sink.text("  (no mod has been registered with the reporter)");
            sink.line();
            return;
        }
        for (std::size_t i = 0; i < count; ++i) {
            sink.text("  [");
            sink.dec(static_cast<std::uint32_t>(i + 1), 2);
            sink.text("] ");
            sink.column(g_mods[i].id, CrashReporter::kLabelCapacity / 2);
            sink.put(' ');
            sink.column(g_mods[i].version[0] != '\0' ? g_mods[i].version : "(no version)", 16);
            sink.text(g_mods[i].loaded ? " loaded" : " FAILED TO RUN");
            sink.line();
        }
    }

    // Writes registered hooks from the lock-free crash mirror.
    void writeHooks(FileSink& sink) noexcept
    {
        const std::size_t count = crashAttributionTable(g_hookTable, kMaxAttributedHooks);
        if (count == 0) {
            sink.text("  (no hook was registered -- either none had been installed yet, or the "
                      "crash came before the loader got that far)");
            sink.line();
            return;
        }
        for (std::size_t i = 0; i < count; ++i) {
            sink.text("  0x");
            sink.hex(static_cast<std::uint32_t>(g_hookTable[i].target), 8);
            sink.text(" +");
            sink.dec(static_cast<std::uint32_t>(g_hookTable[i].stolenBytes), 2);
            sink.text(" owner=");
            sink.column(g_hookTable[i].owner, 20);
            sink.put(' ');
            sink.text(g_hookTable[i].name, HookAttribution::kTextCapacity);
            sink.line();
        }
    }

    void writeThreadTable(FileSink& sink) noexcept
    {
        bool any = false;
        for (const ThreadSlot& slot : g_threads) {
            const std::uint32_t threadId = slot.threadId.load(std::memory_order_acquire);
            if (threadId == 0)
                continue;
            any = true;
            sink.text("  tid ");
            sink.dec(threadId);
            sink.text(" role=");
            sink.column(describe(static_cast<ThreadRole>(slot.role.load(std::memory_order_acquire))), 8);
            sink.text(" mod='");
            sink.text(slot.activeMod, CrashReporter::kLabelCapacity);
            sink.text("' hook='");
            sink.text(slot.activeHook, CrashReporter::kLabelCapacity);
            sink.put('\'');
            sink.line();
        }
        if (!any) {
            sink.text("  (no thread declared a role)");
            sink.line();
        }
    }

    void writeModules(FileSink& sink) noexcept
    {
        const std::size_t count = g_moduleCount.load(std::memory_order_acquire);
        for (std::size_t i = 0; i < count; ++i) {
            sink.text("  ");
            sink.hex(static_cast<std::uint32_t>(g_modules[i].base), 8);
            sink.text(" size 0x");
            sink.hex(g_modules[i].size, 8);
            sink.text("  ");
            sink.text(g_modules[i].name);
            sink.line();
        }
        if (count == 0) {
            sink.text("  (the module list could not be read)");
            sink.line();
        }
    }

    void writeReport(const EXCEPTION_RECORD& record, const CONTEXT& context,
                     const char* source, const SYSTEMTIME& now) noexcept
    {
        if (!g_sink.open(g_reportPath))
            return;

        FileSink& sink = g_sink;
        const std::uint32_t faultingThread = GetCurrentThreadId();

        sink.text("=== CrabeLoader crash report ===");
        sink.line();

        sink.column("Loader", 14);
        sink.text(": v");
        sink.text(crabe::build::label().data());
        sink.text(" (built ");
        sink.text(crabe::build::time().data());
        sink.text(", commit ");
        sink.text(crabe::build::commit().data());
        sink.put(')');
        sink.line();

        sink.column("Report time", 14);
        sink.text(": ");
        writeStamp(sink, now);
        sink.line();

        sink.column("Reported by", 14);
        sink.text(": ");
        sink.text(source);
        sink.line();

        sink.column("Process", 14);
        sink.text(": ");
        sink.text(g_processPath);
        sink.text(" (pid ");
        sink.dec(GetCurrentProcessId());
        sink.put(')');
        sink.line();

        sink.column("Game profile", 14);
        sink.text(": ");
        if (g_profileKnown.load(std::memory_order_acquire) && g_profileId[0] != '\0') {
            sink.text(g_profileId);
            sink.text(" \"");
            sink.text(g_profileName);
            sink.put('"');
        } else if (g_profileKnown.load(std::memory_order_acquire)) {
            sink.text("(none -- this build is not in the profile table; the loader ran degraded)");
        } else {
            sink.text("(not detected -- the crash happened before detection ran)");
        }
        sink.line();

        sink.column("Game identity", 14);
        sink.text(": TimeDateStamp 0x");
        sink.hex(g_profileTimeDateStamp, 8);
        sink.text("  SizeOfImage 0x");
        sink.hex(g_profileSizeOfImage, 8);
        sink.text("  CheckSum 0x");
        sink.hex(g_profileCheckSum, 8);
        sink.line();
        sink.line();

        sink.text("--- Exception ---");
        sink.line();

        sink.column("Code", 14);
        sink.text(": 0x");
        sink.hex(record.ExceptionCode, 8);
        sink.text(" (");
        sink.text(exceptionName(record.ExceptionCode));
        sink.put(')');
        sink.line();

        sink.column("Address", 14);
        sink.text(": ");
        writeAddress(sink, reinterpret_cast<std::uintptr_t>(record.ExceptionAddress));
        sink.line();

        HookAttribution covering{};
        if (crashAttribution(reinterpret_cast<std::uintptr_t>(record.ExceptionAddress), covering)) {
            sink.column("In hook", 14);
            sink.text(": ");
            sink.text(covering.name, HookAttribution::kTextCapacity);
            sink.text(" owned by '");
            sink.text(covering.owner, HookAttribution::kTextCapacity);
            sink.text("' (patched 0x");
            sink.hex(static_cast<std::uint32_t>(covering.target), 8);
            sink.text(" +");
            sink.dec(static_cast<std::uint32_t>(covering.stolenBytes));
            sink.text(" bytes)");
            sink.line();
        }

        if ((record.ExceptionCode == static_cast<DWORD>(EXCEPTION_ACCESS_VIOLATION) ||
             record.ExceptionCode == static_cast<DWORD>(EXCEPTION_IN_PAGE_ERROR)) &&
            record.NumberParameters >= 2) {
            sink.column("Access", 14);
            sink.text(": ");
            switch (record.ExceptionInformation[0]) {
                case 0: sink.text("read from 0x"); break;
                case 1: sink.text("write to 0x"); break;
                case 8: sink.text("execute at 0x"); break;
                default: sink.text("unknown operation at 0x"); break;
            }
            sink.hex(static_cast<std::uint32_t>(record.ExceptionInformation[1]), 8);
            sink.line();
        }

        sink.column("Thread", 14);
        sink.text(": ");
        sink.dec(faultingThread);
        sink.line();
        sink.line();

        sink.text("--- Loader context ---");
        sink.line();
        writeLoaderContext(sink, faultingThread);
        writeBreadcrumbs(sink);
        sink.line();

        sink.text("--- Loaded mods ---");
        sink.line();
        writeMods(sink);
        sink.line();

        sink.text("--- Installed hooks (address +bytes) ---");
        sink.line();
        writeHooks(sink);
        sink.line();

        sink.text("--- Registers ---");
        sink.line();
        writeRegisters(sink, context);
        sink.line();

        sink.text("--- Stack (module+RVA; symbols ");
        sink.text(g_symbolsReady.load(std::memory_order_acquire) ? "on" : "off");
        sink.text(") ---");
        sink.line();
        writeStack(sink, context);
        sink.line();

        sink.text("--- Loader threads ---");
        sink.line();
        writeThreadTable(sink);
        sink.line();

        sink.text("--- Modules (");
        sink.dec(static_cast<std::uint32_t>(g_moduleCount.load(std::memory_order_acquire)));
        sink.text(") ---");
        sink.line();
        writeModules(sink);
        sink.line();

        sink.column("Minidump", 14);
        sink.text(": ");
        sink.text(g_dumpPath);
        sink.line();
        sink.text("=== end of report ===");
        sink.line();

        sink.close();
    }

    void writeMinidump(EXCEPTION_POINTERS* pointers) noexcept
    {
        HANDLE rawFile = CreateFileA(g_dumpPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (rawFile == INVALID_HANDLE_VALUE)
            return;

        std::unique_ptr<void, decltype(&::CloseHandle)> file(rawFile, &::CloseHandle);

        MINIDUMP_EXCEPTION_INFORMATION information{};
        information.ThreadId = GetCurrentThreadId();
        information.ExceptionPointers = pointers;
        information.ClientPointers = FALSE;

        const auto type = static_cast<MINIDUMP_TYPE>(
            MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory |
            MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file.get(), type,
                          pointers ? &information : nullptr, nullptr, nullptr);
    }

    bool alreadyReported(const EXCEPTION_RECORD& record) noexcept
    {
        const auto address = reinterpret_cast<std::uintptr_t>(record.ExceptionAddress);
        const std::uint32_t thread = GetCurrentThreadId();
        return g_lastCode.load(std::memory_order_acquire) == record.ExceptionCode &&
               g_lastAddress.load(std::memory_order_acquire) == address &&
               g_lastThread.load(std::memory_order_acquire) == thread;
    }

    void reportNow(EXCEPTION_POINTERS* pointers, const char* source) noexcept
    {
        if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord)
            return;
        if (!g_installed.load(std::memory_order_acquire))
            return;

        const EXCEPTION_RECORD& record = *pointers->ExceptionRecord;

        const std::size_t ordinal = g_reportsWritten.load(std::memory_order_acquire);
        if (ordinal >= CrashReporter::kMaxReports)
            return;
        if (alreadyReported(record))
            return;

        if (g_inHandler.exchange(true, std::memory_order_acq_rel))
            return;

        g_lastCode.store(record.ExceptionCode, std::memory_order_release);
        g_lastAddress.store(reinterpret_cast<std::uintptr_t>(record.ExceptionAddress),
                            std::memory_order_release);
        g_lastThread.store(GetCurrentThreadId(), std::memory_order_release);

        SYSTEMTIME now{};
        GetLocalTime(&now);
        buildPath(g_reportPath, ".txt", now, ordinal);
        buildPath(g_dumpPath, ".dmp", now, ordinal);

        snapshotModules();

        __try {
            writeReport(record, *pointers->ContextRecord, source, now);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_sink.close();
        }

        __try {
            writeMinidump(pointers);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }

        g_reportsWritten.fetch_add(1, std::memory_order_acq_rel);
        g_inHandler.store(false, std::memory_order_release);
    }

    LONG CALLBACK vectoredHandler(EXCEPTION_POINTERS* pointers) noexcept
    {
        if (!pointers || !pointers->ExceptionRecord)
            return EXCEPTION_CONTINUE_SEARCH;

        const DWORD code = pointers->ExceptionRecord->ExceptionCode;
        if (classifyException(code, IsDebuggerPresent() != FALSE) == FilterDecision::PassThrough)
            return EXCEPTION_CONTINUE_SEARCH;

        reportNow(pointers, "vectored handler (priority 1)");

        return EXCEPTION_CONTINUE_SEARCH;
    }

    LONG WINAPI unhandledFilter(EXCEPTION_POINTERS* pointers) noexcept
    {
        if (pointers && pointers->ExceptionRecord &&
            classifyException(pointers->ExceptionRecord->ExceptionCode,
                              IsDebuggerPresent() != FALSE) == FilterDecision::Report) {
            reportNow(pointers, "unhandled exception filter");
        }

        if (g_previousFilter)
            return g_previousFilter(pointers);
        return EXCEPTION_EXECUTE_HANDLER;
    }

} // namespace

bool CrashReporter::install(const char* reportDirectory) noexcept
{
    if (g_installed.load(std::memory_order_acquire))
        return true;

    copyLabel(g_reportDirectory, MAX_PATH, reportDirectory ? reportDirectory : ".");

    std::size_t length = std::strlen(g_reportDirectory);
    while (length > 1 && (g_reportDirectory[length - 1] == '\\' ||
                          g_reportDirectory[length - 1] == '/')) {
        g_reportDirectory[--length] = '\0';
    }
    for (std::size_t i = 0; i < length; ++i) {
        if (g_reportDirectory[i] == '/')
            g_reportDirectory[i] = '\\';
    }

    CreateDirectoryA(g_reportDirectory, nullptr);

    GetModuleFileNameA(nullptr, g_processPath, MAX_PATH);
    g_processPath[MAX_PATH - 1] = '\0';

    snapshotModules();

    g_vectoredHandle = AddVectoredExceptionHandler(1, &vectoredHandler);
    g_previousFilter = SetUnhandledExceptionFilter(&unhandledFilter);

    g_installed.store(true, std::memory_order_release);
    return g_vectoredHandle != nullptr;
}

void CrashReporter::completeInstall() noexcept
{
    if (g_symbolsReady.load(std::memory_order_acquire))
        return;

    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES |
                  SYMOPT_FAIL_CRITICAL_ERRORS);
    if (SymInitialize(GetCurrentProcess(), nullptr, TRUE))
        g_symbolsReady.store(true, std::memory_order_release);

    snapshotModules();
}

void CrashReporter::uninstall() noexcept
{
    if (g_vectoredHandle) {
        RemoveVectoredExceptionHandler(g_vectoredHandle);
        g_vectoredHandle = nullptr;
    }
    SetUnhandledExceptionFilter(g_previousFilter);
    g_previousFilter = nullptr;

    if (g_symbolsReady.exchange(false, std::memory_order_acq_rel))
        SymCleanup(GetCurrentProcess());

    g_installed.store(false, std::memory_order_release);
}

void CrashReporter::setGameProfile(std::string_view id, std::string_view displayName,
                                   std::uint32_t timeDateStamp, std::uint32_t sizeOfImage,
                                   std::uint32_t checkSum) noexcept
{
    copyLabel(g_profileId, kLabelCapacity, id);
    copyLabel(g_profileName, kLabelCapacity, displayName);
    g_profileTimeDateStamp = timeDateStamp;
    g_profileSizeOfImage = sizeOfImage;
    g_profileCheckSum = checkSum;
    g_profileKnown.store(true, std::memory_order_release);
}

void CrashReporter::setActiveMod(std::string_view modId) noexcept
{
    if (ThreadSlot* slot = claimSlot())
        copyLabel(slot->activeMod, kLabelCapacity, modId);
}

void CrashReporter::setActiveHook(std::string_view hookLabel) noexcept
{
    if (ThreadSlot* slot = claimSlot())
        copyLabel(slot->activeHook, kLabelCapacity, hookLabel);
}

void CrashReporter::pushBreadcrumb(std::string_view text) noexcept
{
    g_breadcrumbs.push(GetCurrentThreadId(), text);
}

void CrashReporter::pushBreadcrumb(std::string_view text, std::string_view detail) noexcept
{
    g_breadcrumbs.push(GetCurrentThreadId(), text, detail);
}

void CrashReporter::declareThreadRole(ThreadRole role) noexcept
{
    ThreadSlot* slot = claimSlot();
    if (!slot)
        return;

    std::uint8_t unknown = static_cast<std::uint8_t>(ThreadRole::Unknown);
    slot->role.compare_exchange_strong(unknown, static_cast<std::uint8_t>(role),
                                       std::memory_order_acq_rel);
}

void CrashReporter::currentHook(char* out, std::size_t capacity) noexcept
{
    if (!out || capacity == 0)
        return;
    out[0] = '\0';

    const ThreadSlot* slot = findSlot(GetCurrentThreadId());
    if (!slot)
        return;

    std::size_t i = 0;
    for (; i < capacity - 1 && i < kLabelCapacity && slot->activeHook[i] != '\0'; ++i)
        out[i] = slot->activeHook[i];
    out[i] = '\0';
}

void CrashReporter::registerMod(std::string_view id, std::string_view version,
                                bool loaded) noexcept
{
    const std::size_t index = g_modCount.load(std::memory_order_acquire);
    if (index >= kMaxRegisteredMods)
        return;

    copyLabel(g_mods[index].id, kLabelCapacity, id);
    copyLabel(g_mods[index].version, kVersionCapacity, version);
    g_mods[index].loaded = loaded;
    g_modCount.store(index + 1, std::memory_order_release);
}

void CrashReporter::forgetMods() noexcept
{
    g_modCount.store(0, std::memory_order_release);
}

BreadcrumbRing& CrashReporter::breadcrumbs() noexcept
{
    return g_breadcrumbs;
}

std::size_t CrashReporter::reportsWritten() noexcept
{
    return g_reportsWritten.load(std::memory_order_acquire);
}

const char* CrashReporter::lastReportPath() noexcept
{
    return g_reportPath;
}

const char* CrashReporter::lastDumpPath() noexcept
{
    return g_dumpPath;
}

ScopedHook::ScopedHook(std::string_view label) noexcept
{
    CrashReporter::currentHook(_previous, sizeof(_previous));
    CrashReporter::setActiveHook(label);
}

ScopedHook::~ScopedHook()
{
    CrashReporter::setActiveHook(_previous);
}

ScopedMod::ScopedMod(std::string_view modId) noexcept
{
    CrashReporter::setActiveMod(modId);
}

ScopedMod::~ScopedMod()
{
    CrashReporter::setActiveMod("");
}

} // namespace crabe::infrastructure
