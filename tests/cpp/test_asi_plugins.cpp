/*
** CrabeLoader
** File description:
** Tests .asi plugin support: crabe.toml's [plugins], discovery order and skip reasons, PE classification,
** real loads of three probe plugins built next to this test, and the plugin API they call.
** Works in a temporary folder and leaves the real game alone.
**
** Authors: @LucasLhomme, @lightningcal
*/

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <windows.h>

#include "application/plugin_api.hpp"
#include "domain/asi_plugins.hpp"
#include "domain/config.hpp"
#include "infrastructure/asi_loader.hpp"

namespace {

/// Records what the plugin API asked of the loader, instead of logging, hooking or running Lua.
class FakeHost final : public crabe::application::PluginHost {
public:
    struct Hook {
        std::string owner;
        std::string name;
        void* target;
    };

    std::vector<std::pair<int, std::string>> logs;
    std::vector<Hook> hooks;
    std::vector<std::pair<std::string, std::string>> lua;

    /// Keeps the line.
    void log(int level, std::string_view line) override { logs.emplace_back(level, std::string(line)); }

    /// Keeps the hook and accepts it.
    bool installHook(const std::string& owner, std::string_view name, void* target, void*, void**) override
    {
        hooks.push_back({owner, std::string(name), target});
        return true;
    }

    /// Forgets the owner's hooks and says how many there were.
    std::size_t removeHooks(const std::string& owner) override
    {
        return static_cast<std::size_t>(std::erase_if(hooks, [&](const Hook& hook) { return hook.owner == owner; }));
    }

    /// Keeps the chunk.
    void queueLua(const std::string& owner, std::string code) override { lua.emplace_back(owner, std::move(code)); }

    /// Whether a logged line contains the text.
    [[nodiscard]] bool logged(std::string_view text) const
    {
        for (const auto& entry : logs)
            if (entry.second.find(text) != std::string::npos)
                return true;
        return false;
    }
};

using crabe::domain::AsiCandidate;
using crabe::domain::Config;
using crabe::domain::PluginBinary;
using crabe::infrastructure::AsiLoadStatus;
namespace fs = std::filesystem;

/// Asserts that a boolean condition is satisfied or exits immediately.
void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << std::endl;
        std::exit(1);
    }
}

/// Parses crabe.toml text, failing the test when the parser refuses it.
Config parseConfig(std::string_view text)
{
    auto parsed = Config::parse(text, "crabe.toml");
    require(parsed.has_value(), "test crabe.toml parses");
    return std::move(*parsed);
}

/// Creates an empty folder for one test under the system temporary folder.
fs::path freshFolder(std::string_view name)
{
    const fs::path folder = fs::temp_directory_path() / std::format("crabe_asi_{}_{}", name, GetCurrentProcessId());
    std::error_code error;
    fs::remove_all(folder, error);
    fs::create_directories(folder);
    return folder;
}

/// Writes text to a file, creating its folder.
void writeText(const fs::path& path, std::string_view text)
{
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << text;
}

/// Returns each candidate as "<path relative to root>|<skip reason>", in discovery order.
std::vector<std::string> describe(const std::vector<AsiCandidate>& candidates, const fs::path& root)
{
    std::vector<std::string> out;
    for (const AsiCandidate& candidate : candidates)
        out.push_back(fs::relative(candidate.path, root).generic_string() + "|" + candidate.skipReason);
    return out;
}

/// Builds the first bytes of a PE file with the given machine and characteristics.
std::vector<std::byte> peHeader(std::uint16_t machine, std::uint16_t characteristics)
{
    std::vector<std::byte> bytes(512, std::byte{0});
    bytes[0] = std::byte{'M'};
    bytes[1] = std::byte{'Z'};
    bytes[0x3C] = std::byte{0x80};
    bytes[0x80] = std::byte{'P'};
    bytes[0x81] = std::byte{'E'};
    bytes[0x84] = static_cast<std::byte>(machine & 0xFF);
    bytes[0x85] = static_cast<std::byte>(machine >> 8);
    bytes[0x96] = static_cast<std::byte>(characteristics & 0xFF);
    bytes[0x97] = static_cast<std::byte>(characteristics >> 8);
    return bytes;
}

