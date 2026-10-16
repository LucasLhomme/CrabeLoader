#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <windows.h>

#include "infrastructure/loader_instance.hpp"
#include "installer/game_installer.hpp"
#include "installer/install_plan.hpp"
#include "installer/sha256.hpp"
#include "installer/steam_locator.hpp"
#include "shared/loader_marker.hpp"

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

/// Builds deterministic bytes, optionally starting like a Windows executable.
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

/// Builds executable bytes carrying a description as UTF-16, the way a version resource holds it.
std::vector<std::byte> makeMarked(std::size_t size, unsigned char seed, std::wstring_view description)
{
    std::vector<std::byte> bytes = makeBytes(size, seed, true);
    std::memcpy(bytes.data() + size / 2, description.data(), description.size() * sizeof(wchar_t));
    return bytes;
}

/// Builds a stand-in CrabeLoader.asi.
std::vector<std::byte> makeLoader(std::size_t size, unsigned char seed)
{
    return makeMarked(size, seed, crabe::shared::kLoaderDescription);
}

/// Builds a stand-in bootstrap dinput8.dll.
std::vector<std::byte> makeBootstrap(std::size_t size, unsigned char seed)
{
    return makeMarked(size, seed, crabe::shared::kBootstrapDescription);
}

struct Release {
    std::vector<std::byte> loader;
    std::vector<std::byte> bootstrap;

    [[nodiscard]] InstallPayload payload() const { return {loader, bootstrap}; }
};

/// Builds the two files of one release.
Release makeRelease(unsigned char seed)
{
    return {makeLoader(80 * 1024 + seed, seed), makeBootstrap(8 * 1024 + seed, seed)};
}

/// Writes bytes to a file, replacing it.
void writeFile(const fs::path& path, const std::vector<std::byte>& bytes)
{
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(static_cast<bool>(stream), "test helper could not write a file");
}

/// Reads a whole file.
std::vector<std::byte> readFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    const std::vector<char> raw((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes(raw.size());
    std::ranges::transform(raw, bytes.begin(), [](char c) { return static_cast<std::byte>(c); });
    return bytes;
}

/// Opens a file the way a running game holds a loaded DLL, so it cannot be renamed.
HANDLE holdOpen(const fs::path& path)
{
    HANDLE held = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    require(held != INVALID_HANDLE_VALUE, "test helper could not hold a file open");
    return held;
}

/// Whether the installer left a staging or retired file behind.
bool hasLeftovers(const fs::path& folder)
{
    for (const auto& entry : fs::directory_iterator(folder)) {
        const std::string name = entry.path().filename().string();
        if (name.ends_with(kStagingSuffix) || name.ends_with(kRetiredSuffix))
            return true;
    }
    return false;
}

struct GameFiles {
    const std::vector<std::byte>* loader{nullptr};
    const std::vector<std::byte>* bootstrap{nullptr};
    const std::vector<std::byte>* legacyProxy{nullptr};
    const std::vector<std::byte>* legacyBackup{nullptr};
    bool withExecutable{true};
};

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

    /// Creates a game folder holding the given files.
    [[nodiscard]] fs::path makeGameFolder(const std::string& name, const GameFiles& files) const
    {
        const fs::path folder = _path / name;
        fs::create_directories(folder);
        if (files.withExecutable)
            writeFile(folder / std::string(kGameExecutable), makeBytes(2048, 1, true));
        if (files.loader)
            writeFile(folder / std::string(kLoaderName), *files.loader);
        if (files.bootstrap)
            writeFile(folder / std::string(kBootstrapName), *files.bootstrap);
        if (files.legacyProxy)
            writeFile(folder / std::string(kLegacyProxyName), *files.legacyProxy);
        if (files.legacyBackup)
            writeFile(folder / std::string(kLegacyBackupName), *files.legacyBackup);
        return folder;
    }

private:
    fs::path _path;
};

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

