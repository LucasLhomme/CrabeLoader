/*
** CrabeLoader
** File description:
** Declares the Disney Infinity figure crypto: identity block decoding, data block decrypt and seal.
** Every block is one AES-128-ECB block under a per-tag key derived from the 7-byte UID.
** Data blocks carry the engine CRC-32 of bytes 0..11, big-endian in bytes 12..15.
**
** Authors: @LucasLhomme
*/

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace crabe::infrastructure::portal {

struct IdBlock {
    std::uint32_t sku{0};
    std::uint32_t mfgDate{0};
    std::uint16_t version{0};
    std::uint8_t flags{0};
    bool crcValid{false};
};

// Derives the per-tag key from the UID, decrypts the identity block (sector 0, block 1) and checks
// its magic. Returns std::nullopt when the magic does not match, i.e. the tag is not an Infinity
// figure or the UID and data disagree; a CRC mismatch only clears crcValid.
[[nodiscard]] std::optional<IdBlock> DecodeIdBlock(
    std::span<const std::uint8_t, 7> uid,
    std::span<const std::uint8_t, 16> encryptedBlock);

// Inverse of DecryptDataBlock: writes the CRC of bytes 0..11 into bytes 12..15 and encrypts the
// block with the per-tag key, ready for the base's write command. Returns std::nullopt when
// encryption fails.
[[nodiscard]] std::optional<std::array<std::uint8_t, 16>> SealDataBlock(
    std::span<const std::uint8_t, 7> uid,
    std::array<std::uint8_t, 16> plainBlock);

// Decrypts one data block of a figure (experience, skills, SWID) with the per-tag key and checks
// its CRC. Returns std::nullopt when the block was never written (all zero on the tag), cannot be
// decrypted, or fails its CRC.
[[nodiscard]] std::optional<std::array<std::uint8_t, 16>> DecryptDataBlock(
    std::span<const std::uint8_t, 7> uid,
    std::span<const std::uint8_t, 16> encryptedBlock);

} // namespace crabe::infrastructure::portal
