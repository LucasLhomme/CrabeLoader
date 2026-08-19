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

    const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2.0f;
    ImGui::BeginChild("MenuScroll", ImVec2(0.0f, -footerHeight), false);

    for (size_t i = 0; i < view.items.size(); ++i) {
        // Lua indexes from 1.
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable(view.items[i].c_str()))
            Menu::get().requestActivate(static_cast<int>(i) + 1);
        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::Separator();

    if (ImGui::Button("Back"))
        Menu::get().requestBack();
    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
        Menu::get().requestRefresh();

    if (!view.status.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(view.status.c_str());
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