/// Verifies the two markers that tell CrabeLoader.asi and its bootstrap from any other file, and each other.
void testMarkers()
{
    const std::vector<std::byte> loader = makeLoader(80 * 1024, 5);
    const std::vector<std::byte> bootstrap = makeBootstrap(8 * 1024, 5);
    const std::vector<std::byte> other = makeBytes(80 * 1024, 5, true);
    require(crabe::shared::containsLoaderMarker(loader.data(), loader.size()), "CrabeLoader carries the loader marker");
    require(!crabe::shared::containsLoaderMarker(bootstrap.data(), bootstrap.size()), "the bootstrap does not");
    require(crabe::shared::containsBootstrapMarker(bootstrap.data(), bootstrap.size()), "the bootstrap has its own");
    require(!crabe::shared::containsBootstrapMarker(loader.data(), loader.size()), "a loader is no bootstrap");
    require(!crabe::shared::containsLoaderMarker(other.data(), other.size()), "another file has no loader marker");
    require(!crabe::shared::containsBootstrapMarker(other.data(), other.size()), "nor a bootstrap marker");
    require(!crabe::shared::containsLoaderMarker(nullptr, 100), "no bytes hold no marker");
    require(!crabe::shared::containsLoaderMarker(loader.data(), 10), "a short block holds no marker");
}

/// Verifies which copy runs the loader when two are loaded.
void testLoaderRank()
{
    using crabe::infrastructure::loaderRank;
    require(loaderRank(L"bink2w32.dll") < loaderRank(L"dinput8.dll"), "a 1.x bink2w32.dll copy outranks dinput8.dll");
    require(loaderRank(L"DINPUT8.DLL") == loaderRank(L"dinput8.dll"), "names compare without case");
    require(loaderRank(L"dinput8.dll") < loaderRank(L"CrabeLoader.asi"), "a 1.1 development dinput8.dll outranks the .asi");
}

/// Verifies the decision table.
void testPlan()
{
    using K = FileKind;
    require(planInstall({true, K::Missing, K::Missing, K::Other, false}) == Plan::FreshInstall,
            "a vanilla folder is a fresh install");
    require(planInstall({true, K::Loader, K::Bootstrap, K::Other, false}) == Plan::Update,
            "our own two files are an update");
    require(planInstall({true, K::Missing, K::Loader, K::Other, false}) == Plan::Update,
            "a 1.1 development dinput8.dll (the whole loader) is an update");
    require(planInstall({true, K::Loader, K::Missing, K::Other, false}) == Plan::Update,
            "a CrabeLoader.asi without its bootstrap is an update");
    require(planInstall({true, K::Missing, K::Other, K::Other, false}) == Plan::BootstrapSlotTaken,
            "a foreign dinput8.dll is never replaced");
    require(planInstall({true, K::Other, K::Missing, K::Other, false}) == Plan::LoaderSlotTaken,
            "a foreign CrabeLoader.asi is never replaced");
    require(planInstall({true, K::Bootstrap, K::Missing, K::Other, false}) == Plan::LoaderSlotTaken,
            "a bootstrap renamed CrabeLoader.asi is not taken for the loader");
    require(planInstall({true, K::Missing, K::Missing, K::Loader, true}) == Plan::FreshInstall,
            "a 1.x install with its kept original migrates");
    require(planInstall({true, K::Missing, K::Missing, K::Loader, false}) == Plan::LegacyOriginalMissing,
            "a 1.x install whose original is gone is refused");
    require(planInstall({true, K::Missing, K::Missing, K::Missing, false}) == Plan::FreshInstall,
            "a missing bink2w32.dll does not stop the install");
    require(planInstall({false, K::Missing, K::Missing, K::Other, false}) == Plan::NotAGameFolder,
            "a folder without the game executable is refused");

    require(restoresLegacyOriginal({true, K::Missing, K::Missing, K::Loader, true}), "a 1.x proxy gives way to the original");
    require(restoresLegacyOriginal({true, K::Missing, K::Missing, K::Missing, true}), "a missing proxy is restored");
    require(!restoresLegacyOriginal({true, K::Missing, K::Missing, K::Other, true}), "a game file is left alone");
    require(!restoresLegacyOriginal({true, K::Missing, K::Missing, K::Loader, false}), "nothing to restore without a backup");
}

