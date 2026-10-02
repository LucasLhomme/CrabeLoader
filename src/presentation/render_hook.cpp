/*
** CrabeLoader
** File description:
** Hooks Present and ResizeBuffers, owns the ImGui device objects and applies the window mode.
** A window mode change is applied here because only this thread may touch the swap chain.
** Persists nothing itself; the stored mode is read and written through domain::Config.
**
** Authors: @LucasLhomme
*/

#include <atomic>
#include <cfloat>
#include <chrono>
#include <dxgi.h>
#include <filesystem>
#include <string>
#include <thread>

#include "presentation/render_hook.hpp"

#include <algorithm>
#include "imgui/imgui_internal.h"
#include "presentation/draw_buffer.hpp"
#include "application/loader.hpp"
#include "domain/config.hpp"
#include "infrastructure/crash_handler.hpp"
#include "infrastructure/crash_reporter.hpp"
#include "shared/logger.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace crabe::presentation {

namespace {
    constexpr int kPresentVtableIndex = 8;
    constexpr int kSetFullscreenStateVtableIndex = 10;
    constexpr int kGetFullscreenStateVtableIndex = 11;
    constexpr int kResizeBuffersVtableIndex = 13;
    constexpr int kResizeTargetVtableIndex = 14;
    constexpr wchar_t kDummyClassName[] = L"CrabeLoaderDummyWindow";

    // Window styles the loader owns. Clip bits are carried over from whatever
    // the game set; every other bit is decided here.
    constexpr LONG kClipStyles = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    constexpr LONG kBorderlessStyle = WS_POPUP | WS_VISIBLE;
    constexpr LONG kWindowedStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
}

// Returns the singleton instance of RenderHook.
RenderHook& RenderHook::get()
{
    static RenderHook instance;
    return instance;
}

// Reads the window mode crabe::domain::Config::active() already loaded
// (migrating crabe_window_mode.cfg on first run is Config::load()'s job,
// not this one's -- see src/domain/config.cpp).
WindowMode RenderHook::loadWindowModeConfig()
{
    return crabe::domain::Config::active().windowMode() == crabe::domain::ConfigWindowMode::Windowed
        ? WindowMode::Windowed
        : WindowMode::BorderlessWindowed;
}

// Persists the active window mode into crabe.toml's [display] section.
// crabe_window_mode.cfg is legacy: Config::load() migrates it once and this
// loader never writes to it again.
void RenderHook::saveWindowModeConfig(WindowMode mode)
{
    const crabe::domain::ConfigWindowMode configMode = (mode == WindowMode::BorderlessWindowed)
        ? crabe::domain::ConfigWindowMode::Borderless
        : crabe::domain::ConfigWindowMode::Windowed;
    crabe::domain::Config::active().setWindowMode(std::filesystem::current_path(), configMode);
}

