#ifndef RENDER_HOOK_HPP_
#define RENDER_HOOK_HPP_

#include <atomic>
#include <cstdint>
#include <d3d11.h>
#include <windows.h>

#include "presentation/overlay.hpp"
#include "infrastructure/hook.hpp"

namespace crabe::presentation {

enum class WindowMode {
    Windowed,
    BorderlessWindowed,
};

class RenderHook {
    public:
        // Returns the singleton instance of RenderHook.
        static RenderHook& get();

        // Resolves DXGI vtable entries and installs hooks for render interception.
        bool initialize();

        // Removes all active DirectX and window hooks and releases resources.
        void uninitialize();

        // Toggles visibility of the debug overlay console.
        void toggleMenu();

        // Returns true if the debug overlay is currently open.
        bool isMenuOpen() const;

        // Thread-safely requests a change of window display mode.
        void requestWindowMode(WindowMode mode);

        // Returns the current requested window display mode.
        WindowMode getCurrentWindowMode() const noexcept { return _requestedWindowMode.load(); }

        // Returns the underlying Win32 window handle.
        HWND getHwnd() const noexcept { return _hwnd; }

    private:
        RenderHook() = default;
        ~RenderHook() = default;
        RenderHook(const RenderHook&) = delete;
        RenderHook& operator=(const RenderHook&) = delete;

        // Reads window mode configuration file or defaults to borderless.
        static WindowMode loadWindowModeConfig();

        // Writes active window mode to the configuration file.
        static void saveWindowModeConfig(WindowMode mode);

        // Resolves Present, ResizeBuffers, and SetFullscreenState vtable pointers.
        static bool resolveSwapChainFunctions(uintptr_t& outPresent,
                                              uintptr_t& outResizeBuffers,
                                              uintptr_t& outSetFullscreenState);

        // Flags cursor visibility as stale from any thread.
        void updateCursorVisibility();

        // Applies a pending cursor visibility change. Render thread only.
        void applyPendingCursorVisibility();

        // Initializes ImGui Win32/DX11 backends once swapchain device is ready.
        void ensureBackendInit(IDXGISwapChain* swapChain);

        // Releases any existing Direct3D 11 render target view.
        void releaseRenderTarget();

        // Creates a Direct3D 11 render target view for swapchain backbuffer.
        void createRenderTarget(IDXGISwapChain* swapChain);

        // Applies pending window styles and dimensions on the render thread.
        void applyPendingWindowMode(IDXGISwapChain* swapChain);

        // Hook for IDXGISwapChain::Present driving ImGui rendering.
        static HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);

        // Hook for IDXGISwapChain::ResizeBuffers managing render target lifecycle.
        static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                                UINT width, UINT height, DXGI_FORMAT newFormat,
                                                UINT swapChainFlags);

        // Hook for IDXGISwapChain::SetFullscreenState enforcing windowed mode.
        static HRESULT __stdcall hkSetFullscreenState(IDXGISwapChain* swapChain, BOOL fullscreen,
                                                     IDXGIOutput* target);

        // Hook for SetCursorPos to suppress cursor centering while overlay is open.
        static BOOL WINAPI hkSetCursorPos(int X, int Y);

        // Subclassed window procedure handling hotkeys, alt-tab, and input routing.
        static LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        typedef HRESULT(__stdcall* t_Present)(IDXGISwapChain*, UINT, UINT);
        typedef HRESULT(__stdcall* t_ResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
        typedef HRESULT(__stdcall* t_SetFullscreenState)(IDXGISwapChain*, BOOL, IDXGIOutput*);
        typedef BOOL(WINAPI* t_SetCursorPos)(int, int);

        // Returns pointer to the original Present method.
        t_Present originalPresent() const;

        // Returns pointer to the original ResizeBuffers method.
        t_ResizeBuffers originalResizeBuffers() const;

        // Returns pointer to the original SetFullscreenState method.
        t_SetFullscreenState originalSetFullscreenState() const;

        // Returns pointer to the original SetCursorPos function.
        t_SetCursorPos originalSetCursorPos() const;

        crabe::infrastructure::Hook _hookPresent;
        crabe::infrastructure::Hook _hookResizeBuffers;
        crabe::infrastructure::Hook _hookSetFullscreenState;
        crabe::infrastructure::Hook _hookSetCursorPos;

        Overlay _overlay;

        ID3D11Device* _device = nullptr;
        ID3D11DeviceContext* _context = nullptr;
        ID3D11RenderTargetView* _renderTargetView = nullptr;
        HWND _hwnd = nullptr;
        WNDPROC _originalWndProc = nullptr;
        std::atomic<bool> _backendInitialized{false};
        std::atomic<bool> _menuOpen{false};
        std::atomic<bool> _cursorDirty{false};

        LONG_PTR _originalStyle = 0;
        RECT _originalRect{};

        std::atomic<bool> _windowModeDirty{false};
        std::atomic<WindowMode> _requestedWindowMode{WindowMode::BorderlessWindowed};
};

} // namespace crabe::presentation

#endif /* !RENDER_HOOK_HPP_ */
