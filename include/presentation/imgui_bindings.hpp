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