// Resolves swapchain vtable function pointers using a temporary dummy device.
bool RenderHook::resolveSwapChainFunctions(uintptr_t& outPresent,
                                          uintptr_t& outResizeBuffers,
                                          uintptr_t& outSetFullscreenState,
                                          uintptr_t& outGetFullscreenState,
                                          uintptr_t& outResizeTarget)
{
    outPresent = 0;
    outResizeBuffers = 0;
    outSetFullscreenState = 0;
    outGetFullscreenState = 0;
    outResizeTarget = 0;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kDummyClassName;
    RegisterClassExW(&wc);

    HWND dummyHwnd = CreateWindowExW(0, kDummyClassName, L"CrabeLoader", WS_OVERLAPPEDWINDOW,
                                    0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    if (!dummyHwnd) {
        UnregisterClassW(kDummyClassName, wc.hInstance);
        crabe::shared::Logger::getInstance().error("RenderHook: failed to create dummy window.");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC scDesc{};
    scDesc.BufferDesc.Width = 100;
    scDesc.BufferDesc.Height = 100;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc.Count = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 1;
    scDesc.OutputWindow = dummyHwnd;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scDesc, &swapChain, &device, nullptr, &context);

    bool ok = false;
    if (SUCCEEDED(hr) && swapChain) {
        void** vtable = *reinterpret_cast<void***>(swapChain);
        outPresent = reinterpret_cast<uintptr_t>(vtable[kPresentVtableIndex]);
        outSetFullscreenState = reinterpret_cast<uintptr_t>(vtable[kSetFullscreenStateVtableIndex]);
        outGetFullscreenState = reinterpret_cast<uintptr_t>(vtable[kGetFullscreenStateVtableIndex]);
        outResizeBuffers = reinterpret_cast<uintptr_t>(vtable[kResizeBuffersVtableIndex]);
        outResizeTarget = reinterpret_cast<uintptr_t>(vtable[kResizeTargetVtableIndex]);
        ok = true;
    } else {
        crabe::shared::Logger::getInstance().error("RenderHook: D3D11CreateDeviceAndSwapChain failed (0x{:X}).",
                                    static_cast<uint32_t>(hr));
    }

    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
    DestroyWindow(dummyHwnd);
    UnregisterClassW(kDummyClassName, wc.hInstance);

    return ok;
}

// Installs MinHook hooks on DXGI Present, ResizeBuffers, SetFullscreenState and ResizeTarget.
bool RenderHook::initialize()
{
    uintptr_t presentAddr = 0;
    uintptr_t resizeBuffersAddr = 0;
    uintptr_t setFullscreenStateAddr = 0;
    uintptr_t getFullscreenStateAddr = 0;
    uintptr_t resizeTargetAddr = 0;

    if (!resolveSwapChainFunctions(presentAddr, resizeBuffersAddr, setFullscreenStateAddr,
                                   getFullscreenStateAddr, resizeTargetAddr)) {
        crabe::shared::Logger::getInstance().error("RenderHook: failed to resolve swapchain vtable.");
        return false;
    }

    bool allInstalled = true;
    allInstalled &= _hookPresent.installLogged(presentAddr,
                            reinterpret_cast<void*>(&RenderHook::hkPresent),
                            "RenderHook", "IDXGISwapChain::Present");
    allInstalled &= _hookResizeBuffers.installLogged(resizeBuffersAddr,
                            reinterpret_cast<void*>(&RenderHook::hkResizeBuffers),
                            "RenderHook", "IDXGISwapChain::ResizeBuffers");

    // Without these the game owns the display: the retail exe puts its swap
    // chain in exclusive fullscreen on its own (the window then carries DXGI's
    // WS_EX_TOPMOST and no frame), and every window style set below is
    // ignored. Their absence is not fatal -- applyPendingWindowMode still
    // drops an exclusive swap chain at the first Present -- so they are not
    // counted in allInstalled. Set and Get go together: refusing fullscreen
    // without reporting it as granted freezes the game (it waits for
    // GetFullscreenState to agree, calling ResizeTarget in a loop).
    _hookSetFullscreenState.installLogged(setFullscreenStateAddr,
                            reinterpret_cast<void*>(&RenderHook::hkSetFullscreenState),
                            "RenderHook", "IDXGISwapChain::SetFullscreenState");
    _hookGetFullscreenState.installLogged(getFullscreenStateAddr,
                            reinterpret_cast<void*>(&RenderHook::hkGetFullscreenState),
                            "RenderHook", "IDXGISwapChain::GetFullscreenState");
    _hookResizeTarget.installLogged(resizeTargetAddr,
                            reinterpret_cast<void*>(&RenderHook::hkResizeTarget),
                            "RenderHook", "IDXGISwapChain::ResizeTarget");

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto targetSetCursorPos = reinterpret_cast<void*>(GetProcAddress(user32, "SetCursorPos"));
        if (targetSetCursorPos) {
            _hookSetCursorPos.installLogged(reinterpret_cast<uintptr_t>(targetSetCursorPos),
                                           reinterpret_cast<void*>(&RenderHook::hkSetCursorPos),
                                           "RenderHook", "SetCursorPos");
        }
        auto targetShowWindow = reinterpret_cast<void*>(GetProcAddress(user32, "ShowWindow"));
        if (targetShowWindow) {
            _hookShowWindow.installLogged(reinterpret_cast<uintptr_t>(targetShowWindow),
                                          reinterpret_cast<void*>(&RenderHook::hkShowWindow),
                                          "RenderHook", "ShowWindow");
        }
    }

    // Applied at the first Present, once the game's window is known. It used
    // to start clean, so the configured mode was never applied at startup and
    // the game simply stayed in whatever display mode it chose for itself.
    _requestedWindowMode = loadWindowModeConfig();
    _persistWindowMode = false;
    _windowModeDirty = true;

    return allInstalled;
}

// Restores window procedure, removes all hooks, and releases D3D11 resources.
void RenderHook::uninitialize()
{
    if (_originalWndProc && _hwnd) {
        SetWindowLongPtrW(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_originalWndProc));
        _originalWndProc = nullptr;
    }

    releaseRenderTarget();

    _hookPresent.remove();
    _hookSetFullscreenState.remove();
    _hookGetFullscreenState.remove();
    _hookResizeTarget.remove();
    _hookResizeBuffers.remove();
    _hookSetCursorPos.remove();
    _hookShowWindow.remove();

    if (_backendInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        _backendInitialized = false;
    }

    if (_context) { _context->Release(); _context = nullptr; }
    if (_device) { _device->Release(); _device = nullptr; }
}

