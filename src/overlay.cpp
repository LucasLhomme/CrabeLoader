/*
** CrabeLoader
** File description:
** overlay
*/

#include <string>
#include <windows.h>

#include "overlay/overlay.hpp"
#include "loader/loader.hpp"
#include "logger/logger.hpp"

Overlay::Overlay()
{
    initialize();
}

Overlay::~Overlay()
{
    uninitialize();
}

void Overlay::setupFlag()
{
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoSavedSettings | 
        ImGuiWindowFlags_NoFocusOnAppearing | 
        ImGuiWindowFlags_NoNav;
}

void Overlay::defaultSettings()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;

    ImVec2 window_pos = ImVec2(work_pos.x + 10.0f, work_pos.y + 10.0f);
    ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f); // top-left pivot

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver, window_pos_pivot);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
}

void Overlay::drawConsoleTab()
{
    const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

    ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const std::string& line : Logger::getInstance().getHistory()) {
        ImGui::TextUnformatted(line.c_str());
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

void Overlay::submitConsoleInput()
{
    std::string input(_consoleInputBuffer);

    size_t start = input.find_first_not_of(" \t");
    if (start == std::string::npos) 
        return;
    input.erase(0, start);
    Logger::getInstance().info("> {}", input);

    if (input.front() == '=')
        input = "return " + input.substr(1);
    
    Loader::get().queueConsoleSnippet(input);
}

void Overlay::renderOverlay()
{
    Overlay::setupFlag();
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