/// Verifies a fresh install, then an update, byte for byte, with the game's own files untouched.
void testFreshInstallThenUpdate(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const Release v1 = makeRelease(5);
    const Release v2 = makeRelease(7);
    const fs::path folder = temp.makeGameFolder("fresh", {nullptr, nullptr, &bink, nullptr, true});
    const GameInstaller installer(folder);

    const FolderFacts facts = installer.inspect();
    require(facts.loader == FileKind::Missing && facts.bootstrap == FileKind::Missing &&
            facts.legacyProxy == FileKind::Other, "the facts of a vanilla folder");

    const auto first = installer.install(v1.payload());
    require(first.has_value(), "a fresh install must succeed");
    require(first->performed == Plan::FreshInstall && !first->legacyOriginalRestored, "the first install is a fresh install");
    require(first->modsFolderCreated && first->readmeCreated, "the mods folder and readme are created");
    require(readFile(folder / std::string(kLoaderName)) == v1.loader, "CrabeLoader.asi is in place");
    require(readFile(folder / std::string(kBootstrapName)) == v1.bootstrap, "dinput8.dll is the bootstrap");
    require(readFile(folder / std::string(kLegacyProxyName)) == bink, "the game's bink2w32.dll is never touched");
    require(!fs::exists(folder / std::string(kLegacyBackupName)), "no backup is made");
    require(!hasLeftovers(folder), "no staging or retired file may be left behind");

    const FolderFacts installed = installer.inspect();
    require(installed.loader == FileKind::Loader && installed.bootstrap == FileKind::Bootstrap,
            "both installed files are recognised");

    writeFile(folder / std::string(kModsFolderName) / std::string(kReadmeName), makeBytes(10, 0, false));
    const auto second = installer.install(v2.payload());
    require(second.has_value() && second->performed == Plan::Update, "the second install is an update");
    require(!second->modsFolderCreated && !second->readmeCreated, "an update creates nothing that exists");
    require(readFile(folder / std::string(kLoaderName)) == v2.loader, "CrabeLoader.asi is the new one");
    require(readFile(folder / std::string(kBootstrapName)) == v2.bootstrap, "dinput8.dll is the new bootstrap");
    require(readFile(folder / std::string(kModsFolderName) / std::string(kReadmeName)) == makeBytes(10, 0, false),
            "an existing readme must never be overwritten");
    require(!hasLeftovers(folder), "an update leaves nothing behind");

    const auto third = installer.install(v2.payload());
    require(third.has_value() && readFile(folder / std::string(kLegacyProxyName)) == bink,
            "installing the same version twice is harmless");
}

/// Verifies that a 1.1 development install, where dinput8.dll was the whole loader, becomes the two files.
void testUpgradeFromSingleFile(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> singleFile = makeLoader(90 * 1024, 2);
    const Release release = makeRelease(5);
    const fs::path folder = temp.makeGameFolder("single_file", {nullptr, &singleFile, &bink, nullptr, true});

    const auto report = GameInstaller(folder).install(release.payload());
    require(report.has_value() && report->performed == Plan::Update, "the single-file install is updated");
    require(readFile(folder / std::string(kBootstrapName)) == release.bootstrap, "dinput8.dll is now the bootstrap");
    require(readFile(folder / std::string(kLoaderName)) == release.loader, "CrabeLoader.asi was added");
    require(!hasLeftovers(folder), "the old single file is not left behind");
}

