/*
** CrabeLoader
** File description:
** Implements native resolution presets and frame rate limiting for SettingsVideo.
** Sorts and deduplicates resolutions natively before populating the video settings menu.
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
ResolutionComputation computeResolutionList(std::uint32_t currentWidth, std::uint32_t currentHeight)
{
    std::vector<ResolutionPreset> list;
    list.reserve(kResolutionPresets.size() + 1);

    for (const auto& preset : kResolutionPresets) {
        list.push_back(preset);
    }
    if (currentWidth > 0 && currentHeight > 0) {
        list.push_back({ currentWidth, currentHeight });
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
        if (list[i].width == currentWidth && list[i].height == currentHeight) {
            activeIndex = static_cast<std::uint32_t>(i + 1);
            break;
        }
    }

    return { std::move(list), activeIndex };
}

// Lua C callback injecting calculated resolution modes into SettingsVideo.
int __cdecl nativeInjectResolutions(void* L)
{
    UINT curW = 1920;
    UINT curH = 1080;
    crabe::presentation::RenderHook::get().getRenderResolution(curW, curH);

    const auto result = computeResolutionList(curW, curH);

    std::string widths;
    std::string heights;
    std::string texts;
    for (std::size_t i = 0; i < result.modes.size(); ++i) {
        if (i > 0) {
            widths += ',';
            heights += ',';
            texts += ',';
        }
        widths += std::to_string(result.modes[i].width);
        heights += std::to_string(result.modes[i].height);
        texts += std::format("\"{}x{}\"", result.modes[i].width, result.modes[i].height);
    }

    const std::string script = std::format(
        "local s = SettingsVideo\n"
        "if s then\n"
        "    s.resolutionWidths = {{{}}}\n"
        "    s.resolutionHeights = {{{}}}\n"
        "    s.resolutionText = {{{}}}\n"
        "    s.resolutionIndex = {}\n"
        "    s.oldResolutionIndex = {}\n"
        "end\n",
        widths, heights, texts, result.activeIndex, result.activeIndex);

    crabe::infrastructure::LuaCall::get().runSnippet(L, script);
    crabe::shared::Logger::getInstance().info(
        "PatchResolution: populated {} resolutions in SettingsVideo (active: {}x{})",
        result.modes.size(), curW, curH);
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

    ok = lua.registerNativeFunction(L, "Crabe", "_patchInjectResolutions", &nativeInjectResolutions) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchGetFrameLimit", &nativeGetFrameLimit) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchSetFrameLimit", &nativeSetFrameLimit) && ok;

    if (!ok) {
        shared::Logger::getInstance().error("PatchResolution: failed to register native callbacks.");
        return false;
    }

    std::string choicesStr;
    for (std::size_t i = 0; i < kFrameRateLimits.size(); ++i) {
        if (i > 0)
            choicesStr += ", ";
        if (kFrameRateLimits[i] == 0) {
            choicesStr += "\"Unlimited\"";
        } else {
            choicesStr += std::format("\"{} FPS\"", kFrameRateLimits[i]);
        }
    }

    const std::string registerScript = std::format(
        "if Crabe and Crabe.Settings then\n"
        "    if Crabe.Settings.onBuild and not _G.__crabeResolutionOnBuildRegistered then\n"
        "        _G.__crabeResolutionOnBuildRegistered = true\n"
        "        Crabe.Settings.onBuild(\"SettingsVideo\", Crabe._patchInjectResolutions)\n"
        "    end\n"
        "    if Crabe.Settings.addOption then\n"
        "        Crabe.Settings.addOption(\"SettingsVideo\", {{\n"
        "            id = \"crabeFrameLimit\",\n"
        "            text = \"Frame Rate Limit\",\n"
        "            widgetType = \"LR_Toggle\",\n"
        "            choices = {{ {} }},\n"
        "            get = Crabe._patchGetFrameLimit,\n"
        "            set = Crabe._patchSetFrameLimit,\n"
        "        }})\n"
        "    end\n"
        "end\n",
        choicesStr);

    ok = lua.runSnippet(L, registerScript);
    if (ok) {
        shared::Logger::getInstance().info("PatchResolution: applied native resolution presets and frame limit option.");
    } else {
        shared::Logger::getInstance().error("PatchResolution: failed to apply native resolution settings.");
    }
    return ok;
}

} // namespace crabe::infrastructure::patch
