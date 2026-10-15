/*
** CrabeLoader
** File description:
** Tests how the bootstrap dinput8.dll starts CrabeLoader.asi, using the probe plugins as the loader.
** Every outcome is covered: started, missing, no export, refused by DllMain, crashed in the export.
** Works in a temporary folder; the entry-point patch itself is only checked in game.
**
** Authors: @LucasLhomme
*/

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include <windows.h>

#include "bootstrap/load_loader.hpp"
#include "shared/loader_marker.hpp"

namespace {

using crabe::bootstrap::LoaderStart;
namespace fs = std::filesystem;

/// Asserts that a boolean condition is satisfied or exits immediately.
void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << std::endl;
        std::exit(1);
    }
}

/// Copies a probe into its own folder under a given name, as the bootstrap would find it.
fs::path placeAs(const fs::path& probe, std::string_view folderName, const wchar_t* fileName)
{
    const fs::path folder = fs::temp_directory_path() / std::format("crabe_boot_{}_{}", folderName, GetCurrentProcessId());
    std::error_code error;
    fs::remove_all(folder, error);
    fs::create_directories(folder);
    fs::copy_file(probe, folder / fileName);
    return folder / fileName;
}

/// Reads an environment variable of this process, empty when unset.
std::wstring environment(const wchar_t* name)
{
    std::wstring value(64, L'\0');
    value.resize(GetEnvironmentVariableW(name, value.data(), static_cast<DWORD>(value.size())));
    return value;
}

/// The two descriptions are distinct markers, so the bootstrap never passes for a loader copy.
void testMarkers()
{
    const std::wstring_view loader = crabe::shared::kLoaderDescription;
    const std::wstring_view bootstrap = crabe::shared::kBootstrapDescription;
    require(crabe::shared::containsLoaderMarker(loader.data(), loader.size() * sizeof(wchar_t)), "loader marker");
    require(!crabe::shared::containsLoaderMarker(bootstrap.data(), bootstrap.size() * sizeof(wchar_t)),
            "the bootstrap description is no loader marker");
    require(crabe::shared::containsBootstrapMarker(bootstrap.data(), bootstrap.size() * sizeof(wchar_t)),
            "bootstrap marker");
    require(!crabe::shared::containsBootstrapMarker(loader.data(), loader.size() * sizeof(wchar_t)),
            "the loader description is no bootstrap marker");
}

/// Starts each probe as CrabeLoader.asi and checks the outcome the bootstrap would log.
void testStart(const fs::path& probe, const fs::path& refusingProbe, const fs::path& crashingProbe)
{
    const fs::path missing = fs::temp_directory_path() / L"crabe_boot_none" / crabe::bootstrap::kLoaderFileName;
    const auto absent = crabe::bootstrap::startLoader(missing);
    require(absent.outcome == LoaderStart::Missing, "a missing CrabeLoader.asi is reported, not loaded");
    require(crabe::bootstrap::describe(absent).find("not next to the game") != std::string::npos, "missing text");

    const auto started = crabe::bootstrap::startLoader(placeAs(probe, "ok", crabe::bootstrap::kLoaderFileName));
    require(started.outcome == LoaderStart::Started, "the loader starts and its export is called");
    require(environment(L"CRABE_BOOTSTRAP_CALLED") == L"yes", "CrabeLoaderBootstrap ran");
    require(crabe::bootstrap::describe(started).empty(), "a normal start logs nothing");

    const auto refused = crabe::bootstrap::startLoader(placeAs(refusingProbe, "refused", crabe::bootstrap::kLoaderFileName));
    require(refused.outcome == LoaderStart::Failed && refused.code == ERROR_DLL_INIT_FAILED,
            "a DllMain returning FALSE is a failed load with its Windows error");

    const auto crashed = crabe::bootstrap::startLoader(placeAs(crashingProbe, "crash", crabe::bootstrap::kLoaderFileName));
    require(crashed.outcome == LoaderStart::Crashed && crashed.code == EXCEPTION_ACCESS_VIOLATION,
            "an export that faults is caught with its exception code");

    const fs::path junk = placeAs(refusingProbe, "junk", crabe::bootstrap::kLoaderFileName);
    fs::resize_file(junk, 100);
    const auto broken = crabe::bootstrap::startLoader(junk);
    require(broken.outcome == LoaderStart::Failed, "a broken file is a failed load");
    require(crabe::bootstrap::describe(broken).find("Windows error") != std::string::npos, "failure text");
}

} // namespace

/// Runs every test; the three probe paths come from CTest.
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    testMarkers();
    if (argc >= 4) {
        testStart(argv[1], argv[2], argv[3]);
    } else {
        std::cout << "[SKIP] starts: pass the asi_probe, asi_probe_refuses and asi_probe_init_crash paths" << std::endl;
    }
    std::cout << "[PASS] test_bootstrap" << std::endl;
    return 0;
}