/// Verifies that a 1.x install becomes the two-file install with the game's own Bink DLL back in place.
void testMigrationFrom1x(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> oldLoader = makeLoader(70 * 1024, 4);
    const Release release = makeRelease(5);

    {
        const fs::path folder = temp.makeGameFolder("from_1x", {nullptr, nullptr, &oldLoader, &bink, true});
        const auto report = GameInstaller(folder).install(release.payload());
        require(report.has_value() && report->legacyOriginalRestored, "a 1.x install migrates");
        require(readFile(folder / std::string(kLoaderName)) == release.loader, "CrabeLoader.asi is in place");
        require(readFile(folder / std::string(kBootstrapName)) == release.bootstrap, "dinput8.dll is the bootstrap");
        require(readFile(folder / std::string(kLegacyProxyName)) == bink, "bink2w32.dll is the game's own again");
        require(!fs::exists(folder / std::string(kLegacyBackupName)), "the kept original is no longer a separate file");
        require(!fs::exists(folder / std::string(kLegacyRetiredName)), "the old loader is gone");
        require(!hasLeftovers(folder), "nothing is left behind");
    }
    {
        const fs::path folder = temp.makeGameFolder("from_1x_verified", {nullptr, nullptr, nullptr, &bink, true});
        const auto report = GameInstaller(folder).install(release.payload());
        require(report.has_value() && report->legacyOriginalRestored, "a missing bink2w32.dll is restored from the backup");
        require(readFile(folder / std::string(kLegacyProxyName)) == bink, "the backup became bink2w32.dll");
    }
    {
        const std::vector<std::byte> steamBink = makeBytes(41 * 1024, 9, true);
        const fs::path folder = temp.makeGameFolder("from_1x_steam", {nullptr, nullptr, &steamBink, &bink, true});
        const auto report = GameInstaller(folder).install(release.payload());
        require(report.has_value() && !report->legacyOriginalRestored, "a game bink2w32.dll beside an old backup stays");
        require(readFile(folder / std::string(kLegacyProxyName)) == steamBink, "the game's file is untouched");
        require(readFile(folder / std::string(kLegacyBackupName)) == bink, "and so is the old backup");
    }
}

/// Verifies that every refusal leaves the folder exactly as it was found.
void testRefusalsChangeNothing(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> asiLoader = makeBytes(41 * 1024, 11, true);
    const std::vector<std::byte> oldLoader = makeLoader(70 * 1024, 4);
    const Release release = makeRelease(5);

    {
        const fs::path folder = temp.makeGameFolder("foreign_dinput8", {nullptr, &asiLoader, &bink, nullptr, true});
        const auto result = GameInstaller(folder).install(release.payload());
        require(!result.has_value() && result.error().code == InstallErrorCode::BootstrapSlotTaken,
                "a foreign dinput8.dll must be refused");
        require(readFile(folder / std::string(kBootstrapName)) == asiLoader, "a refused install must not touch it");
        require(!fs::exists(folder / std::string(kLoaderName)), "no CrabeLoader.asi is written");
        require(!hasLeftovers(folder), "a refused install must not leave a staging file");
        require(!fs::exists(folder / std::string(kModsFolderName)), "a refused install must not create mods");
    }
    {
        const fs::path folder = temp.makeGameFolder("foreign_asi", {&asiLoader, nullptr, &bink, nullptr, true});
        const auto result = GameInstaller(folder).install(release.payload());
        require(!result.has_value() && result.error().code == InstallErrorCode::LoaderSlotTaken,
                "a foreign CrabeLoader.asi must be refused");
        require(readFile(folder / std::string(kLoaderName)) == asiLoader, "and left untouched");
        require(!fs::exists(folder / std::string(kBootstrapName)), "no dinput8.dll is written");
    }
    {
        const fs::path folder = temp.makeGameFolder("old_without_backup", {nullptr, nullptr, &oldLoader, nullptr, true});
        const auto result = GameInstaller(folder).install(release.payload());
        require(!result.has_value() && result.error().code == InstallErrorCode::LegacyOriginalMissing,
                "a 1.x install without its original must be refused");
        require(readFile(folder / std::string(kLegacyProxyName)) == oldLoader, "the old loader stays");
        require(!fs::exists(folder / std::string(kLoaderName)) && !fs::exists(folder / std::string(kBootstrapName)),
                "nothing is written");
    }
    {
        const fs::path folder = temp.makeGameFolder("no_exe", {nullptr, nullptr, &bink, nullptr, false});
        const auto result = GameInstaller(folder).install(release.payload());
        require(!result.has_value() && result.error().code == InstallErrorCode::NotAGameFolder,
                "a folder without the game must be refused");
        require(!fs::exists(folder / std::string(kLoaderName)), "a non game folder must not be touched");
    }
    {
        const fs::path folder = temp.makeGameFolder("bad_payloads", {nullptr, nullptr, &bink, nullptr, true});
        const GameInstaller installer(folder);
        const auto refused = [&](const std::vector<std::byte>& loader, const std::vector<std::byte>& bootstrap) {
            const auto result = installer.install({loader, bootstrap});
            return !result.has_value() && result.error().code == InstallErrorCode::PayloadInvalid;
        };
        require(refused(bink, release.bootstrap), "a DLL that is not CrabeLoader must be refused as the loader");
        require(refused(release.loader, bink), "a DLL that is not the bootstrap must be refused as the bootstrap");
        require(refused(release.bootstrap, release.loader), "the two files swapped must be refused");
        require(refused(makeBytes(100, 1, true), release.bootstrap), "a tiny loader must be refused");
        require(refused(release.loader, makeBytes(100, 1, true)), "a tiny bootstrap must be refused");
        std::vector<std::byte> notExecutable = makeLoader(80 * 1024, 1);
        notExecutable[0] = std::byte{'X'};
        require(refused(notExecutable, release.bootstrap), "a loader that is not a Windows executable must be refused");
        require(!fs::exists(folder / std::string(kLoaderName)) && !fs::exists(folder / std::string(kBootstrapName)),
                "a bad payload must not touch the folder");
    }
}

