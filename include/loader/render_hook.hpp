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

// Hooks IDXGISwapChain::Present/ResizeBuffers to reach the game's real D3D11
// device/context/HWND -- there is no other way to get them from inside an
// injected DLL -- and drives the per-frame ImGui render sequence from there.
//
// RenderHook owns every D3D11/Win32 hooking detail; Overlay only owns the
// ImGui context and the UI content drawn each frame. This mirrors how LuaCall
// owns Lua hook mechanics while Loader owns mod logic.
// No native in the whole game exposes window/fullscreen state to Lua (see
// docs/nativedb.md) -- this is a pure Win32 concern, hence it lives here
// rather than as a Game.* wrapper.
enum class WindowMode {
    Windowed,
    BorderlessWindowed,
};

class RenderHook {
    public:
        static RenderHook& get();

        bool initialize();
        void uninitialize();

        void toggleMenu();
        bool isMenuOpen() const;

        // Thread-safe: called from the Lua-owning thread (a registered native),
        // not the render thread. Only records the request; hkPresent applies it
        // on the next frame, since Win32 window calls belong on the thread that
        // owns the window and touching D3D state off the render thread is asking
        // for trouble.
        void requestWindowMode(WindowMode mode);

    protected:
    private:
        RenderHook() = default;
        ~RenderHook() = default;
        RenderHook(const RenderHook&) = delete;
        RenderHook& operator=(const RenderHook&) = delete;

        // Resolves Present (vtable slot 8) and ResizeBuffers (slot 13) by
        // standing up a throwaway device+swapchain on a hidden window and
        // reading its vtable: the returned pointers point into the graphics
        // driver, so they stay valid after the dummy objects are released.
        static bool resolveSwapChainFunctions(uintptr_t& outPresent, uintptr_t& outResizeBuffers);

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

        // The window's style/rect as the game created it, captured once so
        // Game.SetWindowMode("windowed") can restore it exactly rather than
        // guessing a size.
        LONG_PTR _originalStyle = 0;
        RECT _originalRect{};

        std::atomic<bool> _windowModeDirty{false};
        std::atomic<WindowMode> _requestedWindowMode{WindowMode::Windowed};
};

#endif /* !RENDER_HOOK_HPP_ */
