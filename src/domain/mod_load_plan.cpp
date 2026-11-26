/*
** CrabeLoader
** File description:
** Discovers mods and resolves one immutable load plan shared by VFS and Lua execution.
** Profile filtering precedes identity assignment and dependency resolution.
** Executes no Lua and indexes no assets; those consumers use the same accepted candidates.
**
** Authors: @Weresthebeef
*/
#include "domain/mod_load_plan.hpp"
#include "domain/mod_id.hpp"
#include "shared/logger.hpp"
#include "shared/version.hpp"
#include <utility>
namespace crabe::domain {
// Constructs the common startup/rescan plan before either consumer publishes new state.
ModLoadPlan discoverModLoadPlan(const std::filesystem::path& folder,
    const std::function<bool(std::string_view)>& enabled) {
    auto& logger = crabe::shared::Logger::getInstance();
    if (!std::filesystem::exists(folder)) return ModLoadPlan{.folder = folder};
    std::vector<ModCandidate> candidates;

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            if (folderName.empty() || folderName[0] == '.' || folderName[0] == '_')
                continue;
            ModCandidate candidate;
            candidate.name = std::move(folderName);
            candidate.path = entry.path();
            candidate.isDirectory = true;
            candidate.manifest = ModManifest(entry.path() / "mod.json");
            candidates.push_back(std::move(candidate));
        } else if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            ModCandidate candidate;
            candidate.name = entry.path().stem().string();
            candidate.path = entry.path();
            candidate.isDirectory = false;
            candidates.push_back(std::move(candidate));
        }
    }

    for (ModCandidate& candidate : candidates) {
        if (candidate.manifest.isMalformed() && candidate.manifest.diagnostic().has_value()) {
            logger.error("ModManager: mod '{}' has an unusable mod.json: {}",
                         candidate.name, candidate.manifest.diagnostic()->what());

        } else if (candidate.manifest.isValid() && !candidate.manifest.getIgnoredKeys().empty()) {
            std::string keys;
            for (const std::string& key : candidate.manifest.getIgnoredKeys()) {
                if (!keys.empty())
                    keys += ", ";
                keys += key;
            }
            logger.debug("ModManager: mod '{}': mod.json (manifestVersion {}) has keys this "
                         "loader does not read: {}",
                         candidate.name, candidate.manifest.getManifestVersion(), keys);
        }
    }

    {
        std::vector<ModCandidate> enabledModCandidates;
        enabledModCandidates.reserve(candidates.size());
        for (ModCandidate& candidate : candidates) {
            const std::string provisionalId =
                (candidate.manifest.isValid() && !candidate.manifest.getId().empty())
                    ? candidate.manifest.getId()
                    : synthesiseModId(candidate.name);
            if (!enabled || enabled(provisionalId)) {
                enabledModCandidates.push_back(std::move(candidate));
            } else {
                logger.debug("ModPlan: mod '{}' ({}) skipped: disabled by active profile.", candidate.name, provisionalId);
            }
        }
        candidates = std::move(enabledModCandidates);
    }

    std::vector<ModIdCandidate> idModCandidates;
    idModCandidates.reserve(candidates.size());
    for (const ModCandidate& candidate : candidates) {
        idModCandidates.push_back(ModIdCandidate{
            .declaredId = (candidate.manifest.isValid() && !candidate.manifest.getId().empty())
                              ? candidate.manifest.getId()
                              : std::string{},
            .folderName = candidate.name,
            .sortKey = candidate.path.generic_string(),
        });
    }

    std::vector<ModIdRenumbering> renumbered;
    const std::vector<std::string> assignedIds = assignModIds(idModCandidates, &renumbered);
    for (std::size_t index = 0; index < candidates.size(); ++index)
        candidates[index].id = assignedIds[index];

    for (const ModIdRenumbering& collision : renumbered) {
        logger.warning("ModManager: mod '{}' would be called '{}', which is already taken; "
                       "calling it '{}' instead. Rename the folder, or give it a mod.json with "
                       "an \"id\", if anything needs to depend on it by name.",
                       candidates[collision.index].name, collision.wantedId, collision.chosenId);
    }

    std::vector<ResolverInput> inputs;
    inputs.reserve(candidates.size());
    for (const ModCandidate& candidate : candidates)
        inputs.push_back(ResolverInput{ candidate.id, &candidate.manifest });

    const SemVer loaderVersion(crabe::version::Major, crabe::version::Minor,
                               crabe::version::Patch);
    auto resolution = resolve(inputs, loaderVersion);

    logger.info("ModManager: {} mod(s) found in '{}': {} to load, {} rejected (loader v{}).",
                candidates.size(), folder.string(), resolution.loadOrder.size(),
                resolution.rejected.size(), crabe::version::String);

    return ModLoadPlan{folder, std::move(candidates), std::move(resolution)};
}
}
