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
#include <intrin.h>
#include <string>
#include <thread>

#include "presentation/render_hook.hpp"

#include <algorithm>
#include "imgui/imgui_internal.h"
#include "presentation/draw_buffer.hpp"
#include "presentation/texture_cache.hpp"
#include "application/loader.hpp"
#include "application/plugin_api.hpp"
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

    constexpr LONG kClipStyles = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    constexpr LONG kBorderlessStyle = WS_POPUP | WS_VISIBLE;
    constexpr LONG kWindowedStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

    struct DelayLoadDescriptor {
        DWORD attributes;
        DWORD dllNameRva;
        DWORD moduleHandleRva;
        DWORD importAddressTableRva;
        DWORD importNameTableRva;
        DWORD boundIatRva;
        DWORD unloadIatRva;
        DWORD timeDateStamp;
    };
}

// Returns the singleton instance of RenderHook.
RenderHook& RenderHook::get()
{
    static RenderHook instance;
    return instance;
}

// Reads the configured window mode from active domain configuration.
WindowMode RenderHook::loadWindowModeConfig()
{
    return crabe::domain::Config::active().windowMode() == crabe::domain::ConfigWindowMode::Windowed
        ? WindowMode::Windowed
        : WindowMode::BorderlessWindowed;
}

// Persists the active window mode into the display configuration.
void RenderHook::saveWindowModeConfig(WindowMode mode)
{
    const crabe::domain::ConfigWindowMode configMode = (mode == WindowMode::BorderlessWindowed)
        ? crabe::domain::ConfigWindowMode::Borderless
        : crabe::domain::ConfigWindowMode::Windowed;
    crabe::domain::Config::active().setWindowMode(std::filesystem::current_path(), configMode);
}

// Patches the main module's delay-load IAT entry for d3d11.dll to intercept
// D3D11CreateDeviceAndSwapChain instantly upon process attach, eliminating
// startup display-mode flicker without thread race conditions.
bool RenderHook::installEarlyDelayLoadHook()
{
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (!base) return false;

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

    const auto& delayDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
    if (delayDir.VirtualAddress == 0 || delayDir.Size == 0) return false;

    auto* desc = reinterpret_cast<const DelayLoadDescriptor*>(base + delayDir.VirtualAddress);
    while (desc->dllNameRva != 0) {
        const char* dllName = reinterpret_cast<const char*>(base + desc->dllNameRva);
        if (_stricmp(dllName, "d3d11.dll") == 0) {
            auto* iat = reinterpret_cast<void**>(base + desc->importAddressTableRva);
            DWORD oldProtect = 0;
            if (VirtualProtect(iat, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
                *iat = reinterpret_cast<void*>(&RenderHook::hkD3D11CreateDeviceAndSwapChain);
                VirtualProtect(iat, sizeof(void*), oldProtect, &oldProtect);
                crabe::shared::Logger::getInstance().info(
                    "RenderHook: installed early delay-load IAT interceptor for D3D11CreateDeviceAndSwapChain.");
                return true;
            }
            break;
        }
        ++desc;
    }
    return false;
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
        auto targetScreenToClient = reinterpret_cast<void*>(GetProcAddress(user32, "ScreenToClient"));
        if (targetScreenToClient) {
            _hookScreenToClient.installLogged(reinterpret_cast<uintptr_t>(targetScreenToClient),
                                              reinterpret_cast<void*>(&RenderHook::hkScreenToClient),
                                              "RenderHook", "ScreenToClient");
        }
        auto targetClientToScreen = reinterpret_cast<void*>(GetProcAddress(user32, "ClientToScreen"));
        if (targetClientToScreen) {
            _hookClientToScreen.installLogged(reinterpret_cast<uintptr_t>(targetClientToScreen),
                                              reinterpret_cast<void*>(&RenderHook::hkClientToScreen),
                                              "RenderHook", "ClientToScreen");
        }
    }

    HMODULE d3d11 = GetModuleHandleW(L"d3d11.dll");
    if (!d3d11) d3d11 = LoadLibraryW(L"d3d11.dll");
    if (d3d11) {
        auto targetD3D11Create = reinterpret_cast<void*>(GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain"));
        if (targetD3D11Create) {
            _hookD3D11CreateDeviceAndSwapChain.installLogged(
                reinterpret_cast<uintptr_t>(targetD3D11Create),
                reinterpret_cast<void*>(&RenderHook::hkD3D11CreateDeviceAndSwapChain),
                "RenderHook", "D3D11CreateDeviceAndSwapChain");
        }
    }

    _requestedWindowMode = loadWindowModeConfig();
    _persistWindowMode = false;
    _windowModeDirty = true;
    setFrameLimit(crabe::domain::Config::active().frameLimit(), false);

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
    _hookScreenToClient.remove();
    _hookClientToScreen.remove();
    _hookD3D11CreateDeviceAndSwapChain.remove();

    DrawBuffer::setImageResolver(nullptr);
    TextureCache::get().release();

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

// Sets and optionally persists the frame rate limit.
void RenderHook::setFrameLimit(std::uint32_t fps, bool persist)
{
    const std::uint32_t limit = crabe::domain::Config::clampFrameLimit(fps);
    if (_frameLimit.exchange(limit) != limit) {
        if (limit == 0)
            crabe::shared::Logger::getInstance().info("RenderHook: frame limit removed.");
        else
            crabe::shared::Logger::getInstance().info("RenderHook: frame limit set to {} FPS.", limit);
    }
    if (persist)
        crabe::domain::Config::active().setFrameLimit(std::filesystem::current_path(), limit);
}

namespace {
    constexpr DWORD kHighResolutionTimer = 0x00000002;

    // Returns the cached performance counter frequency in ticks per second.
    long long performanceFrequency()
    {
        static const long long frequency = [] {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            return value.QuadPart;
        }();
        return frequency;
    }

    // Returns the current high-resolution performance counter tick count.
    long long performanceNow()
    {
        LARGE_INTEGER value{};
        QueryPerformanceCounter(&value);
        return value.QuadPart;
    }
}

// Calculates the effective sync interval based on refresh rate and frame limits.
UINT RenderHook::effectiveSyncInterval(UINT requested) noexcept
{
    if (_refreshCheckFrames++ % 120 == 0) {
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode))
            _displayRefreshHz.store(mode.dmDisplayFrequency);
    }

    const std::uint32_t limit = _frameLimit.load();
    const std::uint32_t refresh = _displayRefreshHz.load();
    if (requested != 0 && (limit == 0 || (refresh != 0 && limit >= refresh)))
        return 0;
    return requested;
}

