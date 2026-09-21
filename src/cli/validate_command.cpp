/*
** CrabeLoader
** File description:
** Implements validate: the on-disk checks a parsed manifest cannot make on its own.
** What a folder would run is domain/mod_entry.hpp's answer, the same one the loader acts on.
** Re-checks no schema rule; ModManifest::parse already refused anything malformed.
**
** Authors: @LucasLhomme
*/

#include "cli/validate_command.hpp"

#include "domain/mod_entry.hpp"
#include "domain/mod_id.hpp"
#include "domain/mod_manifest.hpp"
#include "third_party/json.hpp"

#include <format>

namespace crabe::cli {

    namespace {

        // Entry resolution used to be duplicated here, with a comment asking
        // the next reader to keep this copy in step with ModManager's by eye.
        // They had already drifted on the case that matters: this one refused
        // a folder holding no Lua, the loader reported it loaded. Both now ask
        // crabe::domain::planModEntry, which is also what makes the rule
        // testable without a Lua state.

        [[nodiscard]] std::string joinIgnoredKeys(const std::vector<std::string>& keys)
        {
            std::string out;
            for (const std::string& key : keys) {
                if (!out.empty())
                    out += ", ";
                out += key;
            }
            return out;
        }

        // Shared tail of validateMod() for every case that resolves to an
        // actual (possibly absent) mod.json: `manifestPath` may or may not
        // exist on disk -- ModManifest's constructor treats a missing file
        // as the (non-defect) Absent state -- and `modDir` is the directory
        // an entry script would be resolved against.
        [[nodiscard]] ValidateReport validateManifest(const std::filesystem::path& manifestPath,
                                                       const std::filesystem::path& modDir)
        {
            ValidateReport report;
            report.target = modDir;

            const crabe::domain::ModManifest manifest(manifestPath);
            report.hasManifest = !manifest.isAbsent();

            if (manifest.isMalformed()) {
                const crabe::domain::ManifestDiagnostic& diagnostic = *manifest.diagnostic();
                report.findings.push_back(
                    ValidateFinding{ Severity::Error, diagnostic.what(), diagnostic.line, diagnostic.column });
                return report;
            }

            const std::string modName = modDir.filename().string();

            if (manifest.isValid() && !manifest.getId().empty()) {
                report.id = manifest.getId();
            } else {
                report.id = crabe::domain::synthesiseModId(modName);
                report.idSynthesised = true;
                if (manifest.isValid()) {
                    report.findings.push_back(ValidateFinding{
                        Severity::Warning,
                        std::format("manifestVersion {} declares no \"id\"; the loader will call "
                                    "this mod \"{}\" -- give it an explicit id if anything should "
                                    "be able to depend on it by name",
                                    manifest.getManifestVersion(), report.id) });
                }
            }

            if (manifest.isValid()) {
                report.version = manifest.getVersion();
                if (!manifest.getVersion().empty() && !manifest.getSemVer().has_value()) {
                    report.findings.push_back(ValidateFinding{
                        Severity::Warning,
                        std::format("\"version\": \"{}\" is not a valid semantic version -- "
                                    "manifestVersion 0 tolerates this, but the resolver cannot "
                                    "compare it against any dependency's version range",
                                    manifest.getVersion()) });
                }

                if (!manifest.getIgnoredKeys().empty()) {
                    report.findings.push_back(ValidateFinding{
                        Severity::Warning,
                        std::format("mod.json has keys this loader does not read: {}",
                                    joinIgnoredKeys(manifest.getIgnoredKeys())) });
                }
            }

            const crabe::domain::ModEntryPlan plan =
                crabe::domain::planModEntry(modDir, modName, manifest);
            report.entryScript = plan.entryScript;
            report.entryScriptExists = !plan.runsNothing();
            if (!report.entryScriptExists) {
                report.findings.push_back(ValidateFinding{
                    Severity::Error,
                    std::format("entry script '{}' does not exist, and '{}' holds no .lua file "
                                "either",
                                plan.entryScript.string(), modDir.string()) });
            }

            return report;
        }

        [[nodiscard]] nlohmann::json toJson(const ValidateReport& report)
        {
            nlohmann::json findings = nlohmann::json::array();
            for (const ValidateFinding& finding : report.findings) {
                findings.push_back({
                    { "severity", finding.severity == Severity::Error ? "error" : "warning" },
                    { "message", finding.message },
                    { "line", finding.line },
                    { "column", finding.column },
                });
            }

            return nlohmann::json{
                { "target", report.target.generic_string() },
                { "hasManifest", report.hasManifest },
                { "id", report.id },
                { "idSynthesised", report.idSynthesised },
                { "version", report.version },
                { "entryScript", report.entryScript.generic_string() },
                { "entryScriptExists", report.entryScriptExists },
                { "exitCode", report.exitCode() },
                { "ok", report.exitCode() == 0 },
                { "findings", findings },
            };
        }

    } // namespace

