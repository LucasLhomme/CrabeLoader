/*
** CrabeLoader
** File description:
** Declares the loader ImGui overlay: console log filtering and commands.
** Runs on the render thread inside Present, so nothing here may call into Lua.
** Draws no mod UI directly; a mod frame arrives through presentation/draw_buffer.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef OVERLAY_HPP_
#define OVERLAY_HPP_

#include <string>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "shared/logger.hpp"

namespace crabe::presentation {

class Overlay {
    public:
        Overlay();
        ~Overlay();
        void initialize();
        void uninitialize();
        void defaultSettings();
        void renderOverlay();

    protected:
    private:
        void drawConsoleTab();
        void submitConsoleInput();
        bool isLevelVisible(crabe::shared::LogLevel level) const;

        char _consoleInputBuffer[1024] = {};

        // Debug is opt-in: it is the noisy per-keypress/per-hook-install level,
        // the other three are what you want on by default.
        bool _showDebug = false;
        bool _showInfo = true;
        bool _showWarning = true;
        bool _showError = true;
};

} // namespace crabe::presentation

#endif /* !OVERLAY_HPP_ */
