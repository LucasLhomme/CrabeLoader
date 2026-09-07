/*
** CrabeLoader
** File description:
** overlay
*/

#include <string>
#include <windows.h>

#include "overlay/overlay.hpp"
#include "loader/loader.hpp"
#include "loader/menu.hpp"
#include "logger/logger.hpp"

Overlay::Overlay()
{
    initialize();
}

Overlay::~Overlay()
{
    uninitialize();
}

void Overlay::defaultSettings()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;

    ImVec2 window_pos = ImVec2(work_pos.x + 10.0f, work_pos.y + 10.0f);
    ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver, window_pos_pivot);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
}

bool Overlay::isLevelVisible(LogLevel level) const
{
    switch (level) {
        case LogLevel::DEBUG:   return _showDebug;
        case LogLevel::INFO:    return _showInfo;
        case LogLevel::WARNING: return _showWarning;
        case LogLevel::ERR:     return _showError;
        default:                return true;
    }
}

// Draws the view the game thread last reported, and turns clicks into queued
// requests. Nothing here touches Lua: the render thread must not.
void Overlay::renderModMenu(bool* open)
{
    ImGui::SetNextWindowSize(ImVec2(360.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Crabe Menu", open)) {
        ImGui::End();
        return;
    }

    if (!_menuRequested) {
        _menuRequested = true;
        Menu::get().requestRefresh();
    }

    Menu::View view = Menu::get().view();

    if (view.items.empty()) {
        ImGui::TextDisabled("No mod has registered a menu entry.");
        ImGui::TextDisabled("Mods declare them with Crabe.Menu.register{...}.");
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted(view.title.c_str());
    ImGui::Separator();

    const int count = static_cast<int>(view.items.size());

    // A different view is a different list, so start at the top rather than
    // leaving the selection on a row index that now means something else.
    if (view.title != _menuTitle) {
        _menuTitle = view.title;
        _menuCursor = 0;
        _menuScrollTo = true;
    }
    if (_menuCursor >= count) _menuCursor = count - 1;
    if (_menuCursor < 0) _menuCursor = 0;

    // Arrows and Enter are the reliable way in: the game re-centres the mouse
    // every frame, so the pointer cannot be trusted to sit on a row.
    auto moveCursor = [&](int delta) {
        _menuCursor = (_menuCursor + delta % count + count) % count;
        _menuScrollTo = true;
    };

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) moveCursor(1);
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) moveCursor(-1);
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown, true)) moveCursor(5);
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp, true)) moveCursor(-5);
    if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) { _menuCursor = 0; _menuScrollTo = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_End, false)) { _menuCursor = count - 1; _menuScrollTo = true; }

    // Lua indexes from 1 throughout.
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
        Menu::get().requestActivate(_menuCursor + 1);

    if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false) ||
        ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
        Menu::get().requestBack();

    const float footerHeight = ImGui::GetStyle().ItemSpacing.y * 2.0f
                             + ImGui::GetTextLineHeightWithSpacing() * 2.5f;
    ImGui::BeginChild("MenuScroll", ImVec2(0.0f, -footerHeight), false);

    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);

        const bool selected = (i == _menuCursor);
        if (ImGui::Selectable(view.items[i].c_str(), selected)) {
            // Clicking still works, and moves the keyboard cursor with it so
            // the two never disagree about what is selected.
            _menuCursor = i;
            Menu::get().requestActivate(i + 1);
        }
        if (selected && _menuScrollTo)
            ImGui::SetScrollHereY(0.5f);

        ImGui::PopID();
    }
    _menuScrollTo = false;

    ImGui::EndChild();
    ImGui::Separator();

    // The status line carries the error text when a handler raises, so it gets
    // its own wrapped block. Putting it after the buttons on one line pushed
    // anything long straight off the right edge of the window.
    if (!view.status.empty()) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(view.status.c_str());
        ImGui::PopTextWrapPos();
    } else {
        ImGui::TextDisabled("Arrows move  -  Enter selects  -  Backspace goes back");
    }

    ImGui::End();
}

void Overlay::drawConsoleTab()
{
    ImGui::Checkbox("Debug", &_showDebug); ImGui::SameLine();
    ImGui::Checkbox("Info", &_showInfo); ImGui::SameLine();
    ImGui::Checkbox("Warning", &_showWarning); ImGui::SameLine();
    ImGui::Checkbox("Error", &_showError);

    const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

    ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const LogEntry& entry : Logger::getInstance().getHistory()) {
        if (isLevelVisible(entry.level))
            ImGui::TextUnformatted(entry.text.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::PushItemWidth(-1.0f);
    if (ImGui::InputText("##ConsoleInput", _consoleInputBuffer, sizeof(_consoleInputBuffer),
                        ImGuiInputTextFlags_EnterReturnsTrue)) {
        submitConsoleInput();
        _consoleInputBuffer[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::PopItemWidth();
}

// Trimming, the "> {}" echo and the "=expr" -> "return tostring(expr)"
// rewrite all live in Loader::queueConsoleSnippet now, shared with
// Loader::drainRemoteCommandFile so both entry points behave identically.
void Overlay::submitConsoleInput()
{
    Loader::get().queueConsoleSnippet(_consoleInputBuffer);
}

void Overlay::renderOverlay()
{
    Overlay::defaultSettings();

    ImGui::Begin("CrabeLoader Overlay");

    if (ImGui::BeginTabBar("MainTabBar")) {
        if (ImGui::BeginTabItem("Main")) {
            ImGui::Text("Hello from CrabeLoader!");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Console")) {
            drawConsoleTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void Overlay::initialize()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
}

void Overlay::uninitialize()
{
    ImGui::DestroyContext();
}
