/*
** CrabeLoader
** File description:
** Implements MinHook interception for Win32 CreateFileA and CreateFileW calls.
** Substitutes requested asset paths with loose files from active mods under SEH armor.
** Indexing and priority rules are delegated to infrastructure/vfs_override_manager.cpp.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/vfs_hook.hpp"

#include <filesystem>
#include <string>

#include "infrastructure/crash_handler.hpp"
#include "infrastructure/vfs_override_manager.hpp"
#include "shared/logger.hpp"

namespace crabe::infrastructure {

namespace {

using CreateFileAFn = HANDLE(WINAPI*)(
    LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

using CreateFileWFn = HANDLE(WINAPI*)(
    LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

thread_local bool t_reentrancyGuard = false;

struct ReentrancyScope final {
    ReentrancyScope() noexcept { t_reentrancyGuard = true; }
    ~ReentrancyScope() noexcept { t_reentrancyGuard = false; }
    ReentrancyScope(const ReentrancyScope&) = delete;
    ReentrancyScope& operator=(const ReentrancyScope&) = delete;
};

std::string wideToUtf8(LPCWSTR wideStr)
{
    if (!wideStr || wideStr[0] == L'\0')
        return {};

    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (requiredSize <= 1)
        return {};

    std::string result(static_cast<std::size_t>(requiredSize - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, result.data(), requiredSize, nullptr, nullptr);
    return result;
}

} // namespace

VfsHook& VfsHook::get() noexcept
{
    static VfsHook instance;
    return instance;
}

bool VfsHook::initialize()
{
    if (_initialized)
        return true;

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) {
        logger.error("VfsHook: failed to get handle for kernel32.dll.");
        return false;
    }

    auto pCreateFileA = reinterpret_cast<void*>(GetProcAddress(kernel32, "CreateFileA"));
    auto pCreateFileW = reinterpret_cast<void*>(GetProcAddress(kernel32, "CreateFileW"));

    if (!pCreateFileA || !pCreateFileW) {
        logger.error("VfsHook: failed to resolve CreateFile addresses in kernel32.dll.");
        return false;
    }

    bool okA = _hookCreateFileA.install(pCreateFileA, reinterpret_cast<void*>(&VfsHook::hkCreateFileA),
                                        "VfsHook::CreateFileA");
    bool okW = _hookCreateFileW.install(pCreateFileW, reinterpret_cast<void*>(&VfsHook::hkCreateFileW),
                                        "VfsHook::CreateFileW");

    if (!okA || !okW) {
        logger.error("VfsHook: failed to hook Win32 CreateFile routines (A: {}, W: {}).", okA, okW);
        _hookCreateFileA.remove();
        _hookCreateFileW.remove();
        return false;
    }

    _initialized = true;
    logger.info("VfsHook: Win32 CreateFileA and CreateFileW detours successfully installed.");
    return true;
}

void VfsHook::uninitialize()
{
    if (!_initialized)
        return;

    _hookCreateFileA.remove();
    _hookCreateFileW.remove();
    _initialized = false;
    crabe::shared::Logger::getInstance().info("VfsHook: Win32 CreateFile detours uninstalled.");
}

bool VfsHook::isHooked() const noexcept
{
    return _initialized;
}

HANDLE WINAPI VfsHook::hkCreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    VfsHook& self = get();
    auto orig = reinterpret_cast<CreateFileAFn>(self._hookCreateFileA.getOriginal());

    if (t_reentrancyGuard || !lpFileName || !orig) {
        return orig ? orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                           dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
                    : INVALID_HANDLE_VALUE;
    }

    ReentrancyScope guard;
    HANDLE handle = INVALID_HANDLE_VALUE;

    bool executed = CrashHandler::runGuarded([&]() {
        std::filesystem::path overridePath;
        if (VfsOverrideManager::get().resolve(lpFileName, overridePath)) {
            handle = orig(overridePath.string().c_str(), dwDesiredAccess, dwShareMode,
                          lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
                          hTemplateFile);
        } else {
            handle = orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                          dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
    }, "VfsHook::hkCreateFileA");

    if (!executed && orig) {
        handle = orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                      dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    return handle;
}

HANDLE WINAPI VfsHook::hkCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    VfsHook& self = get();
    auto orig = reinterpret_cast<CreateFileWFn>(self._hookCreateFileW.getOriginal());

    if (t_reentrancyGuard || !lpFileName || !orig) {
        return orig ? orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                           dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
                    : INVALID_HANDLE_VALUE;
    }

    ReentrancyScope guard;
    HANDLE handle = INVALID_HANDLE_VALUE;

    bool executed = CrashHandler::runGuarded([&]() {
        std::string narrowName = wideToUtf8(lpFileName);
        std::filesystem::path overridePath;
        if (!narrowName.empty() && VfsOverrideManager::get().resolve(narrowName, overridePath)) {
            handle = orig(overridePath.c_str(), dwDesiredAccess, dwShareMode,
                          lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
                          hTemplateFile);
        } else {
            handle = orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                          dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
    }, "VfsHook::hkCreateFileW");

    if (!executed && orig) {
        handle = orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                      dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    return handle;
}

} // namespace crabe::infrastructure
