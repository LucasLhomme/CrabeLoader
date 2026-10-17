#pragma once
#include <dxgi.h>
namespace crabe::presentation {
// Optional runtime on the game's existing device. RenderHook owns all calls.
// Opt in with Crabe/ReShade32.dll; no root ReShade graphics proxy may coexist.
class ReShadeBridge final {
public:
    static void present(IDXGISwapChain* swapChain, UINT flags);
    static void beforeResize(IDXGISwapChain* swapChain);
    static void shutdown();
};
}
