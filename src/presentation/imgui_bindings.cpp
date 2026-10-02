/*
** CrabeLoader
** File description:
** Registers the ImGui and Crabe.ImGui tables, every entry recording into the draw buffer.
** A widget result is whatever the render thread measured last frame, so a new widget reads false.
** Calls no ImGui function here -- the replay happens inside Present.
**
** Authors: @LucasLhomme
*/

#include "presentation/imgui_bindings.hpp"
#include "presentation/draw_buffer.hpp"
#include "infrastructure/lua_call.hpp"
#include "shared/logger.hpp"
#include "imgui/imgui.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace crabe::presentation {

namespace {

using crabe::presentation::DrawBuffer;
using crabe::presentation::DrawCommand;
using crabe::presentation::DrawOp;
using crabe::presentation::WidgetResult;

/// Records `command` and reports what the render thread measured for it on the
/// previous frame, leaving `out` untouched when the widget is new.
bool recordAndRead(DrawCommand command, WidgetResult& out)
{
    std::uint32_t id = DrawBuffer::get().record(std::move(command));
    return DrawBuffer::get().tryResult(id, out);
}

/// Records `command` without reading any state back.
void recordOnly(DrawCommand command)
{
    DrawBuffer::get().record(std::move(command));
}

/// Reads argument `idx` as a string, or an empty string when absent.
std::string argText(void* L, int idx)
{
    const char* value = crabe::infrastructure::LuaCall::get().argToString(L, idx);
    return value ? std::string(value) : std::string();
}

/// Queues an ImGui window and pushes its visibility and close-button state.
int __cdecl luaBegin(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::Begin;
    command.label = argText(L, 1);

    int top = lua.getTop(L);
    if (top >= 3) {
        command.b0 = lua.argToBoolean(L, 2, true);
        command.i0 = static_cast<int>(lua.argToNumber(L, 3, 0.0));
    } else if (top == 2) {
        if (lua.isNumber(L, 2)) {
            command.i0 = static_cast<int>(lua.argToNumber(L, 2, 0.0));
        } else {
            command.b0 = lua.argToBoolean(L, 2, true);
        }
    }

    WidgetResult result;
    result.flag = true;
    recordAndRead(std::move(command), result);
    lua.pushBoolean(L, result.flag);
    lua.pushBoolean(L, result.open);
    return 2;
}

/// Queues the end of the current ImGui window.
int __cdecl luaEnd(void* L)
{
    (void)L;
    DrawCommand command;
    command.op = DrawOp::End;
    recordOnly(std::move(command));
    return 0;
}

/// Queues unformatted text in the current ImGui window.
int __cdecl luaText(void* L)
{
    DrawCommand command;
    command.op = DrawOp::Text;
    command.text = argText(L, 1);
    recordOnly(std::move(command));
    return 0;
}

/// Queues colored text using RGBA float components.
int __cdecl luaTextColored(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::TextColored;
    command.f0 = static_cast<float>(lua.argToNumber(L, 1, 1.0));
    command.f1 = static_cast<float>(lua.argToNumber(L, 2, 1.0));
    command.f2 = static_cast<float>(lua.argToNumber(L, 3, 1.0));
    command.f3 = static_cast<float>(lua.argToNumber(L, 4, 1.0));
    command.text = argText(L, 5);
    recordOnly(std::move(command));
    return 0;
}

/// Queues greyed-out text, used for hints and empty-state messages.
int __cdecl luaTextDisabled(void* L)
{
    DrawCommand command;
    command.op = DrawOp::TextDisabled;
    command.text = argText(L, 1);
    recordOnly(std::move(command));
    return 0;
}

/// Queues a selectable row and pushes whether it was clicked.
int __cdecl luaSelectable(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::Selectable;
    command.label = argText(L, 1);
    command.b0 = lua.argToBoolean(L, 2, false);
    command.i0 = static_cast<int>(lua.argToNumber(L, 3, 0.0));

    WidgetResult result;
    recordAndRead(std::move(command), result);
    lua.pushBoolean(L, result.flag);
    return 1;
}

/// Queues a scroll so that the previous row is brought into view.
int __cdecl luaSetScrollHereY(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::SetScrollHereY;
    command.f0 = static_cast<float>(lua.argToNumber(L, 1, 0.5));
    recordOnly(std::move(command));
    return 0;
}

/// Queues a button and pushes whether it was clicked on the previous frame.
int __cdecl luaButton(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::Button;
    command.label = argText(L, 1);
    command.f0 = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    command.f1 = static_cast<float>(lua.argToNumber(L, 3, 0.0));

    WidgetResult result;
    recordAndRead(std::move(command), result);
    lua.pushBoolean(L, result.flag);
    return 1;
}

/// Queues a checkbox and pushes its state, defaulting to the supplied value.
int __cdecl luaCheckbox(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    bool value = lua.argToBoolean(L, 2, false);

    DrawCommand command;
    command.op = DrawOp::Checkbox;
    command.label = argText(L, 1);
    command.b0 = value;

    WidgetResult result;
    result.flag = value;
    recordAndRead(std::move(command), result);
    lua.pushBoolean(L, result.flag);
    return 1;
}

/// Queues a float slider and pushes its value, defaulting to the supplied one.
int __cdecl luaSliderFloat(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    double value = lua.argToNumber(L, 2, 0.0);

    DrawCommand command;
    command.op = DrawOp::SliderFloat;
    command.label = argText(L, 1);
    command.f0 = static_cast<float>(value);
    command.f1 = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    command.f2 = static_cast<float>(lua.argToNumber(L, 4, 1.0));

    WidgetResult result;
    result.number = value;
    recordAndRead(std::move(command), result);
    lua.pushNumber(L, result.number);
    return 1;
}

/// Queues an integer slider and pushes its value, defaulting to the supplied one.
int __cdecl luaSliderInt(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    double value = lua.argToNumber(L, 2, 0.0);

    DrawCommand command;
    command.op = DrawOp::SliderInt;
    command.label = argText(L, 1);
    command.i0 = static_cast<int>(value);
    command.f1 = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    command.f2 = static_cast<float>(lua.argToNumber(L, 4, 100.0));

    WidgetResult result;
    result.number = value;
    recordAndRead(std::move(command), result);
    lua.pushNumber(L, result.number);
    return 1;
}

/// Queues a text input and pushes its buffer plus a changed flag.
int __cdecl luaInputText(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    std::string value = argText(L, 2);

    auto maxLen = static_cast<std::size_t>(lua.argToNumber(L, 3, 256.0));
    if (maxLen < 1)
        maxLen = 256;
    else if (maxLen > 65536)
        maxLen = 65536;

    DrawCommand command;
    command.op = DrawOp::InputText;
    command.label = argText(L, 1);
    command.text = value;
    command.i0 = static_cast<int>(maxLen);

    WidgetResult result;
    result.text = value;
    recordAndRead(std::move(command), result);
    lua.pushString(L, result.text);
    lua.pushBoolean(L, result.flag);
    return 2;
}

/// Queues placement of the next widget on the current line.
int __cdecl luaSameLine(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::SameLine;
    command.f0 = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    command.f1 = static_cast<float>(lua.argToNumber(L, 2, -1.0));
    recordOnly(std::move(command));
    return 0;
}

/// Queues a horizontal separator line.
int __cdecl luaSeparator(void* L)
{
    (void)L;
    DrawCommand command;
    command.op = DrawOp::Separator;
    recordOnly(std::move(command));
    return 0;
}

/// Queues vertical spacing before the next widget.
int __cdecl luaSpacing(void* L)
{
    (void)L;
    DrawCommand command;
    command.op = DrawOp::Spacing;
    recordOnly(std::move(command));
    return 0;
}

/// Queues a scrolling child region and pushes its visibility.
int __cdecl luaBeginChild(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::BeginChild;
    command.label = argText(L, 1);
    command.f0 = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    command.f1 = static_cast<float>(lua.argToNumber(L, 3, 0.0));
    command.b0 = lua.argToBoolean(L, 4, false);
    command.i0 = static_cast<int>(lua.argToNumber(L, 5, 0.0));

    WidgetResult result;
    result.flag = true;
    recordAndRead(std::move(command), result);
    lua.pushBoolean(L, result.flag);
    return 1;
}

/// Queues the end of the current scrolling child region.
int __cdecl luaEndChild(void* L)
{
    (void)L;
    DrawCommand command;
    command.op = DrawOp::EndChild;
    recordOnly(std::move(command));
    return 0;
}

/// Queues a tab bar and pushes whether it was opened.
int __cdecl luaBeginTabBar(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::BeginTabBar;
    command.label = argText(L, 1);
    command.i0 = static_cast<int>(lua.argToNumber(L, 2, 0.0));

    bool opened = DrawBuffer::get().recordScope(std::move(command), true);
    lua.pushBoolean(L, opened);
    return 1;
}

/// Queues the end of the current tab bar.
int __cdecl luaEndTabBar(void* L)
{
    (void)L;
    DrawCommand command;
    command.op = DrawOp::EndTabBar;
    recordOnly(std::move(command));
    return 0;
}

/// Queues a tab and pushes whether it is the selected one. A tab never seen
/// before reports unselected, so its body starts being recorded one frame later.
int __cdecl luaBeginTabItem(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::BeginTabItem;
    command.label = argText(L, 1);
    command.i0 = static_cast<int>(lua.argToNumber(L, 2, 0.0));

    bool selected = DrawBuffer::get().recordScope(std::move(command), false);
    lua.pushBoolean(L, selected);
    return 1;
}

/// Queues the end of the current tab.
int __cdecl luaEndTabItem(void* L)
{
    (void)L;
    DrawCommand command;
    command.op = DrawOp::EndTabItem;
    recordOnly(std::move(command));
    return 0;
}

/// Queues the position of the next created window.
int __cdecl luaSetNextWindowPos(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::SetNextWindowPos;
    command.f0 = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    command.f1 = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    command.i0 = static_cast<int>(lua.argToNumber(L, 3, 0.0));
    recordOnly(std::move(command));
    return 0;
}

/// Queues the size of the next created window.
int __cdecl luaSetNextWindowSize(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::SetNextWindowSize;
    command.f0 = static_cast<float>(lua.argToNumber(L, 1, 0.0));
    command.f1 = static_cast<float>(lua.argToNumber(L, 2, 0.0));
    command.i0 = static_cast<int>(lua.argToNumber(L, 3, 0.0));
    recordOnly(std::move(command));
    return 0;
}

/// Queues a click test against the previously queued widget.
int __cdecl luaIsItemClicked(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    DrawCommand command;
    command.op = DrawOp::IsItemClicked;
    command.i0 = static_cast<int>(lua.argToNumber(L, 1, 0.0));

    WidgetResult result;
    recordAndRead(std::move(command), result);
    lua.pushBoolean(L, result.flag);
    return 1;
}

/// Reads argument `idx` as a packed 0xAABBGGRR colour, the layout ImGui uses.
std::uint32_t argColor(void* L, int idx)
{
    double value = crabe::infrastructure::LuaCall::get().argToNumber(L, idx, 0.0);
    if (value <= 0.0)
        return 0;
    if (value >= 4294967295.0)
        return 0xFFFFFFFFu;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}

float argFloat(void* L, int idx, double fallback = 0.0)
{
    return static_cast<float>(crabe::infrastructure::LuaCall::get().argToNumber(L, idx, fallback));
}

/// Queues keyboard focus for the next widget, or `offset` widgets further on.
int __cdecl luaSetKeyboardFocusHere(void* L)
{
    DrawCommand command;
    command.op = DrawOp::SetKeyboardFocusHere;
    command.i0 = static_cast<int>(argFloat(L, 1));
    recordOnly(std::move(command));
    return 0;
}

/// DrawRect(x, y, w, h, color [, rounding [, thickness]]): filled, or an
/// outline when thickness is above zero. Drawn behind every ImGui window.
int __cdecl luaDrawRect(void* L)
{
    DrawCommand command;
    command.op = DrawOp::ShapeRect;
    command.f0 = argFloat(L, 1);
    command.f1 = argFloat(L, 2);
    command.f2 = argFloat(L, 3);
    command.f3 = argFloat(L, 4);
    command.c0 = argColor(L, 5);
    command.f4 = argFloat(L, 6);
    command.i0 = static_cast<int>(argFloat(L, 7));
    recordOnly(std::move(command));
    return 0;
}

/// DrawGradient(x, y, w, h, colorA, colorB [, horizontal]): colorA at the top
/// (or the left edge when horizontal) fading into colorB.
int __cdecl luaDrawGradient(void* L)
{
    DrawCommand command;
    command.op = DrawOp::ShapeGradient;
    command.f0 = argFloat(L, 1);
    command.f1 = argFloat(L, 2);
    command.f2 = argFloat(L, 3);
    command.f3 = argFloat(L, 4);
    command.c0 = argColor(L, 5);
    command.c1 = argColor(L, 6);
    command.b0 = crabe::infrastructure::LuaCall::get().argToBoolean(L, 7, false);
    recordOnly(std::move(command));
    return 0;
}

/// DrawLine(x1, y1, x2, y2, color [, thickness]).
int __cdecl luaDrawLine(void* L)
{
    DrawCommand command;
    command.op = DrawOp::ShapeLine;
    command.f0 = argFloat(L, 1);
    command.f1 = argFloat(L, 2);
    command.f2 = argFloat(L, 3);
    command.f3 = argFloat(L, 4);
    command.c0 = argColor(L, 5);
    command.f4 = argFloat(L, 6, 1.0);
    recordOnly(std::move(command));
    return 0;
}

/// DrawCircle(x, y, radius, color [, thickness]): filled unless thickness > 0.
int __cdecl luaDrawCircle(void* L)
{
    DrawCommand command;
    command.op = DrawOp::ShapeCircle;
    command.f0 = argFloat(L, 1);
    command.f1 = argFloat(L, 2);
    command.f2 = argFloat(L, 3);
    command.c0 = argColor(L, 4);
    command.f4 = argFloat(L, 5);
    recordOnly(std::move(command));
    return 0;
}

/// DrawText(x, y, text, color [, size [, font [, align [, width [, shadow]]]]]).
/// font: 0 default, 1 body, 2 display. align: 0 left, 1 centre, 2 right inside width.
int __cdecl luaDrawText(void* L)
{
    DrawCommand command;
    command.op = DrawOp::ShapeText;
    command.f0 = argFloat(L, 1);
    command.f1 = argFloat(L, 2);
    command.text = argText(L, 3);
    command.c0 = argColor(L, 4);
    command.f3 = argFloat(L, 5);
    command.i1 = static_cast<int>(argFloat(L, 6));
    command.i0 = static_cast<int>(argFloat(L, 7));
    command.f2 = argFloat(L, 8);
    command.c1 = argColor(L, 9);
    recordOnly(std::move(command));
    return 0;
}

/// Pushes the width and height the render thread saw last frame (0, 0 before it ran).
int __cdecl luaGetDisplaySize(void* L)
{
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    float width = 0.0f;
    float height = 0.0f;
    DrawBuffer::get().displaySize(width, height);
    lua.pushNumber(L, width);
    lua.pushNumber(L, height);
    return 2;
}

}

