/*
** CrabeLoader
** File description:
** render_hook
*/

#include <dxgi.h>

#include "loader/render_hook.hpp"
#include "logger/logger.hpp"

// imgui_impl_win32.h deliberately hides this declaration behind '#if 0' to
// avoid forcing <windows.h> on every include site; callers are expected to
// copy this forward declaration, which is what we do here.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {
    constexpr int kPresentVtableIndex = 8;
    constexpr int kResizeBuffersVtableIndex = 13;
    constexpr wchar_t kDummyClassName[] = L"CrabeLoaderDummyWindow";

    // A resolved address of 0 means "not found": skip it rather than patch a
    // guess, since a wrong address overwrites live code and crashes the host
    // process on the next execution.
    bool installOne(Hook& hook, uintptr_t addr, void* detour, const char* name)
    {
        Logger& logger = Logger::getInstance();

        if (addr == 0) {
            logger.warning("RenderHook: {} skipped (address not resolved).", name);
            return false;
        }
        if (!hook.install(reinterpret_cast<void*>(addr), detour)) {
            logger.error("RenderHook: failed to hook {} at 0x{:X}.", name, addr);
            return false;
        }
        logger.debug("RenderHook: {} hooked.", name);
        return true;
    }
}

RenderHook& RenderHook::get()
{
    static RenderHook instance;
    return instance;
}

bool RenderHook::resolveSwapChainFunctions(uintptr_t& outPresent, uintptr_t& outResizeBuffers)
{
    outPresent = 0;
    outResizeBuffers = 0;

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
        Logger::getInstance().error("RenderHook: failed to create the dummy window.");
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
        outResizeBuffers = reinterpret_cast<uintptr_t>(vtable[kResizeBuffersVtableIndex]);
        ok = true;
    } else {
        Logger::getInstance().error("RenderHook: D3D11CreateDeviceAndSwapChain failed (hr 0x{:X}).",
                                    static_cast<uint32_t>(hr));
    }

    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
    DestroyWindow(dummyHwnd);
    UnregisterClassW(kDummyClassName, wc.hInstance);

    return ok;
}

bool RenderHook::initialize()
{
    uintptr_t presentAddr = 0;
    uintptr_t resizeBuffersAddr = 0;

    if (!resolveSwapChainFunctions(presentAddr, resizeBuffersAddr)) {
        Logger::getInstance().error("RenderHook: failed to resolve IDXGISwapChain's vtable.");
        return false;
    }

    bool anyInstalled = false;
    anyInstalled |= installOne(_hookPresent, presentAddr,
                            reinterpret_cast<void*>(&RenderHook::hkPresent), "IDXGISwapChain::Present");
    anyInstalled |= installOne(_hookResizeBuffers, resizeBuffersAddr,
                            reinterpret_cast<void*>(&RenderHook::hkResizeBuffers), "IDXGISwapChain::ResizeBuffers");

    return anyInstalled;
}

void RenderHook::uninitialize()
{
    if (_originalWndProc && _hwnd) {
        SetWindowLongPtrW(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_originalWndProc));
        _originalWndProc = nullptr;
    }

    releaseRenderTarget();

    _hookPresent.remove();
    _hookResizeBuffers.remove();

    if (_backendInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        _backendInitialized = false;
    }

    if (_context) { _context->Release(); _context = nullptr; }
    if (_device) { _device->Release(); _device = nullptr; }
}

void RenderHook::toggleMenu()
{
    _menuOpen = !_menuOpen;
    ImGui::GetIO().MouseDrawCursor = _menuOpen;
    if (_menuOpen) ClipCursor(nullptr);

    Logger::getInstance().debug("RenderHook: overlay {}.", _menuOpen ? "opened" : "closed");
}

bool RenderHook::isMenuOpen() const
{
    return _menuOpen;
}

void RenderHook::requestWindowMode(WindowMode mode)
{
    _requestedWindowMode = mode;
    _windowModeDirty = true;
}

