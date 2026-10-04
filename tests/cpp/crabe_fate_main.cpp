/*
** CrabeLoader
** File description:
** CRABE-FATE: Automated Regression & Golden Reference Test Suite.
** Inspired by FFmpeg's FATE (FFmpeg Automated Testing Environment).
** Executes real sample mods and compares outputs against golden .ref files.
**
** Authors: @LucasLhomme
*/

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "domain/dependency_resolver.hpp"
#include "domain/mod_manifest.hpp"
#include "domain/semver.hpp"
#include "infrastructure/vfs_override_manager.hpp"

namespace {

namespace fs = std::filesystem;

/// Color codes for console output.
namespace color {
    constexpr std::string_view reset   = "\033[0m";
    constexpr std::string_view bold    = "\033[1m";
    constexpr std::string_view red     = "\033[31m";
    constexpr std::string_view green   = "\033[32m";
    constexpr std::string_view yellow  = "\033[33m";
    constexpr std::string_view cyan    = "\033[36m";
} // namespace color

/// Locates the repository root by searching upward for tests/fate/samples.
[[nodiscard]] fs::path findRepoRoot(const fs::path& startDir)
{
    fs::path current = fs::absolute(startDir);
    for (int i = 0; i < 6; ++i) {
        if (fs::exists(current / "tests" / "fate" / "samples")) {
            return current;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    return startDir;
}

/// Normalizes line endings to LF (\n) and trims trailing whitespace.
[[nodiscard]] std::string normalizeOutput(std::string_view input)
{
    std::string result;
    result.reserve(input.size());

    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\r') {
            continue; // Skip CR
        }
        result.push_back(input[i]);
    }
    while (!result.empty() && (result.back() == '\n' || result.back() == ' ' || result.back() == '\t')) {
        result.pop_back();
    }
    result.push_back('\n');
    return result;
}

/// Splits a string into lines for diff computation.
[[nodiscard]] std::vector<std::string> splitLines(std::string_view text)
{
    std::vector<std::string> lines;
    std::istringstream stream{std::string(text)};
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

/// Reads the entire contents of a file into a string.
[[nodiscard]] std::string readFile(const fs::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

/// Writes text to a file, replacing its content.
void writeFile(const fs::path& filePath, std::string_view content)
{
    fs::create_directories(filePath.parent_path());
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    file << content;
}

// ---------------------------------------------------------------------------
// Individual Test Case Implementations
// ---------------------------------------------------------------------------

std::string runTest01Minimal(const fs::path& repoRoot)
{
    std::ostringstream out;
    const fs::path sampleDir = repoRoot / "tests" / "fate" / "samples" / "01_minimal";
    const fs::path manifestPath = sampleDir / "mod.json";

    auto manifestResult = crabe::domain::ModManifest::load(manifestPath);
    if (!manifestResult) {
        out << "ERROR: Failed to load manifest: " << manifestResult.error().what() << "\n";
        return out.str();
    }

    const auto& manifest = *manifestResult;
    out << "TEST: 01_minimal\n";
    out << "MANIFEST_STATE: " << (manifest.isValid() ? "Valid" : "NotValid") << "\n";
    out << "MANIFEST_VERSION: " << manifest.getManifestVersion() << "\n";
    out << "ID: " << manifest.getId() << "\n";
    out << "NAME: " << manifest.getName() << "\n";
    out << "VERSION: " << manifest.getVersion() << "\n";
    out << "ENTRY: " << manifest.getEntry() << "\n";

    // Resolve order
    std::vector<crabe::domain::ModManifest> manifests;
    manifests.push_back(manifest);

    auto resolution = crabe::domain::resolve(manifests, crabe::domain::SemVer(0, 2, 0));
    out << "RESOLVE_STATUS: " << (resolution.rejected.empty() ? "OK" : "REJECTED") << "\n";
    out << "LOAD_ORDER:\n";
    for (std::size_t i = 0; i < resolution.loadOrder.size(); ++i) {
        out << "  " << (i + 1) << ". " << resolution.loadOrder[i] << " (" << manifest.getVersion() << ")\n";
    }

    return out.str();
}

std::string runTest02Apostrophe(const fs::path& repoRoot)
{
    std::ostringstream out;
    const fs::path sampleDir = repoRoot / "tests" / "fate" / "samples" / "02_apostrophe";
    const fs::path modDir = sampleDir / "Bob's Mod";
    const fs::path manifestPath = modDir / "mod.json";

    auto manifestResult = crabe::domain::ModManifest::load(manifestPath);
    if (!manifestResult) {
        out << "ERROR: Failed to load manifest: " << manifestResult.error().what() << "\n";
        return out.str();
    }

    const auto& manifest = *manifestResult;
    out << "TEST: 02_apostrophe\n";
    out << "MANIFEST_STATE: " << (manifest.isValid() ? "Valid" : "NotValid") << "\n";
    out << "ID: " << manifest.getId() << "\n";
    out << "NAME: " << manifest.getName() << "\n";

    // VFS Scan
    auto& vfs = crabe::infrastructure::VfsOverrideManager::get();
    vfs.clear();
    vfs.scanModsDirectory(sampleDir);

    fs::path resolvedP3d;
    bool okP3d = vfs.resolve("characters/bob.p3d", resolvedP3d);
    out << "VFS_RESOLVE_P3D: " << (okP3d ? "OK" : "FAILED") << "\n";

    fs::path resolvedAssets;
    bool okAssets = vfs.resolve("assets\\characters\\bob.p3d", resolvedAssets);
    out << "VFS_RESOLVE_ASSETS_PREFIX: " << (okAssets ? "OK" : "FAILED") << "\n";

    std::vector<crabe::domain::ModManifest> manifests;
    manifests.push_back(manifest);
    auto resolution = crabe::domain::resolve(manifests, crabe::domain::SemVer(0, 2, 0));
    out << "RESOLVE_STATUS: " << (resolution.rejected.empty() ? "OK" : "REJECTED") << "\n";
    out << "LOAD_ORDER:\n";
    for (std::size_t i = 0; i < resolution.loadOrder.size(); ++i) {
        out << "  " << (i + 1) << ". " << resolution.loadOrder[i] << " (" << manifest.getVersion() << ")\n";
    }

    return out.str();
}

std::string runTest03DepChain(const fs::path& repoRoot)
{
    std::ostringstream out;
    const fs::path sampleDir = repoRoot / "tests" / "fate" / "samples" / "03_dep_chain";

    std::vector<crabe::domain::ModManifest> manifests;
    for (const auto& entry : fs::directory_iterator(sampleDir)) {
        if (entry.is_directory()) {
            const fs::path manifestPath = entry.path() / "mod.json";
            auto m = crabe::domain::ModManifest::load(manifestPath);
            if (m && m->isValid()) {
                manifests.push_back(std::move(*m));
            }
        }
    }

    auto resolution = crabe::domain::resolve(manifests, crabe::domain::SemVer(0, 2, 0));

    out << "TEST: 03_dep_chain\n";
    out << "LOAD_ORDER_COUNT: " << resolution.loadOrder.size() << "\n";
    out << "REJECTED_COUNT: " << resolution.rejected.size() << "\n";
    out << "LOAD_ORDER:\n";
    for (std::size_t i = 0; i < resolution.loadOrder.size(); ++i) {
        out << "  " << (i + 1) << ". " << resolution.loadOrder[i] << "\n";
    }

    return out.str();
}

std::string runTest04Circular(const fs::path& repoRoot)
{
    std::ostringstream out;
    const fs::path sampleDir = repoRoot / "tests" / "fate" / "samples" / "04_circular_error";

    std::vector<crabe::domain::ModManifest> manifests;
    for (const auto& entry : fs::directory_iterator(sampleDir)) {
        if (entry.is_directory()) {
            const fs::path manifestPath = entry.path() / "mod.json";
            auto m = crabe::domain::ModManifest::load(manifestPath);
            if (m && m->isValid()) {
                manifests.push_back(std::move(*m));
            }
        }
    }

    auto resolution = crabe::domain::resolve(manifests, crabe::domain::SemVer(0, 2, 0));

    out << "TEST: 04_circular_error\n";
    out << "LOAD_ORDER_COUNT: " << resolution.loadOrder.size() << "\n";
    out << "REJECTED_COUNT: " << resolution.rejected.size() << "\n";
    out << "REJECTIONS:\n";
    for (const auto& rej : resolution.rejected) {
        out << "  - id: " << rej.id << ", reason: "
            << (rej.reason == crabe::domain::Rejection::CyclicDependency ? "CyclicDependency" : "Other")
            << "\n";
    }

    return out.str();
}

std::string runTest05VfsTree(const fs::path& repoRoot)
{
    std::ostringstream out;
    const fs::path sampleDir = repoRoot / "tests" / "fate" / "samples" / "05_vfs_tree";

    auto& vfs = crabe::infrastructure::VfsOverrideManager::get();
    vfs.clear();
    vfs.scanModsDirectory(sampleDir);

    out << "TEST: 05_vfs_tree\n";
    out << "TOTAL_OVERRIDES: " << vfs.getOverrideCount() << "\n";

    fs::path resolvedSora;
    if (vfs.resolve("characters/sora.p3d", resolvedSora)) {
        out << "RESOLVE_SORA_ORIGIN: " << (resolvedSora.string().contains("override_pack") ? "override_pack" : "base_pack") << "\n";
    } else {
        out << "RESOLVE_SORA_ORIGIN: NOT_FOUND\n";
    }

    fs::path resolvedHud;
    if (vfs.resolve("textures/hud.tx", resolvedHud)) {
        out << "RESOLVE_HUD_ORIGIN: " << (resolvedHud.string().contains("base_pack") ? "base_pack" : "override_pack") << "\n";
    } else {
        out << "RESOLVE_HUD_ORIGIN: NOT_FOUND\n";
    }

    fs::path resolvedMenu;
    if (vfs.resolve("/assets/ui/screens/menu.tx", resolvedMenu)) {
        out << "RESOLVE_MENU_ORIGIN: " << (resolvedMenu.string().contains("base_pack") ? "base_pack" : "override_pack") << "\n";
    } else {
        out << "RESOLVE_MENU_ORIGIN: NOT_FOUND\n";
    }

    fs::path resolvedTbody;
    if (vfs.resolve("characters/sor_sora.tbody", resolvedTbody)) {
        out << "RESOLVE_TBODY_ORIGIN: " << (resolvedTbody.string().contains("override_pack") ? "override_pack" : "other") << "\n";
    } else {
        out << "RESOLVE_TBODY_ORIGIN: NOT_FOUND\n";
    }

    fs::path resolvedCase;
    bool caseOk = vfs.resolve("ASSETS\\TEXTURES\\SORA.TX", resolvedCase);
    out << "RESOLVE_CASE_INSENSITIVE: " << (caseOk ? "OK" : "FAILED") << "\n";

    fs::path nonExistent;
    bool noneOk = vfs.resolve("characters/mickey.p3d", nonExistent);
    out << "RESOLVE_NON_EXISTENT: " << (!noneOk ? "NOT_FOUND" : "FOUND") << "\n";

    return out.str();
}

std::string runTest06Malformed(const fs::path& repoRoot)
{
    std::ostringstream out;
    const fs::path sampleDir = repoRoot / "tests" / "fate" / "samples" / "06_malformed_json";
    const fs::path manifestPath = sampleDir / "broken_mod" / "mod.json";

    auto manifestResult = crabe::domain::ModManifest::load(manifestPath);

    out << "TEST: 06_malformed_json\n";
    if (!manifestResult) {
        const auto& diag = manifestResult.error();
        out << "MANIFEST_STATE: Malformed\n";
        out << "ERROR_CODE: " << (diag.code == crabe::domain::ManifestError::InvalidJson ? "InvalidJson" : "Other") << "\n";
        out << "IS_ABSENT: " << (diag.isAbsent() ? "true" : "false") << "\n";
        out << "HAS_DIAGNOSTIC: true\n";
    } else {
        out << "MANIFEST_STATE: UnexpectedValid\n";
    }

    return out.str();
}

// ---------------------------------------------------------------------------
// Test Runner Harness
// ---------------------------------------------------------------------------

struct TestCase {
    std::string name;
    std::string (*runner)(const fs::path& repoRoot);
};

const std::vector<TestCase> kTestCases = {
    {"01_minimal",       &runTest01Minimal},
    {"02_apostrophe",    &runTest02Apostrophe},
    {"03_dep_chain",     &runTest03DepChain},
    {"04_circular_error",&runTest04Circular},
    {"05_vfs_tree",      &runTest05VfsTree},
    {"06_malformed_json",&runTest06Malformed},
};

} // namespace

int main(int argc, char* argv[])
{
    bool generateRefs = false;
    fs::path customRoot;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--gen" || arg == "-g" || arg == "GEN=1") {
            generateRefs = true;
        } else if (arg == "--root" && i + 1 < argc) {
            customRoot = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: crabe_fate [OPTIONS]\n"
                      << "  --gen, -g      Regenerate reference golden files (.ref)\n"
                      << "  --root <path>  Specify CrabeLoader repository root path\n"
                      << "  --help, -h     Show this help message\n";
            return 0;
        }
    }

