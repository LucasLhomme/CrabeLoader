/*
** CrabeLoader
** File description:
** Decides whether this copy of the DLL runs the loader or only forwards, when two are loaded.
** A game folder can hold CrabeLoader as bink2w32.dll from 1.x and as dinput8.dll from 1.1 at once.
** Only reads modules already in the process, so it is safe to call from DllMain.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_LOADER_INSTANCE_HPP_
#define CRABELOADER_INFRASTRUCTURE_LOADER_INSTANCE_HPP_

#include <windows.h>

#include <string>
#include <string_view>

namespace crabe::infrastructure {

    /// Ranks the names a CrabeLoader copy can have: bink2w32.dll first, then dinput8.dll, then any other name.
    /// The copy with the best rank runs the loader, so an install that still has a 1.x proxy keeps exactly one.
    /// The comparison ignores case, as Windows does for module names.
    [[nodiscard]] int loaderRank(std::wstring_view moduleFileName) noexcept;

    /// Returns the file name of another loaded CrabeLoader copy that outranks `self`, or an empty string.
    /// A copy is recognised by the loader description in its version resource, which every release carries.
    /// When this returns a name, `self` must only forward its exports and start nothing.
    [[nodiscard]] std::wstring outrankingLoaderCopy(HMODULE self);

    /// Returns the file name a module was loaded under ("dinput8.dll"), or an empty string.
    [[nodiscard]] std::wstring moduleFileName(HMODULE module);

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_LOADER_INSTANCE_HPP_ */
