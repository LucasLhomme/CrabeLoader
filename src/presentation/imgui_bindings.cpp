#include "presentation/imgui_bindings.hpp"
#include "infrastructure/luacall.hpp"
#include "shared/logger.hpp"
#include "imgui/imgui.h"

#include <cstddef>
#include <cstring>
#include <vector>

namespace {

/// Begins an ImGui window and pushes its visibility boolean to Lua.
int __cdecl luaBegin(void* L)
{
    LuaCall& lua = LuaCall::get();
    const char* name = lua.argToString(L, 1);
    int top = lua.getTop(L);
    bool visible = false;
    if (top >= 3) {
        bool openVal = lua.argToBoolean(L, 2, true);
        auto flags = static_cast<ImGuiWindowFlags>(lua.argToNumber(L, 3, 0.0));
        visible = ImGui::Begin(name ? name : "", &openVal, flags);
    } else if (top == 2) {
        if (lua.isNumber(L, 2)) {
            auto flags = static_cast<ImGuiWindowFlags>(lua.argToNumber(L, 2, 0.0));
            visible = ImGui::Begin(name ? name : "", nullptr, flags);
        } else {
            bool openVal = lua.argToBoolean(L, 2, true);
            visible = ImGui::Begin(name ? name : "", &openVal, 0);
        }
    } else {
        visible = ImGui::Begin(name ? name : "", nullptr, 0);
    }
    lua.pushBoolean(L, visible);
    return 1;
}

/// Ends the current ImGui window.
int __cdecl luaEnd(void* L)
{
    (void)L;
    ImGui::End();
    return 0;
}

/// Renders unformatted text in the current ImGui window.
int __cdecl luaText(void* L)
{
    const char* text = LuaCall::get().argToString(L, 1);
    ImGui::TextUnformatted(text ? text : "");
    return 0;
}

/// Renders colored text using RGBA float components.
int __cdecl luaTextColored(void* L)
{
    LuaCall& lua = LuaCall::get();
    auto r = static_cast<float>(lua.argToNumber(L, 1, 1.0));
    auto g = static_cast<float>(lua.argToNumber(L, 2, 1.0));
    auto b = static_cast<float>(lua.argToNumber(L, 3, 1.0));
    auto a = static_cast<float>(lua.argToNumber(L, 4, 1.0));
    const char* text = lua.argToString(L, 5);
    ImGui::TextColored(ImVec4(r, g, b, a), "%s", text ? text : "");
    return 0;
}

/// Renders a button and returns whether it was clicked.
int __cdecl luaButton(void* L)
{
    LuaCall& lua = LuaCall::get();
    const char* label = lua.argToString(L, 1);
    auto w = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    auto h = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    bool clicked = ImGui::Button(label ? label : "", ImVec2(w, h));
    lua.pushBoolean(L, clicked);
    return 1;
}

/// Renders a checkbox and returns its new boolean state.
int __cdecl luaCheckbox(void* L)
{
    LuaCall& lua = LuaCall::get();
    const char* label = lua.argToString(L, 1);
    bool value = lua.argToBoolean(L, 2, false);
    ImGui::Checkbox(label ? label : "", &value);
    lua.pushBoolean(L, value);
    return 1;
}

/// Renders a float slider and returns its updated value.
int __cdecl luaSliderFloat(void* L)
{
    LuaCall& lua = LuaCall::get();
    const char* label = lua.argToString(L, 1);
    auto val = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    auto min = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    auto max = static_cast<float>(lua.argToNumber(L, 4, 1.0));
    ImGui::SliderFloat(label ? label : "", &val, min, max);
    lua.pushNumber(L, static_cast<double>(val));
    return 1;
}

/// Renders an integer slider and returns its updated value.
int __cdecl luaSliderInt(void* L)
{
    LuaCall& lua = LuaCall::get();
    const char* label = lua.argToString(L, 1);
    auto val = static_cast<int>(lua.argToNumber(L, 2, 0.0));
    auto min = static_cast<int>(lua.argToNumber(L, 3, 0.0));
    auto max = static_cast<int>(lua.argToNumber(L, 4, 100.0));
    ImGui::SliderInt(label ? label : "", &val, min, max);
    lua.pushNumber(L, static_cast<double>(val));
    return 1;
}

/// Renders a text input field and returns updated text and changed flag.
int __cdecl luaInputText(void* L)
{
    LuaCall& lua = LuaCall::get();
    const char* label = lua.argToString(L, 1);
    const char* val = lua.argToString(L, 2);
    auto maxLen = static_cast<size_t>(lua.argToNumber(L, 3, 256.0));
    if (maxLen < 1) {
        maxLen = 256;
    } else if (maxLen > 65536) {
        maxLen = 65536;
    }
    std::vector<char> buffer(maxLen + 1, '\0');
    if (val) {
        std::strncpy(buffer.data(), val, maxLen);
        buffer[maxLen] = '\0';
    }
    bool changed = ImGui::InputText(label ? label : "", buffer.data(), buffer.size());
    lua.pushString(L, buffer.data());
    lua.pushBoolean(L, changed);
    return 2;
}

/// Positions the next widget on the same line.
int __cdecl luaSameLine(void* L)
{
    LuaCall& lua = LuaCall::get();
    auto offset = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    auto spacing = static_cast<float>(lua.argToNumber(L, 2, -1.0));
    ImGui::SameLine(offset, spacing);
    return 0;
}

/// Renders a horizontal separator line.
int __cdecl luaSeparator(void* L)
{
    (void)L;
    ImGui::Separator();
    return 0;
}

/// Adds vertical spacing before the next widget.
int __cdecl luaSpacing(void* L)
{
    (void)L;
    ImGui::Spacing();
    return 0;
}

/// Begins a scrolling child region and returns its visibility.
int __cdecl luaBeginChild(void* L)
{
    LuaCall& lua = LuaCall::get();
    const char* id = lua.argToString(L, 1);
    auto w = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    auto h = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    bool border = lua.argToBoolean(L, 4, false);
    auto flags = static_cast<ImGuiWindowFlags>(lua.argToNumber(L, 5, 0.0));
    ImGuiChildFlags childFlags = border ? ImGuiChildFlags_Border : ImGuiChildFlags_None;
    bool visible = ImGui::BeginChild(id ? id : "", ImVec2(w, h), childFlags, flags);
    lua.pushBoolean(L, visible);
    return 1;
}

/// Ends the current scrolling child region.
int __cdecl luaEndChild(void* L)
{
    (void)L;
    ImGui::EndChild();
    return 0;
}

/// Sets the position for the next created window.
int __cdecl luaSetNextWindowPos(void* L)
{
    LuaCall& lua = LuaCall::get();
    auto x = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    auto y = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    auto cond = static_cast<ImGuiCond>(lua.argToNumber(L, 3, 0.0));
    ImGui::SetNextWindowPos(ImVec2(x, y), cond);
    return 0;
}

/// Sets the size for the next created window.
int __cdecl luaSetNextWindowSize(void* L)
{
    LuaCall& lua = LuaCall::get();
    auto w = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    auto h = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    auto cond = static_cast<ImGuiCond>(lua.argToNumber(L, 3, 0.0));
    ImGui::SetNextWindowSize(ImVec2(w, h), cond);
    return 0;
}

/// Checks if the previous item was clicked with the specified mouse button.
int __cdecl luaIsItemClicked(void* L)
{
    LuaCall& lua = LuaCall::get();
    auto button = static_cast<ImGuiMouseButton>(lua.argToNumber(L, 1, 0.0));
    bool clicked = ImGui::IsItemClicked(button);
    lua.pushBoolean(L, clicked);
    return 1;
}

}

