/*
** CrabeLoader
** File description:
** Declares the video resolution and frame rate limit patch for SettingsVideo.
** Populates display presets and frame rate limiting natively into the engine.
** All resolution sorting, deduplication, and limit dispatching reside in C++.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_PATCH_RESOLUTION_HPP_
#define CRABELOADER_INFRASTRUCTURE_PATCH_RESOLUTION_HPP_

namespace crabe::infrastructure::patch {

class PatchResolution final {
public:
    // Applies native resolution presets and frame rate limiting to SettingsVideo.
    static bool apply(void* L);
};

} // namespace crabe::infrastructure::patch

#endif // CRABELOADER_INFRASTRUCTURE_PATCH_RESOLUTION_HPP_
