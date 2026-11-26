/*
** CrabeLoader
** File description:
** Exercises production discovery, dependency precedence, explicit mounts and conflict provenance.
** Fixtures deliberately disagree with folder order and include disabled and rejected mods.
** No game is launched and every fixture stays in a unique temporary directory.
**
** Authors: @Weresthebeef
*/
#include "infrastructure/vfs_override_manager.hpp"
#include "third_party/json.hpp"
#include "domain/mod_entry.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using crabe::infrastructure::VfsOverrideManager;
using nlohmann::json;
namespace {
int checks = 0;
// Counts assertions and aborts with a diagnostic on failure.
void require(bool ok, const char* message) { ++checks; if (!ok) throw std::runtime_error(message); }
// Writes a fixture file, creating its parent directories.
void write(const std::filesystem::path& path, std::string_view text) {
    std::filesystem::create_directories(path.parent_path()); std::ofstream out(path); out << text;
}
// Writes a valid v1 manifest with optional ordering and mount fields.
void manifest(const std::filesystem::path& path, const char* id, json fields = json::object()) {
    fields["manifestVersion"] = 1; fields["id"] = id; fields["version"] = "1.0.0";
    write(path / "mod.json", fields.dump());
}
}
int main(int argc, char** argv) {
    if (argc == 4 && std::string_view(argv[1]) == "--inspect") {
        auto& index = VfsOverrideManager::get(); index.initialize(argv[2]);
        const auto plan = index.getModPlan();
        if (!plan) return 2;
        json report{{"loadOrder", plan->resolution.loadOrder}, {"assets", index.getOverrideCount()},
            {"mainLuaMounted", index.resolve("main.lua").has_value()}, {"readmeMounted", index.resolve("readme.txt").has_value()}};
        report["rejected"] = json::array();
        for (const auto& rejected : plan->resolution.rejected)
            report["rejected"].push_back({{"id", rejected.id}, {"reason", std::string(crabe::domain::describe(rejected.reason))}, {"detail", rejected.detail}});
        report["conflicts"] = json::array();
        for (const auto& key : index.getConflictPaths()) {
            const auto detail = index.explain(key);
            report["conflicts"].push_back({{"path", key}, {"winner", detail.winner->modId}, {"reason", detail.winner->reason}, {"providers", detail.providers.size()}});
        }
        std::ofstream output(argv[3]); output << report.dump(2); return output ? 0 : 2;
    }
    const auto root = std::filesystem::temp_directory_path() /
        ("crabe_vfs_framework_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto mods = root / "mods";
    auto& vfs = VfsOverrideManager::get();
    try {
        const auto after = mods / "aa_after", before = mods / "zz_before";
        manifest(after, "local.after", {{"loadAfter", {"local.before"}}});
        manifest(before, "local.before");
        write(after / "assets/ui/shared.bin", "later"); write(before / "assets/ui/shared.bin", "earlier");
        write(after / "main.lua", "return true"); write(before / "README.txt", "metadata");
        write(after / "characters/avatar.lua", "definition"); write(after / "skilltrees/x.patch", "patch");
        write(after / "assets/scripts/game.lua", "game asset intentionally mounted");
        manifest(mods / "missing", "local.missing", {{"dependencies", {{{"id", "local.absent"}}}}});
        write(mods / "missing/assets/ui/rejected.bin", "must not mount");
        manifest(mods / "future", "local.future", {{"minLoaderVersion", "999.0.0"}});
        write(mods / "future/assets/ui/future.bin", "must not mount");
        write(mods / "broken/mod.json", "{"); write(mods / "broken/assets/ui/broken.bin", "must not mount");
        for (const auto& source : std::vector<std::string>{".", "..", "../outside", "C:/outside", "/outside", "art/../outside"}) {
            json body{{"assetMounts", {{{"source", source}, {"target", "ui"}}}}};
            require(!crabe::domain::ModManifest::parse(body.dump(), "fixture.json"), "unsafe source mount passed validation");
        }
        require(!crabe::domain::ModManifest::parse(R"({"assetMounts":[{"source":"art","target":"../ui"}]})", "fixture.json"), "unsafe target mount passed validation");
        vfs.initialize(root);
        require(vfs.resolve("ui/shared.bin") == after / "assets/ui/shared.bin", "folder order overrode dependency order");
        const auto plan = vfs.getModPlan();
        require(plan && plan->resolution.loadOrder == std::vector<std::string>{"local.before", "local.after"}, "unexpected common load plan");
        require(plan->resolution.rejected.size() == 3, "rejected mods not retained in common plan");
        require(plan->resolution.findRejection("local.missing") && plan->resolution.findRejection("local.missing")->reason == crabe::domain::Rejection::MissingDependency, "missing dependency fixture was not resolved correctly");
        require(!vfs.resolve("ui/rejected.bin") && !vfs.resolve("ui/future.bin") && !vfs.resolve("ui/broken.bin"), "rejected assets leaked into index");
        require(!vfs.resolve("main.lua") && !vfs.resolve("readme.txt") && !vfs.resolve("characters/avatar.lua") && !vfs.resolve("skilltrees/x.patch"), "mod metadata mounted as game assets");
        require(vfs.resolve("scripts/game.lua").has_value(), "intentional game script in assets was excluded");
        auto detail = vfs.explain("ASSETS\\UI\\SHARED.BIN");
        require(detail.providers.size() == 2 && detail.winner && detail.winner->modId == "local.after", "conflict providers or winner incorrect");
        require(detail.winner->reason == "later resolved mod wins", "precedence explanation incorrect");
        require(vfs.getConflictPaths() == std::vector<std::string>{"ui/shared.bin"}, "conflict paths not stable");
        require(vfs.explain("ui/absent.bin").providers.empty(), "unmounted path acquired provenance");
        vfs.initialize(root, [](std::string_view id) { return id != "local.after"; });
        require(vfs.resolve("ui/shared.bin") == before / "assets/ui/shared.bin", "disabled provider still mounted");
        require(vfs.getConflictPaths().empty(), "disabled provider remains in diagnostics");
        manifest(mods / "dependent", "local.dependent", {{"dependencies", {{{"id", "local.after"}}}}});
        write(mods / "dependent/assets/ui/dependent.bin", "rejected when prerequisite disabled");
        vfs.initialize(root, [](std::string_view id) { return id != "local.after"; });
        require(!vfs.resolve("ui/dependent.bin"), "dependent of disabled mod still mounted");
        manifest(mods / "cycle_a", "local.cycle-a", {{"dependencies", {{{"id", "local.cycle-b"}}}}});
        manifest(mods / "cycle_b", "local.cycle-b", {{"dependencies", {{{"id", "local.cycle-a"}}}}});
        write(mods / "cycle_a/assets/ui/cycle.bin", "rejected cycle");

        manifest(mods / "custom", "local.custom", {{"assetMounts", {{{"source", "art"}, {"target", "ui"}}, {{"source", "revision"}, {"target", "ui"}}}}});
        write(mods / "custom/art/portrait.bin", "v1"); write(mods / "custom/revision/portrait.bin", "v2");
        write(mods / "custom/assets/ui/not_declared.bin", "not mounted");
        manifest(mods / "empty", "local.empty", {{"assetMounts", json::array()}});
        write(mods / "empty/assets/ui/disabled_mount.bin", "not mounted");
        vfs.initialize(root);
        require(!vfs.resolve("ui/cycle.bin"), "cyclic mod mounted assets");
        detail = vfs.explain("ui/portrait.bin");
        require(detail.winner && detail.winner->physicalPath == mods / "custom/revision/portrait.bin", "explicit remapping failed");
        require(detail.providers.size() == 2 && detail.winner->reason == "later declared mount wins", "mount conflict explanation incorrect");
        require(!vfs.resolve("ui/not_declared.bin") && !vfs.resolve("ui/disabled_mount.bin"), "explicit mounts did not replace defaults");
        require(crabe::domain::planModEntry(mods / "custom", "custom", crabe::domain::ModManifest(mods / "custom/mod.json")).isContentOnly(), "asset-only mod treated as empty");
        const auto bankOnly = mods / "bank_only";
        write(bankOnly / "bank_dependencies.json", R"({"version":1,"banks":{}})");
        require(crabe::domain::planModEntry(bankOnly, "bank_only", crabe::domain::ModManifest{}).isContentOnly(), "bank patch-only mod treated as empty");
        std::filesystem::remove(bankOnly / "bank_dependencies.json");
        std::filesystem::remove(bankOnly);

        manifest(mods / "unsafe", "local.unsafe", {{"assetMounts", {{{"source", "../outside"}, {"target", "ui"}}}}});
        write(mods / "unsafe/assets/ui/unsafe.bin", "rejected");
        vfs.scanModsDirectory(mods);
        require(!vfs.resolve("ui/unsafe.bin"), "unsafe mount declaration accepted");
        write(after / "textures/aa/aa11.tbody", "nested"); write(after / "textures/aa11.tbody", "exact");
        vfs.scanModsDirectory(mods);
        detail = vfs.explain("textures/aa11.tbody");
        require(detail.winner && !detail.winner->alias && detail.winner->physicalPath == after / "textures/aa11.tbody", "texture alias defeated exact path");
        require(detail.providers.size() == 2, "same texture provider counted multiple times");
        const auto stable = vfs.getModPlan();
        const auto previous = vfs.resolve("ui/shared.bin");
        manifest(mods / "failure", "local.failure"); write(mods / "failure/assets", "not a directory");
        vfs.scanModsDirectory(mods);
        require(vfs.getModPlan() == stable && vfs.resolve("ui/shared.bin") == previous, "failed scan partially published plan/index");
        std::filesystem::remove(mods / "failure/assets");
        std::filesystem::remove(after / "assets/ui/shared.bin");
        vfs.scanModsDirectory(mods);
        require(vfs.resolve("ui/shared.bin") == before / "assets/ui/shared.bin", "rescan retained deleted winner");
        require(vfs.explain("ui/shared.bin").providers.size() == 1, "rescan retained stale provider");
        vfs.scanModsDirectory(root / "absent_mods");
        require(vfs.getOverrideCount() == 0 && vfs.getConflictPaths().empty() && vfs.getModPlan(), "empty plan left stale mappings");
        vfs.clear();
        require(std::filesystem::equivalent(root.parent_path(), std::filesystem::temp_directory_path()) && root.filename().string().starts_with("crabe_vfs_framework_"), "unexpected fixture cleanup path");
        std::filesystem::remove_all(root);
        std::cout << "[vfs-framework] " << checks << " checks passed\n"; return 0;
    } catch (const std::exception& error) {
        std::cerr << "[vfs-framework] " << error.what() << "\n";
        vfs.clear(); std::error_code ec;
        if (std::filesystem::equivalent(root.parent_path(), std::filesystem::temp_directory_path()) && root.filename().string().starts_with("crabe_vfs_framework_")) std::filesystem::remove_all(root, ec); return 1;
    }
}
