/*
** CrabeLoader
** File description:
** Declares the in-game key and controller bindings patch for InGameSettingsMenu.
** Integrates keyboard, mouse, and gamepad remapping rows directly into game settings.
** Interacts directly with the input remapping subsystem through InputRemap.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_PATCH_KEYBIND_HPP_
#define CRABELOADER_INFRASTRUCTURE_PATCH_KEYBIND_HPP_

namespace crabe::infrastructure::patch {

class PatchKeybind final {
public:
    // Applies native keybind options and hooks to InGameSettingsMenu.
    static bool apply(void* L);
};

} // namespace crabe::infrastructure::patch

#endif // CRABELOADER_INFRASTRUCTURE_PATCH_KEYBIND_HPP_