    const fs::path repoRoot = customRoot.empty() ? findRepoRoot(fs::current_path()) : fs::absolute(customRoot);
    const fs::path refDir = repoRoot / "tests" / "fate" / "ref";

    std::cout << color::bold << color::cyan
              << "============================================================\n"
              << "       CRABE-FATE: Automated Regression Test Suite          \n"
              << "============================================================"
              << color::reset << "\n";
    std::cout << "Repository root: " << repoRoot.string() << "\n";
    if (generateRefs) {
        std::cout << color::yellow << "[MODE] Generating / Updating Golden Reference Files...\n" << color::reset;
    }

    int passedCount = 0;
    int failedCount = 0;
    auto globalStart = std::chrono::high_resolution_clock::now();

    for (const auto& test : kTestCases) {
        auto testStart = std::chrono::high_resolution_clock::now();
        std::string rawOutput = test.runner(repoRoot);
        std::string normalizedOutput = normalizeOutput(rawOutput);
        auto testEnd = std::chrono::high_resolution_clock::now();
        double elapsedUs = std::chrono::duration<double, std::micro>(testEnd - testStart).count();

        const fs::path refFile = refDir / (test.name + ".ref");

        if (generateRefs) {
            writeFile(refFile, normalizedOutput);
            std::cout << color::green << "  [GEN]  " << color::reset
                      << "fate-" << std::left << std::setw(22) << test.name
                      << " -> " << refFile.filename().string() << "\n";
            passedCount++;
            continue;
        }

        if (!fs::exists(refFile)) {
            std::cout << color::red << "  [MISS] " << color::reset
                      << "fate-" << std::left << std::setw(22) << test.name
                      << " (Missing .ref file: " << refFile.string() << ")\n";
            failedCount++;
            continue;
        }

        std::string expected = normalizeOutput(readFile(refFile));

        if (normalizedOutput == expected) {
            std::cout << color::green << "  [PASS] " << color::reset
                      << "fate-" << std::left << std::setw(22) << test.name
                      << " (" << std::fixed << std::setprecision(1) << elapsedUs << " us)\n";
            passedCount++;
        } else {
            std::cout << color::red << "  [FAIL] " << color::reset
                      << "fate-" << std::left << std::setw(22) << test.name
                      << color::red << " [MISMATCH]\n" << color::reset;

            auto actualLines = splitLines(normalizedOutput);
            auto expectedLines = splitLines(expected);

            std::cout << color::yellow << "  --- Expected vs Actual Diff ---\n" << color::reset;
            std::size_t maxLines = std::max(actualLines.size(), expectedLines.size());
            for (std::size_t l = 0; l < maxLines; ++l) {
                std::string exp = (l < expectedLines.size()) ? expectedLines[l] : "<EOF>";
                std::string act = (l < actualLines.size()) ? actualLines[l] : "<EOF>";
                if (exp != act) {
                    std::cout << color::red << "  - [" << (l + 1) << "] " << exp << "\n"
                              << color::green << "  + [" << (l + 1) << "] " << act << "\n" << color::reset;
                }
            }
            failedCount++;
        }
    }

    auto globalEnd = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(globalEnd - globalStart).count();

    std::cout << color::bold << color::cyan
              << "------------------------------------------------------------\n"
              << color::reset;

    if (failedCount == 0) {
        std::cout << color::bold << color::green
                  << "  SUCCESS: All " << passedCount << " test(s) passed in "
                  << std::fixed << std::setprecision(2) << totalMs << " ms.\n"
                  << color::reset;
        std::cout << color::bold << color::cyan
                  << "============================================================\n"
                  << color::reset;
        return 0;
    }

    std::cout << color::bold << color::red
              << "  FAILURE: " << failedCount << " test(s) failed, " << passedCount
              << " passed in " << std::fixed << std::setprecision(2) << totalMs << " ms.\n"
              << color::reset;
    std::cout << color::bold << color::cyan
              << "============================================================\n"
              << color::reset;
    return 1;
}
