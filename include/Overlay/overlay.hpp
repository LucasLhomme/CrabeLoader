/*
** CrabeLoader
** File description:
** Overlay
*/

#ifndef OVERLAY_HPP_
#define OVERLAY_HPP_

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
        void setupFlag();
        void defaultSettings();
        void renderOverlay();
        void openOverlay();
        void closeOverlay();

    protected:
    private:
        void drawConsoleTab();
        void submitConsoleInput();
        bool isLevelVisible(LogLevel level) const;

        char _consoleInputBuffer[1024] = {};

        // Debug is opt-in: it is the noisy per-keypress/per-hook-install level,
        // the other three are what you want on by default.
        bool _showDebug = false;
        bool _showInfo = true;
        bool _showWarning = true;
        bool _showError = true;
};

#endif /* !OVERLAY_HPP_ */
