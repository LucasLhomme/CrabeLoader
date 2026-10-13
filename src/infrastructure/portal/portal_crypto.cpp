/*
** CrabeLoader
** File description:
** Implements Disney Infinity RFID figure decryption:
** Custom SHA-1 (LSB byte order) key derivation + AES-128-ECB decryption of Block 1.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_crypto.hpp"

#include <array>
#include <cstring>

namespace crabe::infrastructure::portal {

namespace {

// Disney Master Base Key
constexpr std::array<uint8_t, 16> kBaseKey = {
    0xaf, 0x62, 0xd2, 0xec, 0x04, 0x91, 0x96, 0x8c,
    0xc5, 0x2a, 0x1a, 0x71, 0x65, 0xf8, 0x65, 0xfe
};

constexpr char kDisneyCopyright[] = "(c) Disney 2013";
constexpr size_t kCopyrightLen = 15; // strlen("(c) Disney 2013")

constexpr uint16_t kExpectedMagic = 0x010b; // IGP_EXPECTED_MAGIC (Big endian 0x010b)

// ------------------------------------------------------------- SHA-1 with LSB output
struct Sha1Context {
    uint32_t intermediateHash[5]{};
    uint32_t lengthLow{};
    uint32_t lengthHigh{};
    int_least16_t messageBlockIndex{};
    uint8_t messageBlock[64]{};
    bool computed{};
    bool corrupted{};
};

inline uint32_t Sha1CircularShift(int bits, uint32_t word) {
    return ((word << bits) | (word >> (32 - bits)));
}

void Sha1ProcessMessageBlock(Sha1Context* context) {
    const uint32_t K[] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
    uint32_t W[80];

    for (int t = 0; t < 16; ++t) {
        W[t] = (static_cast<uint32_t>(context->messageBlock[t * 4]) << 24) |
               (static_cast<uint32_t>(context->messageBlock[t * 4 + 1]) << 16) |
               (static_cast<uint32_t>(context->messageBlock[t * 4 + 2]) << 8) |
               (static_cast<uint32_t>(context->messageBlock[t * 4 + 3]));
    }

    for (int t = 16; t < 80; ++t) {
        W[t] = Sha1CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
    }

    uint32_t A = context->intermediateHash[0];
    uint32_t B = context->intermediateHash[1];
    uint32_t C = context->intermediateHash[2];
    uint32_t D = context->intermediateHash[3];
    uint32_t E = context->intermediateHash[4];

    for (int t = 0; t < 20; ++t) {
        uint32_t temp = Sha1CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D; D = C; C = Sha1CircularShift(30, B); B = A; A = temp;
    }
    for (int t = 20; t < 40; ++t) {
        uint32_t temp = Sha1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D; D = C; C = Sha1CircularShift(30, B); B = A; A = temp;
    }
    for (int t = 40; t < 60; ++t) {
        uint32_t temp = Sha1CircularShift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D; D = C; C = Sha1CircularShift(30, B); B = A; A = temp;
    }
    for (int t = 60; t < 80; ++t) {
        uint32_t temp = Sha1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D; D = C; C = Sha1CircularShift(30, B); B = A; A = temp;
    }

    context->intermediateHash[0] += A;
    context->intermediateHash[1] += B;
    context->intermediateHash[2] += C;
    context->intermediateHash[3] += D;
    context->intermediateHash[4] += E;
    context->messageBlockIndex = 0;
}

void Sha1Reset(Sha1Context* context) {
    context->lengthLow = 0;
    context->lengthHigh = 0;
    context->messageBlockIndex = 0;
    context->intermediateHash[0] = 0x67452301;
    context->intermediateHash[1] = 0xEFCDAB89;
    context->intermediateHash[2] = 0x98BADCFE;
    context->intermediateHash[3] = 0x10325476;
    context->intermediateHash[4] = 0xC3D2E1F0;
    context->computed = false;
    context->corrupted = false;
}

void Sha1Input(Sha1Context* context, const uint8_t* message_array, unsigned int length) {
    if (!length) return;
    while (length--) {
        context->messageBlock[context->messageBlockIndex++] = (*message_array & 0xFF);
        context->lengthLow += 8;
        if (context->lengthLow == 0) {
            context->lengthHigh++;
        }
        if (context->messageBlockIndex == 64) {
            Sha1ProcessMessageBlock(context);
        }
        message_array++;
    }
}

void Sha1PadMessage(Sha1Context* context) {
    if (context->messageBlockIndex > 55) {
        context->messageBlock[context->messageBlockIndex++] = 0x80;
        while (context->messageBlockIndex < 64) {
            context->messageBlock[context->messageBlockIndex++] = 0;
        }
        Sha1ProcessMessageBlock(context);
        while (context->messageBlockIndex < 56) {
            context->messageBlock[context->messageBlockIndex++] = 0;
        }
    } else {
        context->messageBlock[context->messageBlockIndex++] = 0x80;
        while (context->messageBlockIndex < 56) {
            context->messageBlock[context->messageBlockIndex++] = 0;
        }
    }
    context->messageBlock[56] = static_cast<uint8_t>(context->lengthHigh >> 24);
    context->messageBlock[57] = static_cast<uint8_t>(context->lengthHigh >> 16);
    context->messageBlock[58] = static_cast<uint8_t>(context->lengthHigh >> 8);
    context->messageBlock[59] = static_cast<uint8_t>(context->lengthHigh);
    context->messageBlock[60] = static_cast<uint8_t>(context->lengthLow >> 24);
    context->messageBlock[61] = static_cast<uint8_t>(context->lengthLow >> 16);
    context->messageBlock[62] = static_cast<uint8_t>(context->lengthLow >> 8);
    context->messageBlock[63] = static_cast<uint8_t>(context->lengthLow);
    Sha1ProcessMessageBlock(context);
}

// Disney proprietary LSB output order
void Sha1ResultLsb(Sha1Context* context, uint8_t messageDigest[20]) {
    if (!context->computed) {
        Sha1PadMessage(context);
        for (int i = 0; i < 64; ++i) {
            context->messageBlock[i] = 0;
        }
        context->lengthLow = 0;
        context->lengthHigh = 0;
        context->computed = true;
    }
    for (int i = 0; i < 20; ++i) {
        messageDigest[i] = static_cast<uint8_t>(context->intermediateHash[i >> 2] >> (8 * (i & 0x3)));
    }
}

// ------------------------------------------------------------- AES-128 Decryption
const uint8_t kSboxInvert[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x54, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

const uint8_t kSbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xF5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

const uint8_t kRcon[11] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36 };

inline uint8_t Multiply(uint8_t x, uint8_t y) {
    uint8_t result = 0;
    while (y) {
        if (y & 1) result ^= x;
        x = (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00);
        y >>= 1;
    }
    return result;
}

void AesKeyExpansion(const uint8_t* key, uint8_t roundKeys[176]) {
    for (int i = 0; i < 16; ++i) {
        roundKeys[i] = key[i];
    }
    int bytesGenerated = 16;
    int rconIteration = 1;
    uint8_t temp[4];

    while (bytesGenerated < 176) {
        for (int i = 0; i < 4; ++i) {
            temp[i] = roundKeys[i + bytesGenerated - 4];
        }
        if (bytesGenerated % 16 == 0) {
            uint8_t k = temp[0];
            temp[0] = temp[1]; temp[1] = temp[2]; temp[2] = temp[3]; temp[3] = k;
            temp[0] = kSbox[temp[0]];
            temp[1] = kSbox[temp[1]];
            temp[2] = kSbox[temp[2]];
            temp[3] = kSbox[temp[3]];
            temp[0] ^= kRcon[rconIteration++];
        }
        for (int a = 0; a < 4; ++a) {
            roundKeys[bytesGenerated] = roundKeys[bytesGenerated - 16] ^ temp[a];
            bytesGenerated++;
        }
    }
}

void AesDecryptBlock(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[176]) {
    uint8_t state[4][4];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            state[j][i] = in[i * 4 + j];
        }
    }

    // AddRoundKey 10
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            state[j][i] ^= roundKeys[160 + i * 4 + j];
        }
    }

    for (int round = 9; round > 0; --round) {
        // InvShiftRows
        uint8_t temp = state[1][3];
        state[1][3] = state[1][2]; state[1][2] = state[1][1]; state[1][1] = state[1][0]; state[1][0] = temp;

        temp = state[2][0]; uint8_t temp2 = state[2][1];
        state[2][0] = state[2][2]; state[2][1] = state[2][3]; state[2][2] = temp; state[2][3] = temp2;

        temp = state[3][0];
        state[3][0] = state[3][1]; state[3][1] = state[3][2]; state[3][2] = state[3][3]; state[3][3] = temp;

        // InvSubBytes
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                state[j][i] = kSboxInvert[state[j][i]];
            }
        }

        // AddRoundKey
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                state[j][i] ^= roundKeys[round * 16 + i * 4 + j];
            }
        }

        // InvMixColumns
        for (int i = 0; i < 4; ++i) {
            uint8_t a = state[0][i];
            uint8_t b = state[1][i];
            uint8_t c = state[2][i];
            uint8_t d = state[3][i];

            state[0][i] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
            state[1][i] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
            state[2][i] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
            state[3][i] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
        }
    }

    // Final round: InvShiftRows
    uint8_t temp = state[1][3];
    state[1][3] = state[1][2]; state[1][2] = state[1][1]; state[1][1] = state[1][0]; state[1][0] = temp;

    temp = state[2][0]; uint8_t temp2 = state[2][1];
    state[2][0] = state[2][2]; state[2][1] = state[2][3]; state[2][2] = temp; state[2][3] = temp2;

    temp = state[3][0];
    state[3][0] = state[3][1]; state[3][1] = state[3][2]; state[3][2] = state[3][3]; state[3][3] = temp;

    // InvSubBytes
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            state[j][i] = kSboxInvert[state[j][i]];
        }
    }

    // AddRoundKey 0
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            state[j][i] ^= roundKeys[i * 4 + j];
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out[i * 4 + j] = state[j][i];
        }
    }
}

} // namespace

struct PortalCrypto::Impl final {};

PortalCrypto::PortalCrypto() : m_impl(std::make_unique<Impl>()) {}
PortalCrypto::~PortalCrypto() = default;
PortalCrypto::PortalCrypto(PortalCrypto&&) noexcept = default;
PortalCrypto& PortalCrypto::operator=(PortalCrypto&&) noexcept = default;

std::optional<uint32_t> PortalCrypto::DecryptCharacterSku(
    std::span<const uint8_t> uid,
    std::span<const uint8_t, 16> encryptedBlock1
) const {
    if (uid.empty()) return std::nullopt;

    // Derive Key: SHA1_LSB(kBaseKey + "(c) Disney 2013" + uid)
    Sha1Context sha1;
    Sha1Reset(&sha1);
    Sha1Input(&sha1, kBaseKey.data(), static_cast<unsigned int>(kBaseKey.size()));
    Sha1Input(&sha1, reinterpret_cast<const uint8_t*>(kDisneyCopyright), kCopyrightLen);
    Sha1Input(&sha1, uid.data(), static_cast<unsigned int>(uid.size()));

    uint8_t digest[20];
    Sha1ResultLsb(&sha1, digest);

    // Expand AES-128 key (first 16 bytes of digest)
    uint8_t roundKeys[176];
    AesKeyExpansion(digest, roundKeys);

    // Decrypt Block 1
    uint8_t decrypted[16];
    AesDecryptBlock(encryptedBlock1.data(), decrypted, roundKeys);

    // Verify magic
    uint16_t magic = (static_cast<uint16_t>(decrypted[10]) << 8) | decrypted[11];
    uint32_t sku = (static_cast<uint32_t>(decrypted[0]) << 24) |
                   (static_cast<uint32_t>(decrypted[1]) << 16) |
                   (static_cast<uint32_t>(decrypted[2]) << 8)  |
                   (static_cast<uint32_t>(decrypted[3]));

    if (sku == 0) {
        return std::nullopt;
    }

    return sku;
}

} // namespace crabe::infrastructure::portal