// Throttles frame presentation to maintain the target frame rate limit.
void RenderHook::paceFrame()
{
    const long long frequency = performanceFrequency();
    long long now = performanceNow();

    if (_fpsWindowStart == 0)
        _fpsWindowStart = now;
    ++_fpsFrames;
    if (now - _fpsWindowStart >= frequency / 2) {
        _renderFps = static_cast<float>(static_cast<double>(_fpsFrames) * static_cast<double>(frequency)
                                        / static_cast<double>(now - _fpsWindowStart));
        _fpsWindowStart = now;
        _fpsFrames = 0;
    }

    const std::uint32_t limit = _frameLimit.load();
    if (limit == 0) {
        _nextFrameTicks = 0;
        return;
    }

    const long long period = frequency / limit;
    if (_nextFrameTicks == 0 || now - _nextFrameTicks > period)
        _nextFrameTicks = now;

    if (!_frameTimer) {
        HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, kHighResolutionTimer, TIMER_ALL_ACCESS);
        if (!timer)
            timer = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
        _frameTimer.reset(timer);
    }

    const long long spinTicks = frequency * 3 / 2000;
    for (;;) {
        now = performanceNow();
        const long long remaining = _nextFrameTicks - now;
        if (remaining <= 0)
            break;
        if (remaining > spinTicks) {
            const long long sleepTicks = remaining - spinTicks;
            if (_frameTimer) {
                LARGE_INTEGER due{};
                due.QuadPart = -(sleepTicks * 10000000LL / frequency);
                if (SetWaitableTimer(_frameTimer.get(), &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(_frameTimer.get(), INFINITE);
                else
                    Sleep(1);
            } else {
                Sleep(1);
            }
        } else {
            YieldProcessor();
        }
    }
    _nextFrameTicks += period;
}

// Requests a window mode change to be applied on the render thread.
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
    if (!swapChain)
        return false;
    BOOL fullscreen = FALSE;
    const HRESULT state = _hookGetFullscreenState.isInstalled()
        ? originalGetFullscreenState()(swapChain, &fullscreen, nullptr)
        : swapChain->GetFullscreenState(&fullscreen, nullptr);
    if (FAILED(state) || !fullscreen)
        return false;

    _gameWantsFullscreen = true;

    const HRESULT hr = _hookSetFullscreenState.isInstalled()
        ? originalSetFullscreenState()(swapChain, FALSE, nullptr)
        : swapChain->SetFullscreenState(FALSE, nullptr);
    crabe::shared::Logger::getInstance().info("RenderHook: left exclusive fullscreen (0x{:X}).",
                                              static_cast<uint32_t>(hr));
    return SUCCEEDED(hr);
}