// Flags the cursor state as stale. Called from the window thread, so the ImGui
// IO write itself is deferred to Present (Architecture Blueprint, Rule 3).
void RenderHook::updateCursorVisibility()
{
    _cursorDirty.store(true);
    if (_menuOpen.load()) {
        ClipCursor(nullptr);
    }
}

// Applies a pending cursor state on the render thread. The mouse cursor is only
// shown for the Insert console overlay; the F5 mod menu is keyboard driven.
void RenderHook::applyPendingCursorVisibility()
{
    if (!_cursorDirty.exchange(false))
        return;
    ImGui::GetIO().MouseDrawCursor = _menuOpen.load();
}

// Toggles visibility of the debug overlay and updates mouse cursor.
void RenderHook::toggleMenu()
{
    _menuOpen = !_menuOpen;
    updateCursorVisibility();
    crabe::shared::Logger::getInstance().debug("RenderHook: overlay {}.", _menuOpen ? "opened" : "closed");
}

// Returns whether the debug console overlay is currently visible.
bool RenderHook::isMenuOpen() const
{
    return _menuOpen;
}

// Enqueues a window mode change to be applied, and saved, on the render thread.
void RenderHook::requestWindowMode(WindowMode mode)
{
    _requestedWindowMode = mode;
    _persistWindowMode = true;
    _windowModeDirty = true;
}

// A windowed rect for the window's monitor, sized from the back buffer and
// centred on the work area. Used when there is no remembered windowed rect
// (first switch to windowed, or the remembered one is off every monitor).
static RECT windowedRectFor(HWND hwnd, UINT backBufferWidth, UINT backBufferHeight, LONG style)
{
    UINT clientWidth = 1280;
    UINT clientHeight = 720;
    if (backBufferWidth > 100 && backBufferHeight > 100) {
        clientWidth = backBufferWidth;
        clientHeight = backBufferHeight;
    }

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    RECT work{ 0, 0, 1920, 1080 };
    if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitorInfo))
        work = monitorInfo.rcWork;

    const LONG workWidth = work.right - work.left;
    const LONG workHeight = work.bottom - work.top;

    // The back buffer is usually the whole monitor. A window that size plus a
    // title bar does not fit on the screen it came from, so it is scaled down,
    // keeping the aspect ratio, to leave the taskbar and the frame visible --
    // otherwise "windowed" looks identical to borderless.
    RECT frame{ 0, 0, static_cast<LONG>(clientWidth), static_cast<LONG>(clientHeight) };
    AdjustWindowRect(&frame, static_cast<DWORD>(style & ~WS_VISIBLE), FALSE);
    LONG outerWidth = frame.right - frame.left;
    LONG outerHeight = frame.bottom - frame.top;

    if (outerWidth > workWidth || outerHeight > workHeight) {
        const double scale = 0.85 * (std::min)(static_cast<double>(workWidth) / outerWidth,
                                               static_cast<double>(workHeight) / outerHeight);
        outerWidth = static_cast<LONG>(outerWidth * scale);
        outerHeight = static_cast<LONG>(outerHeight * scale);
    }

    RECT out{};
    out.left = work.left + (workWidth - outerWidth) / 2;
    out.top = work.top + (workHeight - outerHeight) / 2;
    out.right = out.left + outerWidth;
    out.bottom = out.top + outerHeight;
    return out;
}

// Calls SetFullscreenState(FALSE) through the trampoline when the swap chain is
// in exclusive fullscreen. The game gets there on its own, possibly before the
// hook was installed, so the state is checked rather than assumed.
bool RenderHook::leaveExclusiveFullscreen(IDXGISwapChain* swapChain)
{
    // The real state, not the one hkGetFullscreenState reports to the game.
    if (!swapChain)
        return false;
    BOOL fullscreen = FALSE;
    const HRESULT state = _hookGetFullscreenState.isInstalled()
        ? originalGetFullscreenState()(swapChain, &fullscreen, nullptr)
        : swapChain->GetFullscreenState(&fullscreen, nullptr);
    if (FAILED(state) || !fullscreen)
        return false;

    // An exclusive swap chain is one the game asked for, possibly before the
    // hooks existed: from here on it must keep seeing it as granted.
    _gameWantsFullscreen = true;

    const HRESULT hr = _hookSetFullscreenState.isInstalled()
        ? originalSetFullscreenState()(swapChain, FALSE, nullptr)
        : swapChain->SetFullscreenState(FALSE, nullptr);
    crabe::shared::Logger::getInstance().info("RenderHook: left exclusive fullscreen (0x{:X}).",
                                              static_cast<uint32_t>(hr));
    return SUCCEEDED(hr);
}

