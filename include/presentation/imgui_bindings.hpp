#ifndef IMGUI_BINDINGS_HPP_
#define IMGUI_BINDINGS_HPP_

class ImGuiBindings final {
public:
    ImGuiBindings() = delete;

    /// Registers Dear ImGui functions to Lua table ImGui and Crabe.ImGui.
    static void registerBindings(void* L);
};

#endif
