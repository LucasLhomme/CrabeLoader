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

/// Hooks Win32 file creation primitives to redirect game requests to loose mod assets.
class VfsHook final {
public:
    static VfsHook& get() noexcept;

    /// Installs MinHook detours on Win32 CreateFileA and CreateFileW in kernel32.dll.
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

    Hook _hookCreateFileA;
    Hook _hookCreateFileW;
    bool _initialized{false};
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_VFS_HOOK_HPP_
