#include <dxgi.h>
#include <fstream>
#include <string>

#include "presentation/render_hook.hpp"
#include "application/loader.hpp"
#include "domain/ModManager.hpp"
#include "infrastructure/crash_handler.hpp"
#include "shared/logger.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

// Reads window mode configuration file or defaults to borderless.
WindowMode RenderHook::loadWindowModeConfig()
{
    std::ifstream file("crabe_window_mode.cfg");
    if (!file.is_open()) {
        return WindowMode::BorderlessWindowed;
    }
    std::string mode;
    file >> mode;
    if (mode == "windowed") {
        return WindowMode::Windowed;
    }
    return WindowMode::BorderlessWindowed;
}

// Writes active window mode to the configuration file.
void RenderHook::saveWindowModeConfig(WindowMode mode)
{
    std::ofstream file("crabe_window_mode.cfg", std::ios::trunc);
    if (file.is_open()) {
        file << (mode == WindowMode::BorderlessWindowed ? "borderless" : "windowed");
    }
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
        Logger::getInstance().error("RenderHook: failed to create dummy window.");
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
        Logger::getInstance().error("RenderHook: D3D11CreateDeviceAndSwapChain failed (0x{:X}).",
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
        Logger::getInstance().error("RenderHook: failed to resolve swapchain vtable.");
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

    if (_backendInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        _backendInitialized = false;
    }

    if (_context) { _context->Release(); _context = nullptr; }
    if (_device) { _device->Release(); _device = nullptr; }
}

// Sets cursor visibility state based on whether menus are currently open.
void RenderHook::updateCursorVisibility()
{
    // The mouse cursor is ONLY shown for the Insert console overlay (_menuOpen).
    // The F5 mod menu (_modMenuOpen) is purely keyboard/gamepad driven.
    bool showCursor = _menuOpen.load();
    ImGui::GetIO().MouseDrawCursor = showCursor;
    if (showCursor) {
        ClipCursor(nullptr);
    }
}

// Toggles visibility of the debug overlay and updates mouse cursor.
void RenderHook::toggleMenu()
{
    _menuOpen = !_menuOpen;
    updateCursorVisibility();
    Logger::getInstance().debug("RenderHook: overlay {}.", _menuOpen ? "opened" : "closed");
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

// Changes window style and dimensions and updates swapchain buffers if needed.
void RenderHook::applyPendingWindowMode([[maybe_unused]] IDXGISwapChain* swapChain)
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

        // Force WS_EX_APPWINDOW so the game always remains visible on the Windows taskbar
        LONG_PTR exStyle = GetWindowLongPtrW(_hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_APPWINDOW;
        exStyle &= ~WS_EX_TOOLWINDOW;
        SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, exStyle);

        SetWindowLongPtrW(_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(_hwnd, HWND_TOP, targetRect.left, targetRect.top,
                    targetRect.right - targetRect.left, targetRect.bottom - targetRect.top,
                    SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        targetRect = _originalRect;
        int w = targetRect.right - targetRect.left;
        int h = targetRect.bottom - targetRect.top;
        if (w > 100 && h > 100) {
            LONG_PTR exStyle = GetWindowLongPtrW(_hwnd, GWL_EXSTYLE);
            exStyle |= WS_EX_APPWINDOW;
            exStyle &= ~WS_EX_TOOLWINDOW;
            SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, exStyle);

            SetWindowLongPtrW(_hwnd, GWL_STYLE, _originalStyle | WS_VISIBLE);
            SetWindowPos(_hwnd, HWND_TOP, targetRect.left, targetRect.top, w, h,
                        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    }

    ShowWindow(_hwnd, SW_SHOW);
    BringWindowToTop(_hwnd);
    SetForegroundWindow(_hwnd);
    saveWindowModeConfig(currentMode);

    Logger::getInstance().info("RenderHook: window mode set to {}.",
                               currentMode == WindowMode::BorderlessWindowed ? "borderless" : "windowed");
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

// Releases the active backbuffer render target view.
void RenderHook::releaseRenderTarget()
{
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

    ImGui_ImplWin32_Init(_hwnd);
    ImGui_ImplDX11_Init(_device, _context);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    _originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&RenderHook::hkWndProc)));

    createRenderTarget(swapChain);

    _backendInitialized = true;
    Logger::getInstance().debug("RenderHook: ImGui DX11/Win32 initialized (hwnd 0x{:X}).",
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

// Drives frame rendering, pending mode application, and UI overlay rendering.
HRESULT __stdcall RenderHook::hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    RenderHook& self = RenderHook::get();

    self.ensureBackendInit(swapChain);
    self.applyPendingWindowMode(swapChain);

    if (self._backendInitialized && self._device && self._context) {
        if (!self._renderTargetView) {
            self.createRenderTarget(swapChain);
        }

        if (self._renderTargetView) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (self._menuOpen)
                self._overlay.renderOverlay();

            ImGui::Render();
            self._context->OMSetRenderTargets(1, &self._renderTargetView, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
    }

    return self.originalPresent()(swapChain, syncInterval, flags);
}

// Releases and recreates render target view across swapchain resizing.
HRESULT __stdcall RenderHook::hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                            UINT width, UINT height, DXGI_FORMAT newFormat,
                                            UINT swapChainFlags)
{
    RenderHook& self = RenderHook::get();

    self.releaseRenderTarget();

    HRESULT hr = self.originalResizeBuffers()(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
    if (SUCCEEDED(hr) && self._backendInitialized && self._device) {
        self.createRenderTarget(swapChain);
    }
    return hr;
}

// Intercepts input messages, handles alt-tab cursor release, and routes hotkeys.
LRESULT CALLBACK RenderHook::hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RenderHook& self = RenderHook::get();

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
        Loader::get().onKeyEvent(static_cast<int>(wParam), msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    }

    if (msg == WM_ACTIVATE) {
        if (LOWORD(wParam) == WA_INACTIVE) {
            ClipCursor(nullptr);
        } else {
            self.updateCursorVisibility();
        }
    } else if (msg == WM_KILLFOCUS) {
        ClipCursor(nullptr);
    } else if (msg == WM_SETFOCUS) {
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
        if (wParam == VK_TAB && (lParam & (1 << 29))) {
            ClipCursor(nullptr);
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        if (wParam == VK_F4 && (lParam & (1 << 29))) {
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    }

    if (msg == WM_SYSCOMMAND) {
        WPARAM cmd = wParam & 0xFFF0;
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

    return CallWindowProcW(self._originalWndProc, hwnd, msg, wParam, lParam);
}
