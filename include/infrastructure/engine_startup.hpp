/*
** CrabeLoader
** File description:
** Declares the patch that removes the game's startup logo movies.
** Redirects the logo loop of the engine's PlayStartupFMVS past its movie playback.
** Knows no gameplay rules: it only shortens the launch when crabe.toml asks for it.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_STARTUP_HPP_
#define ENGINE_STARTUP_HPP_

namespace crabe::infrastructure {

class EngineStartup final {
public:
    static EngineStartup& get();

    // Sends the startup logo loop straight to the end of each pass, so no logo movie plays.
    // Returns true when the patch is in place, false if the profile or the code does not match.
    [[nodiscard]] bool skipLogos();

private:
    EngineStartup() = default;

    bool _applied{false};
};

} // namespace crabe::infrastructure

#endif