// Puts the window in the requested mode. Idempotent: it compares the window's
// actual style and rect with the target and touches nothing when they already
// match, so re-asserting the mode (the game asking for fullscreen again, a
// display change) costs nothing and never moves the window.
void RenderHook::applyPendingWindowMode(IDXGISwapChain* swapChain)
{
    // The flag is kept until the window is known, so a request made before the
    // first Present (the configured mode, at startup) is not lost.
    if (!_hwnd || !_windowModeDirty.exchange(false))
        return;

    leaveExclusiveFullscreen(swapChain);

    const WindowMode mode = _requestedWindowMode.load();
    const bool persist = _persistWindowMode.exchange(false);

    const LONG currentStyle = static_cast<LONG>(GetWindowLongPtrW(_hwnd, GWL_STYLE));
    const LONG currentExStyle = static_cast<LONG>(GetWindowLongPtrW(_hwnd, GWL_EXSTYLE));
    RECT currentRect{};
    GetWindowRect(_hwnd, &currentRect);
    const bool decorated = (currentStyle & WS_CAPTION) == WS_CAPTION;
    const bool wasOurWindowed = _hasAppliedWindowMode && _appliedWindowMode == WindowMode::Windowed && decorated;

    // Remember where the user left the decorated window before taking it
    // borderless, so the next switch back puts it there. Not when maximized:
    // that rect is the monitor, not a placement anyone chose.
    if (mode == WindowMode::BorderlessWindowed && wasOurWindowed && !IsZoomed(_hwnd) && !IsIconic(_hwnd)) {
        _windowedRect = currentRect;
        _hasWindowedRect = true;
    }

    LONG targetStyle = 0;
    RECT targetRect{};
    if (mode == WindowMode::BorderlessWindowed) {
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfoW(MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST), &monitorInfo)) {
            crabe::shared::Logger::getInstance().error("RenderHook: no monitor for the game window; mode not applied.");
            return;
        }
        targetStyle = kBorderlessStyle | (currentStyle & kClipStyles);
        targetRect = monitorInfo.rcMonitor;
    } else {
        targetStyle = kWindowedStyle | (currentStyle & kClipStyles);
        if (wasOurWindowed) {
            // Already windowed by us: the user may have moved, resized or
            // maximized it since, and a re-assert must not undo that.
            targetStyle |= currentStyle & WS_MAXIMIZE;
            targetRect = currentRect;
        } else if (_hasWindowedRect && MonitorFromRect(&_windowedRect, MONITOR_DEFAULTTONULL)) {
            targetRect = _windowedRect;
        } else {
            targetRect = windowedRectFor(_hwnd, _backBufferWidth, _backBufferHeight, targetStyle);
        }
    }

    // TOPMOST is what DXGI leaves behind after exclusive fullscreen; it keeps
    // every other window (and the Alt+Tab switcher) behind the game.
    const LONG targetExStyle = (currentExStyle | WS_EX_APPWINDOW) & ~(WS_EX_TOOLWINDOW | WS_EX_TOPMOST);

    const bool unchanged = targetStyle == currentStyle && targetExStyle == currentExStyle
        && EqualRect(&targetRect, &currentRect);
    if (!unchanged) {
        SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, targetExStyle);
        SetWindowLongPtrW(_hwnd, GWL_STYLE, targetStyle);

        // Activating is only right when the game already had the foreground;
        // otherwise a re-assert while the user is in another app would pull
        // the game back in front of them.
        UINT flags = SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOOWNERZORDER;
        if (GetForegroundWindow() != _hwnd)
            flags |= SWP_NOACTIVATE;
        SetWindowPos(_hwnd, HWND_NOTOPMOST, targetRect.left, targetRect.top,
                     targetRect.right - targetRect.left, targetRect.bottom - targetRect.top, flags);
    }

    _appliedWindowMode = mode;
    _hasAppliedWindowMode = true;
    if (persist)
        saveWindowModeConfig(mode);

    if (unchanged)
        return;

    // The rect the window actually ended up with, not the one that was asked for.
    RECT applied{};
    GetWindowRect(_hwnd, &applied);
    crabe::shared::Logger::getInstance().info(
        "RenderHook: window mode set to {} ({}x{} at {},{}, back buffer {}x{}).",
        mode == WindowMode::BorderlessWindowed ? "borderless" : "windowed",
        applied.right - applied.left, applied.bottom - applied.top, applied.left, applied.top,
        _backBufferWidth, _backBufferHeight);
}

