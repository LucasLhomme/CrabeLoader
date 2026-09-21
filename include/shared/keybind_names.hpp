/*
** CrabeLoader
** File description:
** Declares the parse from a config key name such as F4 or Insert to a virtual-key code.
** Lives in shared because it needs the VK_* constants domain/config.hpp must stay free of.
** Chooses no fallback for an unknown name; application/loader.hpp decides that and warns.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_SHARED_KEYBIND_NAMES_HPP_
#define CRABELOADER_SHARED_KEYBIND_NAMES_HPP_

#include <optional>
#include <string_view>

namespace crabe::shared {

    // Parses a config-file key name ("F4", "Insert", "A", "5", ...) to a
    // Win32 virtual-key code. Case-insensitive.
    //
    // Lives here rather than in domain::Config because it needs
    // <windows.h>'s VK_* constants, and Config (like ModManifest) stays free
    // of Win32 headers; Config only ever holds the raw string from
    // [keybinds]. application::Loader is the caller: it resolves that string
    // with this function and falls back to whichever hardcoded VK_F4 /
    // VK_INSERT default was already in effect on nullopt, which is what
    // makes an unrecognised name degrade gracefully instead of leaving a
    // keybind unbound.
    [[nodiscard]] std::optional<int> parseVirtualKeyName(std::string_view name);

} // namespace crabe::shared

#endif /* !CRABELOADER_SHARED_KEYBIND_NAMES_HPP_ */
