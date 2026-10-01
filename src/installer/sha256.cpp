/*
** CrabeLoader
** File description:
** Implements SHA-256 with the CNG primitives, one hash object per call.
** Every handle is released by its owner, including on the early returns.
** Compares nothing; callers match the hex against what they know.
**
** Authors: @LucasLhomme
*/

#include "installer/sha256.hpp"

#include <array>
#include <fstream>
#include <ios>
#include <vector>

#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace crabe::installer {

    namespace {

        constexpr std::size_t kDigestBytes = 32;
        constexpr std::size_t kReadChunkBytes = 64 * 1024;
        constexpr std::string_view kHexDigits = "0123456789abcdef";

        class Sha256Hasher final {
        public:
            Sha256Hasher()
            {
                if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&_algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
                    return;

                DWORD objectBytes = 0;
                DWORD written = 0;
                if (!BCRYPT_SUCCESS(BCryptGetProperty(_algorithm, BCRYPT_OBJECT_LENGTH,
                                                      reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes),
                                                      &written, 0)))
                    return;

                _object.resize(objectBytes);
                _ready = BCRYPT_SUCCESS(BCryptCreateHash(_algorithm, &_hash, _object.data(),
                                                         static_cast<ULONG>(_object.size()), nullptr, 0, 0));
            }

            ~Sha256Hasher()
            {
                if (_hash)
                    BCryptDestroyHash(_hash);
                if (_algorithm)
                    BCryptCloseAlgorithmProvider(_algorithm, 0);
            }

            Sha256Hasher(const Sha256Hasher&) = delete;
            Sha256Hasher& operator=(const Sha256Hasher&) = delete;
            Sha256Hasher(Sha256Hasher&&) = delete;
            Sha256Hasher& operator=(Sha256Hasher&&) = delete;

            [[nodiscard]] bool update(std::span<const std::byte> data) noexcept
            {
                if (!_ready)
                    return false;
                if (data.empty())
                    return true;
                return BCRYPT_SUCCESS(BCryptHashData(_hash, reinterpret_cast<PUCHAR>(const_cast<std::byte*>(data.data())),
                                                     static_cast<ULONG>(data.size()), 0));
            }

            [[nodiscard]] std::optional<std::string> finish()
            {
                if (!_ready)
                    return std::nullopt;

                std::array<UCHAR, kDigestBytes> digest{};
                if (!BCRYPT_SUCCESS(BCryptFinishHash(_hash, digest.data(), static_cast<ULONG>(digest.size()), 0)))
                    return std::nullopt;

                std::string hex;
                hex.reserve(kDigestBytes * 2);
                for (const UCHAR byte : digest) {
                    hex += kHexDigits[byte >> 4];
                    hex += kHexDigits[byte & 0x0F];
                }
                return hex;
            }

        private:
            BCRYPT_ALG_HANDLE _algorithm{nullptr};
            BCRYPT_HASH_HANDLE _hash{nullptr};
            std::vector<UCHAR> _object;
            bool _ready{false};
        };

    } // namespace

    std::optional<std::string> sha256Hex(std::span<const std::byte> data)
    {
        Sha256Hasher hasher;
        if (!hasher.update(data))
            return std::nullopt;
        return hasher.finish();
    }

    std::optional<std::string> sha256FileHex(const std::filesystem::path& file)
    {
        std::ifstream stream(file, std::ios::binary);
        if (!stream.is_open())
            return std::nullopt;

        Sha256Hasher hasher;
        std::vector<std::byte> chunk(kReadChunkBytes);
        while (stream) {
            stream.read(reinterpret_cast<char*>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
            const std::streamsize got = stream.gcount();
            if (got > 0 && !hasher.update(std::span<const std::byte>(chunk.data(), static_cast<std::size_t>(got))))
                return std::nullopt;
        }
        if (!stream.eof())
            return std::nullopt;
        return hasher.finish();
    }

} // namespace crabe::installer