// Scales this frame's queued mouse positions from client to back-buffer pixels
// and makes DisplaySize the back buffer. The game does not resize its buffers
// in a smaller window (it still believes it is fullscreen), DXGI stretches the
// back buffer into the client area, and ImGui must draw in back-buffer pixels
// to be stretched along with it -- otherwise the overlay shrinks into the
// top-left corner and clicks land in the wrong place. Identity when the two
// sizes match, which is the borderless case.
void RenderHook::mapImGuiToBackBuffer()
{
    if (_backBufferWidth == 0 || _backBufferHeight == 0)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 client = io.DisplaySize;
    io.DisplaySize = ImVec2(static_cast<float>(_backBufferWidth), static_cast<float>(_backBufferHeight));
    if (client.x <= 0.0f || client.y <= 0.0f)
        return;

    const float scaleX = io.DisplaySize.x / client.x;
    const float scaleY = io.DisplaySize.y / client.y;
    if (scaleX == 1.0f && scaleY == 1.0f)
        return;

    // Every queued event is from this frame: ConfigInputTrickleEventQueue is
    // off (ensureBackendInit), so NewFrame drains the queue completely and no
    // position can be scaled twice.
    for (ImGuiInputEvent& event : GImGui->InputEventsQueue) {
        if (event.Type != ImGuiInputEventType_MousePos || event.MousePos.PosX == -FLT_MAX)
            continue;
        event.MousePos.PosX *= scaleX;
        event.MousePos.PosY *= scaleY;
    }
}

// Returns the trampoline to the original Present method.
RenderHook::t_Present RenderHook::originalPresent() const
{
    return reinterpret_cast<t_Present>(_hookPresent.getOriginal());
}

// Returns the trampoline to the original ResizeBuffers method.
RenderHook::t_ResizeBuffers RenderHook::originalResizeBuffers() const
{
    return reinterpret_cast<t_ResizeBuffers>(_hookResizeBuffers.getOriginal());
}

// Returns the trampoline to the original SetFullscreenState method.
RenderHook::t_SetFullscreenState RenderHook::originalSetFullscreenState() const
{
    return reinterpret_cast<t_SetFullscreenState>(_hookSetFullscreenState.getOriginal());
}

// Returns the trampoline to the original GetFullscreenState method.
RenderHook::t_GetFullscreenState RenderHook::originalGetFullscreenState() const
{
    return reinterpret_cast<t_GetFullscreenState>(_hookGetFullscreenState.getOriginal());
}

// Returns the trampoline to the original ResizeTarget method.
RenderHook::t_ResizeTarget RenderHook::originalResizeTarget() const
{
    return reinterpret_cast<t_ResizeTarget>(_hookResizeTarget.getOriginal());
}

// Returns the trampoline to the original SetCursorPos function.
RenderHook::t_SetCursorPos RenderHook::originalSetCursorPos() const
{
    return reinterpret_cast<t_SetCursorPos>(_hookSetCursorPos.getOriginal());
}

// Returns the trampoline to the original ShowWindow function.
RenderHook::t_ShowWindow RenderHook::originalShowWindow() const
{
    return reinterpret_cast<t_ShowWindow>(_hookShowWindow.getOriginal());
}

// Releases the active backbuffer render target view.
void RenderHook::releaseRenderTarget()
{
    if (_context) {
        ID3D11RenderTargetView* nullViews[1] = { nullptr };
        _context->OMSetRenderTargets(1, nullViews, nullptr);
        _context->Flush();
    }
    if (_renderTargetView) {
        _renderTargetView->Release();
        _renderTargetView = nullptr;
    }
}

// Obtains backbuffer 0 and creates a single render target view.
void RenderHook::createRenderTarget(IDXGISwapChain* swapChain)
{
    if (!_device || !swapChain) return;

    releaseRenderTarget();

    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)))) {
        D3D11_TEXTURE2D_DESC backBufferDesc{};
        backBuffer->GetDesc(&backBufferDesc);
        _backBufferWidth = backBufferDesc.Width;
        _backBufferHeight = backBufferDesc.Height;
        _device->CreateRenderTargetView(backBuffer, nullptr, &_renderTargetView);
        backBuffer->Release();
    }
}

