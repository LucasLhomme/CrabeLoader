/*
** CrabeLoader
** File description:
** overlay
*/

#include <format>
#include <string>
#include <windows.h>

#include "presentation/overlay.hpp"
#include "application/loader.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"

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

    std::string title = std::format("CrabeLoader v{} Overlay", Crabe::Version::String);
    ImGui::Begin(title.c_str());

    if (ImGui::BeginTabBar("MainTabBar")) {
        if (ImGui::BeginTabItem("Main")) {
            ImGui::Text("CrabeLoader v%s (Built: %s)", Crabe::Version::String.data(), Crabe::Version::BuildDate.data());
            ImGui::Separator();
            ImGui::Text("Press F5 to toggle the In-Game Mod Menu.");
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr; // Do not litter game root with imgui.ini
    io.LogFilename = nullptr; // Do not litter game root with imgui_log.txt

    ImGui::StyleColorsDark();
}

void Overlay::uninitialize()
{
    ImGui::DestroyContext();
}
