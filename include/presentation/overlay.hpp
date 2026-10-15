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
        // Constructs the overlay presentation instance.
        Overlay();

        // Destroys the overlay presentation instance.
        ~Overlay();

        // Initializes ImGui overlay state and fonts.
        void initialize();

        // Cleans up ImGui overlay state.
        void uninitialize();

        // Resets overlay settings to default values.
        void defaultSettings();

        // Renders the main overlay UI window.
        void renderOverlay();

    protected:
    private:
        // Loads custom typography fonts into the ImGui font atlas.
        static void loadFonts(ImGuiIO& io);

        // Draws the console log tab.
        void drawConsoleTab();

        // Draws the VFS inspection tab.
        void drawVfsTab();

        // Submits text entered in the console input box.
        void submitConsoleInput();

        // Checks whether a given log level is currently visible in the console.
        bool isLevelVisible(crabe::shared::LogLevel level) const;

        char _consoleInputBuffer[1024] = {};
        char _vfsFilterBuffer[256] = {};

        bool _showDebug = false;
        bool _showInfo = true;
        bool _showWarning = true;
        bool _showError = true;
};

} // namespace crabe::presentation

#endif /* !OVERLAY_HPP_ */
