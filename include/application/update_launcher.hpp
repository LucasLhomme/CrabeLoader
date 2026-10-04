/*
** CrabeLoader
** File description:
** Declares the one call that starts the update check on a background thread.
** It runs at most once per process, which is what makes "No" last until the next launch.
** Holds no state a mod could reach; Lua never sees it.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_APPLICATION_UPDATE_LAUNCHER_HPP_
#define CRABELOADER_APPLICATION_UPDATE_LAUNCHER_HPP_

namespace crabe::application {

    // Starts the check and returns at once; a no-op when disabled or when already started.
    // Called before the game-build gate: an unrecognised build is when a newer loader helps most.
    void startUpdateCheckInBackground(bool enabled);

} // namespace crabe::application

#endif /* !CRABELOADER_APPLICATION_UPDATE_LAUNCHER_HPP_ */
