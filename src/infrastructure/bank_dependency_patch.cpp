/*
** CrabeLoader
** File description:
** Builds additive bankloads dependency overlays from small per-mod JSON patches.
** The PC table is XML encrypted with AES-128 CTR, using a zero big-endian counter.
** VFS selects the effective source and redirects reads; this module never edits game files.
**
** Authors: @Weresthebeef
*/
#include "infrastructure/bank_dependency_patch.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <msxml6.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <atomic>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include "third_party/json.hpp"

namespace crabe::infrastructure {
namespace {
using Microsoft::WRL::ComPtr;
// Turns API failures into a diagnostic handled at the VFS boundary.
void check(HRESULT result) { if (FAILED(result)) throw std::runtime_error("Windows XML operation failed"); }
// Owns the XML automation string without a comsupp runtime dependency.
struct Text {
    BSTR value{};
    explicit Text(const wchar_t* s) : value(SysAllocString(s)) { if (!value) throw std::bad_alloc(); }
    Text() = default;
    ~Text() { SysFreeString(value); }
    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;
};
// Balances COM initialization on the scanning thread, tolerating an existing apartment.
struct Apartment {
    HRESULT status{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};
    Apartment() { if (FAILED(status) && status != RPC_E_CHANGED_MODE) check(status); }
    ~Apartment() { if (SUCCEEDED(status)) CoUninitialize(); }
};
// Owns the CNG algorithm and key handles used by CTR and SHA-256.
struct Crypto {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_KEY_HANDLE key{};
    ~Crypto() { if (key) BCryptDestroyKey(key); if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0); }
};
// Reads bounded binary files without accepting truncated reads.
std::string read(const std::filesystem::path& path, std::size_t limit) {
    const auto size = std::filesystem::file_size(path);
    if (!size || size > limit) throw std::runtime_error("bank patch input has invalid size: " + path.string());
    std::string bytes(static_cast<std::size_t>(size), '\0');
    std::ifstream in(path, std::ios::binary);
    if (!in.read(bytes.data(), static_cast<std::streamsize>(size))) throw std::runtime_error("cannot read " + path.string());
    return bytes;
}
// Converts UTF-8 XML to a wide automation string, rejecting invalid encoding.
std::wstring wide(std::string_view text) {
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!count) throw std::runtime_error("bankloads XML is not UTF-8");
    std::wstring out(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), count);
    return out;
}
// Converts the serialized DOM back to UTF-8 before encryption.
std::string narrow(BSTR text) {
    if (!text) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, SysStringLen(text), nullptr, 0, nullptr, nullptr);
    if (!count) throw std::runtime_error("cannot encode XML");
    std::string out(count, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, SysStringLen(text), out.data(), count, nullptr, nullptr);
    return out;
}
// Normalizes bank identifiers for duplicate detection.
std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
// Accepts a sound filename, excluding paths, control bytes and unsupported asset types.
bool validName(const std::string& name) {
    if (name.empty() || name.size() > 160 || name.find("..") != std::string::npos) return false;
    for (unsigned char c : name)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
    const auto value = lower(name);
    return value.ends_with(".bnk") || value.ends_with(".dbnk") || value.ends_with(".dlg");
}
// Encrypts or decrypts the PC bank table with a big-endian 128-bit CTR counter.
std::string crypt(std::string bytes) {
    Crypto crypto;
    if (BCryptOpenAlgorithmProvider(&crypto.algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0) < 0 ||
        BCryptSetProperty(crypto.algorithm, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_ECB)), sizeof(BCRYPT_CHAIN_MODE_ECB), 0) < 0)
        throw std::runtime_error("AES unavailable");
    std::array<UCHAR,16> key{0x68,0x1b,0xbe,0xea,0x63,0x16,0x01,0x88,0xf9,0xb7,0x94,0x51,0x04,0xa5,0x14,0x99};
    if (BCryptGenerateSymmetricKey(crypto.algorithm, &crypto.key, nullptr, 0, key.data(), static_cast<ULONG>(key.size()), 0) < 0)
        throw std::runtime_error("AES key creation failed");
    std::array<UCHAR,16> counter{}, block{};
    for (std::size_t offset = 0; offset < bytes.size(); offset += 16) {
        ULONG written{};
        if (BCryptEncrypt(crypto.key, counter.data(), 16, nullptr, nullptr, 0, block.data(), 16, &written, 0) < 0 || written != 16)
            throw std::runtime_error("AES operation failed");
        for (std::size_t i = 0; i < std::min<std::size_t>(16, bytes.size() - offset); ++i) bytes[offset+i] ^= block[i];
        for (int i = 15; i >= 0 && ++counter[i] == 0; --i) { }
    }
    return bytes;
}
// Addresses generated files by their encrypted content so old readers keep a stable snapshot.
std::string digest(const std::string& bytes) {
    Crypto crypto;
    if (BCryptOpenAlgorithmProvider(&crypto.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) throw std::runtime_error("SHA unavailable");
    std::array<UCHAR,32> hash{};
    if (BCryptHash(crypto.algorithm, nullptr, 0, reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())), static_cast<ULONG>(bytes.size()), hash.data(), 32) < 0)
        throw std::runtime_error("SHA failed");
    std::string out;
    for (auto byte : hash) { out += "0123456789abcdef"[byte >> 4]; out += "0123456789abcdef"[byte & 15]; }
    return out;
}
// Gets a node's XML tag name.
std::string tag(IXMLDOMNode* node) { Text text; check(node->get_nodeName(&text.value)); return narrow(text.value); }
// Gets a node's decoded text content.
std::string content(IXMLDOMNode* node) { Text text; check(node->get_text(&text.value)); return narrow(text.value); }
}

