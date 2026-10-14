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

/// Builds a stand-in CrabeLoader DLL: executable bytes carrying the loader description as UTF-16.
std::vector<std::byte> makeLoader(std::size_t size, unsigned char seed)
{
    std::vector<std::byte> bytes = makeBytes(size, seed, true);
    const auto description = crabe::shared::kLoaderDescription;
    std::memcpy(bytes.data() + size / 2, description.data(), description.size() * sizeof(wchar_t));
    return bytes;
}

/// Writes bytes to a file, replacing it.
void writeFile(const fs::path& path, const std::vector<std::byte>& bytes)
{
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

struct GameFiles {
    const std::vector<std::byte>* loader{nullptr};
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

/// Verifies the marker that tells a CrabeLoader DLL from any other file.
void testLoaderMarker()
{
    const std::vector<std::byte> loader = makeLoader(80 * 1024, 5);
    const std::vector<std::byte> other = makeBytes(80 * 1024, 5, true);
    require(crabe::shared::containsLoaderMarker(loader.data(), loader.size()), "a CrabeLoader DLL carries the marker");
    require(!crabe::shared::containsLoaderMarker(other.data(), other.size()), "another file does not");
    require(!crabe::shared::containsLoaderMarker(nullptr, 100), "no bytes hold no marker");
    require(!crabe::shared::containsLoaderMarker(loader.data(), 10), "a short block holds no marker");
}

/// Verifies which copy runs the loader when two are loaded.
void testLoaderRank()
{
    using crabe::infrastructure::loaderRank;
    require(loaderRank(L"bink2w32.dll") < loaderRank(L"dinput8.dll"), "a 1.x bink2w32.dll copy outranks dinput8.dll");
    require(loaderRank(L"DINPUT8.DLL") == loaderRank(L"dinput8.dll"), "names compare without case");
    require(loaderRank(L"dinput8.dll") < loaderRank(L"CrabeLoader.asi"), "dinput8.dll outranks any other name");
}

/// Verifies the decision table.
void testPlan()
{
    require(planInstall({true, FileKind::Missing, FileKind::Other, false}) == Plan::FreshInstall,
            "a vanilla folder is a fresh install");
    require(planInstall({true, FileKind::Loader, FileKind::Other, false}) == Plan::Update,
            "our own dinput8.dll is an update");
    require(planInstall({true, FileKind::Other, FileKind::Other, false}) == Plan::LoaderSlotTaken,
            "a foreign dinput8.dll is never replaced");
    require(planInstall({true, FileKind::Missing, FileKind::Loader, true}) == Plan::FreshInstall,
            "a 1.x install with its kept original migrates");
    require(planInstall({true, FileKind::Missing, FileKind::Loader, false}) == Plan::LegacyOriginalMissing,
            "a 1.x install whose original is gone is refused");
    require(planInstall({true, FileKind::Missing, FileKind::Missing, false}) == Plan::FreshInstall,
            "a missing bink2w32.dll does not stop the install");
    require(planInstall({false, FileKind::Missing, FileKind::Other, false}) == Plan::NotAGameFolder,
            "a folder without the game executable is refused");

    require(restoresLegacyOriginal({true, FileKind::Missing, FileKind::Loader, true}), "a 1.x proxy gives way to the original");
    require(restoresLegacyOriginal({true, FileKind::Missing, FileKind::Missing, true}), "a missing proxy is restored");
    require(!restoresLegacyOriginal({true, FileKind::Missing, FileKind::Other, true}), "a game file is left alone");
    require(!restoresLegacyOriginal({true, FileKind::Missing, FileKind::Loader, false}), "nothing to restore without a backup");
}

/// Verifies a fresh install, then an update, byte for byte, with the game's own files untouched.
void testFreshInstallThenUpdate(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> payloadV1 = makeLoader(80 * 1024, 5);
    const std::vector<std::byte> payloadV2 = makeLoader(90 * 1024, 7);
    const fs::path folder = temp.makeGameFolder("fresh", {nullptr, &bink, nullptr, true});
    const GameInstaller installer(folder);

    const FolderFacts facts = installer.inspect();
    require(facts.loader == FileKind::Missing && facts.legacyProxy == FileKind::Other, "the facts of a vanilla folder");

    const auto first = installer.install(payloadV1);
    require(first.has_value(), "a fresh install must succeed");
    require(first->performed == Plan::FreshInstall && !first->legacyOriginalRestored, "the first install is a fresh install");
    require(first->modsFolderCreated && first->readmeCreated, "the mods folder and readme are created");
    require(readFile(folder / std::string(kLoaderName)) == payloadV1, "dinput8.dll must now be the loader");
    require(readFile(folder / std::string(kLegacyProxyName)) == bink, "the game's bink2w32.dll is never touched");
    require(!fs::exists(folder / std::string(kLegacyBackupName)), "no backup is made any more");
    require(!fs::exists(folder / std::string(kStagingName)), "no staging file may be left behind");

    writeFile(folder / std::string(kModsFolderName) / std::string(kReadmeName), makeBytes(10, 0, false));
    const auto second = installer.install(payloadV2);
    require(second.has_value() && second->performed == Plan::Update, "the second install is an update");
    require(!second->modsFolderCreated && !second->readmeCreated, "an update creates nothing that exists");
    require(readFile(folder / std::string(kLoaderName)) == payloadV2, "dinput8.dll must be the new loader");
    require(readFile(folder / std::string(kModsFolderName) / std::string(kReadmeName)) == makeBytes(10, 0, false),
            "an existing readme must never be overwritten");

    const auto third = installer.install(payloadV2);
    require(third.has_value() && readFile(folder / std::string(kLegacyProxyName)) == bink,
            "installing the same version twice is harmless");
}

/// Verifies that a 1.x install becomes a dinput8.dll install with the game's own Bink DLL back in place.
void testMigrationFrom1x(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> oldLoader = makeLoader(70 * 1024, 4);
    const std::vector<std::byte> payload = makeLoader(80 * 1024, 5);

    {
        const fs::path folder = temp.makeGameFolder("from_1x", {nullptr, &oldLoader, &bink, true});
        const auto report = GameInstaller(folder).install(payload);
        require(report.has_value() && report->legacyOriginalRestored, "a 1.x install migrates");
        require(readFile(folder / std::string(kLoaderName)) == payload, "dinput8.dll is the loader");
        require(readFile(folder / std::string(kLegacyProxyName)) == bink, "bink2w32.dll is the game's own again");
        require(!fs::exists(folder / std::string(kLegacyBackupName)), "the kept original is no longer a separate file");
        require(!fs::exists(folder / std::string(kLegacyRetiredName)), "the old loader is gone");
    }
    {
        const fs::path folder = temp.makeGameFolder("from_1x_verified", {nullptr, nullptr, &bink, true});
        const auto report = GameInstaller(folder).install(payload);
        require(report.has_value() && report->legacyOriginalRestored, "a missing bink2w32.dll is restored from the backup");
        require(readFile(folder / std::string(kLegacyProxyName)) == bink, "the backup became bink2w32.dll");
    }
    {
        const std::vector<std::byte> steamBink = makeBytes(41 * 1024, 9, true);
        const fs::path folder = temp.makeGameFolder("from_1x_steam", {nullptr, &steamBink, &bink, true});
        const auto report = GameInstaller(folder).install(payload);
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
    const std::vector<std::byte> payload = makeLoader(80 * 1024, 5);

    {
        const fs::path folder = temp.makeGameFolder("foreign_dinput8", {&asiLoader, &bink, nullptr, true});
        const auto result = GameInstaller(folder).install(payload);
        require(!result.has_value() && result.error().code == InstallErrorCode::LoaderSlotTaken,
                "a foreign dinput8.dll must be refused");
        require(readFile(folder / std::string(kLoaderName)) == asiLoader, "a refused install must not touch it");
        require(!fs::exists(folder / std::string(kStagingName)), "a refused install must not leave a staging file");
        require(!fs::exists(folder / std::string(kModsFolderName)), "a refused install must not create mods");
    }
    {
        const fs::path folder = temp.makeGameFolder("old_without_backup", {nullptr, &oldLoader, nullptr, true});
        const auto result = GameInstaller(folder).install(payload);
        require(!result.has_value() && result.error().code == InstallErrorCode::LegacyOriginalMissing,
                "a 1.x install without its original must be refused");
        require(readFile(folder / std::string(kLegacyProxyName)) == oldLoader, "the old loader stays");
        require(!fs::exists(folder / std::string(kLoaderName)), "no dinput8.dll is written");
    }
    {
        const fs::path folder = temp.makeGameFolder("no_exe", {nullptr, &bink, nullptr, false});
        const auto result = GameInstaller(folder).install(payload);
        require(!result.has_value() && result.error().code == InstallErrorCode::NotAGameFolder,
                "a folder without the game must be refused");
        require(!fs::exists(folder / std::string(kLoaderName)), "a non game folder must not be touched");
    }
    {
        const fs::path folder = temp.makeGameFolder("bad_payloads", {nullptr, &bink, nullptr, true});
        const GameInstaller installer(folder);
        require(installer.install(bink).error().code == InstallErrorCode::PayloadInvalid,
                "a DLL that is not CrabeLoader must be refused as the payload");
        require(installer.install(makeBytes(100, 1, true)).error().code == InstallErrorCode::PayloadInvalid,
                "a tiny payload must be refused");
        std::vector<std::byte> notExecutable = makeLoader(80 * 1024, 1);
        notExecutable[0] = std::byte{'X'};
        require(installer.install(notExecutable).error().code == InstallErrorCode::PayloadInvalid,
                "a payload that is not a Windows executable must be refused");
        require(!fs::exists(folder / std::string(kLoaderName)), "a bad payload must not touch the folder");
    }
}

/// Verifies that a held-open file makes the install fail without losing anything.
void testLockedFilesAreSurvived(const TempFolder& temp)
{
    const std::vector<std::byte> bink = makeBytes(40 * 1024, 3, true);
    const std::vector<std::byte> oldLoader = makeLoader(70 * 1024, 4);
    const std::vector<std::byte> payloadV1 = makeLoader(80 * 1024, 5);
    const std::vector<std::byte> payloadV2 = makeLoader(90 * 1024, 7);

    {
        const fs::path folder = temp.makeGameFolder("locked_update", {&payloadV1, &bink, nullptr, true});
        HANDLE held = holdOpen(folder / std::string(kLoaderName));
        const auto result = GameInstaller(folder).install(payloadV2);
        CloseHandle(held);
        require(!result.has_value(), "a loaded dinput8.dll must make the update fail");
        require(result.error().code == InstallErrorCode::FileInUse || result.error().code == InstallErrorCode::AccessDenied,
                "a locked loader must be reported as in use or denied");
        require(readFile(folder / std::string(kLoaderName)) == payloadV1, "the loader must be intact after the failure");
        require(!fs::exists(folder / std::string(kStagingName)), "the staging file must be cleaned up");
    }
    {
        const fs::path folder = temp.makeGameFolder("locked_migration", {nullptr, &oldLoader, &bink, true});
        HANDLE held = holdOpen(folder / std::string(kLegacyProxyName));
        const auto result = GameInstaller(folder).install(payloadV1);
        CloseHandle(held);
        require(!result.has_value(), "a loaded 1.x bink2w32.dll must make the migration fail");
        require(!fs::exists(folder / std::string(kLoaderName)), "the new dinput8.dll is taken back");
        require(readFile(folder / std::string(kLegacyProxyName)) == oldLoader, "the old loader is where it was");
        require(readFile(folder / std::string(kLegacyBackupName)) == bink, "the kept original is intact");
        require(!fs::exists(folder / std::string(kLegacyRetiredName)), "nothing is left half moved");
    }
    {
        const fs::path folder = temp.makeGameFolder("locked_backup", {nullptr, &oldLoader, &bink, true});
        HANDLE held = holdOpen(folder / std::string(kLegacyBackupName));
        const auto result = GameInstaller(folder).install(payloadV1);
        CloseHandle(held);
        require(!result.has_value(), "a held backup must make the migration fail");
        require(!fs::exists(folder / std::string(kLoaderName)), "the new dinput8.dll is taken back");
        require(readFile(folder / std::string(kLegacyProxyName)) == oldLoader, "the old loader was put back");
        require(readFile(folder / std::string(kLegacyBackupName)) == bink, "the kept original is intact");
        require(!fs::exists(folder / std::string(kLegacyRetiredName)), "no retired copy is left behind");
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
    testLoaderMarker();
    testLoaderRank();
    testPlan();
    testFreshInstallThenUpdate(temp);
    testMigrationFrom1x(temp);
    testRefusalsChangeNothing(temp);
    testLockedFilesAreSurvived(temp);
    testSteamLocator(temp);
    std::cout << "[PASS] test_installer" << std::endl;
    return 0;
}