void RenderHook::applyPendingWindowMode(IDXGISwapChain* swapChain)
{
    if (!_windowModeDirty.exchange(false))
        return;
    if (!_hwnd)
        return;

    // Exclusive fullscreen bypasses the compositor and owns the display mode
    // directly; drop out of it first or GWL_STYLE alone won't make this
    // borderless-*windowed*.
    BOOL wasFullscreen = FALSE;
    swapChain->GetFullscreenState(&wasFullscreen, nullptr);
    if (wasFullscreen)
        swapChain->SetFullscreenState(FALSE, nullptr);

    RECT targetRect;
    if (_requestedWindowMode == WindowMode::BorderlessWindowed) {
        HMONITOR monitor = MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfoW(monitor, &monitorInfo))
            return;

        targetRect = monitorInfo.rcMonitor;
        SetWindowLongPtrW(_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(_hwnd, HWND_TOP, targetRect.left, targetRect.top,
                    targetRect.right - targetRect.left, targetRect.bottom - targetRect.top,
                    SWP_FRAMECHANGED | SWP_NOZORDER);
    } else {
        targetRect = _originalRect;
        SetWindowLongPtrW(_hwnd, GWL_STYLE, _originalStyle);
        SetWindowPos(_hwnd, HWND_TOP, targetRect.left, targetRect.top,
                    targetRect.right - targetRect.left, targetRect.bottom - targetRect.top,
                    SWP_FRAMECHANGED | SWP_NOZORDER);
    }

    // The swap chain's back buffer didn't resize with the window; release
    // every view onto it first (same constraint as hkResizeBuffers).
    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    releaseRenderTarget();
    swapChain->ResizeBuffers(0, targetRect.right - targetRect.left, targetRect.bottom - targetRect.top,
                            DXGI_FORMAT_UNKNOWN, desc.Flags);

    bool isBorderless = _requestedWindowMode.load() == WindowMode::BorderlessWindowed;
    Logger::getInstance().info("RenderHook: window mode set to {}.", isBorderless ? "borderless" : "windowed");
}

RenderHook::t_Present RenderHook::originalPresent() const
{
    return reinterpret_cast<t_Present>(_hookPresent.getOriginal());
}

RenderHook::t_ResizeBuffers RenderHook::originalResizeBuffers() const
{
    return reinterpret_cast<t_ResizeBuffers>(_hookResizeBuffers.getOriginal());
}

void RenderHook::releaseRenderTarget()
{
    if (_renderTargetView) {
        _renderTargetView->Release();
        _renderTargetView = nullptr;
    }
}

void RenderHook::createRenderTarget(IDXGISwapChain* swapChain)
{
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)))) {
        _device->CreateRenderTargetView(backBuffer, nullptr, &_renderTargetView);
        backBuffer->Release();
    }
}

void RenderHook::ensureBackendInit(IDXGISwapChain* swapChain)
{
    if (_backendInitialized) return;

    if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&_device))))
        return;
    _device->GetImmediateContext(&_context);

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    _hwnd = desc.OutputWindow;

    // Captured once, before anything ever changes it, so a later "windowed"
    // request has something exact to restore rather than guessing a size.
    _originalStyle = GetWindowLongPtrW(_hwnd, GWL_STYLE);
    GetWindowRect(_hwnd, &_originalRect);

    ImGui_ImplWin32_Init(_hwnd);
    ImGui_ImplDX11_Init(_device, _context);

    _originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&RenderHook::hkWndProc)));

    createRenderTarget(swapChain);

    _backendInitialized = true;
    Logger::getInstance().debug("RenderHook: ImGui DX11/Win32 backends initialized (hwnd 0x{:X}).",
                                reinterpret_cast<uintptr_t>(_hwnd));
}

HRESULT __stdcall RenderHook::hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    RenderHook& self = RenderHook::get();

    self.ensureBackendInit(swapChain);

    if (self._backendInitialized) {
        self.applyPendingWindowMode(swapChain);

        if (!self._renderTargetView)
            self.createRenderTarget(swapChain);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (self._menuOpen)
            self._overlay.renderOverlay();

        ImGui::Render();
        self._context->OMSetRenderTargets(1, &self._renderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return self.originalPresent()(swapChain, syncInterval, flags);
}

HRESULT __stdcall RenderHook::hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                            UINT width, UINT height, DXGI_FORMAT newFormat,
                                            UINT swapChainFlags)
{
    RenderHook& self = RenderHook::get();

    // All views onto the back buffer (our RTV) must be released before the
    // real ResizeBuffers runs, or it fails; the next Present recreates it.
    self.releaseRenderTarget();

    return self.originalResizeBuffers()(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

LRESULT CALLBACK RenderHook::hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RenderHook& self = RenderHook::get();

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    if (self._menuOpen) {
        ImGuiIO& io = ImGui::GetIO();
        bool isMouseMsg = (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST);
        bool isKeyboardMsg = (msg == WM_KEYDOWN || msg == WM_KEYUP ||
                            msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_CHAR);

        // Overlay open: keep clicks/keys the player uses to drive the ImGui
        // UI from also reaching the game underneath.
        if ((isMouseMsg && io.WantCaptureMouse) || (isKeyboardMsg && io.WantCaptureKeyboard))
            return 0;
    }

    return CallWindowProcW(self._originalWndProc, hwnd, msg, wParam, lParam);
}