// Applies the requested window mode styles and geometry to the game window.
void RenderHook::applyPendingWindowMode(IDXGISwapChain* swapChain)
{
    if (!_hwnd || !_windowModeDirty.exchange(false))
        return;

    if (IsIconic(_hwnd)) {
        _windowModeDirty = true;
        return;
    }

    leaveExclusiveFullscreen(swapChain);

    const WindowMode mode = _requestedWindowMode.load();
    const bool persist = _persistWindowMode.exchange(false);

    const LONG currentStyle = static_cast<LONG>(GetWindowLongPtrW(_hwnd, GWL_STYLE));
    const LONG currentExStyle = static_cast<LONG>(GetWindowLongPtrW(_hwnd, GWL_EXSTYLE));
    RECT currentRect{};
    GetWindowRect(_hwnd, &currentRect);
    const bool decorated = (currentStyle & WS_CAPTION) == WS_CAPTION;
    const bool wasOurWindowed = _hasAppliedWindowMode && _appliedWindowMode == WindowMode::Windowed && decorated;

    if (wasOurWindowed && !IsZoomed(_hwnd) && !IsIconic(_hwnd) && currentRect.left > -30000) {
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
        if (wasOurWindowed && currentRect.left > -30000) {
            targetStyle |= currentStyle & WS_MAXIMIZE;
            targetRect = currentRect;
        } else if (_hasWindowedRect && _windowedRect.left > -30000 && MonitorFromRect(&_windowedRect, MONITOR_DEFAULTTONULL)) {
            targetRect = _windowedRect;
        } else {
            targetRect = windowedRectFor(_hwnd, _backBufferWidth, _backBufferHeight, targetStyle);
        }
    }

    const LONG targetExStyle = (currentExStyle | WS_EX_APPWINDOW) & ~(WS_EX_TOOLWINDOW | WS_EX_TOPMOST);

    const bool unchanged = targetStyle == currentStyle && targetExStyle == currentExStyle
        && EqualRect(&targetRect, &currentRect);
    if (!unchanged) {
        SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, targetExStyle);
        SetWindowLongPtrW(_hwnd, GWL_STYLE, targetStyle);

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

    RECT applied{};
    GetWindowRect(_hwnd, &applied);
    crabe::shared::Logger::getInstance().info(
        "RenderHook: window mode set to {} ({}x{} at {},{}, back buffer {}x{}).",
        mode == WindowMode::BorderlessWindowed ? "borderless" : "windowed",
        applied.right - applied.left, applied.bottom - applied.top, applied.left, applied.top,
        _backBufferWidth, _backBufferHeight);
}

// Scales queued ImGui mouse events from client coordinates to back-buffer pixels.
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

// Returns the trampoline to the original D3D11CreateDeviceAndSwapChain function.
RenderHook::t_D3D11CreateDeviceAndSwapChain RenderHook::originalD3D11CreateDeviceAndSwapChain() const
{
    return reinterpret_cast<t_D3D11CreateDeviceAndSwapChain>(_hookD3D11CreateDeviceAndSwapChain.getOriginal());
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

    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(swapChain->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)))) {
        factory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
        factory->Release();
    }

    LONG_PTR initialExStyle = GetWindowLongPtrW(_hwnd, GWL_EXSTYLE);
    if (initialExStyle & WS_EX_TOPMOST) {
        initialExStyle &= ~WS_EX_TOPMOST;
        SetWindowLongPtrW(_hwnd, GWL_EXSTYLE, initialExStyle);
    }
    SetWindowPos(_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    ImGui_ImplWin32_Init(_hwnd);
    ImGui_ImplDX11_Init(_device, _context);
    TextureCache::get().attach(_device);
    DrawBuffer::setImageResolver(&TextureCache::resolveForDrawBuffer);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::GetIO().ConfigInputTrickleEventQueue = false;

    _originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&RenderHook::hkWndProc)));

    createRenderTarget(swapChain);

    _backendInitialized = true;
    crabe::shared::Logger::getInstance().debug("RenderHook: ImGui DX11/Win32 initialized (hwnd 0x{:X}).",
                                reinterpret_cast<uintptr_t>(_hwnd));
}

