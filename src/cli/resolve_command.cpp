/*
** CrabeLoader
** File description:
** Implements resolve: reads a mods directory, assigns ids, then calls the real resolver.
** Id assignment goes through crabe::domain::assignModIds, the same policy the loader applies.
** Decides no order itself -- every ordering rule is in src/domain/dependency_resolver.cpp.
**
** Authors: @LucasLhomme
*/

#include "cli/resolve_command.hpp"

#include "domain/mod_id.hpp"
#include "domain/mod_manifest.hpp"
#include "shared/version.hpp"
#include "third_party/json.hpp"

#include <format>
#include <map>
#include <utility>

namespace crabe::cli {

    namespace {

        struct Candidate {
            std::string name;
            std::filesystem::path path;
            crabe::domain::ModManifest manifest;
        };

        [[nodiscard]] std::string_view reasonName(crabe::domain::Rejection reason)
        {
            using crabe::domain::Rejection;
            switch (reason) {
            case Rejection::MissingDependency:
                return "MissingDependency";
            case Rejection::VersionMismatch:
                return "VersionMismatch";
            case Rejection::CyclicDependency:
                return "CyclicDependency";
            case Rejection::LoaderTooOld:
                return "LoaderTooOld";
            case Rejection::LoaderTooNew:
                return "LoaderTooNew";
            case Rejection::DuplicateSupersededId:
                return "DuplicateSupersededId";
            case Rejection::DeclaredConflict:
                return "DeclaredConflict";
            case Rejection::InvalidManifest:
                return "InvalidManifest";
            }
            return "Unknown";
        }

        [[nodiscard]] nlohmann::json toJson(const ResolveReport& report)
        {
            nlohmann::json loadOrder = nlohmann::json::array();
            for (const ResolvedEntry& entry : report.loadOrder) {
                loadOrder.push_back({
                    { "id", entry.id },
                    { "folder", entry.folder },
                    { "version", entry.version },
                });
            }

            nlohmann::json rejected = nlohmann::json::array();
            for (const RejectedEntry& entry : report.rejected) {
                rejected.push_back({
                    { "id", entry.id },
                    { "folder", entry.folder },
                    { "version", entry.version },
                    { "reason", std::string(reasonName(entry.reason)) },
                    { "reasonText", std::string(crabe::domain::describe(entry.reason)) },
                    { "detail", entry.detail },
                });
            }

            return nlohmann::json{
                { "modsDir", report.modsDir.generic_string() },
                { "loaderVersion", report.loaderVersion },
                { "exitCode", report.exitCode() },
                { "ok", report.exitCode() == 0 },
                { "loadOrder", loadOrder },
                { "rejected", rejected },
                { "notes", report.notes },
            };
        }

    } // namespace

    std::expected<ResolveReport, std::string> resolveModsDir(const std::filesystem::path& modsDir)
    {
        std::error_code ec;
        if (!std::filesystem::is_directory(modsDir, ec))
            return std::unexpected(std::format("'{}' is not a directory", modsDir.string()));

        // ---- Steps 1-2, copied from ModManager::discoverAndLoadMods: find
        // the candidates the same way, and give a malformed mod.json the
        // same absent-manifest fallback (see domain/mod_manager.cpp for why
        // that is the right answer for a loader, not a resolver). ----
        std::vector<Candidate> candidates;
        std::vector<std::string> notes;

        for (const auto& entry : std::filesystem::directory_iterator(modsDir, ec)) {
            if (entry.is_directory()) {
                std::string folderName = entry.path().filename().string();
                if (folderName.empty() || folderName[0] == '.' || folderName[0] == '_')
                    continue;
                Candidate candidate;
                candidate.name = std::move(folderName);
                candidate.path = entry.path();
                candidate.manifest = crabe::domain::ModManifest(entry.path() / "mod.json");
                candidates.push_back(std::move(candidate));
            } else if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                Candidate candidate;
                candidate.name = entry.path().stem().string();
                candidate.path = entry.path();
                candidates.push_back(std::move(candidate));
            }
        }

        for (Candidate& candidate : candidates) {
            if (candidate.manifest.isMalformed() && candidate.manifest.diagnostic().has_value()) {
                notes.push_back(std::format("'{}' has an unusable mod.json, treated as if it had "
                                            "none: {}",
                                            candidate.name, candidate.manifest.diagnostic()->what()));
                candidate.manifest = crabe::domain::ModManifest{};
            }
        }