    ValidateReport validateMod(const std::filesystem::path& path)
    {
        std::error_code ec;
        const bool isDirectory = std::filesystem::is_directory(path, ec);
        const bool isFile = std::filesystem::is_regular_file(path, ec);

        if (!isDirectory && !isFile) {
            ValidateReport report;
            report.target = path;
            report.findings.push_back(
                ValidateFinding{ Severity::Error, std::format("'{}' does not exist", path.string()) });
            return report;
        }

        if (isFile) {
            if (path.extension() == ".json")
                return validateManifest(path, path.parent_path());

            // A standalone script mod (mods/<name>.lua): nothing declares an
            // id or a version, so there is nothing left to check beyond "is
            // it actually there, and does it hold anything".
            ValidateReport report;
            report.target = path;
            report.hasManifest = false;
            report.entryScript = path;
            report.entryScriptExists = true;
            report.id = crabe::domain::synthesiseModId(path.stem().string());
            report.idSynthesised = true;
            if (std::filesystem::is_empty(path, ec)) {
                report.findings.push_back(
                    ValidateFinding{ Severity::Warning,
                                     std::format("'{}' is empty; nothing will run", path.string()) });
            }
            return report;
        }

        const std::filesystem::path manifestPath = path / "mod.json";
        if (!std::filesystem::exists(manifestPath, ec)) {
            // A bare directory of .lua files is a legitimate mod shape (see
            // domain/mod_manifest.hpp's ManifestState::Absent) -- but if
            // there is nothing to run either, that is worth a warning.
            ValidateReport report;
            report.target = path;
            report.hasManifest = false;
            report.id = crabe::domain::synthesiseModId(path.filename().string());
            report.idSynthesised = true;

            const crabe::domain::ModEntryPlan plan = crabe::domain::planModEntry(
                path, path.filename().string(), crabe::domain::ModManifest{});
            report.entryScript = plan.entryScript;
            report.entryScriptExists = !plan.runsNothing();
            if (!report.entryScriptExists) {
                report.findings.push_back(ValidateFinding{
                    Severity::Warning,
                    "no mod.json and no .lua files directly inside this folder; nothing will load" });
            }
            return report;
        }

        return validateManifest(manifestPath, path);
    }

    int ValidateReport::exitCode() const noexcept
    {
        bool hasError = false;
        bool hasWarning = false;
        for (const ValidateFinding& finding : findings) {
            if (finding.severity == Severity::Error)
                hasError = true;
            else
                hasWarning = true;
        }
        if (hasError)
            return 1;
        if (hasWarning)
            return 2;
        return 0;
    }

    void printHuman(const ValidateReport& report, std::ostream& out)
    {
        out << "validate: " << report.target.string() << "\n";
        if (report.hasManifest) {
            out << "  id:      " << report.id << (report.idSynthesised ? " (synthesised)" : "") << "\n";
            if (!report.version.empty())
                out << "  version: " << report.version << "\n";
        } else {
            out << "  id:      " << report.id << " (no mod.json; the loader synthesises this)\n";
        }
        out << "  entry:   " << report.entryScript.string() << (report.entryScriptExists ? "" : " (missing)")
            << "\n";

        if (report.findings.empty()) {
            out << "  OK\n";
            return;
        }

        for (const ValidateFinding& finding : report.findings) {
            out << "  " << (finding.severity == Severity::Error ? "error" : "warning") << ": "
                << finding.message;
            if (finding.line != 0)
                out << " (" << finding.line << ":" << finding.column << ")";
            out << "\n";
        }
    }

    int runValidate(std::span<const std::string> args, std::ostream& out, std::ostream& err)
    {
        std::filesystem::path target;
        bool json = false;

        for (const std::string& arg : args) {
            if (arg == "--json") {
                json = true;
            } else if (target.empty()) {
                target = arg;
            } else {
                err << "validate: unexpected argument '" << arg << "'\n";
                return 1;
            }
        }

        if (target.empty()) {
            err << "validate: usage: crabe-cli validate <path> [--json]\n";
            return 1;
        }

        const ValidateReport report = validateMod(target);
        if (json)
            out << toJson(report).dump(2) << "\n";
        else
            printHuman(report, out);

        return report.exitCode();
    }

} // namespace crabe::cli
