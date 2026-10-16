/*
** CrabeLoader
** File description:
** Declares a primitive that runs a callback on the main thread just before the exe's entry point.
** That is after every DLL the game imports has started, and before any game code has run.
** Knows nothing of what the callback does; main.cpp uses it to load .asi plugins.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_ENTRY_POINT_HOOK_HPP_
#define CRABELOADER_INFRASTRUCTURE_ENTRY_POINT_HOOK_HPP_

namespace crabe::infrastructure {

    using EntryCallback = void (*)();

    /// Detours the executable's entry point so `callback` runs once there, then the game starts as usual.
    /// Call it from DllMain of a DLL the executable imports, while the entry point has not run yet.
    /// Returns false when the detour could not be installed; the callback is then never called.
    [[nodiscard]] bool runBeforeEntryPoint(EntryCallback callback);

} // namespace crabe::infrastructure

#endif /* !CRABELOADER_INFRASTRUCTURE_ENTRY_POINT_HOOK_HPP_ */
