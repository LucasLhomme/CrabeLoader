/*
** CrabeLoader
** File description:
** Declares the borderless window mode toggle patch for SettingsVideo.
** Exposes native window mode toggling directly inside the video settings menu.
** Interacts directly with the display subsystem through RenderHook.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_PATCH_BORDERLESS_HPP_
#define CRABELOADER_INFRASTRUCTURE_PATCH_BORDERLESS_HPP_

namespace crabe::infrastructure::patch {

class PatchBorderless final {
public:
    // Injects the borderless window toggle option into SettingsVideo.
    static bool apply(void* L);
};

} // namespace crabe::infrastructure::patch

#endif // CRABELOADER_INFRASTRUCTURE_PATCH_BORDERLESS_HPP_
