/*
** CrabeLoader
** File description:
** gateway -- figure registry slots, built in memory
*/

#include "loader/gateway.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

namespace {

// Fixed key and IV, recovered from the shipped files and verified by
// re-encrypting a whole gateway byte-for-byte (374 slot keys, 4651 strings,
// 1383 numbers, zero mismatch). The fixed IV is what makes the cipher
// deterministic, so identical plaintexts produce identical ciphertexts.
constexpr std::array<uint8_t, 16> kKey = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
constexpr char kIv[17] = "0123456789012345";

// Plaintext type tags used by the game.
constexpr uint8_t kTagNumber = 1;
constexpr uint8_t kTagString = 2;

// _G key the slot table is stored under, and the byte sequence that identifies
// the gateway chunk. Identical across gateway.lua and its _in3/_mba/_moa/_dor
// siblings.
constexpr std::array<uint8_t, 16> kContainerKey = {
    0x37, 0x53, 0xf1, 0xa7, 0xa1, 0x82, 0xe5, 0xe0,
    0xd9, 0x4a, 0x11, 0x5d, 0xdc, 0x34, 0x3f, 0xac
};

// sku_id window. The game's own rows sit in 1000001-1000029, 1000100-1000129
// and 1000302-1000338, and the Breeze mod takes 1000300/1000301, so this range
// cannot collide with either.
constexpr uint32_t kSkuBase = 1000340;
constexpr uint32_t kSkuSpan = 660;

/* ---------------------------------------------------------------- AES-128 */

const uint8_t kSbox[256] = {
    0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76,
    0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0,
    0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15,
    0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75,
    0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84,
    0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF,
    0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8,
    0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2,
    0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73,
    0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB,
    0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79,
    0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08,
    0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A,
    0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E,
    0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF,
    0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16
};
const uint8_t kRcon[10] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36};

// Doubling in GF(2^8): a shift, then the field's reduction polynomial when
// the high bit carried out.
uint8_t xtime(uint8_t a)
{
    return static_cast<uint8_t>((a << 1) ^ ((a & 0x80) ? 0x1B : 0x00));
}

// Multiplication in GF(2^8), by doubling and adding. Only used for the small
// MixColumns constants, so the loop is short.
uint8_t mul(uint8_t a, uint8_t b)
{
    uint8_t r = 0;
    while (b) {
        if (b & 1)
            r ^= a;
        a = xtime(a);
        b >>= 1;
    }
    return r;
}

using RoundKeys = std::array<std::array<uint8_t, 16>, 11>;

// FIPS-197 key schedule: 44 words derived from the 128-bit key, regrouped as
// eleven round keys.
RoundKeys expandKey()
{
    uint8_t w[44][4];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            w[i][j] = kKey[i * 4 + j];

    for (int i = 4; i < 44; ++i) {
        uint8_t t[4] = { w[i - 1][0], w[i - 1][1], w[i - 1][2], w[i - 1][3] };
        if (i % 4 == 0) {
            uint8_t tmp = t[0];
            t[0] = kSbox[t[1]];
            t[1] = kSbox[t[2]];
            t[2] = kSbox[t[3]];
            t[3] = kSbox[tmp];
            t[0] ^= kRcon[i / 4 - 1];
        }
        for (int j = 0; j < 4; ++j)
            w[i][j] = w[i - 4][j] ^ t[j];
    }

    RoundKeys rk{};
    for (int r = 0; r < 11; ++r)
        for (int c = 0; c < 4; ++c)
            for (int j = 0; j < 4; ++j)
                rk[r][c * 4 + j] = w[r * 4 + c][j];
    return rk;
}

// The schedule, expanded once. The key never varies, so neither does this.
const RoundKeys& roundKeys()
{
    static const RoundKeys rk = expandKey();
    return rk;
}

// One AES-128 block, encrypted in place. Encryption only -- nothing here ever
// needs to read a slot back.
void encryptBlock(uint8_t s[16])
{
    const RoundKeys& rk = roundKeys();

    for (int i = 0; i < 16; ++i)
        s[i] ^= rk[0][i];

    for (int round = 1; round <= 10; ++round) {
        for (int i = 0; i < 16; ++i)
            s[i] = kSbox[s[i]];

        // ShiftRows on the column-major state of FIPS-197.
        uint8_t t[16];
        for (int i = 0; i < 16; ++i)
            t[i] = s[(i + (i % 4) * 4) % 16];
        std::memcpy(s, t, 16);

        if (round != 10) {
            for (int c = 0; c < 4; ++c) {
                uint8_t* col = s + c * 4;
                uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                col[0] = static_cast<uint8_t>(mul(a0, 2) ^ mul(a1, 3) ^ a2 ^ a3);
                col[1] = static_cast<uint8_t>(a0 ^ mul(a1, 2) ^ mul(a2, 3) ^ a3);
                col[2] = static_cast<uint8_t>(a0 ^ a1 ^ mul(a2, 2) ^ mul(a3, 3));
                col[3] = static_cast<uint8_t>(mul(a0, 3) ^ a1 ^ a2 ^ mul(a3, 2));
            }
        }

        for (int i = 0; i < 16; ++i)
            s[i] ^= rk[round][i];
    }
}