/// Registers all ImGui bindings to global ImGui and Crabe.ImGui tables.
void ImGuiBindings::registerBindings(void* L)
{
    if (!L) {
        return;
    }
    LuaCall& lua = LuaCall::get();
    lua.runSnippet(L, "ImGui = ImGui or {}; Crabe = Crabe or {}; Crabe.ImGui = ImGui;");

    struct Entry {
        const char* name;
        LuaCall::t_lua_cfunction fn;
    };

    static constexpr Entry kEntries[] = {
        { "Begin", &luaBegin },
        { "End", &luaEnd },
        { "Text", &luaText },
        { "TextColored", &luaTextColored },
        { "Button", &luaButton },
        { "Checkbox", &luaCheckbox },
        { "SliderFloat", &luaSliderFloat },
        { "SliderInt", &luaSliderInt },
        { "InputText", &luaInputText },
        { "SameLine", &luaSameLine },
        { "Separator", &luaSeparator },
        { "Spacing", &luaSpacing },
        { "BeginChild", &luaBeginChild },
        { "EndChild", &luaEndChild },
        { "SetNextWindowPos", &luaSetNextWindowPos },
        { "SetNextWindowSize", &luaSetNextWindowSize },
        { "IsItemClicked", &luaIsItemClicked }
    };

    for (const auto& entry : kEntries) {
        if (!lua.registerNativeFunction(L, "ImGui", entry.name, entry.fn)) {
            Logger::getInstance().error("ImGuiBindings: failed to register ImGui.{}", entry.name);
        }
    }
    lua.runSnippet(L, "Crabe.ImGui = ImGui;");
}
