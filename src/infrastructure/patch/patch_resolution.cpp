/*
** CrabeLoader
** File description:
** Implements native resolution presets and frame rate limiting for SettingsVideo.
** Sorts and deduplicates resolutions natively before querying by the settings menu.
** Dispatches frame rate cap updates directly through the presentation subsystem.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/patch/patch_resolution.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "infrastructure/lua_call.hpp"
#include "presentation/render_hook.hpp"
#include "shared/logger.hpp"

namespace {

struct ResolutionPreset {
    std::uint32_t width{0};
    std::uint32_t height{0};
};

constexpr std::array<ResolutionPreset, 28> kResolutionPresets = {{
    { 640,  480 },
    { 800,  600 },
    { 1024, 768 },
    { 1280, 960 },
    { 1280, 1024 },
    { 1600, 1200 },
    { 1280, 800 },
    { 1440, 900 },
    { 1680, 1050 },
    { 1920, 1200 },
    { 2560, 1600 },
    { 960,  540 },
    { 1024, 576 },
    { 1136, 640 },
    { 1280, 720 },
    { 1360, 768 },
    { 1366, 768 },
    { 1600, 900 },
    { 1920, 1080 },
    { 2560, 1440 },
    { 2560, 1080 },
    { 3440, 1440 },
    { 3840, 1600 },
    { 5120, 1440 },
    { 5120, 2160 },
    { 3840, 2160 },
    { 5120, 2880 },
    { 7680, 4320 },
}};

constexpr std::array<std::uint32_t, 11> kFrameRateLimits = {
    0, 30, 45, 60, 75, 90, 120, 144, 160, 180, 240
};

struct ResolutionComputation {
    std::vector<ResolutionPreset> modes;
    std::uint32_t activeIndex{1};
};

// Computes sorted and deduplicated resolution entries with active mode index.
ResolutionComputation computeResolutionList()
{
    UINT curW = 1920;
    UINT curH = 1080;
    crabe::presentation::RenderHook::get().getRenderResolution(curW, curH);

    std::vector<ResolutionPreset> list;
    list.reserve(kResolutionPresets.size() + 1);

    for (const auto& preset : kResolutionPresets) {
        list.push_back(preset);
    }
    if (curW > 0 && curH > 0) {
        list.push_back({ curW, curH });
    }

    std::sort(list.begin(), list.end(), [](const ResolutionPreset& a, const ResolutionPreset& b) {
        if (a.width != b.width)
            return a.width < b.width;
        return a.height < b.height;
    });

    auto last = std::unique(list.begin(), list.end(), [](const ResolutionPreset& a, const ResolutionPreset& b) {
        return a.width == b.width && a.height == b.height;
    });
    list.erase(last, list.end());

    std::uint32_t activeIndex = 1;
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i].width == curW && list[i].height == curH) {
            activeIndex = static_cast<std::uint32_t>(i + 1);
            break;
        }
    }

    return { std::move(list), activeIndex };
}

// Lua C callback returning total resolution count and active 1-based index.
int __cdecl nativeGetResolutionCount(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const auto result = computeResolutionList();
    lua.pushNumber(L, static_cast<double>(result.modes.size()));
    lua.pushNumber(L, static_cast<double>(result.activeIndex));
    return 2;
}

// Lua C callback returning width, height, and display text for 1-based index.
int __cdecl nativeGetResolution(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const int index = static_cast<int>(lua.argToNumber(L, 1, 1.0));
    const auto result = computeResolutionList();

    if (index >= 1 && index <= static_cast<int>(result.modes.size())) {
        const auto& mode = result.modes[static_cast<std::size_t>(index - 1)];
        lua.pushNumber(L, static_cast<double>(mode.width));
        lua.pushNumber(L, static_cast<double>(mode.height));
        lua.pushString(L, std::format("{}x{}", mode.width, mode.height));
        return 3;
    }
    return 0;
}

// Lua C callback returning total count of supported frame rate limit choices.
int __cdecl nativeGetFrameLimitChoicesCount(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    lua.pushNumber(L, static_cast<double>(kFrameRateLimits.size()));
    return 1;
}

// Lua C callback returning choice label for 1-based frame rate limit index.
int __cdecl nativeGetFrameLimitChoice(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const int index = static_cast<int>(lua.argToNumber(L, 1, 1.0));
    if (index >= 1 && index <= static_cast<int>(kFrameRateLimits.size())) {
        const auto fps = kFrameRateLimits[static_cast<std::size_t>(index - 1)];
        if (fps == 0) {
            lua.pushString(L, "Unlimited");
        } else {
            lua.pushString(L, std::format("{} FPS", fps));
        }
        return 1;
    }
    return 0;
}

// Lua C callback returning 1-based index of current frame rate limit.
int __cdecl nativeGetFrameLimit(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const std::uint32_t current = crabe::presentation::RenderHook::get().getFrameLimit();
    int matchIndex = 1;
    for (std::size_t i = 0; i < kFrameRateLimits.size(); ++i) {
        if (kFrameRateLimits[i] == current) {
            matchIndex = static_cast<int>(i + 1);
            break;
        }
    }
    lua.pushNumber(L, static_cast<double>(matchIndex));
    return 1;
}

// Lua C callback setting frame rate limit from selected choice index.
int __cdecl nativeSetFrameLimit(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    const int index = static_cast<int>(lua.argToNumber(L, 3, 1.0));
    if (index >= 1 && index <= static_cast<int>(kFrameRateLimits.size())) {
        const std::uint32_t fps = kFrameRateLimits[static_cast<std::size_t>(index - 1)];
        crabe::presentation::RenderHook::get().setFrameLimit(fps, true);
    }
    return 0;
}

} // namespace

namespace crabe::infrastructure::patch {

// Applies native resolution presets and frame rate limiting to SettingsVideo.
bool PatchResolution::apply(void* L)
{
    if (!L)
        return false;

    auto& lua = LuaCall::get();
    bool ok = true;

    ok = lua.registerNativeFunction(L, "Crabe", "_patchGetResolutionCount", &nativeGetResolutionCount) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchGetResolution", &nativeGetResolution) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchGetFrameLimitChoicesCount", &nativeGetFrameLimitChoicesCount) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchGetFrameLimitChoice", &nativeGetFrameLimitChoice) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchGetFrameLimit", &nativeGetFrameLimit) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchSetFrameLimit", &nativeSetFrameLimit) && ok;

    if (!ok) {
        shared::Logger::getInstance().error("PatchResolution: failed to register native callbacks.");
        return false;
    }

    shared::Logger::getInstance().info("PatchResolution: registered native resolution and frame limit callbacks.");
    return true;
}

} // namespace crabe::infrastructure::patch
