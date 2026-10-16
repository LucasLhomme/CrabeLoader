/*
** CrabeLoader
** File description:
** Implements the figure registry slot builder: AES-128-CBC, sku allocation, Lua source emission.
** The key, IV and container key were recovered from the shipped files and verified byte-for-byte.
** Decrypts nothing and touches no disk; src/application/loader_content.cpp decides when to run it.
**
** Authors: @LucasLhomme
*/

#include "application/gateway.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "third_party/json.hpp"

namespace {

constexpr std::array<uint8_t, 16> kKey = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
constexpr char kIv[17] = "0123456789012345";

constexpr uint8_t kTagNumber = 1;
constexpr uint8_t kTagString = 2;

constexpr std::array<uint8_t, 16> kContainerKey = {
    0x37, 0x53, 0xf1, 0xa7, 0xa1, 0x82, 0xe5, 0xe0,
    0xd9, 0x4a, 0x11, 0x5d, 0xdc, 0x34, 0x3f, 0xac
};

constexpr uint32_t kSkuBase = 1000340;
constexpr uint32_t kSkuSpan = 660;

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

// Doubles in GF(2^8).
uint8_t xtime(uint8_t a)
{
    return static_cast<uint8_t>((a << 1) ^ ((a & 0x80) ? 0x1B : 0x00));
}

// Multiplies in GF(2^8).
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

// Expands 128-bit key into round keys.
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

// Returns cached round keys.
const RoundKeys& roundKeys()
{
    static const RoundKeys rk = expandKey();
    return rk;
}

// Encrypts one 16-byte block in place.
void encryptBlock(uint8_t s[16])
{
    const RoundKeys& rk = roundKeys();

    for (int i = 0; i < 16; ++i)
        s[i] ^= rk[0][i];

    for (int round = 1; round <= 10; ++round) {
        for (int i = 0; i < 16; ++i)
            s[i] = kSbox[s[i]];

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

// Encrypts plaintext using AES-128-CBC with PKCS7 padding.
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

// Encrypts payload prefixed with a type tag.
std::string tagged(uint8_t tag, const void* payload, size_t len)
{
    std::string plain;
    plain.push_back(static_cast<char>(tag));
    plain.append(static_cast<const char*>(payload), len);
    return aesCbcPkcs7(plain);
}

} // namespace

namespace crabe::application::gateway {

std::string encryptString(const std::string& value)
{
    return tagged(kTagString, value.data(), value.size());
}

std::string encryptNumber(double value)
{
    return tagged(kTagNumber, &value, sizeof(value));
}

std::string luaLiteral(const std::string& bytes)
{
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
    uint64_t h = 0;
    for (unsigned char c : name)
        h = (h * 31 + c) % 2147483648ULL;

    uint32_t sku = kSkuBase + static_cast<uint32_t>(h % kSkuSpan);
    return std::to_string(sku);
}

namespace {

// Returns lowercase copy of string.
std::string lowered(std::string s)
{
    std::ranges::transform(s, s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Returns unique entries by name, recording duplicate warnings.
std::vector<Entry> firstDeclarations(std::vector<Entry> entries, std::vector<std::string>& issues)
{
    std::vector<Entry> unique;
    std::set<std::string> names;
    for (Entry& entry : entries) {
        if (names.insert(lowered(entry.name)).second) {
            unique.push_back(std::move(entry));
        } else {
            issues.push_back("'" + entry.name + "' declared again in " + entry.origin +
                             " -- the first declaration is kept, this one is ignored");
        }
    }
    return unique;
}

// Claims explicit SKU if not already taken.
std::optional<std::string> claimExplicit(const Entry& entry, std::set<std::string>& taken,
                                         std::vector<std::string>& issues)
{
    if (taken.insert(entry.sku).second)
        return entry.sku;
    issues.push_back("sku_id " + entry.sku + " of '" + entry.name + "' (" + entry.origin +
                     ") is already used by another mod character -- no registry slot "
                     "written; remove sku_id to get a free one");
    return std::nullopt;
}

// Derives and claims free SKU within mod range.
std::optional<std::string> claimDerived(const Entry& entry, std::set<std::string>& taken,
                                        std::vector<std::string>& issues)
{
    const uint32_t start = static_cast<uint32_t>(std::stoul(allocateSku(entry.name)));
    uint32_t sku = start;
    for (uint32_t step = 0; step < kSkuSpan && taken.contains(std::to_string(sku)); ++step)
        sku = kSkuBase + (sku - kSkuBase + 1) % kSkuSpan;

    if (!taken.insert(std::to_string(sku)).second) {
        issues.push_back("no free sku_id left in the mod range for '" + entry.name + "' (" +
                         entry.origin + ")");
        return std::nullopt;
    }
    if (sku != start) {
        issues.push_back("derived sku_id " + std::to_string(start) + " of '" + entry.name +
                         "' was taken, moved to " + std::to_string(sku));
    }
    return std::to_string(sku);
}

} // namespace

std::vector<std::string> resolveSkus(std::vector<Entry>& entries)
{
    std::vector<std::string> issues;
    std::vector<Entry> unique = firstDeclarations(std::move(entries), issues);

    auto derived = std::ranges::stable_partition(unique, [](const Entry& e) { return !e.sku.empty(); });
    std::ranges::sort(derived, {}, &Entry::name);

    std::set<std::string> taken;
    std::vector<Entry> resolved;
    for (Entry& entry : unique) {
        std::optional<std::string> sku = entry.sku.empty() ? claimDerived(entry, taken, issues)
                                                           : claimExplicit(entry, taken, issues);
        if (!sku)
            continue;
        entry.sku = std::move(*sku);
        resolved.push_back(std::move(entry));
    }

    entries = std::move(resolved);
    return issues;
}

const std::string& containerKey()
{
    static const std::string key(reinterpret_cast<const char*>(kContainerKey.data()),
                                 kContainerKey.size());
    return key;
}

std::vector<Entry> parseExposedCharacters(const std::string& luaSource)
{
    static const std::regex kExpose(R"rx(exposeCharacter\s*(?:\(\s*)?\{([^}]*)\})rx");
    static const std::regex kName(R"rx([Nn]ame\s*=\s*["']([^"'\r\n]+)["'])rx");
    static const std::regex kSku(R"rx(sku_id\s*=\s*["']([^"'\r\n]+)["'])rx");
    static const std::regex kBase(R"rx(baseCharacter\s*=\s*["']([^"'\r\n]+)["'])rx");
    static const std::regex kProg(R"rx([Pp]rogressionTree\s*=\s*["']([^"'\r\n]+)["'])rx");
    static const std::regex kIcon(R"rx([Ii]con\s*=\s*["']([^"'\r\n]+)["'])rx");

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
        if (std::regex_search(body, m, kBase))
            entry.baseCharacter = m[1].str();
        if (std::regex_search(body, m, kProg))
            entry.progressionTree = m[1].str();
        if (std::regex_search(body, m, kIcon))
            entry.icon = m[1].str();

