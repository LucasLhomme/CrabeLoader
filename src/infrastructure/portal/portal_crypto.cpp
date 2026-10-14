/*
** CrabeLoader
** File description:
** Disney Infinity RFID identity block decoding.
** The per-tag AES-128 key is SHA-1(baseKey || "(c) Disney 2013" || uid) taken in the
** engine's "LSB" word order, and the identity block is one AES-128-ECB block.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_crypto.hpp"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cstring>

#pragma comment(lib, "bcrypt.lib")

namespace crabe::infrastructure::portal {

namespace {

constexpr std::array<std::uint8_t, 16> kBaseKey = {
    0xaf, 0x62, 0xd2, 0xec, 0x04, 0x91, 0x96, 0x8c,
    0xc5, 0x2a, 0x1a, 0x71, 0x65, 0xf8, 0x65, 0xfe
};

constexpr char kDisneyCopyright[] = "(c) Disney 2013";
constexpr std::size_t kCopyrightLen = sizeof(kDisneyCopyright) - 1;

// Identity block layout (decrypted).
constexpr std::size_t kSkuOffset = 0;
constexpr std::size_t kMfgOffset = 4;
constexpr std::size_t kFlagsOffset = 7;
constexpr std::size_t kVersionOffset = 8;
constexpr std::size_t kMagicOffset = 10;
constexpr std::size_t kCrcOffset = 12;
constexpr std::uint16_t kExpectedMagic = 0xd11f;

class AlgHandle final {
public:
    AlgHandle(LPCWSTR id, ULONG flags) {
        if (BCryptOpenAlgorithmProvider(&m_handle, id, nullptr, flags) != 0) {
            m_handle = nullptr;
        }
    }
    ~AlgHandle() {
        if (m_handle) BCryptCloseAlgorithmProvider(m_handle, 0);
    }
    AlgHandle(const AlgHandle&) = delete;
    AlgHandle& operator=(const AlgHandle&) = delete;
    [[nodiscard]] BCRYPT_ALG_HANDLE get() const { return m_handle; }

private:
    BCRYPT_ALG_HANDLE m_handle{nullptr};
};

class KeyHandle final {
public:
    KeyHandle() = default;
    ~KeyHandle() {
        if (m_handle) BCryptDestroyKey(m_handle);
    }
    KeyHandle(const KeyHandle&) = delete;
    KeyHandle& operator=(const KeyHandle&) = delete;
    [[nodiscard]] BCRYPT_KEY_HANDLE* out() { return &m_handle; }
    [[nodiscard]] BCRYPT_KEY_HANDLE get() const { return m_handle; }

private:
    BCRYPT_KEY_HANDLE m_handle{nullptr};
};

// The engine's SHA1Result_LSB writes every 32-bit state word least-significant byte first,
// which is the standard digest with the bytes of each 4-byte group reversed.
bool DeriveKey(std::span<const std::uint8_t, 7> uid, std::array<std::uint8_t, 16>& key) {
    AlgHandle sha(BCRYPT_SHA1_ALGORITHM, 0);
    if (!sha.get()) return false;

    std::array<std::uint8_t, 16 + kCopyrightLen + 7> material{};
    std::memcpy(material.data(), kBaseKey.data(), kBaseKey.size());
    std::memcpy(material.data() + kBaseKey.size(), kDisneyCopyright, kCopyrightLen);
    std::memcpy(material.data() + kBaseKey.size() + kCopyrightLen, uid.data(), uid.size());

    std::array<std::uint8_t, 20> digest{};
    if (BCryptHash(sha.get(), nullptr, 0, material.data(), static_cast<ULONG>(material.size()),
                   digest.data(), static_cast<ULONG>(digest.size())) != 0) {
        return false;
    }
    for (std::size_t word = 0; word < 4; ++word) {
        for (std::size_t b = 0; b < 4; ++b) {
            key[word * 4 + b] = digest[word * 4 + (3 - b)];
        }
    }
    return true;
}

bool AesEcbDecrypt(const std::array<std::uint8_t, 16>& key,
                   std::span<const std::uint8_t, 16> in,
                   std::array<std::uint8_t, 16>& out) {
    AlgHandle aes(BCRYPT_AES_ALGORITHM, 0);
    if (!aes.get()) return false;

    const auto* mode = reinterpret_cast<const UCHAR*>(BCRYPT_CHAIN_MODE_ECB);
    if (BCryptSetProperty(aes.get(), BCRYPT_CHAINING_MODE, const_cast<PUCHAR>(mode),
                          static_cast<ULONG>((wcslen(BCRYPT_CHAIN_MODE_ECB) + 1) * sizeof(wchar_t)), 0) != 0) {
        return false;
    }

    KeyHandle handle;
    if (BCryptGenerateSymmetricKey(aes.get(), handle.out(), nullptr, 0,
                                   const_cast<PUCHAR>(key.data()), static_cast<ULONG>(key.size()), 0) != 0) {
        return false;
    }

    ULONG written = 0;
    if (BCryptDecrypt(handle.get(), const_cast<PUCHAR>(in.data()), static_cast<ULONG>(in.size()),
                      nullptr, nullptr, 0, out.data(), static_cast<ULONG>(out.size()), &written, 0) != 0) {
        return false;
    }
    return written == out.size();
}

bool AesEcbEncrypt(const std::array<std::uint8_t, 16>& key,
                   const std::array<std::uint8_t, 16>& in,
                   std::array<std::uint8_t, 16>& out) {
    AlgHandle aes(BCRYPT_AES_ALGORITHM, 0);
    if (!aes.get()) return false;

    const auto* mode = reinterpret_cast<const UCHAR*>(BCRYPT_CHAIN_MODE_ECB);
    if (BCryptSetProperty(aes.get(), BCRYPT_CHAINING_MODE, const_cast<PUCHAR>(mode),
                          static_cast<ULONG>((wcslen(BCRYPT_CHAIN_MODE_ECB) + 1) * sizeof(wchar_t)), 0) != 0) {
        return false;
    }

    KeyHandle handle;
    if (BCryptGenerateSymmetricKey(aes.get(), handle.out(), nullptr, 0,
                                   const_cast<PUCHAR>(key.data()), static_cast<ULONG>(key.size()), 0) != 0) {
        return false;
    }

    ULONG written = 0;
    if (BCryptEncrypt(handle.get(), const_cast<PUCHAR>(in.data()), static_cast<ULONG>(in.size()),
                      nullptr, nullptr, 0, out.data(), static_cast<ULONG>(out.size()), &written, 0) != 0) {
        return false;
    }
    return written == out.size();
}

// The engine's crc32_calc: reflected CRC-32 (poly 0xEDB88320), initial value 0, no final xor.
std::uint32_t Crc32(std::span<const std::uint8_t> data) {
    std::uint32_t crc = 0;
    for (const std::uint8_t byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) ? (crc >> 1) ^ 0xEDB88320U : (crc >> 1);
        }
    }
    return crc;
}

std::uint32_t ReadBe32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) | (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]);
}

} // namespace

std::optional<IdBlock> DecodeIdBlock(std::span<const std::uint8_t, 7> uid,
                                     std::span<const std::uint8_t, 16> encryptedBlock) {
    std::array<std::uint8_t, 16> key{};
    if (!DeriveKey(uid, key)) return std::nullopt;

    std::array<std::uint8_t, 16> plain{};
    if (!AesEcbDecrypt(key, encryptedBlock, plain)) return std::nullopt;

    const std::uint16_t magic = static_cast<std::uint16_t>((plain[kMagicOffset] << 8) | plain[kMagicOffset + 1]);
    if (magic != kExpectedMagic) return std::nullopt;

    IdBlock id;
    id.sku = ReadBe32(&plain[kSkuOffset]);
    id.mfgDate = ReadBe32(&plain[kMfgOffset]);
    id.flags = plain[kFlagsOffset];
    id.version = static_cast<std::uint16_t>((plain[kVersionOffset] << 8) | plain[kVersionOffset + 1]);
    id.crcValid = Crc32(std::span<const std::uint8_t>(plain.data(), kCrcOffset)) == ReadBe32(&plain[kCrcOffset]);
    return id;
}

std::optional<std::array<std::uint8_t, 16>> DecryptDataBlock(std::span<const std::uint8_t, 7> uid,
                                                             std::span<const std::uint8_t, 16> encryptedBlock) {
    // The engine treats an all-zero block on the tag as never written.
    if (std::all_of(encryptedBlock.begin(), encryptedBlock.end(), [](std::uint8_t b) { return b == 0; })) {
        return std::nullopt;
    }
    std::array<std::uint8_t, 16> key{};
    std::array<std::uint8_t, 16> plain{};
    if (!DeriveKey(uid, key) || !AesEcbDecrypt(key, encryptedBlock, plain)) return std::nullopt;
    if (Crc32(std::span<const std::uint8_t>(plain.data(), kCrcOffset)) != ReadBe32(&plain[kCrcOffset])) {
        return std::nullopt;
    }
    return plain;
}

std::optional<std::array<std::uint8_t, 16>> SealDataBlock(std::span<const std::uint8_t, 7> uid,
                                                          std::array<std::uint8_t, 16> plainBlock) {
    const std::uint32_t crc = Crc32(std::span<const std::uint8_t>(plainBlock.data(), kCrcOffset));
    for (std::size_t i = 0; i < 4; ++i) {
        plainBlock[kCrcOffset + i] = static_cast<std::uint8_t>(crc >> (24 - 8 * i));
    }
    std::array<std::uint8_t, 16> key{};
    std::array<std::uint8_t, 16> encrypted{};
    if (!DeriveKey(uid, key) || !AesEcbEncrypt(key, plainBlock, encrypted)) return std::nullopt;
    return encrypted;
}

} // namespace crabe::infrastructure::portal
