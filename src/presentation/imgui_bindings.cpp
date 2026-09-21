#include "presentation/imgui_bindings.hpp"
#include "presentation/draw_buffer.hpp"
#include "infrastructure/lua_call.hpp"
#include "shared/logger.hpp"
#include "imgui/imgui.h"

#include <cstddef>
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
        { "IsItemClicked", &luaIsItemClicked }
    };

    for (const auto& entry : kEntries) {
        if (!lua.registerNativeFunction(L, "ImGui", entry.name, entry.fn)) {
            crabe::shared::Logger::getInstance().error("ImGuiBindings: failed to register ImGui.{}", entry.name);
        }
    }
    lua.runSnippet(L, "Crabe.ImGui = ImGui;");
}

} // namespace crabe::presentation