// Builds an additive overlay atomically; any failure leaves the effective source untouched.
std::optional<std::filesystem::path> buildBankDependencyOverlay(
    const std::filesystem::path& source, const std::vector<std::filesystem::path>& patches,
    const std::filesystem::path& cache, const std::function<bool(std::string_view)>& soundExists) {
    if (patches.empty()) return std::nullopt;
    if (patches.size() > 64) throw std::runtime_error("too many bank dependency patch files");
    Apartment apartment;
    ComPtr<IXMLDOMDocument2> doc;
    check(CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&doc)));
    check(doc->put_async(VARIANT_FALSE));
    check(doc->put_validateOnParse(VARIANT_FALSE));
    check(doc->put_resolveExternals(VARIANT_FALSE));
    check(doc->put_preserveWhiteSpace(VARIANT_TRUE));
    VARIANT prohibit; VariantInit(&prohibit); prohibit.vt = VT_BOOL; prohibit.boolVal = VARIANT_TRUE;
    Text property(L"ProhibitDTD"); check(doc->setProperty(property.value, prohibit));
    auto xml = wide(crypt(read(source, 16 * 1024 * 1024)));
    if (xml.find(L'\0') != std::wstring::npos) throw std::runtime_error("bankloads contains NUL");
    Text input(xml.c_str()); VARIANT_BOOL loaded{}; check(doc->loadXML(input.value, &loaded));
    if (loaded != VARIANT_TRUE) throw std::runtime_error("bankloads XML cannot be parsed");
    ComPtr<IXMLDOMElement> root; check(doc->get_documentElement(&root));
    if (!root || tag(root.Get()) != "bankloads") throw std::runtime_error("unexpected bankloads root");
    std::map<std::string, ComPtr<IXMLDOMNode>> banks;
    std::map<std::string, std::set<std::string>> edges;
    ComPtr<IXMLDOMNodeList> nodes; check(root->get_childNodes(&nodes)); long length{}; check(nodes->get_length(&length));
    for (long i = 0; i < length; ++i) {
        ComPtr<IXMLDOMNode> node; check(nodes->get_item(i, &node));
        if (tag(node.Get()) != "bank") continue;
        ComPtr<IXMLDOMNamedNodeMap> attributes; check(node->get_attributes(&attributes));
        Text nameKey(L"name"); ComPtr<IXMLDOMNode> name; check(attributes->getNamedItem(nameKey.value, &name));
        if (!name) throw std::runtime_error("bank record has no name");
        auto key = lower(content(name.Get()));
        if (!banks.emplace(key, node).second) throw std::runtime_error("duplicate bank record: " + key);
        ComPtr<IXMLDOMNodeList> children; check(node->get_childNodes(&children)); long count{}; check(children->get_length(&count));
        for (long j = 0; j < count; ++j) {
            ComPtr<IXMLDOMNode> child; check(children->get_item(j, &child));
            if (tag(child.Get()) == "loaditem") edges[key].insert(lower(content(child.Get())));
        }
    }
    std::size_t additions = 0;
    for (const auto& patch : patches) {
        const auto json = nlohmann::json::parse(read(patch, 128 * 1024));
        if (!json.is_object() || json.size() != 2 || !json.contains("version") || !json["version"].is_number_integer() || json["version"] != 1 ||
            !json.contains("banks") || !json["banks"].is_object() || json["banks"].empty() || json["banks"].size() > 1024)
            throw std::runtime_error("invalid dependency patch schema: " + patch.string());
        for (auto row = json["banks"].begin(); row != json["banks"].end(); ++row) {
            const auto key = lower(row.key());
            if (!validName(row.key()) || !key.ends_with(".bnk") || !banks.contains(key) || !soundExists(key) || !row.value().is_array() || row.value().size() > 256)
                throw std::runtime_error("unknown or invalid target bank: " + row.key());
            for (const auto& value : row.value()) {
                if (!value.is_string()) throw std::runtime_error("dependency must be a filename string");
                const auto dependency = value.get<std::string>(); const auto dep = lower(dependency);
                if (!validName(dependency) || !soundExists(dep)) throw std::runtime_error("missing or invalid sound dependency: " + dependency);
                if (edges[key].contains(dep)) continue;
                std::set<std::string> visited;
                std::vector<std::string> pending{dep};
                while (!pending.empty()) {
                    auto current = pending.back(); pending.pop_back();
                    if (current == key) throw std::runtime_error("patch introduces dependency cycle: " + key);
                    if (!visited.insert(current).second) continue;
                    auto next = edges.find(current);
                    if (next != edges.end()) pending.insert(pending.end(), next->second.begin(), next->second.end());
                }
                Text itemTag(L"loaditem"); ComPtr<IXMLDOMElement> item; check(doc->createElement(itemTag.value, &item));
                Text itemValue(wide(dependency).c_str()); check(item->put_text(itemValue.value));
                ComPtr<IXMLDOMNode> appended; check(banks.at(key)->appendChild(item.Get(), &appended));
                edges[key].insert(dep); ++additions;
            }
        }
    }
    if (!additions) return std::nullopt;
    Text output; check(doc->get_xml(&output.value));
    auto bytes = crypt(narrow(output.value));
    if (bytes.size() > 16 * 1024 * 1024) throw std::runtime_error("generated bank table exceeds size limit");
    std::filesystem::create_directories(cache);
    const auto path = cache / ("bankloads-" + digest(bytes) + ".bdp");
    if (std::filesystem::exists(path)) {
        if (read(path, 16 * 1024 * 1024) != bytes) throw std::runtime_error("cached bank overlay is corrupt");
        return path;
    }
    static std::atomic<unsigned long> serial{};
    auto temporary = path;
    temporary += ".tmp-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(serial.fetch_add(1));
    try {
        { std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
          out.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); out.close();
          if (!out) throw std::runtime_error("cannot write bank overlay cache"); }
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH)) {
            if (!std::filesystem::exists(path) || read(path, 16 * 1024 * 1024) != bytes) throw std::runtime_error("cannot publish bank overlay");
            std::filesystem::remove(temporary);
        }
    } catch (...) { std::error_code ec; std::filesystem::remove(temporary, ec); throw; }
    return path;
}
}