/// [plugins] defaults to loading, reads asi and skip, and reports a skip entry that is not a string.
void testConfig()
{
    const Config empty = parseConfig("");
    require(empty.asiEnabled() && empty.asiSkipList().empty(), "no [plugins] section loads every plugin");

    const Config shipped = parseConfig(crabe::domain::buildDefaultConfigText());
    require(shipped.asiEnabled() && shipped.asiSkipList().empty(), "the shipped crabe.toml loads every plugin");
    require(shipped.getIgnoredKeys().empty(), "the shipped crabe.toml has no unknown key");

    const Config custom = parseConfig("[plugins]\nasi = false\nskip = [\"Old.asi\", 3]\n");
    require(!custom.asiEnabled(), "[plugins].asi = false is read");
    require(custom.asiSkipList() == std::vector<std::string>{"Old.asi"}, "[plugins].skip keeps its strings");
    require(custom.getIgnoredKeys().size() == 1, "a skip entry that is not a string is reported");
}

/// The PE header decides between a 32-bit DLL, another architecture, a program and anything else.
void testClassify()
{
    using crabe::domain::classifyPluginBinary;
    require(classifyPluginBinary(peHeader(0x014C, 0x2102)) == PluginBinary::Win32Dll, "x86 DLL");
    require(classifyPluginBinary(peHeader(0x8664, 0x2022)) == PluginBinary::NotWin32, "x64 DLL");
    require(classifyPluginBinary(peHeader(0x014C, 0x0102)) == PluginBinary::NotADll, "x86 program");

    std::vector<std::byte> text(512, std::byte{'a'});
    require(classifyPluginBinary(text) == PluginBinary::NotPortableExecutable, "text file");

    std::vector<std::byte> truncated = peHeader(0x014C, 0x2102);
    truncated.resize(0x90);
    require(classifyPluginBinary(truncated) == PluginBinary::NotPortableExecutable, "header cut short");

    std::vector<std::byte> farHeader = peHeader(0x014C, 0x2102);
    farHeader[0x3C] = std::byte{0xFF};
    farHeader[0x3D] = std::byte{0xFF};
    require(classifyPluginBinary(farHeader) == PluginBinary::NotPortableExecutable, "PE offset out of range");
    require(classifyPluginBinary({}) == PluginBinary::NotPortableExecutable, "empty file");

    require(crabe::domain::pluginBinaryProblem(PluginBinary::Win32Dll).empty(), "a 32-bit DLL has no problem");
    require(!crabe::domain::pluginBinaryProblem(PluginBinary::NotWin32).empty(), "a 64-bit DLL is explained");
}

/// Folders load in order (game folder, scripts, plugins, mods), names sort without case,
/// and every reason to skip a file is reported on the file.
void testDiscovery()
{
    const fs::path root = freshFolder("discovery");
    writeText(root / "z.asi", "");
    writeText(root / "A.ASI", "");
    writeText(root / "notes.txt", "");
    fs::create_directories(root / "folder.asi");
    writeText(root / "scripts" / "b.asi", "");
    writeText(root / "scripts" / "deep" / "hidden.asi", "");
    writeText(root / "plugins" / "c.asi", "");
    writeText(root / "mods" / "alpha" / "alpha.asi", "");
    writeText(root / "mods" / "beta" / "mod.json",
              R"({ "manifestVersion": 1, "id": "com.test.beta", "version": "1.0.0" })");
    writeText(root / "mods" / "beta" / "beta.asi", "");
    writeText(root / "mods" / "_template" / "template.asi", "");

    const std::vector<std::string> all = describe(crabe::domain::discoverAsiPlugins(root, parseConfig("")), root);
    const std::vector<std::string> expected = {
        "A.ASI|", "z.asi|", "scripts/b.asi|", "plugins/c.asi|", "mods/alpha/alpha.asi|", "mods/beta/beta.asi|",
    };
    require(all == expected, "discovery order and folders");

    const Config profile = parseConfig(
        "[general]\nprofile = \"lean\"\n[profiles.lean]\nenabled = [\"local.alpha\"]\n[plugins]\nskip = [\"Z.ASI\"]\n");
    const std::vector<AsiCandidate> filtered = crabe::domain::discoverAsiPlugins(root, profile);
    require(filtered.size() == 6, "skipped plugins stay listed");
    require(filtered[0].shouldLoad(), "A.ASI loads");
    require(filtered[1].skipReason.find("[plugins].skip") != std::string::npos, "skip matches without case");
    require(filtered[4].shouldLoad() && filtered[4].source == "mod local.alpha", "an enabled mod's plugin loads");
    require(filtered[5].skipReason.find("com.test.beta is disabled") != std::string::npos,
            "a disabled mod's plugin is skipped under its declared id");

    const std::vector<AsiCandidate> off = crabe::domain::discoverAsiPlugins(root, parseConfig("[plugins]\nasi = false\n"));
    require(off.size() == 6, "asi = false still lists the plugins");
    for (const AsiCandidate& candidate : off)
        require(candidate.skipReason.find("ASI loading is off") != std::string::npos, "asi = false skips all");

    require(crabe::domain::discoverAsiPlugins(root / "missing", parseConfig("")).empty(), "a missing folder lists nothing");

    std::error_code error;
    fs::remove_all(root, error);
}