        out.push_back(std::move(entry));
    }
    return out;
}

std::string buildSkuTableLua(const std::vector<Entry>& entries)
{
    if (entries.empty())
        return {};

    std::string lua = "Crabe = Crabe or {}\n"
                      "Crabe.VirtualReader = Crabe.VirtualReader or {}\n"
                      "Crabe.VirtualReader._skus = {\n";

    for (const Entry& entry : entries) {
        lua += "  [";
        lua += luaLiteral(entry.name);
        lua += "] = \"";
        lua += entry.sku.empty() ? allocateSku(entry.name) : entry.sku;
        lua += "\",\n";
    }

    lua += "}\n";
    return lua;
}

// Builds Lua code to inject avatar slots into live registry.
std::string buildInjectionLua(const std::vector<Entry>& entries)
{
    if (entries.empty())
        return {};

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
    lua += "]\nif c then\nlocal t, k\n";

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

        lua += "k = ";
        lua += luaLiteral(encryptString(entry.sku.empty() ? allocateSku(entry.name)
                                                          : entry.sku));
        lua += "\nif c[k] == nil then c[k] = t end\n";
    }

    lua += "end\n";
    return lua;
}

std::vector<Entry> parseManifestCharacters(const std::string& jsonSource, const std::string& origin)
{
    std::vector<Entry> out;
    try {
        const auto j = nlohmann::json::parse(jsonSource, nullptr, false);
        if (j.is_discarded() || !j.is_object())
            return out;

        auto parseObj = [&](const nlohmann::json& obj) {
            if (!obj.is_object()) return;
            Entry entry;
            entry.origin = origin;

            if (obj.contains("name") && obj["name"].is_string())
                entry.name = obj["name"].get<std::string>();
            else if (obj.contains("Name") && obj["Name"].is_string())
                entry.name = obj["Name"].get<std::string>();

            if (entry.name.empty()) return;

            if (obj.contains("sku_id") && obj["sku_id"].is_string())
                entry.sku = obj["sku_id"].get<std::string>();
            else if (obj.contains("sku") && obj["sku"].is_string())
                entry.sku = obj["sku"].get<std::string>();

            if (obj.contains("baseCharacter") && obj["baseCharacter"].is_string())
                entry.baseCharacter = obj["baseCharacter"].get<std::string>();
            else if (obj.contains("base_character") && obj["base_character"].is_string())
                entry.baseCharacter = obj["base_character"].get<std::string>();

            if (obj.contains("displayName") && obj["displayName"].is_string())
                entry.displayName = obj["displayName"].get<std::string>();
            else if (obj.contains("display_name") && obj["display_name"].is_string())
                entry.displayName = obj["display_name"].get<std::string>();

            if (obj.contains("icon") && obj["icon"].is_string())
                entry.icon = obj["icon"].get<std::string>();
            else if (obj.contains("Icon") && obj["Icon"].is_string())
                entry.icon = obj["Icon"].get<std::string>();

            if (obj.contains("progressionTree") && obj["progressionTree"].is_string())
                entry.progressionTree = obj["progressionTree"].get<std::string>();
            else if (obj.contains("progression_tree") && obj["progression_tree"].is_string())
                entry.progressionTree = obj["progression_tree"].get<std::string>();

            out.push_back(std::move(entry));
        };

        if (j.contains("character")) {
            parseObj(j["character"]);
        }
        if (j.contains("characters") && j["characters"].is_array()) {
            for (const auto& item : j["characters"]) {
                parseObj(item);
            }
        }
    } catch (...) {
    }
    return out;
}

