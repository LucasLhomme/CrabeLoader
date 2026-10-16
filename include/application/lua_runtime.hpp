/*
** CrabeLoader
** File description:
** Declares the injection of src/api/*.lua into a Lua state and the state test that gates it.
** The game creates several Lua states; only the gameplay one may receive the API.
** Contains none of that API -- it lives in src/api/, embedded by tools/embed_api.py.
**
** Authors: @LucasLhomme
*/

#ifndef LUA_RUNTIME_HPP_
#define LUA_RUNTIME_HPP_

#include <filesystem>

namespace crabe::lua_runtime {

    enum class StateKind {
        Unusable,
        NotTheGame,
        Game,
    };

    // Classifies usability and type of given Lua state.
    StateKind classifyState(void* L);

    // Runs every api/*.lua in filename dependency order.
    bool injectAll(void* L);

    // Returns path to Lua API directory.
    std::filesystem::path apiFolder();

    // Registers C++ native functions into the Lua state.
    bool registerNatives(void* L);

} // namespace crabe::lua_runtime

#endif /* !LUA_RUNTIME_HPP_ */