/// Registers all ImGui bindings to global ImGui and Crabe.ImGui tables.
void ImGuiBindings::registerBindings(void* L)
{
    if (!L) {
        return;
    }
    crabe::infrastructure::LuaCall& lua = crabe::infrastructure::LuaCall::get();
    lua.runSnippet(L, "ImGui = ImGui or {}; Crabe = Crabe or {}; Crabe.ImGui = ImGui;");

    struct Entry {
        const char* name;
        crabe::infrastructure::LuaCall::t_lua_cfunction fn;
    };

    static constexpr Entry kEntries[] = {
        { "Begin", &luaBegin },
        { "End", &luaEnd },
        { "Text", &luaText },
        { "TextColored", &luaTextColored },
        { "TextDisabled", &luaTextDisabled },
        { "Button", &luaButton },
        { "Selectable", &luaSelectable },
        { "SetScrollHereY", &luaSetScrollHereY },
        { "Checkbox", &luaCheckbox },
        { "SliderFloat", &luaSliderFloat },
        { "SliderInt", &luaSliderInt },
        { "InputText", &luaInputText },
        { "SameLine", &luaSameLine },
        { "Separator", &luaSeparator },
        { "Spacing", &luaSpacing },
        { "BeginChild", &luaBeginChild },
        { "EndChild", &luaEndChild },
        { "BeginTabBar", &luaBeginTabBar },
        { "EndTabBar", &luaEndTabBar },
        { "BeginTabItem", &luaBeginTabItem },
        { "EndTabItem", &luaEndTabItem },
        { "SetNextWindowPos", &luaSetNextWindowPos },
        { "SetNextWindowSize", &luaSetNextWindowSize },
        { "IsItemClicked", &luaIsItemClicked },
        { "SetKeyboardFocusHere", &luaSetKeyboardFocusHere },
        { "DrawRect", &luaDrawRect },
        { "DrawGradient", &luaDrawGradient },
        { "DrawLine", &luaDrawLine },
        { "DrawCircle", &luaDrawCircle },
        { "DrawText", &luaDrawText },
        { "GetDisplaySize", &luaGetDisplaySize }
    };

    for (const auto& entry : kEntries) {
        if (!lua.registerNativeFunction(L, "ImGui", entry.name, entry.fn)) {
            crabe::shared::Logger::getInstance().error("ImGuiBindings: failed to register ImGui.{}", entry.name);
        }
    }
    lua.runSnippet(L, "Crabe.ImGui = ImGui;");
}

} // namespace crabe::presentation

