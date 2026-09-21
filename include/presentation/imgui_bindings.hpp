/*
** CrabeLoader
** File description:
** Declares the registration of Dear ImGui into Lua as the ImGui and Crabe.ImGui tables.
** Every binding records into the draw buffer; none of them calls ImGui at record time.
** Replays nothing -- the render thread does that inside presentation/render_hook.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef IMGUI_BINDINGS_HPP_
#define IMGUI_BINDINGS_HPP_

namespace crabe::presentation {

class ImGuiBindings final {
public:
    ImGuiBindings() = delete;

    /// Registers Dear ImGui functions to Lua table ImGui and Crabe.ImGui.
    static void registerBindings(void* L);
};

} // namespace crabe::presentation

#endif