// Initializes device, context, DXGI association, and ImGui backends on first run.
void RenderHook::ensureBackendInit(IDXGISwapChain* swapChain)
{
    if (_backendInitialized) return;

    if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&_device))))
        return;
    _device->GetImmediateContext(&_context);

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    _hwnd = desc.OutputWindow;

    // Prevent DXGI from altering window styles or intercepting Alt+Enter/Alt+Tab
    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(swapChain->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)))) {
        factory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
        factory->Release();
    }

    // Ensure WS_EX_TOPMOST is removed from the window so other apps can take foreground on Alt+Tab
    LONG_PTR initialExStyle = GetWindowLongPtrW(_hwnd, GWL_EXSTYLE);
    if (initialExStyle & WS_EX_TOPMOST) {
        initialExStyle &= ~WS_EX_TOPMOST;
        SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, initialExStyle);
    }
    SetWindowPos(_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    ImGui_ImplWin32_Init(_hwnd);
    ImGui_ImplDX11_Init(_device, _context);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    // mapImGuiToBackBuffer rescales the queued mouse events once per frame,
    // which is only safe if NewFrame consumes the whole queue every frame.
    ImGui::GetIO().ConfigInputTrickleEventQueue = false;

    _originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&RenderHook::hkWndProc)));

    createRenderTarget(swapChain);

    _backendInitialized = true;
    crabe::shared::Logger::getInstance().debug("RenderHook: ImGui DX11/Win32 initialized (hwnd 0x{:X}).",
                                reinterpret_cast<uintptr_t>(_hwnd));
}

// Refuses exclusive fullscreen. Both loader modes are DXGI-windowed, so a
// request to enter fullscreen is answered S_OK without entering it (the game
// may retry on every refocus; that path is a single GetFullscreenState).
// Leaving fullscreen is always passed through.
HRESULT __stdcall RenderHook::hkSetFullscreenState(IDXGISwapChain* swapChain, BOOL fullscreen,
                                                  IDXGIOutput* target)
{
    RenderHook& self = RenderHook::get();
    self._gameWantsFullscreen = fullscreen != FALSE;
    if (!fullscreen)
        return self.originalSetFullscreenState()(swapChain, FALSE, target);

    static std::atomic<bool> announced{false};
    if (!announced.exchange(true))
        crabe::shared::Logger::getInstance().info("RenderHook: game asked for exclusive fullscreen; kept windowed.");

    // Only when the swap chain really was exclusive (entered before the hook
    // existed) is there anything to undo, and the window to re-apply.
    if (self.leaveExclusiveFullscreen(swapChain))
        self._windowModeDirty = true;
    return S_OK;
}

// Reports fullscreen while the game believes it asked for (and got) it. The
// retail exe checks this when it handles window messages and, finding the swap
// chain windowed, retries SetFullscreenState and ResizeTarget without ever
// rendering again: the game froze on its first frame. The swap chain stays
// windowed; only the answer changes, and the output handed back is the one the
// window is on, so a caller that uses it gets a real, AddRef'd object.
HRESULT __stdcall RenderHook::hkGetFullscreenState(IDXGISwapChain* swapChain, BOOL* fullscreen,
                                                  IDXGIOutput** target)
{
    RenderHook& self = RenderHook::get();
    const HRESULT hr = self.originalGetFullscreenState()(swapChain, fullscreen, target);
    if (FAILED(hr) || !self._gameWantsFullscreen.load())
        return hr;

    if (fullscreen)
        *fullscreen = TRUE;
    if (target && !*target)
        swapChain->GetContainingOutput(target);
    return hr;
}

// The loader owns the window size; in a DXGI-windowed swap chain ResizeTarget
// would resize the window to the game's render resolution, so it is swallowed.
// The back buffer still follows the game's ResizeBuffers and is stretched.
HRESULT __stdcall RenderHook::hkResizeTarget(IDXGISwapChain* swapChain, const DXGI_MODE_DESC* newTargetParameters)
{
    (void)swapChain;
    if (newTargetParameters) {
        crabe::shared::Logger::getInstance().debug("RenderHook: ignored ResizeTarget({}x{}).",
                                                   newTargetParameters->Width, newTargetParameters->Height);
    }
    return S_OK;
}

// Intercepts SetCursorPos calls from the game to suppress cursor centering while overlay is open.
BOOL WINAPI RenderHook::hkSetCursorPos(int X, int Y)
{
    RenderHook& self = RenderHook::get();
    if (self._menuOpen.load()) {
        return TRUE;
    }
    return self.originalSetCursorPos()(X, Y);
}

// Intercepts ShowWindow calls from the game to suppress window minimization in borderless mode.
BOOL WINAPI RenderHook::hkShowWindow(HWND hWnd, int nCmdShow)
{
    RenderHook& self = RenderHook::get();
    if (self._hwnd && hWnd == self._hwnd && self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
        if (nCmdShow == SW_MINIMIZE || nCmdShow == SW_FORCEMINIMIZE || nCmdShow == SW_SHOWMINIMIZED) {
            // Block minimization and keep window displayed without stealing focus
            return self.originalShowWindow()(hWnd, SW_SHOWNA);
        }
    }
    return self.originalShowWindow()(hWnd, nCmdShow);
}

