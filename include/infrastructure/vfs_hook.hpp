/*
** CrabeLoader
** File description:
** Declares MinHook detours intercepting Win32 file creation for transparent VFS redirection.
** Runs under Structured Exception Handling (SEH) crash armor and prevents recursive I/O loops.
** Performs no indexing itself; resolution is deferred to infrastructure/vfs_override_manager.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_VFS_HOOK_HPP_
#define CRABELOADER_INFRASTRUCTURE_VFS_HOOK_HPP_

#include <windows.h>
#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

/// Hooks Win32 file creation and existence queries to redirect game requests to loose mod assets.
/// Existence queries matter for files that exist only inside a mod: the engine probes
/// GetFileAttributes / FindFirstFile before opening, and would skip them otherwise.
class VfsHook final {
public:
    static VfsHook& get() noexcept;

    /// Installs MinHook detours on CreateFile, GetFileAttributes and FindFirstFile in kernel32.dll.
    bool initialize();

    /// Uninstalls all active file system interception hooks.
    void uninitialize();

    /// Returns whether file interception hooks are currently installed.
    [[nodiscard]] bool isHooked() const noexcept;

private:
    VfsHook() = default;
    ~VfsHook() = default;

    VfsHook(const VfsHook&) = delete;
    VfsHook& operator=(const VfsHook&) = delete;

    static HANDLE WINAPI hkCreateFileA(
        LPCSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile);

    static HANDLE WINAPI hkCreateFileW(
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile);

    static DWORD WINAPI hkGetFileAttributesA(LPCSTR lpFileName);

    static BOOL WINAPI hkGetFileAttributesExA(
        LPCSTR lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        LPVOID lpFileInformation);

    static HANDLE WINAPI hkFindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);

    static HANDLE WINAPI hkFindFirstFileExA(
        LPCSTR lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        LPVOID lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        LPVOID lpSearchFilter,
        DWORD dwAdditionalFlags);

    static HANDLE WINAPI hkFindFirstFileExW(
        LPCWSTR lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        LPVOID lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        LPVOID lpSearchFilter,
        DWORD dwAdditionalFlags);

    Hook _hookCreateFileA;
    Hook _hookCreateFileW;
    Hook _hookGetFileAttributesA;
    Hook _hookGetFileAttributesExA;
    Hook _hookFindFirstFileA;
    Hook _hookFindFirstFileExA;
    Hook _hookFindFirstFileExW;
    bool _initialized{false};
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_VFS_HOOK_HPP_