// PKCS7-pad, then CBC-chain from the fixed IV. That fixed IV is what makes the
// output reproducible: the same plaintext always encrypts to the same bytes.
std::string aesCbcPkcs7(const std::string& plain)
{
    size_t pad = 16 - (plain.size() % 16);
    std::string data = plain;
    data.append(pad, static_cast<char>(pad));

    std::string out;
    out.reserve(data.size());

    uint8_t prev[16];
    std::memcpy(prev, kIv, 16);

    for (size_t off = 0; off < data.size(); off += 16) {
        uint8_t block[16];
        for (int i = 0; i < 16; ++i)
            block[i] = static_cast<uint8_t>(data[off + i]) ^ prev[i];
        encryptBlock(block);
        std::memcpy(prev, block, 16);
        out.append(reinterpret_cast<char*>(block), 16);
    }
    return out;
}

// Encrypt one value behind its type tag, which is the form the game stores.
std::string tagged(uint8_t tag, const void* payload, size_t len)
{
    std::string plain;
    plain.push_back(static_cast<char>(tag));
    plain.append(static_cast<const char*>(payload), len);
    return aesCbcPkcs7(plain);
}

}

namespace Gateway {

std::string encryptString(const std::string& value)
{
    return tagged(kTagString, value.data(), value.size());
}

std::string encryptNumber(double value)
{
    // Numbers are stored as little-endian IEEE-754 doubles. Every target this
    // builds for is little-endian x86, so the in-memory layout is already right.
    return tagged(kTagNumber, &value, sizeof(value));
}

std::string luaLiteral(const std::string& bytes)
{
    // Printable ASCII passes through so the generated chunk stays readable in a
    // log; everything else becomes a padded decimal escape.
    std::string out;
    out.reserve(bytes.size() * 4 + 2);
    out.push_back('"');
    for (unsigned char c : bytes) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c >= 0x20 && c <= 0x7E) {
            out.push_back(static_cast<char>(c));
        } else {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\%03u", static_cast<unsigned>(c));
            out += buf;
        }
    }
    out.push_back('"');
    return out;
}

std::string allocateSku(const std::string& name)
{
    // Multiply-and-add hash, kept free of bitwise operators so the Lua side can
    // mirror it exactly (Lua 5.1 has no XOR). Values stay well inside a double's
    // exact-integer range, so both languages agree bit for bit.
    uint64_t h = 0;
    for (unsigned char c : name)
        h = (h * 31 + c) % 2147483648ULL;

    uint32_t sku = kSkuBase + static_cast<uint32_t>(h % kSkuSpan);
    return std::to_string(sku);
}

const std::string& containerKey()
{
    static const std::string key(reinterpret_cast<const char*>(kContainerKey.data()),
                                 kContainerKey.size());
    return key;
}

std::vector<Entry> parseExposedCharacters(const std::string& luaSource)
{
    // A scan, not a Lua parse: this runs before any Lua state exists, and only
    // has to recognise the flat table the API documents. A nested table inside
    // the call would end the match early -- no shipped field needs one.
    //
    // Custom raw-string delimiters: the patterns contain )" , which would close
    // a plain R"( ... )" early.
    static const std::regex kExpose(R"rx(exposeCharacter\s*\(\s*\{([^}]*)\})rx");
    static const std::regex kName(R"rx(Name\s*=\s*"([^"]*)")rx");
    static const std::regex kSku(R"rx(sku_id\s*=\s*"([^"]*)")rx");

    std::vector<Entry> out;
    for (auto it = std::sregex_iterator(luaSource.begin(), luaSource.end(), kExpose);
         it != std::sregex_iterator(); ++it) {
        const std::string body = (*it)[1].str();

        std::smatch m;
        if (!std::regex_search(body, m, kName) || m[1].str().empty())
            continue;

        Entry entry;
        entry.name = m[1].str();
        if (std::regex_search(body, m, kSku))
            entry.sku = m[1].str();
        out.push_back(std::move(entry));
    }
    return out;
}

std::string buildInjectionLua(const std::vector<Entry>& entries)
{
    if (entries.empty())
        return {};

    // Constant across every avatar slot in the shipped data.
    static const struct {
        const char* key;
        bool isNumber;
        double number;
        const char* text;
    } kConstantFields[] = {
        { "sku",         true,  4.0, nullptr  },
        { "version",     true,  3.0, nullptr  },
        { "TagType",     false, 0.0, "AVATAR" },
        { "keysum",      true,  0.0, nullptr  },
        { "displayName", false, 0.0, ""       },
        { "playset",     false, 0.0, ""       },
    };

    std::string lua;
    lua.reserve(1024 + entries.size() * 512);

    lua += "-- CrabeLoader: figure registry entries, injected in memory.\n";
    lua += "-- No standard library is available in this Lua state.\n";
    lua += "local c = _G[";
    lua += luaLiteral(containerKey());
    lua += "]\nif c then\nlocal t\n";

    for (const Entry& entry : entries) {
        lua += "t = {}\n";

        lua += "t[";
        lua += luaLiteral(encryptString("name"));
        lua += "] = ";
        lua += luaLiteral(encryptString(entry.name));
        lua += "\n";

        for (const auto& f : kConstantFields) {
            lua += "t[";
            lua += luaLiteral(encryptString(f.key));
            lua += "] = ";
            lua += luaLiteral(f.isNumber ? encryptNumber(f.number)
                                         : encryptString(f.text));
            lua += "\n";
        }

        // An explicit sku_id in the .lua wins: the row will select with that id,
        // so the slot has to carry it too.
        lua += "c[";
        lua += luaLiteral(encryptString(entry.sku.empty() ? allocateSku(entry.name)
                                                          : entry.sku));
        lua += "] = t\n";
    }

    lua += "end\n";
    return lua;
}

}
