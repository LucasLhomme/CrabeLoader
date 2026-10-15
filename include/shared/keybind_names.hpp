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

    // Parses a case-insensitive key name (e.g. "F4", "Insert", "A") to a Win32 virtual-key code.
    // Returns std::nullopt if the key name is unrecognized.
    [[nodiscard]] std::optional<int> parseVirtualKeyName(std::string_view name);

} // namespace crabe::shared

#endif /* !CRABELOADER_SHARED_KEYBIND_NAMES_HPP_ */
