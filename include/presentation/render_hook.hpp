/*
** CrabeLoader
** File description:
** Declares the Present and ResizeBuffers detours, and the window mode the loader can request.
** The only place allowed to touch Direct3D; a window-mode change must happen on this thread.
** Persists nothing itself; the stored window mode lives in domain/config.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef RENDER_HOOK_HPP_
#define RENDER_HOOK_HPP_

#include <atomic>
#include <cstdint>
#include <memory>
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

        // Caps the frames the game presents per second; 0 removes the cap. The
        // game itself never limits its frame rate on PC (vsync off), so this is
        // the only limiter. `persist` also writes [display].frameLimit.
        void setFrameLimit(std::uint32_t fps, bool persist);
        std::uint32_t getFrameLimit() const noexcept { return _frameLimit.load(); }

        // Frames actually presented per second, measured over half-second
        // windows on the render thread. 0 before the first measurement.
        float getRenderFps() const noexcept { return _renderFps.load(); }

        // The size the game renders at (its swap chain's back buffer), which is
        // the resolution the player chose. The engine's own UI_GetCurrentResolution
        // reports the window instead, so 4K on a 1440p screen read as 1440p.
        // False until the first Present has sized the back buffer.
        bool getRenderResolution(UINT& width, UINT& height) const noexcept
        {
            width = _backBufferWidth;
            height = _backBufferHeight;
            return width != 0 && height != 0;
        }

        // Returns the underlying Win32 window handle.
        HWND getHwnd() const noexcept { return _hwnd; }

        // Patches the main executable's delay-load IAT entry for d3d11.dll to intercept
        // D3D11CreateDeviceAndSwapChain immediately at startup, preventing mode-switch flicker.
        static bool installEarlyDelayLoadHook();

    private:
        RenderHook() = default;
        ~RenderHook() = default;
        RenderHook(const RenderHook&) = delete;
        RenderHook& operator=(const RenderHook&) = delete;

        // Reads the window mode from crabe.toml (domain::Config::active()),
        // which has already migrated the legacy crabe_window_mode.cfg (if
        // any) by the time this runs.
        static WindowMode loadWindowModeConfig();

        // Persists the active window mode into crabe.toml's [display]
        // section (domain::Config::active()). The legacy
        // crabe_window_mode.cfg is never written to again.
        static void saveWindowModeConfig(WindowMode mode);

        // Resolves Present, ResizeBuffers, SetFullscreenState, GetFullscreenState
        // and ResizeTarget vtable pointers.
        static bool resolveSwapChainFunctions(uintptr_t& outPresent,
                                              uintptr_t& outResizeBuffers,
                                              uintptr_t& outSetFullscreenState,
                                              uintptr_t& outGetFullscreenState,
                                              uintptr_t& outResizeTarget);

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

        // Waits, on the render thread, until the next frame is due under the
        // frame limit, and counts the frame for getRenderFps().
        void paceFrame();

        // Applies pending window styles and dimensions on the render thread.
        void applyPendingWindowMode(IDXGISwapChain* swapChain);

        // Drops the swap chain out of exclusive fullscreen if the game put it
        // there. Returns true when a transition actually happened.
        bool leaveExclusiveFullscreen(IDXGISwapChain* swapChain);

        // Makes ImGui work in back-buffer pixels: DisplaySize becomes the back
        // buffer size and this frame's mouse positions are scaled from client
        // to back-buffer coordinates. Render thread only, between the backend
        // NewFrame calls and ImGui::NewFrame.
        void mapImGuiToBackBuffer();

        // Hook for IDXGISwapChain::Present driving ImGui rendering.
        static HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);

        // Hook for IDXGISwapChain::ResizeBuffers managing render target lifecycle.
        static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                                UINT width, UINT height, DXGI_FORMAT newFormat,
                                                UINT swapChainFlags);

        // Hook for IDXGISwapChain::SetFullscreenState refusing exclusive fullscreen.
        static HRESULT __stdcall hkSetFullscreenState(IDXGISwapChain* swapChain, BOOL fullscreen,
                                                     IDXGIOutput* target);

        // Hook for IDXGISwapChain::GetFullscreenState reporting the state the game asked for.
        static HRESULT __stdcall hkGetFullscreenState(IDXGISwapChain* swapChain, BOOL* fullscreen,
                                                     IDXGIOutput** target);

        // Hook for IDXGISwapChain::ResizeTarget keeping the game from resizing the window.
        static HRESULT __stdcall hkResizeTarget(IDXGISwapChain* swapChain, const DXGI_MODE_DESC* newTargetParameters);

        // Hook for SetCursorPos to suppress cursor centering while overlay is open.
        static BOOL WINAPI hkSetCursorPos(int X, int Y);

        // Hook for ShowWindow to suppress SW_MINIMIZE in borderless mode for instant Alt+Tab.
        static BOOL WINAPI hkShowWindow(HWND hWnd, int nCmdShow);

        // Hooks for ScreenToClient / ClientToScreen. The engine reads the mouse
        // with GetCursorPos + ScreenToClient and hands that to the UI as back
        // buffer pixels; when the back buffer is not the window's size (4K on a
        // 1440p screen, 21:9 stretched on a 16:9 one) the game's own calls are
        // converted between client and back-buffer pixels. Calls from anywhere
        // else, ImGui included, are untouched.
        static BOOL WINAPI hkScreenToClient(HWND hWnd, LPPOINT point);
        static BOOL WINAPI hkClientToScreen(HWND hWnd, LPPOINT point);

        // Back buffer size over client size for the game window, or false when
        // they match (or either is unknown), in which case nothing is scaled.
        bool backBufferScale(HWND hWnd, float& scaleX, float& scaleY) const;

        // Hook for D3D11CreateDeviceAndSwapChain to prevent startup fullscreen mode-switch flicker.
        static HRESULT WINAPI hkD3D11CreateDeviceAndSwapChain(
            IDXGIAdapter* pAdapter,
            D3D_DRIVER_TYPE DriverType,
            HMODULE Software,
            UINT Flags,
            const D3D_FEATURE_LEVEL* pFeatureLevels,
            UINT FeatureLevels,
            UINT SDKVersion,
            const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
            IDXGISwapChain** ppSwapChain,
            ID3D11Device** ppDevice,
            D3D_FEATURE_LEVEL* pFeatureLevel,
            ID3D11DeviceContext** ppImmediateContext);

        // Subclassed window procedure handling hotkeys, alt-tab, and input routing.
        static LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        typedef HRESULT(__stdcall* t_Present)(IDXGISwapChain*, UINT, UINT);
        typedef HRESULT(__stdcall* t_ResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
        typedef HRESULT(__stdcall* t_SetFullscreenState)(IDXGISwapChain*, BOOL, IDXGIOutput*);
        typedef HRESULT(__stdcall* t_GetFullscreenState)(IDXGISwapChain*, BOOL*, IDXGIOutput**);
        typedef HRESULT(__stdcall* t_ResizeTarget)(IDXGISwapChain*, const DXGI_MODE_DESC*);
        typedef BOOL(WINAPI* t_SetCursorPos)(int, int);
        typedef BOOL(WINAPI* t_ShowWindow)(HWND, int);
        typedef BOOL(WINAPI* t_PointConversion)(HWND, LPPOINT);
        typedef HRESULT(WINAPI* t_D3D11CreateDeviceAndSwapChain)(
            IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
            const D3D_FEATURE_LEVEL*, UINT, UINT,
            const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**,
            ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

        // Returns pointer to the original Present method.
        t_Present originalPresent() const;

        // Returns pointer to the original ResizeBuffers method.
        t_ResizeBuffers originalResizeBuffers() const;

        // Returns pointer to the original SetFullscreenState method.
        t_SetFullscreenState originalSetFullscreenState() const;

        // Returns pointer to the original GetFullscreenState method.
        t_GetFullscreenState originalGetFullscreenState() const;

        // Returns pointer to the original ResizeTarget method.
        t_ResizeTarget originalResizeTarget() const;

        // Returns pointer to the original SetCursorPos function.
        t_SetCursorPos originalSetCursorPos() const;

        // Returns pointer to the original ShowWindow function.
        t_ShowWindow originalShowWindow() const;

        // Returns pointer to the original D3D11CreateDeviceAndSwapChain function.
        t_D3D11CreateDeviceAndSwapChain originalD3D11CreateDeviceAndSwapChain() const;

        crabe::infrastructure::Hook _hookPresent;
        crabe::infrastructure::Hook _hookResizeBuffers;
        crabe::infrastructure::Hook _hookSetFullscreenState;
        crabe::infrastructure::Hook _hookGetFullscreenState;
        crabe::infrastructure::Hook _hookResizeTarget;
        crabe::infrastructure::Hook _hookSetCursorPos;
        crabe::infrastructure::Hook _hookShowWindow;
        crabe::infrastructure::Hook _hookScreenToClient;
        crabe::infrastructure::Hook _hookClientToScreen;
        crabe::infrastructure::Hook _hookD3D11CreateDeviceAndSwapChain;

        Overlay _overlay;

        ID3D11Device* _device = nullptr;
        ID3D11DeviceContext* _context = nullptr;
        ID3D11RenderTargetView* _renderTargetView = nullptr;
        HWND _hwnd = nullptr;
        WNDPROC _originalWndProc = nullptr;
        std::atomic<bool> _backendInitialized{false};
        std::atomic<bool> _menuOpen{false};
        std::atomic<bool> _cursorDirty{false};

        // Back buffer size, refreshed whenever the render target is rebuilt.
        UINT _backBufferWidth = 0;
        UINT _backBufferHeight = 0;

        // Render thread only. The mode the window was last put in, and the
        // decorated rect it had the last time it left windowed mode, so a
        // round trip through borderless puts it back where the user left it.
        bool _hasAppliedWindowMode = false;
        WindowMode _appliedWindowMode{WindowMode::BorderlessWindowed};
        bool _hasWindowedRect = false;
        RECT _windowedRect{};

        // _windowModeDirty asks the render thread to (re)apply the requested
        // mode; _persistWindowMode additionally writes it to crabe.toml, which
        // only an explicit request (Lua, Alt+Enter) does -- re-asserting the
        // mode after the game fought it is not a change of preference.
        std::atomic<bool> _windowModeDirty{false};
        std::atomic<bool> _persistWindowMode{false};

        // What the game last asked SetFullscreenState for. The retail exe
        // stops rendering until GetFullscreenState agrees with its request,
        // so that is what GetFullscreenState reports while the swap chain
        // really stays windowed.
        std::atomic<bool> _gameWantsFullscreen{false};

        // The display mode the game last asked ResizeTarget for. ResizeTarget
        // is swallowed, so a following ResizeBuffers(0, 0) would size the back
        // buffer to the window instead; it is given this size instead, which is
        // the resolution the player chose (4K on a 1440p screen is then
        // rendered at 4K and scaled down, not silently dropped to 1440p).
        std::atomic<std::uint32_t> _frameLimit{0};
        std::atomic<float> _renderFps{0.0f};

        // Render thread only: frame pacing and the frame-rate measurement.
        struct HandleCloser { void operator()(HANDLE handle) const noexcept { if (handle) CloseHandle(handle); } };
        std::unique_ptr<void, HandleCloser> _frameTimer;
        long long _nextFrameTicks = 0;
        long long _fpsWindowStart = 0;
        std::uint32_t _fpsFrames = 0;

        std::atomic<UINT> _targetWidth{0};
        std::atomic<UINT> _targetHeight{0};
        std::atomic<WindowMode> _requestedWindowMode{WindowMode::BorderlessWindowed};
        std::atomic<bool> _isFocused{true};
};

} // namespace crabe::presentation

#endif /* !RENDER_HOOK_HPP_ */