/// Loads the probe plugins for real: one starts, a second load is noticed, a refusal and junk are reported.
void testLoading(const fs::path& probe, const fs::path& refusingProbe)
{
    const fs::path root = freshFolder("loading");
    fs::create_directories(root / "scripts");
    fs::copy_file(probe, root / "scripts" / "probe.asi");
    fs::copy_file(refusingProbe, root / "scripts" / "refuses.asi");
    fs::copy_file(probe, root / "scripts" / "unused.asi");
    writeText(root / "scripts" / "junk.asi", "not a dll");

    const auto candidate = [&](std::string_view name, std::string reason = {}) {
        return AsiCandidate{root / "scripts" / name, "scripts", std::move(reason)};
    };

    const auto first = crabe::infrastructure::loadAsiPlugin(candidate("probe.asi"));
    require(first.status == AsiLoadStatus::Loaded, "probe.asi loads: " + first.detail);
    std::wstring started(16, L'\0');
    started.resize(GetEnvironmentVariableW(L"CRABE_ASI_PROBE", started.data(), 16));
    require(started == L"started", "probe.asi's DllMain ran");

    const auto again = crabe::infrastructure::loadAsiPlugin(candidate("probe.asi"));
    require(again.status == AsiLoadStatus::AlreadyLoaded, "a plugin already in the process is not loaded twice");

    fs::copy_file(probe, root / "scripts" / "loader_copy.asi");
    {
        std::ofstream copy(root / "scripts" / "loader_copy.asi", std::ios::binary | std::ios::app);
        const std::wstring_view marker = L"CrabeLoader Proxy DLL for Disney Infinity 3.0";
        copy.write(reinterpret_cast<const char*>(marker.data()), static_cast<std::streamsize>(marker.size() * 2));
    }
    const auto loaderCopy = crabe::infrastructure::loadAsiPlugin(candidate("loader_copy.asi"));
    require(loaderCopy.status == AsiLoadStatus::Skipped && loaderCopy.detail.find("another copy of CrabeLoader") == 0,
            "a CrabeLoader copy left as .asi is never started beside the running loader");
    require(GetModuleHandleW((root / "scripts" / "loader_copy.asi").c_str()) == nullptr, "and it is not loaded");

    const auto refused = crabe::infrastructure::loadAsiPlugin(candidate("refuses.asi"));
    require(refused.status == AsiLoadStatus::Failed, "a DllMain returning FALSE fails the load");
    require(refused.detail.find("refused") != std::string::npos, "the refusal is explained: " + refused.detail);

    const auto junk = crabe::infrastructure::loadAsiPlugin(candidate("junk.asi"));
    require(junk.status == AsiLoadStatus::Failed && junk.detail == "not a Windows DLL", "junk is refused before Windows");

    const auto skipped = crabe::infrastructure::loadAsiPlugin(candidate("unused.asi", "listed in [plugins].skip"));
    require(skipped.status == AsiLoadStatus::Skipped, "a skipped candidate reports its reason");
    require(GetModuleHandleW((root / "scripts" / "unused.asi").c_str()) == nullptr, "a skipped plugin is not loaded");

    const fs::path game = freshFolder("loader");
    fs::copy_file(probe, game / "first.asi");
    writeText(game / "plugins" / "second.asi", "");
    crabe::infrastructure::AsiLoader& loader = crabe::infrastructure::AsiLoader::get();
    loader.loadAll(game);
    loader.loadAll(root);
    const auto records = loader.records();
    require(records.size() == 2, "loadAll runs once and keeps one record per plugin");
    require(records[0].status == AsiLoadStatus::Loaded && records[0].source == "game folder", "first.asi loads");
    require(records[1].status == AsiLoadStatus::Failed && records[1].detail == "not a Windows DLL",
            "an empty .asi fails as no DLL");
    require(loader.gameRoot() == game, "the searched folder is kept");
}

