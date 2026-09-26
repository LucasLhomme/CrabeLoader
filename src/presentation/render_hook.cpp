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
#include <chrono>
#include <dxgi.h>
#include <filesystem>
#include <string>
#include <thread>

#include "presentation/render_hook.hpp"

#include <algorithm>
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
    constexpr int kResizeBuffersVtableIndex = 13;
    constexpr wchar_t kDummyClassName[] = L"CrabeLoaderDummyWindow";
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
                                          uintptr_t& outSetFullscreenState)
{
    outPresent = 0;
    outResizeBuffers = 0;
    outSetFullscreenState = 0;

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
        outResizeBuffers = reinterpret_cast<uintptr_t>(vtable[kResizeBuffersVtableIndex]);
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

// Installs MinHook hooks on DXGI Present, ResizeBuffers, and SetFullscreenState.
bool RenderHook::initialize()
{
    uintptr_t presentAddr = 0;
    uintptr_t resizeBuffersAddr = 0;
    uintptr_t setFullscreenStateAddr = 0;

    if (!resolveSwapChainFunctions(presentAddr, resizeBuffersAddr, setFullscreenStateAddr)) {
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

    _requestedWindowMode = loadWindowModeConfig();
    _windowModeDirty = false;

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

// Enqueues a window mode change to be applied on the render thread.
void RenderHook::requestWindowMode(WindowMode mode)
{
    _requestedWindowMode = mode;
    _windowModeDirty = true;
}

// A windowed rect for this monitor, sized from the swap chain's back buffer and
// centred on the work area.
//
// Computed rather than restored. _originalRect is whatever the window happened
// to be at the first Present, and by then hkSetFullscreenState may already have
// forced the game out of exclusive fullscreen and into a borderless popup
// covering the monitor -- so "restore the original" restored the borderless
// geometry and changed nothing on screen, while the log below still claimed the
// mode had been set.
static RECT windowedRectFor(HWND hwnd, IDXGISwapChain* swapChain, LONG style)
{
    UINT clientWidth = 1280;
    UINT clientHeight = 720;

    DXGI_SWAP_CHAIN_DESC desc{};
    if (swapChain && SUCCEEDED(swapChain->GetDesc(&desc)) && desc.BufferDesc.Width > 100
        && desc.BufferDesc.Height > 100) {
        clientWidth = desc.BufferDesc.Width;
        clientHeight = desc.BufferDesc.Height;
    }

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    RECT work{ 0, 0, 1920, 1080 };
    if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitorInfo))
        work = monitorInfo.rcWork;

    const LONG workWidth = work.right - work.left;
    const LONG workHeight = work.bottom - work.top;

    // The back buffer is usually the whole monitor, because that is what a
    // borderless window asked for. A window that size plus a title bar does not
    // fit on the screen it came from, so it is scaled down to leave the taskbar
    // and the frame visible -- otherwise "windowed" looks identical to
    // borderless and the toggle appears to do nothing.
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

// Changes window style and dimensions and updates swapchain buffers if needed.
void RenderHook::applyPendingWindowMode(IDXGISwapChain* swapChain)
{
    if (!_windowModeDirty.exchange(false) || !_hwnd) {
        return;
    }

    RECT targetRect{};
    WindowMode currentMode = _requestedWindowMode.load();

    if (currentMode == WindowMode::BorderlessWindowed) {
        HMONITOR monitor = MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfoW(monitor, &monitorInfo)) {
            return;
        }

        targetRect = monitorInfo.rcMonitor;

        // Force WS_EX_APPWINDOW and ensure WS_EX_TOPMOST is removed so other windows can appear in front on Alt+Tab
        LONG_PTR exStyle = GetWindowLongPtrW(_hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_APPWINDOW;
        exStyle &= ~WS_EX_TOOLWINDOW;
        exStyle &= ~WS_EX_TOPMOST;
        SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, exStyle);

        SetWindowLongPtrW(_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(_hwnd, HWND_NOTOPMOST, targetRect.left, targetRect.top,
                    targetRect.right - targetRect.left, targetRect.bottom - targetRect.top,
                    SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        // The style the window had before anything touched it, when that is a
        // real decorated window; a plain overlapped window otherwise, since
        // _originalStyle may have been captured while the game was already
        // borderless and WS_POPUP would leave it borderless.
        LONG style = static_cast<LONG>(_originalStyle);
        if ((style & WS_CAPTION) != WS_CAPTION)
            style = WS_OVERLAPPEDWINDOW;
        style |= WS_VISIBLE;

        targetRect = windowedRectFor(_hwnd, swapChain, style);

        LONG_PTR exStyle = GetWindowLongPtrW(_hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_APPWINDOW;
        exStyle &= ~WS_EX_TOOLWINDOW;
        exStyle &= ~WS_EX_TOPMOST;
        SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, exStyle);

        SetWindowLongPtrW(_hwnd, GWL_STYLE, style);
        SetWindowPos(_hwnd, HWND_NOTOPMOST, targetRect.left, targetRect.top,
                    targetRect.right - targetRect.left, targetRect.bottom - targetRect.top,
                    SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }

    ShowWindow(_hwnd, SW_SHOW);
    BringWindowToTop(_hwnd);
    SetForegroundWindow(_hwnd);
    saveWindowModeConfig(currentMode);

    // The rect the window actually ended up with, not the one that was asked
    // for. The previous version logged success unconditionally, including when
    // a guard above had skipped every call that changes anything -- which is
    // how a toggle that did nothing on screen still read as working.
    RECT applied{};
    GetWindowRect(_hwnd, &applied);
    crabe::shared::Logger::getInstance().info(
        "RenderHook: window mode set to {} ({}x{} at {},{}).",
        currentMode == WindowMode::BorderlessWindowed ? "borderless" : "windowed",
        applied.right - applied.left, applied.bottom - applied.top, applied.left, applied.top);
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

    _originalStyle = GetWindowLongPtrW(_hwnd, GWL_STYLE);
    GetWindowRect(_hwnd, &_originalRect);

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

    _originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&RenderHook::hkWndProc)));

    createRenderTarget(swapChain);

    _backendInitialized = true;
    crabe::shared::Logger::getInstance().debug("RenderHook: ImGui DX11/Win32 initialized (hwnd 0x{:X}).",
                                reinterpret_cast<uintptr_t>(_hwnd));
}

// Intercepts fullscreen toggles and enforces windowed mode for borderless display.
HRESULT __stdcall RenderHook::hkSetFullscreenState(IDXGISwapChain* swapChain, BOOL fullscreen,
                                                  IDXGIOutput* target)
{
    RenderHook& self = RenderHook::get();
    if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
        self.requestWindowMode(WindowMode::BorderlessWindowed);
        return self.originalSetFullscreenState()(swapChain, FALSE, nullptr);
    }
    if (self._requestedWindowMode.load() == WindowMode::Windowed) {
        return self.originalSetFullscreenState()(swapChain, FALSE, nullptr);
    }
    return self.originalSetFullscreenState()(swapChain, fullscreen, target);
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

    if (msg == WM_WINDOWPOSCHANGING) {
        auto* pos = reinterpret_cast<WINDOWPOS*>(lParam);
        if (pos && self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
            if (pos->hwndInsertAfter == HWND_TOPMOST) {
                pos->hwndInsertAfter = HWND_NOTOPMOST;
            }
        }
    }

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