std::string buildExposeCallLua(const Entry& entry)
{
    std::string lua = "Crabe.VirtualReader.exposeCharacter({\n";
    lua += "  Name = " + luaLiteral(entry.name) + ",\n";
    if (!entry.sku.empty()) {
        lua += "  sku_id = " + luaLiteral(entry.sku) + ",\n";
    }
    if (!entry.progressionTree.empty()) {
        lua += "  ProgressionTree = " + luaLiteral(entry.progressionTree) + ",\n";
    }
    if (!entry.icon.empty()) {
        lua += "  Icon = " + luaLiteral(entry.icon) + ",\n";
    }
    lua += "})\n";
    return lua;
}

std::string buildActorListPatchLua(const std::vector<Entry>& entries)
{
    bool hasBase = false;
    for (const auto& entry : entries) {
        if (!entry.baseCharacter.empty()) {
            hasBase = true;
            break;
        }
    }
    if (!hasBase)
        return {};

    std::string lua = "-- CrabeLoader: Auto-patch ActorList for custom characters\n"
                      "if Data and Data.DBName == \"ActorList\" and Data.Records then\n"
                      "  local recs = Data.Records\n"
                      "  local n = #recs\n";

    for (const auto& entry : entries) {
        if (entry.baseCharacter.empty()) continue;
        const std::string lowerBase = lowered(entry.baseCharacter);
        const std::string lowerName = lowered(entry.name);

        lua += "  local hasBase = false\n"
               "  local hasClone = false\n"
               "  for i = 1, n do\n"
               "    local r = recs[i]\n"
               "    if r then\n"
               "      if r.Name == \"" + lowerBase + "\" then hasBase = true end\n"
               "      if r.Name == \"" + lowerName + "\" then hasClone = true end\n"
               "    end\n"
               "  end\n"
               "  if hasBase and not hasClone then\n"
               "    n = n + 1\n"
               "    recs[n] = { Parms = \"DNAFile=characters/" + entry.name + "/" + entry.name + ".dnax\", Type = \"Avatar\", Name = \"" + lowerName + "\" }\n"
               "  end\n";
    }
    lua += "end\n";
    return lua;
}

std::string buildDataMapPatchLua(const std::vector<Entry>& entries)
{
    bool hasBase = false;
    for (const auto& entry : entries) {
        if (!entry.baseCharacter.empty()) {
            hasBase = true;
            break;
        }
    }
    if (!hasBase)
        return {};

    std::string lua = "-- CrabeLoader: Auto-patch DataMap for custom characters\n"
                      "local function cloneFiles(files, from, to)\n"
                      "  local out = {}\n"
                      "  local i = 1\n"
                      "  while files and i <= #files do\n"
                      "    local v = files[i]\n"
                      "    if v == from then v = to end\n"
                      "    out[i] = v\n"
                      "    i = i + 1\n"
                      "  end\n"
                      "  return out\n"
                      "end\n"
                      "local A = DataMap and DataMap.Actor\n"
                      "local C = DataMap and DataMap.Costume\n";

    for (const auto& entry : entries) {
        if (entry.baseCharacter.empty()) continue;
        const std::string lowerBase = lowered(entry.baseCharacter);
        const std::string lowerName = lowered(entry.name);

        lua += "if A and A." + lowerBase + " and not A." + lowerName + " then\n"
               "  local src = A." + lowerBase + "\n"
               "  A." + lowerName + " = { Includes = src.Includes, Files = cloneFiles(src.Files, \"DNA|characters/" + lowerBase + "\", \"DNA|characters/" + lowerName + "/" + lowerName + "\") }\n"
               "end\n"
               "if C and C." + lowerBase + "_default and not C." + lowerName + "_default then\n"
               "  local src = C." + lowerBase + "_default\n"
               "  C." + lowerName + "_default = { Files = cloneFiles(src.Files, \"Entity|characters/" + lowerBase + "/" + lowerBase + "\", \"Entity|characters/" + lowerName + "/" + lowerName + "\") }\n"
               "end\n";
    }
    return lua;
}

} // namespace crabe::application::gateway
