/*
** CrabeLoader
** File description:
** Maps a config key name to a Win32 virtual-key code, case-insensitively.
** Covers the keys an overlay toggle is realistically bound to, not a full keyboard layout.
** Chooses no fallback for an unknown name; src/application/loader.cpp does that and warns.
**
** Authors: @LucasLhomme
*/

#include "shared/keybind_names.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <windows.h>

namespace crabe::shared {

    namespace {

        [[nodiscard]] std::string toUpperAscii(std::string_view text)
        {
            std::string out(text);
            std::ranges::transform(out, out.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
            return out;
        }

        struct NamedKey {
            std::string_view name;
            int code;
        };

        // Every name is spelled the way a person would type it in crabe.toml;
        // matching is case-insensitive (parseVirtualKeyName upper-cases
        // before comparing). Covers the keys a mod menu / overlay toggle is
        // realistically bound to -- not a full keyboard layout.
        constexpr std::array<NamedKey, 35> kNamedKeys = {{
            {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4},
            {"F5", VK_F5}, {"F6", VK_F6}, {"F7", VK_F7}, {"F8", VK_F8},
            {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},
            {"INSERT", VK_INSERT}, {"DELETE", VK_DELETE}, {"HOME", VK_HOME},
            {"END", VK_END}, {"PAGEUP", VK_PRIOR}, {"PAGEDOWN", VK_NEXT},
            {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT},
            {"SPACE", VK_SPACE}, {"TAB", VK_TAB}, {"ESCAPE", VK_ESCAPE}, {"ESC", VK_ESCAPE},
            {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN}, {"BACKSPACE", VK_BACK},
            {"CAPSLOCK", VK_CAPITAL}, {"NUMLOCK", VK_NUMLOCK}, {"SCROLLLOCK", VK_SCROLL},
            {"PAUSE", VK_PAUSE},
            {"SHIFT", VK_SHIFT}, {"CTRL", VK_CONTROL},
        }};

    } // namespace

    std::optional<int> parseVirtualKeyName(std::string_view name)
    {
        const std::string upper = toUpperAscii(name);

        // A single letter or digit: 'A'-'Z' and '0'-'9' share their ASCII
        // value with the matching VK_* constant -- a documented Win32
        // guarantee (VK_0..VK_9 == '0'..'9', VK_A..VK_Z == 'A'..'Z'), not a
        // coincidence being relied on by accident.
        if (upper.size() == 1) {
            const unsigned char c = static_cast<unsigned char>(upper[0]);
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                return static_cast<int>(c);
        }

        for (const NamedKey& candidate : kNamedKeys) {
            if (candidate.name == upper)
                return candidate.code;
        }

        return std::nullopt;
    }

} // namespace crabe::shared
