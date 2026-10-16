/*
** CrabeLoader
** File description:
** Implements native key and controller binding options for SettingsMenu.
** Populates keyboard and gamepad binding rows directly below the Controls option.
** Manages input remapping state transitions and delegates to the controls interface.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/patch/patch_keybind.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <windows.h>
#include <xinput.h>

#include <nlohmann/json.hpp>

#include "infrastructure/lua_call.hpp"
#include "presentation/input_hook.hpp"
#include "presentation/input_remap.hpp"
#include "shared/logger.hpp"

namespace {

enum class DeviceType {
    Keyboard,
    Mouse,
    Pad
};

struct ActionDef {
    const char* id;
    DeviceType device;
    uint32_t defaultCode;
    const char* label;
};

constexpr std::array<ActionDef, 23> kActions = {{
    { "forward",      DeviceType::Keyboard, 0x11,   "Move forward" },
    { "back",         DeviceType::Keyboard, 0x1F,   "Move back" },
    { "left",         DeviceType::Keyboard, 0x1E,   "Move left" },
    { "right",        DeviceType::Keyboard, 0x20,   "Move right" },
    { "jump",         DeviceType::Keyboard, 0x39,   "Jump" },
    { "interact",     DeviceType::Keyboard, 0x12,   "Interact / pick up" },
    { "sprint",       DeviceType::Keyboard, 0x2A,   "Sprint / target" },
    { "block",        DeviceType::Keyboard, 0x1D,   "Block" },
    { "swap",         DeviceType::Keyboard, 0x10,   "Swap hero" },
    { "ribbon",       DeviceType::Keyboard, 0x0F,   "Toy Box menu" },
    { "tool1",        DeviceType::Keyboard, 0x02,   "Previous tool" },
    { "tool2",        DeviceType::Keyboard, 0x03,   "Next tool" },
    { "attack",       DeviceType::Mouse,    0,      "Attack" },
    { "throw",        DeviceType::Mouse,    1,      "Throw / shoot" },
    { "padJump",      DeviceType::Pad,      0x1000, "Jump" },
    { "padAttack",    DeviceType::Pad,      0x4000, "Attack" },
    { "padAlt",       DeviceType::Pad,      0x8000, "Alternate attack" },
    { "padBlock",     DeviceType::Pad,      0x2000, "Block" },
    { "padPause",     DeviceType::Pad,      0x0010, "Pause menu" },
    { "padRibbon",    DeviceType::Pad,      0x0020, "Toy Box menu" },
    { "padSwap",      DeviceType::Pad,      0x0040, "Swap hero" },
    { "padToolLeft",  DeviceType::Pad,      0x0004, "Previous tool" },
    { "padToolRight", DeviceType::Pad,      0x0008, "Next tool" },
}};

struct ActiveRemap {
    DeviceType device;
    uint32_t from;
    uint32_t to;
};

std::vector<ActiveRemap> g_remaps;

struct CaptureState {
    bool active = false;
    std::string actionId;
    DeviceType device = DeviceType::Keyboard;
    std::chrono::steady_clock::time_point startTime{};
    int rowIndex = -1;
    std::string originalDisplay;
};

CaptureState g_capture;

// Returns the file path where keybind settings are stored.
std::filesystem::path getKeybindsPath()
{
    std::filesystem::create_directories("Crabe");
    return "Crabe/keybinds.json";
}

// Writes current active remaps to disk.
void saveRemaps()
{
    const auto path = getKeybindsPath();
    nlohmann::json j;
    j["version"] = 1;
    auto remapsArray = nlohmann::json::array();

    for (const auto& r : g_remaps) {
        nlohmann::json item;
        item["d"] = (r.device == DeviceType::Keyboard) ? "keyboard" :
                    (r.device == DeviceType::Mouse) ? "mouse" : "pad";
        item["f"] = r.from;
        item["t"] = r.to;
        remapsArray.push_back(item);
    }
    j["remaps"] = remapsArray;

    std::ofstream file(path);
    if (file.is_open())
        file << j.dump(2);
}

// Loads saved keybind configurations from disk and applies them to InputRemap.
void loadRemaps()
{
    g_remaps.clear();
    const auto path = getKeybindsPath();
    if (!std::filesystem::exists(path))
        return;

    std::ifstream file(path);
    if (!file.is_open())
        return;

    try {
        nlohmann::json j;
        file >> j;
        if (!j.contains("remaps") || !j["remaps"].is_array())
            return;

        auto& remap = crabe::presentation::InputRemap::get();
        for (const auto& item : j["remaps"]) {
            std::string d = item.value("d", "");
            uint32_t f = item.value("f", 0u);
            uint32_t t = item.value("t", 0u);

            if (d == "keyboard" && f < 256 && t < 256) {
                g_remaps.push_back({ DeviceType::Keyboard, f, t });
                remap.setKey(f, static_cast<int>(t));
            } else if (d == "mouse" && f < 8 && t < 8) {
                g_remaps.push_back({ DeviceType::Mouse, f, t });
                remap.setMouseButton(f, static_cast<int>(t));
            } else if (d == "pad") {
                g_remaps.push_back({ DeviceType::Pad, f, t });
                remap.setPadButton(f, t);
            }
        }
        crabe::shared::Logger::getInstance().info(
            "PatchKeybind: loaded {} remap(s) from keybinds.json", g_remaps.size());
    } catch (const std::exception& e) {
        crabe::shared::Logger::getInstance().error(
            "PatchKeybind: failed to parse keybinds.json: {}", e.what());
    }
}

// Returns the physical input producing the expected target code, or 0xFFFFFFFF if unbound.
uint32_t getProducingInput(DeviceType device, uint32_t expectedCode)
{
    for (const auto& r : g_remaps) {
        if (r.device == device && r.to == expectedCode)
            return r.from;
    }
    for (const auto& r : g_remaps) {
        if (r.device == device && r.from == expectedCode)
            return 0xFFFFFFFF;
    }
    return expectedCode;
}

// Finds the expected code currently assigned to the given physical input.
uint32_t getTargetProduced(DeviceType device, uint32_t physicalInput)
{
    for (const auto& r : g_remaps) {
        if (r.device == device && r.from == physicalInput)
            return r.to;
    }
    return physicalInput;
}

// Maps input device and code to the vanilla Disney Infinity sprite glyph icon name.
std::string getGlyphIcon(DeviceType device, uint32_t code)
{
    if (device == DeviceType::Pad) {
        switch (code) {
            case 0x1000: return "cbi_buttona";
            case 0x2000: return "cbi_buttonb";
            case 0x4000: return "cbi_buttonx";
            case 0x8000: return "cbi_buttony";
            case 0x0100: return "cbi_buttonlb";
            case 0x0200: return "cbi_buttonrb";
            case 0x0010: return "cbi_buttonstart";
            case 0x0020: return "cbi_buttonback";
            case 0x0001: return "cbi_buttondp_u";
            case 0x0002: return "cbi_buttondp_d";
            case 0x0004: return "cbi_buttondp_l";
            case 0x0008: return "cbi_buttondp_r";
            case 0x0040: return "cbi_lstick";
            case 0x0080: return "cbi_rstick";
            default: return "";
        }
    }

    if (device == DeviceType::Mouse) {
        switch (code) {
            case 0: return "cbi_mouselmb";
            case 1: return "cbi_mousermb";
            case 2: return "cbi_mousemmb";
            default: return "";
        }
    }

    switch (code) {
        case 0x01: return "cbi_keyesc";
        case 0x0F: return "cbi_keytab";
        case 0x39: return "cbi_keyspacebar";
        case 0x1C:
        case 0x9C: return "cbi_keyenter";
        case 0x0E: return "cbi_keybackspace";
        case 0x3A: return "cbi_keycaps";
        case 0x2A:
        case 0x36: return "cbi_keyshift";
        case 0x1D:
        case 0x9D: return "cbi_keyctrl";
        case 0x38:
        case 0xB8: return "cbi_keyalt";
        case 0xC8: return "cbi_keyarrowup";
        case 0xD0: return "cbi_keyarrowdown";
        case 0xCB: return "cbi_keyarrowleft";
        case 0xCD: return "cbi_keyarrowright";
        default: break;
    }

    char nameBuf[16] = {};
    LONG lParam = static_cast<LONG>(code) << 16;
    if (GetKeyNameTextA(lParam, nameBuf, sizeof(nameBuf)) == 1) {
        char c = static_cast<char>(std::tolower(nameBuf[0]));
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            return std::format("cbi_key{}", c);
    }
    return "";
}

// Returns the display name for a physical input on the given device.
std::string getInputName(DeviceType device, uint32_t code)
{
    if (code == 0xFFFFFFFF)
        return "(unbound)";

    if (device == DeviceType::Mouse) {
        switch (code) {
            case 0: return "Left click";
            case 1: return "Right click";
            case 2: return "Middle click";
            case 3: return "Mouse 4";
            case 4: return "Mouse 5";
            default: return std::format("Mouse {}", code + 1);
        }
    }

    if (device == DeviceType::Pad) {
        switch (code) {
            case 0x0001: return "D-pad Up";
            case 0x0002: return "D-pad Down";
            case 0x0004: return "D-pad Left";
            case 0x0008: return "D-pad Right";
            case 0x0010: return "Start";
            case 0x0020: return "Back";
            case 0x0040: return "Left stick click";
            case 0x0080: return "Right stick click";
            case 0x0100: return "LB";
            case 0x0200: return "RB";
            case 0x1000: return "A";
            case 0x2000: return "B";
            case 0x4000: return "X";
            case 0x8000: return "Y";
            default: return std::format("Button 0x{:X}", code);
        }
    }

    char nameBuf[64] = {};
    LONG lParam = static_cast<LONG>(code) << 16;
    if (code >= 0x45)
        lParam |= (1 << 24);
    if (GetKeyNameTextA(lParam, nameBuf, sizeof(nameBuf)) > 0)
        return std::string(nameBuf);

    return std::format("Key 0x{:02X}", code);
}

// Formats icon markup for inline rendering in Flash menu rows.
std::string formatRowDisplay(DeviceType device, uint32_t code)
{
    if (code == 0xFFFFFFFF)
        return "(unbound)";

    const std::string icon = getGlyphIcon(device, code);
    if (!icon.empty()) {
        return std::format(
            "<img src=\"img://{}\" width=\"20\" height=\"20\" align=\"baseline\" vspace=\"-19\"/>",
            icon);
    }
    return getInputName(device, code);
}

// Filters actions for the requested page: keyboard or pad.
std::vector<const ActionDef*> getActionsForPage(std::string_view page)
{
    std::vector<const ActionDef*> out;
    const bool isKeyboard = (page == "keyboard");
    for (const auto& act : kActions) {
        if (isKeyboard && act.device != DeviceType::Pad)
            out.push_back(&act);
        else if (!isKeyboard && act.device == DeviceType::Pad)
            out.push_back(&act);
    }
    return out;
}

// Applies a new physical binding to an action, swapping conflicting inputs.
void applyBinding(const std::string& actionId, uint32_t newPhysicalInput)
{
    const ActionDef* action = nullptr;
    for (const auto& act : kActions) {
        if (act.id == actionId) {
            action = &act;
            break;
        }
    }
    if (!action)
        return;

    const DeviceType device = action->device;
    const uint32_t defaultCode = action->defaultCode;

    uint32_t oldTargetOnNewInput = getTargetProduced(device, newPhysicalInput);

    auto it = std::remove_if(g_remaps.begin(), g_remaps.end(), [&](const ActiveRemap& r) {
        return r.device == device && (r.to == defaultCode || r.from == newPhysicalInput);
    });
    g_remaps.erase(it, g_remaps.end());

    if (newPhysicalInput != defaultCode) {
        g_remaps.push_back({ device, newPhysicalInput, defaultCode });

        if (oldTargetOnNewInput != defaultCode && oldTargetOnNewInput != 0xFFFFFFFF) {
            auto it2 = std::remove_if(g_remaps.begin(), g_remaps.end(), [&](const ActiveRemap& r) {
                return r.device == device && r.to == oldTargetOnNewInput;
            });
            g_remaps.erase(it2, g_remaps.end());
            g_remaps.push_back({ device, defaultCode, oldTargetOnNewInput });
        }
    }

    auto& remap = crabe::presentation::InputRemap::get();
    if (device == DeviceType::Keyboard) {
        remap.clearKeys();
        for (const auto& r : g_remaps) {
            if (r.device == DeviceType::Keyboard)
                remap.setKey(r.from, static_cast<int>(r.to));
        }
    } else if (device == DeviceType::Mouse) {
        remap.clearMouseButtons();
        for (const auto& r : g_remaps) {
            if (r.device == DeviceType::Mouse)
                remap.setMouseButton(r.from, static_cast<int>(r.to));
        }
    } else if (device == DeviceType::Pad) {
        remap.clearPadButtons();
        for (const auto& r : g_remaps) {
            if (r.device == DeviceType::Pad)
                remap.setPadButton(r.from, r.to);
        }
    }

    saveRemaps();
}

// Resets bindings on a specific page to defaults.
void resetPage(const std::string& page)
{
    auto& remap = crabe::presentation::InputRemap::get();
    if (page == "keyboard") {
        auto it = std::remove_if(g_remaps.begin(), g_remaps.end(), [](const ActiveRemap& r) {
            return r.device == DeviceType::Keyboard || r.device == DeviceType::Mouse;
        });
        g_remaps.erase(it, g_remaps.end());
        remap.clearKeys();
        remap.clearMouseButtons();
    } else {
        auto it = std::remove_if(g_remaps.begin(), g_remaps.end(), [](const ActiveRemap& r) {
            return r.device == DeviceType::Pad;
        });
        g_remaps.erase(it, g_remaps.end());
        remap.clearPadButtons();
    }
    saveRemaps();
}

// Lua C callback returning total count of actions for the given page.
int __cdecl nativeKeybindGetActionCount(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const char* pageStr = lua.argToString(L, 1);
    const std::string page = pageStr ? pageStr : "keyboard";
    const auto pageActions = getActionsForPage(page);

    lua.pushNumber(L, static_cast<double>(pageActions.size()));
    return 1;
}

// Lua C callback returning action id, label, and formatted display string.
int __cdecl nativeKeybindGetAction(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const char* pageStr = lua.argToString(L, 1);
    const int index = static_cast<int>(lua.argToNumber(L, 2, 1.0));
    const std::string page = pageStr ? pageStr : "keyboard";
    const auto pageActions = getActionsForPage(page);

    if (index >= 1 && index <= static_cast<int>(pageActions.size())) {
        const auto* act = pageActions[static_cast<size_t>(index - 1)];
        uint32_t producing = getProducingInput(act->device, act->defaultCode);
        std::string display = formatRowDisplay(act->device, producing);

        lua.pushString(L, act->id);
        lua.pushString(L, act->label);
        lua.pushString(L, display);
        return 3;
    }

    return 0;
}

// Lua C callback initiating input capture for an action row.
int __cdecl nativeKeybindSelectAction(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport())
        return 0;

    const char* pageStr = lua.argToString(L, 1);
    const int index = static_cast<int>(lua.argToNumber(L, 2, 1.0));
    const std::string page = pageStr ? pageStr : "keyboard";
    const auto pageActions = getActionsForPage(page);

    if (index >= 1 && index <= static_cast<int>(pageActions.size())) {
        const auto* act = pageActions[static_cast<size_t>(index - 1)];
        uint32_t producing = getProducingInput(act->device, act->defaultCode);

        g_capture.active = true;
        g_capture.actionId = act->id;
        g_capture.device = act->device;
        g_capture.startTime = std::chrono::steady_clock::now();
        g_capture.rowIndex = index;
        g_capture.originalDisplay = formatRowDisplay(act->device, producing);

        std::string prompt = (act->device == DeviceType::Keyboard) ? "<press a key>" :
                             (act->device == DeviceType::Mouse) ? "<press mouse button>" :
                             "<press a button>";

        lua.pushString(L, prompt);
        return 1;
    }

    return 0;
}

// Lua C callback resetting bindings for a page.
int __cdecl nativeKeybindResetPage(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    const char* pageStr = lua.argToString(L, 1);
    const std::string page = pageStr ? pageStr : "keyboard";
    resetPage(page);
    return 0;
}

// Lua C callback cancelling an active input capture.
int __cdecl nativeKeybindCancelCapture(void* L)
{
    (void)L;
    g_capture.active = false;
    return 0;
}

// Lua C callback polling input capture progress on each game tick.
int __cdecl nativeKeybindPoll(void* L)
{
    auto& lua = crabe::infrastructure::LuaCall::get();
    if (!lua.hasReturnSupport() || !g_capture.active)
        return 0;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - g_capture.startTime).count();

    bool cancelled = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) || (GetAsyncKeyState(VK_BACK) & 0x8000);
    if (cancelled || elapsed > 8.0) {
        int row = g_capture.rowIndex;
        std::string orig = g_capture.originalDisplay;
        g_capture.active = false;

        lua.pushBoolean(L, false);
        lua.pushNumber(L, static_cast<double>(row));
        lua.pushString(L, orig);
        return 3;
    }

    if (elapsed < 0.15) {
        lua.pushNil(L);
        return 1;
    }

    uint32_t capturedCode = 0xFFFFFFFF;

    if (g_capture.device == DeviceType::Keyboard) {
        for (int vk = 8; vk <= 254; ++vk) {
            if (vk == VK_ESCAPE || vk == VK_BACK || vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU)
                continue;

            if (GetAsyncKeyState(vk) & 0x8000) {
                UINT scan = MapVirtualKeyA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
                if (scan > 0 && scan < 256) {
                    capturedCode = scan;
                    break;
                }
            }
        }
    } else if (g_capture.device == DeviceType::Mouse) {
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) capturedCode = 0;
        else if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) capturedCode = 1;
        else if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) capturedCode = 2;
        else if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) capturedCode = 3;
        else if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) capturedCode = 4;
    } else if (g_capture.device == DeviceType::Pad) {
        auto pad = crabe::presentation::InputHook::get().pad(0);
        if (pad.connected && pad.buttons != 0) {
            constexpr uint16_t kPadButtons[] = {
                0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
                0x0040, 0x0080, 0x0100, 0x0200, 0x1000, 0x2000, 0x4000, 0x8000
            };
            for (uint16_t mask : kPadButtons) {
                if (pad.buttons & mask) {
                    capturedCode = mask;
                    break;
                }
            }
        }
    }

    if (capturedCode != 0xFFFFFFFF) {
        applyBinding(g_capture.actionId, capturedCode);
        int row = g_capture.rowIndex;
        std::string newDisplay = formatRowDisplay(g_capture.device, capturedCode);
        g_capture.active = false;

        lua.pushBoolean(L, true);
        lua.pushNumber(L, static_cast<double>(row));
        lua.pushString(L, newDisplay);
        return 3;
    }

    lua.pushNil(L);
    return 1;
}

// Lua C callback clearing all input remappings.
int __cdecl nativeClearRemaps(void* L)
{
    (void)L;
    g_remaps.clear();
    crabe::presentation::InputRemap::get().clearAll();
    saveRemaps();
    return 0;
}

} // namespace

namespace crabe::infrastructure::patch {

// Applies native keybind options and hooks to SettingsMenu.
bool PatchKeybind::apply(void* L)
{
    if (!L)
        return false;

    loadRemaps();

    auto& lua = LuaCall::get();
    bool ok = true;

    ok = lua.registerNativeFunction(L, "Crabe", "_patchKeybindGetActionCount", &nativeKeybindGetActionCount) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchKeybindGetAction", &nativeKeybindGetAction) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchKeybindSelectAction", &nativeKeybindSelectAction) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchKeybindResetPage", &nativeKeybindResetPage) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchKeybindCancelCapture", &nativeKeybindCancelCapture) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchKeybindPoll", &nativeKeybindPoll) && ok;
    ok = lua.registerNativeFunction(L, "Crabe", "_patchClearRemaps", &nativeClearRemaps) && ok;

    if (!ok) {
        shared::Logger::getInstance().error("PatchKeybind: failed to bind native callbacks.");
        return false;
    }

    shared::Logger::getInstance().info("PatchKeybind: registered native keybind callbacks.");
    return true;
}

} // namespace crabe::infrastructure::patch
