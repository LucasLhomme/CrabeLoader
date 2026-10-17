/*
** CrabeLoader
** File description:
** Declares the render thread lifecycle for the optional ReShade host.
** The runtime is destroyed before resizing and the DLL stays loaded for input hooks.
** Graphics interception and game depth capture are outside this host's responsibility.
**
** Authors: @lightningcal
*/
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