// Drives frame rendering, pending mode application, and UI overlay rendering.
HRESULT __stdcall RenderHook::hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    RenderHook& self = RenderHook::get();

    // This is where the render thread is identified, because it is the only
    // place the loader is certain it is on it -- Present *is* the render
    // thread, by definition (invariant I3). Declared unconditionally rather
    // than behind a thread_local flag: declareThreadRole is idempotent (a
    // sixteen-slot scan and a compare-exchange that fails after the first),
    // and doing it every frame means a second thread that ever calls Present
    // is recorded too, without this file depending on dynamic TLS in a DLL
    // that has DisableThreadLibraryCalls set.
    crabe::infrastructure::CrashReporter::declareThreadRole(
        crabe::infrastructure::ThreadRole::Render);
    static std::atomic<bool> presentAnnounced{false};
    if (!presentAnnounced.exchange(true))
        crabe::infrastructure::CrashReporter::pushBreadcrumb("RenderHook: first Present");

    self.ensureBackendInit(swapChain);
    self.applyPendingWindowMode(swapChain);

    if (self._backendInitialized && self._device && self._context) {
        if (!self._renderTargetView) {
            self.createRenderTarget(swapChain);
        }

        if (self._renderTargetView) {
            self.applyPendingCursorVisibility();

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            self.mapImGuiToBackBuffer();

            if (!self._menuOpen.load()) {
                ImGuiIO& io = ImGui::GetIO();
                io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
                io.MouseDown[0] = false;
                io.MouseDown[1] = false;
                io.MouseDown[2] = false;
                io.MouseWheel = 0.0f;
                io.MouseWheelH = 0.0f;
            }

            ImGui::NewFrame();

            if (self._menuOpen)
                self._overlay.renderOverlay();

            crabe::infrastructure::CrashHandler::runGuarded([]() {
                crabe::presentation::DrawBuffer::get().replay();
            }, "RenderHook::replayDrawBuffer");

            ImGui::Render();
            self._context->OMSetRenderTargets(1, &self._renderTargetView, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            ID3D11RenderTargetView* nullViews[1] = { nullptr };
            self._context->OMSetRenderTargets(1, nullViews, nullptr);
        }
    }

    HRESULT hr = self.originalPresent()(swapChain, syncInterval, flags);
    if (hr == DXGI_STATUS_OCCLUDED) {
        return S_OK;
    }
    if (FAILED(hr)) {
        crabe::shared::Logger::getInstance().error("RenderHook: Present returned 0x{:X}.", static_cast<uint32_t>(hr));
        if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_INVALID_CALL) {
            return S_OK;
        }
    }
    return hr;
}

