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

        char _consoleInputBuffer[256] = {};
};

#endif /* !OVERLAY_HPP_ */