/// Hands the plugin API to a real plugin and checks every call it makes, once per plugin,
/// and that a CrabePluginInit which faults is caught.
void testPluginApi(const fs::path& probe, const fs::path& crashingProbe)
{
    const fs::path root = freshFolder("api");
    fs::copy_file(probe, root / "api_probe.asi");
    fs::copy_file(crashingProbe, root / "init_crash.asi");

    const auto record = crabe::infrastructure::loadAsiPlugin(AsiCandidate{root / "api_probe.asi", "game folder", {}});
    require(record.status == AsiLoadStatus::Loaded && record.module != nullptr, "api_probe.asi loads with its module");

    FakeHost host;
    crabe::application::PluginApi api(host);
    require(api.startPlugins({record}) == 1, "the plugin receives the API");
    require(host.logged("[api_probe.asi] probe started {not a format}"), "log is prefixed and not a format string");
    require(host.logged("api_probe.asi started with plugin API v2"), "the start is logged");
    require(api.value("probe.name") == "api_probe.asi", "pluginName and setValue reach the shared values");
    require(api.value("probe.readBack") == "ok", "getValue returns the full length and cuts the copy short");
    require(api.value("probe.missing") == "ok", "getValue answers -1 for an unset key");
    require(api.value("probe.blankLua") == "ok", "runLua refuses a blank chunk");
    require(api.value("probe.loader").has_value(), "loaderVersion is set");

    require(host.hooks.size() == 1, "installHook reaches the host");
    require(host.hooks[0].owner == "plugin:api_probe.asi", "the hook is owned by the plugin");
    require(host.hooks[0].name == "api_probe.asi:target", "the hook is named after the plugin");
    require(host.lua.size() == 1 && host.lua[0].first == "api_probe.asi", "runLua queues under the plugin's name");
    require(host.lua[0].second == "Crabe.write('probe')", "runLua queues the chunk unchanged");

    require(api.value("probe.renderRegistered") == "ok", "onPresent and onBeforeResize accept a callback");
    int fakeSwapChain = 0;
    api.dispatchPresent(&fakeSwapChain, 1);
    api.dispatchPresent(&fakeSwapChain, 1);
    const std::optional<std::string> present = api.value("probe.present");
    require(present.has_value() && present->starts_with("2 "), "dispatchPresent calls the callback every time");
    require(present->ends_with(" 1 user"), "the Present callback gets the flags and its user pointer");
    api.dispatchBeforeResize(&fakeSwapChain);
    require(host.logged("api_probe.asi raised exception 0xC0000005 in its ResizeBuffers callback"),
            "a faulting render callback is caught and logged");
    api.dispatchPresent(&fakeSwapChain, 1);
    api.dispatchBeforeResize(&fakeSwapChain);
    require(api.value("probe.present")->starts_with("2 "), "a plugin whose render callback faulted is not called again");

    require(api.startPlugins({record}) == 0, "a plugin receives the API only once");
    crabe::infrastructure::AsiLoadRecord system{};
    system.module = GetModuleHandleW(L"kernel32.dll");
    require(api.startPlugins({system}) == 0, "a module without CrabePluginInit is left alone");

    api.setValue("probe.name", std::nullopt);
    require(!api.value("probe.name").has_value(), "an empty value erases the key");

    const auto crashing = crabe::infrastructure::loadAsiPlugin(AsiCandidate{root / "init_crash.asi", "game folder", {}});
    require(crashing.status == AsiLoadStatus::Loaded, "init_crash.asi loads");
    require(api.startPlugins({crashing}) == 0, "a CrabePluginInit that faults is not counted as started");
    require(host.logged("init_crash.asi raised exception 0xC0000005"), "the fault is logged with its code");
}

} // namespace

/// Runs every test; the three probe plugins' paths come from CTest.
int main(int argc, char** argv)
{
    testConfig();
    testClassify();
    testDiscovery();
    if (argc >= 4) {
        testLoading(argv[1], argv[2]);
        testPluginApi(argv[1], argv[3]);
    } else {
        std::cout << "[SKIP] real loads: pass the asi_probe, asi_probe_refuses and asi_probe_init_crash paths"
                  << std::endl;
    }
    std::cout << "[PASS] test_asi_plugins" << std::endl;
    return 0;
}