// Intercepts SetFullscreenState to keep the swap chain windowed.
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

    if (self.leaveExclusiveFullscreen(swapChain))
        self._windowModeDirty = true;
    return S_OK;
}

// Detour for GetFullscreenState reporting fullscreen state to the game when requested.
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

// Detour for ResizeTarget, tracking target resolution while keeping window size controlled.
HRESULT __stdcall RenderHook::hkResizeTarget(IDXGISwapChain* swapChain, const DXGI_MODE_DESC* newTargetParameters)
{
    (void)swapChain;
    if (newTargetParameters) {
        crabe::shared::Logger::getInstance().debug("RenderHook: ignored ResizeTarget({}x{}).",
                                                   newTargetParameters->Width, newTargetParameters->Height);
        if (newTargetParameters->Width > 0 && newTargetParameters->Height > 0) {
            RenderHook& self = RenderHook::get();
            self._targetWidth = newTargetParameters->Width;
            self._targetHeight = newTargetParameters->Height;
        }
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

// Refuses the game's minimize requests, which would drop the window into the taskbar and break ResizeBuffers.
// The render thread repeats the request every frame while unfocused, and a real ShowWindow from it waits on the
// window thread, which waits on rendering during a load: alt-tab froze the game, so a refused request never blocks.
BOOL WINAPI RenderHook::hkShowWindow(HWND hWnd, int nCmdShow)
{
    RenderHook& self = RenderHook::get();
    const bool minimize = nCmdShow == SW_MINIMIZE || nCmdShow == SW_FORCEMINIMIZE || nCmdShow == SW_SHOWMINIMIZED;
    if (!self._hwnd || hWnd != self._hwnd || !minimize)
        return self.originalShowWindow()(hWnd, nCmdShow);

    const BOOL wasVisible = IsWindowVisible(hWnd);
    if (!wasVisible)
        ShowWindowAsync(hWnd, SW_SHOWNA);
    return wasVisible;
}

namespace {
    // True when `address` lies in the game executable's image.
    bool isGameCode(uintptr_t address)
    {
        static const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        static const uintptr_t size = [] {
            const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            return static_cast<uintptr_t>(nt->OptionalHeader.SizeOfImage);
        }();
        return address >= base && address < base + size;
    }
}

// Computes the scale factor between client window dimensions and back-buffer size.
bool RenderHook::backBufferScale(HWND hWnd, float& scaleX, float& scaleY) const
{
    if (!_hwnd || hWnd != _hwnd || _backBufferWidth == 0 || _backBufferHeight == 0)
        return false;
    RECT client{};
    if (!GetClientRect(hWnd, &client) || client.right <= 0 || client.bottom <= 0)
        return false;
    if (static_cast<UINT>(client.right) == _backBufferWidth && static_cast<UINT>(client.bottom) == _backBufferHeight)
        return false;
    scaleX = static_cast<float>(_backBufferWidth) / static_cast<float>(client.right);
    scaleY = static_cast<float>(_backBufferHeight) / static_cast<float>(client.bottom);
    return true;
}

// Client pixels -> back-buffer pixels, for the game's own mouse reads only.
BOOL WINAPI RenderHook::hkScreenToClient(HWND hWnd, LPPOINT point)
{
    RenderHook& self = RenderHook::get();
    const auto original = reinterpret_cast<t_PointConversion>(self._hookScreenToClient.getOriginal());
    const BOOL result = original(hWnd, point);

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (result && point && isGameCode(reinterpret_cast<uintptr_t>(_ReturnAddress()))
        && self.backBufferScale(hWnd, scaleX, scaleY)) {
        static std::atomic<bool> announced{false};
        if (!announced.exchange(true)) {
            crabe::shared::Logger::getInstance().info(
                "RenderHook: game mouse mapped from client to back-buffer pixels (x{:.3f}, y{:.3f}).",
                scaleX, scaleY);
        }
        point->x = static_cast<LONG>(static_cast<float>(point->x) * scaleX);
        point->y = static_cast<LONG>(static_cast<float>(point->y) * scaleY);
    }
    return result;
}

// Back-buffer pixels -> client pixels, the inverse, so the game's cursor re-centring lands
// where it means to.
BOOL WINAPI RenderHook::hkClientToScreen(HWND hWnd, LPPOINT point)
{
    RenderHook& self = RenderHook::get();
    const auto original = reinterpret_cast<t_PointConversion>(self._hookClientToScreen.getOriginal());

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (point && isGameCode(reinterpret_cast<uintptr_t>(_ReturnAddress()))
        && self.backBufferScale(hWnd, scaleX, scaleY)) {
        point->x = static_cast<LONG>(static_cast<float>(point->x) / scaleX);
        point->y = static_cast<LONG>(static_cast<float>(point->y) / scaleY);
    }
    return original(hWnd, point);
}

// Hook for D3D11CreateDeviceAndSwapChain preventing the game from creating an exclusive fullscreen
// swap chain at startup, which causes monitor mode-switch flicker and desktop window reshuffling.
HRESULT WINAPI RenderHook::hkD3D11CreateDeviceAndSwapChain(
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
    ID3D11DeviceContext** ppImmediateContext)
{
    RenderHook& self = RenderHook::get();
    DXGI_SWAP_CHAIN_DESC descCopy{};
    const DXGI_SWAP_CHAIN_DESC* descToUse = pSwapChainDesc;

    if (pSwapChainDesc) {
        descCopy = *pSwapChainDesc;
        if (!descCopy.Windowed) {
            self._gameWantsFullscreen = true;
            descCopy.Windowed = TRUE;
            descToUse = &descCopy;
            crabe::shared::Logger::getInstance().info(
                "RenderHook: forced swap chain creation to windowed mode (suppressed startup mode-switch flicker).");
        }
    }

    t_D3D11CreateDeviceAndSwapChain originalFunc = self.originalD3D11CreateDeviceAndSwapChain();
    if (!originalFunc) {
        HMODULE d3d11 = GetModuleHandleW(L"d3d11.dll");
        if (!d3d11) d3d11 = LoadLibraryW(L"d3d11.dll");
        if (d3d11) {
            originalFunc = reinterpret_cast<t_D3D11CreateDeviceAndSwapChain>(
                GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain"));
        }
    }

    if (!originalFunc) {
        crabe::shared::Logger::getInstance().error("RenderHook: failed to resolve real D3D11CreateDeviceAndSwapChain.");
        return E_FAIL;
    }

    return originalFunc(
        pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels,
        SDKVersion, descToUse, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
}

// Drives frame rendering, pending mode application, and UI overlay rendering.
HRESULT __stdcall RenderHook::hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    RenderHook& self = RenderHook::get();

    crabe::infrastructure::CrashReporter::declareThreadRole(
        crabe::infrastructure::ThreadRole::Render);
    static std::atomic<bool> presentAnnounced{false};
    if (!presentAnnounced.exchange(true))
        crabe::infrastructure::CrashReporter::pushBreadcrumb("RenderHook: first Present");

    self.ensureBackendInit(swapChain);
    self.applyPendingWindowMode(swapChain);
    crabe::application::PluginApi::get().dispatchPresent(swapChain, flags);

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

    static std::atomic<UINT> lastSyncInterval{0xFFFFFFFFu};
    static std::atomic<UINT> lastPresentFlags{0xFFFFFFFFu};
    if (lastSyncInterval.exchange(syncInterval) != syncInterval
        || lastPresentFlags.exchange(flags) != flags) {
        DXGI_SWAP_CHAIN_DESC desc{};
        const bool haveDesc = SUCCEEDED(swapChain->GetDesc(&desc));
        crabe::shared::Logger::getInstance().info(
            "RenderHook: Present syncInterval={} flags=0x{:X}; swap chain effect={} buffers={} windowed={} flags=0x{:X} samples={} {}Hz.",
            syncInterval, flags, haveDesc ? static_cast<int>(desc.SwapEffect) : -1,
            haveDesc ? desc.BufferCount : 0, haveDesc ? static_cast<int>(desc.Windowed) : -1,
            haveDesc ? desc.Flags : 0, haveDesc ? desc.SampleDesc.Count : 0,
            haveDesc && desc.BufferDesc.RefreshRate.Denominator != 0
                ? desc.BufferDesc.RefreshRate.Numerator / desc.BufferDesc.RefreshRate.Denominator : 0);
    }

    self.paceFrame();

    const UINT presentInterval = self.effectiveSyncInterval(syncInterval);
    static std::atomic<bool> syncBypassAnnounced{false};
    if (presentInterval != syncInterval && !syncBypassAnnounced.exchange(true)) {
        crabe::shared::Logger::getInstance().info(
            "RenderHook: the game asks for vsync; presenting without it while the frame limit is unlimited or at or above the display refresh ({} Hz).",
            self._displayRefreshHz.load());
    }
    HRESULT hr = self.originalPresent()(swapChain, presentInterval, flags);
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

    if (width == 0 && height == 0 && self._targetWidth.load() > 0 && self._targetHeight.load() > 0) {
        width = self._targetWidth.load();
        height = self._targetHeight.load();
    }

    if (width == 0 && height == 0 && self._hwnd && (IsIconic(self._hwnd) || self._backBufferWidth > 0)) {
        if (self._backBufferWidth > 0 && self._backBufferHeight > 0) {
            width = self._backBufferWidth;
            height = self._backBufferHeight;
        }
    }

    crabe::shared::Logger::getInstance().info("RenderHook: ResizeBuffers requested ({}x{}, format {}).",
                                              width, height, static_cast<uint32_t>(newFormat));

    crabe::application::PluginApi::get().dispatchBeforeResize(swapChain);
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
        if (pos && pos->hwndInsertAfter == HWND_TOPMOST && !(pos->flags & SWP_NOZORDER))
            pos->hwndInsertAfter = HWND_NOTOPMOST;
    }

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
        wParam = TRUE;
    } else if (msg == WM_NCACTIVATE) {
        return CallWindowProcW(self._originalWndProc, hwnd, msg, TRUE, lParam);
    } else if (msg == WM_SIZE) {
        if (wParam == SIZE_MINIMIZED) {
            return 0;
        }
        if (wParam == SIZE_RESTORED && self._hasWindowedRect) {
            self._windowModeDirty = true;
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

    if (msg == WM_SETCURSOR && self._menuOpen.load()) {
        if (LOWORD(lParam) == HTCLIENT) {
            SetCursor(nullptr);
            return TRUE;
        }
    }

    if (msg == WM_SYSKEYDOWN) {
        if (wParam == VK_RETURN && (lParam & (1 << 29))) {
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
        if (cmd == SC_MINIMIZE) {
            ClipCursor(nullptr);
            if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
                return 0;
            }
            if (!IsIconic(hwnd) && !IsZoomed(hwnd)) {
                RECT r{};
                GetWindowRect(hwnd, &r);
                if (r.left > -30000) {
                    self._windowedRect = r;
                    self._hasWindowedRect = true;
                }
            }
        }
        if (cmd == SC_RESTORE) {
            if (self._requestedWindowMode.load() == WindowMode::BorderlessWindowed) {
                SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                BringWindowToTop(hwnd);
            } else if (self._hasWindowedRect && self._windowedRect.left > -30000) {
                WINDOWPLACEMENT wp{};
                wp.length = sizeof(wp);
                if (GetWindowPlacement(hwnd, &wp)) {
                    wp.rcNormalPosition = self._windowedRect;
                    wp.showCmd = SW_SHOWNORMAL;
                    SetWindowPlacement(hwnd, &wp);
                }
            }
        }
        if (cmd == SC_KEYMENU && lParam != VK_SPACE) {
            return 0;
        }
    }

    if (self._menuOpen.load()) {
        if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
            if (wParam == VK_TAB || wParam == VK_F4 || wParam == VK_SPACE || wParam == VK_MENU)
                return CallWindowProcW(self._originalWndProc, hwnd, msg, wParam, lParam);
        }

        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
            return 0;
        }

        if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR) {
            if (wParam != VK_INSERT && wParam != VK_F5 && wParam != VK_F4) {
                return 0;
            }
        }
    }

    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR) {
        if (crabe::application::Loader::get().isKeyCaptured(static_cast<int>(wParam)))
            return 0;
    }

    return CallWindowProcW(self._originalWndProc, hwnd, msg, wParam, lParam);
}

} // namespace crabe::presentation

