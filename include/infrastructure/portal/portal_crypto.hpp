#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace crabe::infrastructure::portal {

class PortalCrypto final {
public:
    PortalCrypto();
    ~PortalCrypto();

    PortalCrypto(const PortalCrypto&) = delete;
    PortalCrypto& operator=(const PortalCrypto&) = delete;
    PortalCrypto(PortalCrypto&&) noexcept;
    PortalCrypto& operator=(PortalCrypto&&) noexcept;

    // Derives AES key from UID and decrypts Block 1 to extract the Character SKU ID.
    // Returns std::nullopt if decryption fails or if SKU is 0.
    [[nodiscard]] std::optional<uint32_t> DecryptCharacterSku(
        std::span<const uint8_t> uid,
        std::span<const uint8_t, 16> encryptedBlock1
    ) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace crabe::infrastructure::portal