// Releases and recreates render target view across swapchain resizing.
HRESULT __stdcall RenderHook::hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                            UINT width, UINT height, DXGI_FORMAT newFormat,
                                            UINT swapChainFlags)
{
    RenderHook& self = RenderHook::get();

    crabe::shared::Logger::getInstance().info("RenderHook: ResizeBuffers requested ({}x{}, format {}).",
                                              width, height, static_cast<uint32_t>(newFormat));

    self.releaseRenderTarget();

    HRESULT hr = self.originalResizeBuffers()(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
    if (FAILED(hr)) {
        crabe::shared::Logger::getInstance().error("RenderHook: ResizeBuffers failed (0x{:X}).", static_cast<uint32_t>(hr));
    }

    if (SUCCEEDED(hr) && self._backendInitialized && self._device) {
        self.createRenderTarget(swapChain);
    }
    return hr;
}

// Intercepts input messages, handles alt-tab cursor release, and routes hotkeys.
LRESULT CALLBACK RenderHook::hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RenderHook& self = RenderHook::get();

    // Same reasoning as hkPresent: a WndProc runs on the thread that owns the
    // window, so this is where the window thread names itself. If that turns
    // out to be the same thread Present runs on, the slot keeps whichever role
    // was declared first (see declareThreadRole) rather than flip-flopping
    // between the two from launch to launch.
    crabe::infrastructure::CrashReporter::declareThreadRole(
        crabe::infrastructure::ThreadRole::Window);
    static std::atomic<bool> messageAnnounced{false};
    if (!messageAnnounced.exchange(true))
        crabe::infrastructure::CrashReporter::pushBreadcrumb("RenderHook: first window message");

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
        const bool isDown = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        const bool isRepeat = isDown && (lParam & (1 << 30)) != 0;
        crabe::application::Loader::get().onKeyEvent(static_cast<int>(wParam), isDown, isRepeat);
    }

    // The loader owns the window in both modes, and neither is topmost.
    if (msg == WM_WINDOWPOSCHANGING) {
        auto* pos = reinterpret_cast<WINDOWPOS*>(lParam);
        if (pos && pos->hwndInsertAfter == HWND_TOPMOST && !(pos->flags & SWP_NOZORDER))
            pos->hwndInsertAfter = HWND_NOTOPMOST;
    }

    // A resolution or monitor layout change: borderless must re-fit the
    // monitor. Re-asserting is idempotent, so this is harmless in windowed.
    if (msg == WM_DISPLAYCHANGE)
        self._windowModeDirty = true;

    if (msg == WM_ACTIVATE) {
        const bool active = (LOWORD(wParam) != WA_INACTIVE);
        self._isFocused.store(active);
        if (!active) {
            ClipCursor(nullptr);
            if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        } else {
            if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
                SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                BringWindowToTop(hwnd);
            }
            self.updateCursorVisibility();
        }
        // Spoof to the game: always pretend active
        if (LOWORD(wParam) == WA_INACTIVE) {
            wParam = MAKEWPARAM(WA_ACTIVE, HIWORD(wParam));
        }
    } else if (msg == WM_ACTIVATEAPP) {
        const bool active = (wParam != FALSE);
        self._isFocused.store(active);
        if (!active) {
            ClipCursor(nullptr);
            if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        } else {
            if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
                SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                BringWindowToTop(hwnd);
            }
        }
        // Spoof to the game: pretend app is always active
        wParam = TRUE;
    } else if (msg == WM_NCACTIVATE) {
        // Keep active title bar look and avoid game reacting to inactive border
        return CallWindowProcW(self._originalWndProc, hwnd, msg, TRUE, lParam);
    } else if (msg == WM_SIZE) {
        if (wParam == SIZE_MINIMIZED) {
            // Block internal minimization logic
            return 0;
        }
    } else if (msg == WM_KILLFOCUS) {
        self._isFocused.store(false);
        ClipCursor(nullptr);
    } else if (msg == WM_SETFOCUS) {
        self._isFocused.store(true);
        if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
            SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            BringWindowToTop(hwnd);
        }
        self.updateCursorVisibility();
    }

    // When the Insert console overlay is open, ImGui draws the software cursor.
    // Suppress the OS cursor ONLY while the Insert overlay is open, so that
    // the game's native menus (Pause, Level Select, Toy Box) retain their normal cursor.
    if (msg == WM_SETCURSOR && self._menuOpen.load()) {
        if (LOWORD(lParam) == HTCLIENT) {
            SetCursor(nullptr);
            return TRUE;
        }
    }

    if (msg == WM_SYSKEYDOWN) {
        if (wParam == VK_RETURN && (lParam & (1 << 29))) {
            // Held Alt+Enter auto-repeats; one press is one toggle.
            if (lParam & (1 << 30))
                return 0;
            WindowMode next = (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed)
                ? WindowMode::Windowed : WindowMode::BorderlessWindowed;
            self.requestWindowMode(next);
            return 0;
        }
        if (wParam == VK_F4 && (lParam & (1 << 29))) {
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    }

    if (msg == WM_SYSCOMMAND) {
        WPARAM cmd = wParam & 0xFFF0;
        if (cmd == SC_MINIMIZE && self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
            ClipCursor(nullptr);
            return 0; // Prevent window minimization on focus loss in borderless mode
        }
        if (cmd == SC_RESTORE && self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
            SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            BringWindowToTop(hwnd);
        }
        if (cmd == SC_KEYMENU && lParam != VK_SPACE) {
            return 0;
        }
    }

    // Case 1: Insert console overlay is open
    if (self._menuOpen.load()) {
        if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
            if (wParam == VK_TAB || wParam == VK_F4 || wParam == VK_SPACE || wParam == VK_MENU)
                return CallWindowProcW(self._originalWndProc, hwnd, msg, wParam, lParam);
        }

        // Consume all mouse messages so clicks, drags, and wheel do not pass through to the game
        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
            return 0;
        }

        // Consume keyboard messages except hotkeys so console typing doesn't steer the game
        if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR) {
            if (wParam != VK_INSERT && wParam != VK_F5 && wParam != VK_F4) {
                return 0;
            }
        }
    }

    // Case 2: a mod asked for these keys, so the game must not also act on them.
    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR) {
        if (crabe::application::Loader::get().isKeyCaptured(static_cast<int>(wParam)))
            return 0;
    }

    return CallWindowProcW(self._originalWndProc, hwnd, msg, wParam, lParam);
}

} // namespace crabe::presentation