/// Verifies that a held-open file makes the install fail without losing anything.
void testLockedFilesAreSurvived(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> oldLoader = makeLoader(70 * 1024, 4);
    const Release v1 = makeRelease(5);
    const Release v2 = makeRelease(7);
    const auto inUse = [](const auto& result) {
        return !result.has_value() && (result.error().code == InstallErrorCode::FileInUse ||
                                       result.error().code == InstallErrorCode::AccessDenied ||
                                       result.error().code == InstallErrorCode::IoFailure);
    };

    for (const std::string_view locked : {kLoaderName, kBootstrapName}) {
        const fs::path folder = temp.makeGameFolder("locked_update_" + std::string(locked),
                                                    {&v1.loader, &v1.bootstrap, &bink, nullptr, true});
        HANDLE held = holdOpen(folder / std::string(locked));
        const auto result = GameInstaller(folder).install(v2.payload());
        CloseHandle(held);
        require(inUse(result), "a file in use must make the update fail");
        require(readFile(folder / std::string(kLoaderName)) == v1.loader, "CrabeLoader.asi is intact after the failure");
        require(readFile(folder / std::string(kBootstrapName)) == v1.bootstrap, "dinput8.dll is intact after the failure");
        require(!hasLeftovers(folder), "staging and retired files are cleaned up");
    }
    for (const std::string_view locked : {kLegacyProxyName, kLegacyBackupName}) {
        const fs::path folder = temp.makeGameFolder("locked_migration_" + std::string(locked),
                                                    {nullptr, nullptr, &oldLoader, &bink, true});
        HANDLE held = holdOpen(folder / std::string(locked));
        const auto result = GameInstaller(folder).install(v1.payload());
        CloseHandle(held);
        require(!result.has_value(), "a held 1.x file must make the migration fail");
        require(!fs::exists(folder / std::string(kLoaderName)), "the new CrabeLoader.asi is taken back");
        require(!fs::exists(folder / std::string(kBootstrapName)), "the new dinput8.dll is taken back");
        require(readFile(folder / std::string(kLegacyProxyName)) == oldLoader, "the old loader is where it was");
        require(readFile(folder / std::string(kLegacyBackupName)) == bink, "the kept original is intact");
        require(!fs::exists(folder / std::string(kLegacyRetiredName)), "nothing is left half moved");
        require(!hasLeftovers(folder), "no staging file is left behind");
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
    testMarkers();
    testLoaderRank();
    testPlan();
    testFreshInstallThenUpdate(temp);
    testUpgradeFromSingleFile(temp);
    testMigrationFrom1x(temp);
    testRefusalsChangeNothing(temp);
    testLockedFilesAreSurvived(temp);
    testSteamLocator(temp);
    std::cout << "[PASS] test_installer" << std::endl;
    return 0;
}
