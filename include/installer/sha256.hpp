/*
** CrabeLoader
** File description:
** Declares SHA-256 over bytes and over a file, as lowercase hex, through Windows CNG.
** Used to recognise the real bink2w32.dll and to check that a copied file is the one written.
** Adds no dependency: bcrypt.dll ships with every supported Windows.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INSTALLER_SHA256_HPP_
#define CRABELOADER_INSTALLER_SHA256_HPP_

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

namespace crabe::installer {

    [[nodiscard]] std::optional<std::string> sha256Hex(std::span<const std::byte> data);

    // Streams the file in chunks; nullopt when it cannot be opened or read to the end.
    [[nodiscard]] std::optional<std::string> sha256FileHex(const std::filesystem::path& file);

} // namespace crabe::installer

#endif /* !CRABELOADER_INSTALLER_SHA256_HPP_ */
