/*
** CrabeLoader
** File description:
** render_hook
*/

#ifndef RENDER_HOOK_HPP_
#define RENDER_HOOK_HPP_

#include <atomic>
#include <cstdint>
#include <d3d11.h>
#include <windows.h>

#include "overlay/overlay.hpp"
#include "loader/hook.hpp"

// Hooks IDXGISwapChain::Present/ResizeBuffers to reach the game's D3D11
// device/context/HWND from inside an injected DLL, and drives the per-frame
// ImGui render sequence from there. Owns all D3D11/Win32 hooking detail.
enum class WindowMode {
    Windowed,
    BorderlessWindowed,
};

class RenderHook {
    public:
        static RenderHook& get();

        // Hooks IDXGISwapChain::Present/ResizeBuffers to reach the game's D3D11
        // device/context/HWND from inside an injected DLL, then drives the
        // per-frame ImGui render sequence from there.
        bool initialize();
        void uninitialize();

        // The debug overlay (console, log): Insert.
        void toggleMenu();
        bool isMenuOpen() const;

        // The mod menu: F5. A separate window with its own visibility, so the
        // two can be open independently -- a player wants the menu without the
        // developer console behind it.
        void toggleModMenu();
        bool isModMenuOpen() const;

        // Thread-safe; only records the request. hkPresent applies it next
        // frame, since window calls must happen on the thread that owns it.
        void requestWindowMode(WindowMode mode);

    protected:
    private:
        RenderHook() = default;
        ~RenderHook() = default;
        RenderHook(const RenderHook&) = delete;
        RenderHook& operator=(const RenderHook&) = delete;

        // Resolves Present/ResizeBuffers via a throwaway device+swapchain's
        // vtable; the pointers stay valid after the dummy objects are freed.
        static bool resolveSwapChainFunctions(uintptr_t& outPresent, uintptr_t& outResizeBuffers);

        // ImGui draws the cursor whenever either window wants the mouse.
        void updateCursorVisibility();

        void ensureBackendInit(IDXGISwapChain* swapChain);
        void releaseRenderTarget();
        void createRenderTarget(IDXGISwapChain* swapChain);
        void applyPendingWindowMode(IDXGISwapChain* swapChain);

        static HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
        static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                                UINT width, UINT height, DXGI_FORMAT newFormat,
                                                UINT swapChainFlags);
        static LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        typedef HRESULT(__stdcall* t_Present)(IDXGISwapChain*, UINT, UINT);
        typedef HRESULT(__stdcall* t_ResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

        t_Present originalPresent() const;
        t_ResizeBuffers originalResizeBuffers() const;

        Hook _hookPresent;
        Hook _hookResizeBuffers;

        Overlay _overlay;

        ID3D11Device* _device = nullptr;
        ID3D11DeviceContext* _context = nullptr;
        ID3D11RenderTargetView* _renderTargetView = nullptr;
        HWND _hwnd = nullptr;
        WNDPROC _originalWndProc = nullptr;
        std::atomic<bool> _backendInitialized{false};
        std::atomic<bool> _menuOpen{false};
        std::atomic<bool> _modMenuOpen{false};

        LONG_PTR _originalStyle = 0;
        RECT _originalRect{};

        std::atomic<bool> _windowModeDirty{false};
        std::atomic<WindowMode> _requestedWindowMode{WindowMode::Windowed};
};

#endif /* !RENDER_HOOK_HPP_ */
