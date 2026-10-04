/*
** CrabeLoader
** File description:
** Implements MinHook interception for Win32 CreateFile, GetFileAttributes and FindFirstFile.
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

using GetFileAttributesAFn = DWORD(WINAPI*)(LPCSTR);

using GetFileAttributesExAFn = BOOL(WINAPI*)(LPCSTR, GET_FILEEX_INFO_LEVELS, LPVOID);

using FindFirstFileAFn = HANDLE(WINAPI*)(LPCSTR, LPWIN32_FIND_DATAA);

using FindFirstFileExAFn = HANDLE(WINAPI*)(
    LPCSTR, FINDEX_INFO_LEVELS, LPVOID, FINDEX_SEARCH_OPS, LPVOID, DWORD);

using FindFirstFileExWFn = HANDLE(WINAPI*)(
    LPCWSTR, FINDEX_INFO_LEVELS, LPVOID, FINDEX_SEARCH_OPS, LPVOID, DWORD);

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

// Calls `call` with the mod override of an ANSI path when one is indexed, else with the path itself.
// Re-entrant calls and SEH faults fall through to the original path untouched.
template <typename Result, typename Call>
Result redirectAnsi(LPCSTR path, Result failure, const Call& call, const char* scope)
{
    if (t_reentrancyGuard || !path)
        return call(path);

    ReentrancyScope guard;
    Result result = failure;

    bool executed = CrashHandler::runGuarded([&]() {
        std::filesystem::path overridePath;
        if (VfsOverrideManager::get().resolve(path, overridePath))
            result = call(overridePath.string().c_str());
        else
            result = call(path);
    }, scope);

    if (!executed)
        result = call(path);
    return result;
}

// Wide-character counterpart of redirectAnsi.
template <typename Result, typename Call>
Result redirectWide(LPCWSTR path, Result failure, const Call& call, const char* scope)
{
    if (t_reentrancyGuard || !path)
        return call(path);

    ReentrancyScope guard;
    Result result = failure;

    bool executed = CrashHandler::runGuarded([&]() {
        std::string narrowName = wideToUtf8(path);
        std::filesystem::path overridePath;
        if (!narrowName.empty() && VfsOverrideManager::get().resolve(narrowName, overridePath))
            result = call(overridePath.c_str());
        else
            result = call(path);
    }, scope);

    if (!executed)
        result = call(path);
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

    struct Target {
        Hook& hook;
        const char* exportName;
        void* detour;
        const char* label;
    };

    const Target targets[] = {
        {_hookCreateFileA, "CreateFileA", reinterpret_cast<void*>(&VfsHook::hkCreateFileA), "VfsHook::CreateFileA"},
        {_hookCreateFileW, "CreateFileW", reinterpret_cast<void*>(&VfsHook::hkCreateFileW), "VfsHook::CreateFileW"},
        {_hookGetFileAttributesA, "GetFileAttributesA", reinterpret_cast<void*>(&VfsHook::hkGetFileAttributesA),
         "VfsHook::GetFileAttributesA"},
        {_hookGetFileAttributesExA, "GetFileAttributesExA", reinterpret_cast<void*>(&VfsHook::hkGetFileAttributesExA),
         "VfsHook::GetFileAttributesExA"},
        {_hookFindFirstFileA, "FindFirstFileA", reinterpret_cast<void*>(&VfsHook::hkFindFirstFileA),
         "VfsHook::FindFirstFileA"},
        {_hookFindFirstFileExA, "FindFirstFileExA", reinterpret_cast<void*>(&VfsHook::hkFindFirstFileExA),
         "VfsHook::FindFirstFileExA"},
        {_hookFindFirstFileExW, "FindFirstFileExW", reinterpret_cast<void*>(&VfsHook::hkFindFirstFileExW),
         "VfsHook::FindFirstFileExW"},
    };

    for (const Target& target : targets) {
        void* address = reinterpret_cast<void*>(GetProcAddress(kernel32, target.exportName));
        if (!address || !target.hook.install(address, target.detour, target.label)) {
            logger.error("VfsHook: failed to hook kernel32!{}.", target.exportName);
            for (const Target& installed : targets)
                installed.hook.remove();
            return false;
        }
    }

    _initialized = true;
    logger.info("VfsHook: CreateFile, GetFileAttributes and FindFirstFile detours successfully installed.");
    return true;
}

void VfsHook::uninitialize()
{
    if (!_initialized)
        return;

    _hookCreateFileA.remove();
    _hookCreateFileW.remove();
    _hookGetFileAttributesA.remove();
    _hookGetFileAttributesExA.remove();
    _hookFindFirstFileA.remove();
    _hookFindFirstFileExA.remove();
    _hookFindFirstFileExW.remove();
    _initialized = false;
    crabe::shared::Logger::getInstance().info("VfsHook: Win32 file detours uninstalled.");
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
    auto orig = reinterpret_cast<CreateFileAFn>(get()._hookCreateFileA.getOriginal());
    if (!orig)
        return INVALID_HANDLE_VALUE;

    return redirectAnsi(lpFileName, INVALID_HANDLE_VALUE, [&](LPCSTR path) {
        return orig(path, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }, "VfsHook::hkCreateFileA");
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
    auto orig = reinterpret_cast<CreateFileWFn>(get()._hookCreateFileW.getOriginal());
    if (!orig)
        return INVALID_HANDLE_VALUE;

    return redirectWide(lpFileName, INVALID_HANDLE_VALUE, [&](LPCWSTR path) {
        return orig(path, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }, "VfsHook::hkCreateFileW");
}

DWORD WINAPI VfsHook::hkGetFileAttributesA(LPCSTR lpFileName)
{
    auto orig = reinterpret_cast<GetFileAttributesAFn>(get()._hookGetFileAttributesA.getOriginal());
    if (!orig)
        return INVALID_FILE_ATTRIBUTES;

    return redirectAnsi(lpFileName, INVALID_FILE_ATTRIBUTES, [&](LPCSTR path) {
        return orig(path);
    }, "VfsHook::hkGetFileAttributesA");
}

BOOL WINAPI VfsHook::hkGetFileAttributesExA(
    LPCSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation)
{
    auto orig = reinterpret_cast<GetFileAttributesExAFn>(get()._hookGetFileAttributesExA.getOriginal());
    if (!orig)
        return FALSE;

    return redirectAnsi(lpFileName, FALSE, [&](LPCSTR path) {
        return orig(path, fInfoLevelId, lpFileInformation);
    }, "VfsHook::hkGetFileAttributesExA");
}

HANDLE WINAPI VfsHook::hkFindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    auto orig = reinterpret_cast<FindFirstFileAFn>(get()._hookFindFirstFileA.getOriginal());
    if (!orig)
        return INVALID_HANDLE_VALUE;

    return redirectAnsi(lpFileName, INVALID_HANDLE_VALUE, [&](LPCSTR path) {
        return orig(path, lpFindFileData);
    }, "VfsHook::hkFindFirstFileA");
}

HANDLE WINAPI VfsHook::hkFindFirstFileExA(
    LPCSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags)
{
    auto orig = reinterpret_cast<FindFirstFileExAFn>(get()._hookFindFirstFileExA.getOriginal());
    if (!orig)
        return INVALID_HANDLE_VALUE;

    return redirectAnsi(lpFileName, INVALID_HANDLE_VALUE, [&](LPCSTR path) {
        return orig(path, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    }, "VfsHook::hkFindFirstFileExA");
}

HANDLE WINAPI VfsHook::hkFindFirstFileExW(
    LPCWSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags)
{
    auto orig = reinterpret_cast<FindFirstFileExWFn>(get()._hookFindFirstFileExW.getOriginal());
    if (!orig)
        return INVALID_HANDLE_VALUE;

    return redirectWide(lpFileName, INVALID_HANDLE_VALUE, [&](LPCWSTR path) {
        return orig(path, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    }, "VfsHook::hkFindFirstFileExW");
}

} // namespace crabe::infrastructure