        // ---- Step 3: the same pure id assignment ModManager itself calls.
        std::vector<crabe::domain::ModIdCandidate> idCandidates;
        idCandidates.reserve(candidates.size());
        for (const Candidate& candidate : candidates) {
            idCandidates.push_back(crabe::domain::ModIdCandidate{
                .declaredId = (candidate.manifest.isValid() && !candidate.manifest.getId().empty())
                                  ? candidate.manifest.getId()
                                  : std::string{},
                .folderName = candidate.name,
                .sortKey = candidate.path.generic_string(),
            });
        }
        std::vector<crabe::domain::ModIdRenumbering> renumbered;
        const std::vector<std::string> ids = crabe::domain::assignModIds(idCandidates, &renumbered);
        for (const crabe::domain::ModIdRenumbering& collision : renumbered) {
            notes.push_back(std::format(
                "'{}' would be called '{}', which is already taken; calling it '{}' instead",
                candidates[collision.index].name, collision.wantedId, collision.chosenId));
        }

        // ---- Step 4: resolve, with the loader's own compiled-in version.
        std::vector<crabe::domain::ResolverInput> inputs;
        inputs.reserve(candidates.size());
        for (std::size_t i = 0; i < candidates.size(); ++i)
            inputs.push_back(crabe::domain::ResolverInput{ ids[i], &candidates[i].manifest });

        const crabe::domain::SemVer loaderVersion(crabe::version::Major, crabe::version::Minor,
                                                  crabe::version::Patch);
        const crabe::domain::ResolutionResult resolution =
            crabe::domain::resolve(inputs, loaderVersion);

        ResolveReport report;
        report.modsDir = modsDir;
        report.loaderVersion = std::string(crabe::version::String);
        report.notes = std::move(notes);

        for (std::size_t position = 0; position < resolution.loadOrder.size(); ++position) {
            const std::size_t index = resolution.sourceIndex[position];
            report.loadOrder.push_back(ResolvedEntry{
                ids[index], candidates[index].name, candidates[index].manifest.getVersion() });
        }

        // Match each rejection back to a folder on disk, one candidate per
        // rejection of that id -- the same rule ModManager's own report
        // applies, so a duplicated id's several rejections each name a
        // different copy.
        std::vector<bool> loaded(candidates.size(), false);
        for (const std::size_t index : resolution.sourceIndex)
            loaded[index] = true;

        std::multimap<std::string, std::size_t> unranByIdThenIndex;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            if (!loaded[i])
                unranByIdThenIndex.emplace(ids[i], i);
        }

        for (const crabe::domain::RejectedMod& rejection : resolution.rejected) {
            RejectedEntry entry;
            entry.id = rejection.id;
            entry.reason = rejection.reason;
            entry.detail = rejection.detail;

            const auto match = unranByIdThenIndex.find(rejection.id);
            if (match != unranByIdThenIndex.end()) {
                entry.folder = candidates[match->second].name;
                entry.version = candidates[match->second].manifest.getVersion();
                unranByIdThenIndex.erase(match);
            }
            report.rejected.push_back(std::move(entry));
        }

        return report;
    }

    int ResolveReport::exitCode() const noexcept
    {
        return rejected.empty() ? 0 : 1;
    }

    void printHuman(const ResolveReport& report, std::ostream& out)
    {
        out << "resolve: " << report.modsDir.string() << " (loader v" << report.loaderVersion << ")\n";
        for (const std::string& note : report.notes)
            out << "  note: " << note << "\n";

        out << "  " << report.loadOrder.size() << " to load, " << report.rejected.size()
            << " rejected\n";

        for (std::size_t i = 0; i < report.loadOrder.size(); ++i) {
            const ResolvedEntry& entry = report.loadOrder[i];
            out << "  [" << (i + 1) << "/" << report.loadOrder.size() << "] " << entry.id << " "
                << (entry.version.empty() ? "(no version)" : entry.version) << " in '" << entry.folder
                << "'\n";
        }

        for (const RejectedEntry& entry : report.rejected) {
            out << "  [--] " << (entry.id.empty() ? "(unnamed)" : entry.id) << " "
                << (entry.version.empty() ? "(no version)" : entry.version) << " in '"
                << (entry.folder.empty() ? "(no folder)" : entry.folder) << "' -- not loaded, "
                << crabe::domain::describe(entry.reason) << ": " << entry.detail << "\n";
        }
    }

    int runResolve(std::span<const std::string> args, std::ostream& out, std::ostream& err)
    {
        std::filesystem::path modsDir;
        bool json = false;

        for (const std::string& arg : args) {
            if (arg == "--json") {
                json = true;
            } else if (modsDir.empty()) {
                modsDir = arg;
            } else {
                err << "resolve: unexpected argument '" << arg << "'\n";
                return 1;
            }
        }

        if (modsDir.empty()) {
            err << "resolve: usage: crabe-cli resolve <mods-dir> [--json]\n";
            return 1;
        }

        const auto report = resolveModsDir(modsDir);
        if (!report.has_value()) {
            err << "resolve: " << report.error() << "\n";
            return 1;
        }

        if (json)
            out << toJson(*report).dump(2) << "\n";
        else
            printHuman(*report, out);

        return report->exitCode();
    }

} // namespace crabe::cli
