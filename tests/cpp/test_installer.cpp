#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <windows.h>

#include "installer/game_installer.hpp"
#include "installer/install_plan.hpp"
#include "installer/sha256.hpp"
#include "installer/steam_locator.hpp"

namespace {

using namespace crabe::installer;
namespace fs = std::filesystem;

/// Asserts that a boolean condition is satisfied or exits immediately.
void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << std::endl;
        std::exit(1);
    }
}

std::vector<std::byte> makeBytes(std::size_t size, unsigned char seed, bool windowsExecutable)
{
    std::vector<std::byte> bytes(size);
    for (std::size_t i = 0; i < size; ++i)
        bytes[i] = static_cast<std::byte>((i * 31u + seed) & 0xFFu);
    if (windowsExecutable) {
        bytes[0] = std::byte{'M'};
        bytes[1] = std::byte{'Z'};
    }
    return bytes;
}

void writeFile(const fs::path& path, const std::vector<std::byte>& bytes)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(static_cast<bool>(stream), "test helper could not write a file");
}

std::vector<std::byte> readFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    const std::vector<char> raw((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes(raw.size());
    std::ranges::transform(raw, bytes.begin(), [](char c) { return static_cast<std::byte>(c); });
    return bytes;
}

class TempFolder final {
public:
    TempFolder()
        : _path(fs::temp_directory_path() / ("crabe_installer_test_" + std::to_string(GetCurrentProcessId())
                                              + "_" + std::to_string(GetTickCount64())))
    {
        fs::create_directories(_path);
    }

    ~TempFolder()
    {
        std::error_code ignored;
        fs::remove_all(_path, ignored);
    }

    TempFolder(const TempFolder&) = delete;
    TempFolder& operator=(const TempFolder&) = delete;
    TempFolder(TempFolder&&) = delete;
    TempFolder& operator=(TempFolder&&) = delete;

    [[nodiscard]] const fs::path& path() const noexcept { return _path; }

    [[nodiscard]] fs::path makeGameFolder(const std::string& name, const std::vector<std::byte>* proxy,
                                          const std::vector<std::byte>* backup, bool withExecutable) const
    {
        const fs::path folder = _path / name;
        fs::create_directories(folder);
        if (withExecutable)
            writeFile(folder / std::string(kGameExecutable), makeBytes(2048, 1, true));
        if (proxy)
            writeFile(folder / std::string(kProxyName), *proxy);
        if (backup)
            writeFile(folder / std::string(kBackupName), *backup);
        return folder;
    }

private:
    fs::path _path;
};

std::vector<std::string> knownOriginalsOf(const std::vector<std::byte>& original)
{
    return {*sha256Hex(original)};
}

/// Verifies SHA-256 against published vectors, over bytes and over a file.
void testSha256(const TempFolder& temp)
{
    const std::vector<std::byte> abcBytes{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};
    require(sha256Hex(abcBytes) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "SHA-256 of \"abc\" must match the published vector");
    require(sha256Hex({}) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "SHA-256 of nothing must match the published vector");

    const fs::path file = temp.path() / "abc.bin";
    writeFile(file, abcBytes);
    require(sha256FileHex(file) == sha256Hex(abcBytes), "hashing a file must equal hashing its bytes");

    const std::vector<std::byte> big = makeBytes(300 * 1024 + 7, 9, false);
    writeFile(temp.path() / "big.bin", big);
    require(sha256FileHex(temp.path() / "big.bin") == sha256Hex(big), "a multi-chunk file must hash like its bytes");
    require(!sha256FileHex(temp.path() / "missing.bin").has_value(), "a missing file has no hash");
}

/// Verifies the decision table that protects the original DLL.
void testPlan()
{
    require(planInstall({true, ProxySlot::KnownOriginal, false}) == Plan::FreshInstall,
            "a known original with no backup is a fresh install");
    require(planInstall({true, ProxySlot::Other, true}) == Plan::Update,
            "any proxy with a backup beside it is an update");
    require(planInstall({true, ProxySlot::KnownOriginal, true}) == Plan::Update,
            "an original that reappeared beside a backup is still replaced, never renamed over the backup");
    require(planInstall({true, ProxySlot::Missing, true}) == Plan::Update,
            "a missing proxy with a backup is restored by an update");
    require(planInstall({true, ProxySlot::Other, false}) == Plan::ProxyUnrecognized,
            "an unknown file with no backup is never renamed");
    require(planInstall({true, ProxySlot::Missing, false}) == Plan::ProxyMissing,
            "no proxy and no backup is a refusal");
    require(planInstall({false, ProxySlot::KnownOriginal, true}) == Plan::NotAGameFolder,
            "a folder without the game executable is a refusal whatever else it holds");
}

/// Verifies a fresh install, then an update, byte for byte.
void testFreshInstallThenUpdate(const TempFolder& temp)
{
    const std::vector<std::byte> original = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> payloadV1 = makeBytes(80 * 1024, 5, true);
    const std::vector<std::byte> payloadV2 = makeBytes(90 * 1024, 7, true);
    const fs::path folder = temp.makeGameFolder("fresh", &original, nullptr, true);
    const GameInstaller installer(folder, knownOriginalsOf(original));

    require(installer.inspect().proxy == ProxySlot::KnownOriginal, "the original must be recognised by its hash");

    const auto first = installer.install(payloadV1);
    require(first.has_value(), "a fresh install on a known original must succeed");
    require(first->performed == Plan::FreshInstall, "the first install is a fresh install");
    require(first->modsFolderCreated && first->readmeCreated, "the mods folder and readme are created");
    require(readFile(folder / std::string(kProxyName)) == payloadV1, "bink2w32.dll must now be the loader");
    require(readFile(folder / std::string(kBackupName)) == original, "the original must be kept byte for byte");
    require(!fs::exists(folder / std::string(kStagingName)), "no staging file may be left behind");
    require(fs::is_directory(folder / std::string(kModsFolderName)), "the mods folder must exist");
    require(fs::exists(folder / std::string(kModsFolderName) / std::string(kReadmeName)), "the readme must exist");

    writeFile(folder / std::string(kModsFolderName) / std::string(kReadmeName), makeBytes(10, 0, false));
    const auto second = installer.install(payloadV2);
    require(second.has_value(), "an update must succeed");
    require(second->performed == Plan::Update, "the second install is an update");
    require(!second->modsFolderCreated && !second->readmeCreated, "an update creates nothing that exists");
    require(readFile(folder / std::string(kProxyName)) == payloadV2, "bink2w32.dll must be the new loader");
    require(readFile(folder / std::string(kBackupName)) == original, "an update must never touch the backup");
    require(readFile(folder / std::string(kModsFolderName) / std::string(kReadmeName)) == makeBytes(10, 0, false),
            "an existing readme must never be overwritten");
    require(!fs::exists(folder / std::string(kStagingName)), "no staging file may be left after an update");

    const auto third = installer.install(payloadV2);
    require(third.has_value() && readFile(folder / std::string(kBackupName)) == original,
            "installing the same version twice is harmless");
}

/// Verifies that every refusal leaves the folder exactly as it was found.
void testRefusalsChangeNothing(const TempFolder& temp)
{
    const std::vector<std::byte> original = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> stranger = makeBytes(41 * 1024, 11, true);
    const std::vector<std::byte> payload = makeBytes(80 * 1024, 5, true);

    {
        const fs::path folder = temp.makeGameFolder("stranger", &stranger, nullptr, true);
        const GameInstaller installer(folder, knownOriginalsOf(original));
        const auto result = installer.install(payload);
        require(!result.has_value() && result.error().code == InstallErrorCode::ProxyUnrecognized,
                "an unknown bink2w32.dll with no backup must be refused");
        require(readFile(folder / std::string(kProxyName)) == stranger, "a refused install must not touch the file");
        require(!fs::exists(folder / std::string(kBackupName)), "a refused install must not create a backup");
        require(!fs::exists(folder / std::string(kStagingName)), "a refused install must not leave a staging file");
        require(!fs::exists(folder / std::string(kModsFolderName)), "a refused install must not create mods");
    }
    {
        const fs::path folder = temp.makeGameFolder("ours_without_backup", &payload, nullptr, true);
        const GameInstaller installer(folder, knownOriginalsOf(original));
        const auto result = installer.install(payload);
        require(!result.has_value() && result.error().code == InstallErrorCode::ProxyUnrecognized,
                "our own DLL with no backup must never be renamed into the backup slot");
        require(!fs::exists(folder / std::string(kBackupName)), "no backup may be fabricated from our own DLL");
    }
    {
        const fs::path folder = temp.makeGameFolder("no_proxy", nullptr, nullptr, true);
        const GameInstaller installer(folder, knownOriginalsOf(original));
        const auto result = installer.install(payload);
        require(!result.has_value() && result.error().code == InstallErrorCode::ProxyMissing,
                "a missing bink2w32.dll with no backup must be refused");
    }
    {
        const fs::path folder = temp.makeGameFolder("no_exe", &original, nullptr, false);
        const GameInstaller installer(folder, knownOriginalsOf(original));
        const auto result = installer.install(payload);
        require(!result.has_value() && result.error().code == InstallErrorCode::NotAGameFolder,
                "a folder without the game must be refused");
        require(readFile(folder / std::string(kProxyName)) == original, "a non game folder must not be touched");
    }
    {
        const fs::path folder = temp.makeGameFolder("bad_payloads", &original, nullptr, true);
        const GameInstaller installer(folder, knownOriginalsOf(original));
        require(installer.install(original).error().code == InstallErrorCode::PayloadInvalid,
                "installing the game's own DLL as the loader must be refused");
        require(installer.install(makeBytes(100, 1, true)).error().code == InstallErrorCode::PayloadInvalid,
                "a tiny payload must be refused");
        require(installer.install(makeBytes(80 * 1024, 1, false)).error().code == InstallErrorCode::PayloadInvalid,
                "a payload that is not a Windows executable must be refused");
        require(readFile(folder / std::string(kProxyName)) == original, "a bad payload must not touch the folder");
        require(!fs::exists(folder / std::string(kBackupName)), "a bad payload must not create a backup");
    }
}

/// Verifies that a held-open file makes the install fail without losing the original.
void testLockedFilesAreSurvived(const TempFolder& temp)
{
    const std::vector<std::byte> original = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> payloadV1 = makeBytes(80 * 1024, 5, true);
    const std::vector<std::byte> payloadV2 = makeBytes(90 * 1024, 7, true);

    {
        const fs::path folder = temp.makeGameFolder("locked_fresh", &original, nullptr, true);
        const GameInstaller installer(folder, knownOriginalsOf(original));
        HANDLE held = CreateFileW((folder / std::string(kProxyName)).c_str(), GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(held != INVALID_HANDLE_VALUE, "test helper could not hold the file open");
        const auto result = installer.install(payloadV1);
        CloseHandle(held);

        require(!result.has_value(), "a locked original must make the install fail");
        require(result.error().code == InstallErrorCode::FileInUse || result.error().code == InstallErrorCode::AccessDenied,
                "a locked original must be reported as in use or denied");
        require(readFile(folder / std::string(kProxyName)) == original, "the original must be intact after the failure");
        require(!fs::exists(folder / std::string(kBackupName)), "no backup may exist when nothing was renamed");
        require(!fs::exists(folder / std::string(kStagingName)), "the staging file must be cleaned up");
    }
    {
        const fs::path folder = temp.makeGameFolder("locked_update", &payloadV1, &original, true);
        const GameInstaller installer(folder, knownOriginalsOf(original));
        HANDLE held = CreateFileW((folder / std::string(kProxyName)).c_str(), GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(held != INVALID_HANDLE_VALUE, "test helper could not hold the loader open");
        const auto result = installer.install(payloadV2);
        CloseHandle(held);

        require(!result.has_value(), "a locked loader must make the update fail");
        require(readFile(folder / std::string(kProxyName)) == payloadV1, "the loader must be intact after the failure");
        require(readFile(folder / std::string(kBackupName)) == original, "the backup must be intact after the failure");
        require(!fs::exists(folder / std::string(kStagingName)), "the staging file must be cleaned up after an update failure");
    }
}

/// Verifies the Steam library list parser and the game lookup.
void testSteamLocator(const TempFolder& temp)
{
    const std::string vdf =
        "\"libraryfolders\"\n{\n"
        "\t\"0\"\n\t{\n\t\t\"path\"\t\t\"C:\\\\Program Files (x86)\\\\Steam\"\n\t\t\"label\"\t\t\"\"\n"
        "\t\t\"apps\"\n\t\t{\n\t\t\t\"228980\"\t\t\"123\"\n\t\t}\n\t}\n"
        "\t\"1\"\n\t{\n\t\t\"path\"\t\t\"D:\\\\SteamLibrary\"\n\t}\n"
        "}\n";
    const auto libraries = parseLibraryFolders(vdf);
    require(libraries.size() == 2, "two libraries must be parsed");
    require(libraries[0] == fs::path("C:\\Program Files (x86)\\Steam"), "backslash escapes must be undone");
    require(libraries[1] == fs::path("D:\\SteamLibrary"), "the second library must be parsed");
    require(parseLibraryFolders("").empty(), "empty text has no libraries");
    require(parseLibraryFolders("\"path\"").empty(), "a path key with no value is skipped");

    const fs::path library = temp.path() / "steamlib";
    const fs::path game = library / "steamapps" / "common" / std::string(kSteamGameFolderName);
    fs::create_directories(game);
    require(!findGameInLibraries({library}).has_value(), "a library without the executable has no game");
    writeFile(game / std::string(kGameExecutable), makeBytes(16, 1, true));
    require(findGameInLibraries({temp.path() / "nowhere", library}) == game, "the game must be found in the right library");
}

} // namespace

int main()
{
    const TempFolder temp;
    testSha256(temp);
    testPlan();
    testFreshInstallThenUpdate(temp);
    testRefusalsChangeNothing(temp);
    testLockedFilesAreSurvived(temp);
    testSteamLocator(temp);
    std::cout << "[PASS] test_installer" << std::endl;
    return 0;
}
