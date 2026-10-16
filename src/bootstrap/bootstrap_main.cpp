/*
** CrabeLoader
** File description:
** The bootstrap dinput8.dll's entry: patches the game's entry point to start CrabeLoader.asi.
** The patch is a 5-byte jump undone before CrabeLoader loads, so none of it stays in the game.
** Forwards DirectInput through dinput8_proxy.cpp and holds no other logic.
**
** Authors: @LucasLhomme
*/

#include <windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>

#include "bootstrap/load_loader.hpp"

namespace {

    constexpr std::size_t kJumpLength = 5;

    std::uint8_t* g_entry = nullptr;
    void* g_entryJump = nullptr;
    std::array<std::uint8_t, kJumpLength> g_savedEntry{};

    /// Returns the folder of the game's executable.
    std::filesystem::path executableDirectory()
    {
        std::wstring path(MAX_PATH, L'\0');
        for (;;) {
            const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            if (length == 0)
                return std::filesystem::current_path();
            if (length < path.size()) {
                path.resize(length);
                return std::filesystem::path(path).parent_path();
            }
            path.resize(path.size() * 2);
        }
    }

    /// Appends one warning to loader.log, the file CrabeLoader itself writes, so a failed start is visible.
    void logWarning(const std::filesystem::path& folder, const std::string& message) noexcept
    {
        try {
            SYSTEMTIME now{};
            GetLocalTime(&now);
            const std::string line = std::format("[{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}] [WARNING] Bootstrap: {}.\r\n",
                                                 now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
                                                 now.wMilliseconds, message);
            const HANDLE file = CreateFileW((folder / L"loader.log").c_str(), FILE_APPEND_DATA,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
                return;
            DWORD written = 0;
            WriteFile(file, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
            CloseHandle(file);
        } catch (...) {
        }
    }

    /// Starts CrabeLoader.asi from the game's folder and logs it when that goes wrong.
    void startCrabeLoader() noexcept
    {
        try {
            const std::filesystem::path folder = executableDirectory();
            const auto result = crabe::bootstrap::startLoader(folder / crabe::bootstrap::kLoaderFileName);
            if (const std::string problem = crabe::bootstrap::describe(result); !problem.empty())
                logWarning(folder, problem);
        } catch (...) {
        }
    }

    /// Puts the entry point's own bytes back, so the game runs its unmodified code from here on.
    void restoreEntry() noexcept
    {
        DWORD protection = 0;
        if (!VirtualProtect(g_entry, kJumpLength, PAGE_EXECUTE_READWRITE, &protection))
            return;
        std::memcpy(g_entry, g_savedEntry.data(), kJumpLength);
        VirtualProtect(g_entry, kJumpLength, protection, &protection);
        FlushInstructionCache(GetCurrentProcess(), g_entry, kJumpLength);
    }

    /// Runs once at the game's entry point, outside the loader lock and before any game code.
    void __cdecl onEntry() noexcept
    {
        restoreEntry();
        startCrabeLoader();
    }

    /// Stands in for the entry point: keeps the registers the system handed it, then jumps to the real entry.
    __declspec(naked) void entryStub()
    {
        __asm {
            pushad
            pushfd
            call onEntry
            popfd
            popad
            jmp dword ptr [g_entryJump]
        }
    }

    /// Writes a jump to entryStub over the first five bytes of the executable's entry point.
    bool patchEntry() noexcept
    {
        const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE)
            return false;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.AddressOfEntryPoint == 0)
            return false;

        g_entry = reinterpret_cast<std::uint8_t*>(base + nt->OptionalHeader.AddressOfEntryPoint);
        g_entryJump = g_entry;
        std::memcpy(g_savedEntry.data(), g_entry, kJumpLength);

        std::array<std::uint8_t, kJumpLength> jump{0xE9};
        const auto relative = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(&entryStub) -
                                                        (reinterpret_cast<std::uintptr_t>(g_entry) + kJumpLength));
        std::memcpy(jump.data() + 1, &relative, sizeof(relative));

        DWORD protection = 0;
        if (!VirtualProtect(g_entry, kJumpLength, PAGE_EXECUTE_READWRITE, &protection))
            return false;
        std::memcpy(g_entry, jump.data(), kJumpLength);
        VirtualProtect(g_entry, kJumpLength, protection, &protection);
        FlushInstructionCache(GetCurrentProcess(), g_entry, kJumpLength);
        return true;
    }

    /// Starts CrabeLoader from a thread of its own when the bootstrap was loaded after the game started.
    DWORD WINAPI lateStart(LPVOID) noexcept
    {
        startCrabeLoader();
        return 0;
    }

} // namespace

/// Arranges for CrabeLoader.asi to start at the game's entry point when the game imported this DLL at
/// startup; a late load starts it from a new thread instead, since LoadLibrary is not allowed in here.
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;
    DisableThreadLibraryCalls(module);
    if (reserved != nullptr && patchEntry())
        return TRUE;
    if (const HANDLE thread = CreateThread(nullptr, 0, &lateStart, nullptr, 0, nullptr))
        CloseHandle(thread);
    return TRUE;
}
