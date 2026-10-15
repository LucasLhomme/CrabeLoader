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

class VfsHook final {
public:
    // Returns the singleton instance of VfsHook.
    static VfsHook& get() noexcept;

    // Installs Win32 filesystem interception hooks in kernel32.dll.
    bool initialize();

    // Uninstalls all active filesystem interception hooks.
    void uninitialize();

    // Returns whether filesystem interception hooks are installed.
    [[nodiscard]] bool isHooked() const noexcept;

    // Enables or disables logging for texture file resolution paths.
    static void setTraceTextures(bool enabled) noexcept;

private:
    VfsHook() = default;
    ~VfsHook() = default;

    VfsHook(const VfsHook&) = delete;
    VfsHook& operator=(const VfsHook&) = delete;

    // Detour for CreateFileA.
    static HANDLE WINAPI hkCreateFileA(
        LPCSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile);

    // Detour for CreateFileW.
    static HANDLE WINAPI hkCreateFileW(
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile);

    // Detour for GetFileAttributesA.
    static DWORD WINAPI hkGetFileAttributesA(LPCSTR lpFileName);

    // Detour for GetFileAttributesExA.
    static BOOL WINAPI hkGetFileAttributesExA(
        LPCSTR lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        LPVOID lpFileInformation);

    // Detour for FindFirstFileA.
    static HANDLE WINAPI hkFindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);

    // Detour for FindFirstFileExA.
    static HANDLE WINAPI hkFindFirstFileExA(
        LPCSTR lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        LPVOID lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        LPVOID lpSearchFilter,
        DWORD dwAdditionalFlags);

    // Detour for FindFirstFileExW.
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
