/*
** CrabeLoader
** File description:
** Overlay
*/

#ifndef OVERLAY_HPP_
#define OVERLAY_HPP_

#include <string>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "logger/logger.hpp"

class Overlay {
    public:
        Overlay();
        ~Overlay();
        void initialize();
        void uninitialize();
        void defaultSettings();
        void renderOverlay();

        // The mod menu, in its own window. `open` is the caller's visibility
        // flag: ImGui clears it when the title-bar close button is used, so
        // the window and the F5 toggle stay in agreement.
        void renderModMenu(bool* open);

    protected:
    private:
        void drawConsoleTab();
        void submitConsoleInput();
        bool isLevelVisible(LogLevel level) const;

        char _consoleInputBuffer[1024] = {};

        // The first view can only be fetched once a Lua state exists, so it
        // is requested the first time the tab is actually drawn.
        bool _menuRequested = false;

        // Keyboard selection in the mod menu. The game re-centres the mouse
        // every frame, so pointing at an entry is unreliable; arrows and Enter
        // are the primary way to drive it, and this survives between frames.
        int _menuCursor = 0;
        // Set when the cursor moves, so the list scrolls to follow it.
        bool _menuScrollTo = false;
        // Last title drawn. A change means a different view, so the selection
        // goes back to the top instead of pointing at an unrelated row.
        std::string _menuTitle;

        // Debug is opt-in: it is the noisy per-keypress/per-hook-install level,
        // the other three are what you want on by default.
        bool _showDebug = false;
        bool _showInfo = true;
        bool _showWarning = true;
        bool _showError = true;
};

#endif /* !OVERLAY_HPP_ */
