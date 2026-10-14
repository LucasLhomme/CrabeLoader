#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace crabe::infrastructure::portal {

// Decoded identity block (sector 0, block 1) of an Infinity figure.
struct IdBlock {
    std::uint32_t sku{0};
    std::uint32_t mfgDate{0};
    std::uint16_t version{0};
    std::uint8_t flags{0};
    bool crcValid{false};
};

// Derives the per-tag AES key (SHA-1 over base key + copyright + UID, written
// least-significant byte first), decrypts the identity block and checks its
// magic. Returns std::nullopt when the magic does not match, i.e. the tag is
// not an Infinity figure or the UID/data pair is inconsistent.
[[nodiscard]] std::optional<IdBlock> DecodeIdBlock(
    std::span<const std::uint8_t, 7> uid,
    std::span<const std::uint8_t, 16> encryptedBlock);

// Decrypts one data block of a figure (experience, progression, SWID) with the same per-tag key
// and checks its CRC (engine CRC-32 of bytes 0..11, big-endian at 12..15). std::nullopt when the
// block was never written (all zero on the tag), cannot be decrypted, or fails its CRC.
// Inverse of DecryptDataBlock: writes the CRC of bytes 0..11 into 12..15 and encrypts the block
// with the per-tag key, ready for the base's write command.
[[nodiscard]] std::optional<std::array<std::uint8_t, 16>> SealDataBlock(
    std::span<const std::uint8_t, 7> uid,
    std::array<std::uint8_t, 16> plainBlock);

[[nodiscard]] std::optional<std::array<std::uint8_t, 16>> DecryptDataBlock(
    std::span<const std::uint8_t, 7> uid,
    std::span<const std::uint8_t, 16> encryptedBlock);

} // namespace crabe::infrastructure::portal
