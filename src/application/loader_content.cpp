/*
** CrabeLoader
** File description:
** Implements load overrides and chunk patches: changing a chunk before the game compiles it.
** Both are keyed on something recognisable inside the chunk, because the game names few of them.
** Compiles nothing itself; the detours that see each chunk are src/infrastructure/lua_call.cpp.
**
** Authors: @LucasLhomme
*/

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "application/gateway.hpp"
#include "application/loader.hpp"
#include "domain/mod_entry.hpp"
#include "shared/logger.hpp"

namespace crabe::application {

// Two ways to change a chunk the game is about to compile, both keyed on
// something recognisable in the chunk itself:
//
//   load override -- replace the whole chunk source before it compiles
//   patch         -- run extra source right after the chunk's own pcall
//                    returns, so it can edit the tables the chunk just built
//
// Only one patch can be armed at a time: the loadbuffer hook arms it, the
// pcall hook consumes it. skilltrees/ and characters/ are the folders these
// are read from at startup.

namespace {

    using Rules = std::vector<Loader::ChunkRule>;

    // Adds a rule, replacing the source if `key` is already registered. An
    // empty key would match every chunk, so it is dropped instead.
    void upsert(Rules& rules, std::string key, std::string source, std::string label)
    {
        if (key.empty())
            return;

        for (auto& rule : rules) {
            if (rule.key == key) {
                rule.source = std::move(source);
                rule.label = std::move(label);
                return;
            }
        }
        rules.push_back({std::move(key), std::move(source), std::move(label)});
    }

    // First rule whose key appears anywhere in the chunk source, or nullptr.
    // Reads the caller's buffer in place -- this runs on every chunk the game
    // compiles, ~1500 per boot.
    const Loader::ChunkRule* findByContent(const Rules& rules, const char* buff, size_t size)
    {
        if (!buff || size == 0 || rules.empty())
            return nullptr;

        std::string_view haystack(buff, size);
        for (const auto& rule : rules) {
            if (haystack.find(rule.key) != std::string_view::npos)
                return &rule;
        }
        return nullptr;
    }

    // Whole file as a string. Returns false, having logged why, when the file
    // cannot be opened or is empty -- both mean "skip this one, keep going".
    bool readTextFile(const std::filesystem::path& path, const char* folder, std::string& out)
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
        std::string name = path.filename().string();

        std::ifstream file(path, std::ios::binary);
        if (!file) {
            logger.warning("Loader: {}/{} could not be opened, skipping.", folder, name);
            return false;
        }

        out.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (out.empty()) {
            logger.warning("Loader: {}/{} is empty, skipping.", folder, name);
            return false;
        }
        return true;
    }

}

void Loader::registerLoadOverride(std::string matchSubstring, std::string replacement,
                                  std::string label)
{
    upsert(_loadOverrides, std::move(matchSubstring), std::move(replacement), std::move(label));
}

void Loader::clearLoadOverrides()
{
    _loadOverrides.clear();
}

const std::string* Loader::findLoadOverride(const char* buff, size_t size) const
{
    const ChunkRule* rule = findByContent(_loadOverrides, buff, size);
    return rule ? &rule->source : nullptr;
}

void Loader::registerChunkPatch(std::string matchSubstring, std::string patchSource,
                                std::string label)
{
    upsert(_chunkPatches, std::move(matchSubstring), std::move(patchSource), std::move(label));
}

void Loader::armPatchIfMatched(const char* buff, size_t size, int depth)
{
    if (const ChunkRule* rule = findByContent(_chunkPatches, buff, size))
        _armedPatches.push_back(ArmedPatch{ *rule, depth });
}

std::optional<Loader::ChunkRule> Loader::takePatchForDepth(int depth)
{
    // Anything armed deeper than the call that just returned belongs to a
    // chunk that was loaded and never run -- a load that failed, or source the
    // game compiled and threw away. It can never fire, so it is dropped here
    // rather than left to sit in front of the entries that still can.
    while (!_armedPatches.empty() && _armedPatches.back().depth > depth)
        _armedPatches.pop_back();

    if (_armedPatches.empty() || _armedPatches.back().depth != depth)
        return std::nullopt;

    ChunkRule rule = std::move(_armedPatches.back().rule);
    _armedPatches.pop_back();
    return rule;
}

void Loader::registerNamedPatch(std::string exactName, std::string patchSource,
                                std::string label)
{
    upsert(_namedPatches, std::move(exactName), std::move(patchSource), std::move(label));
}

void Loader::armPatchIfNameMatched(const char* name, int depth)
{
    if (!name || _namedPatches.empty())
        return;

    std::string cleanName = name;
    if (!cleanName.empty() && cleanName[0] == '@')
        cleanName.erase(0, 1);
    for (char& c : cleanName) {
        if (c == '\\') c = '/';
    }

    for (const ChunkRule& rule : _namedPatches) {
        std::string cleanRuleKey = rule.key;
        if (!cleanRuleKey.empty() && cleanRuleKey[0] == '@')
            cleanRuleKey.erase(0, 1);
        for (char& c : cleanRuleKey) {
            if (c == '\\') c = '/';
        }

        if (cleanRuleKey == cleanName) {
            crabe::shared::Logger::getInstance().debug(
                "Loader: armed named patch '{}' for '{}' at depth {}.", rule.label, cleanName, depth);
            if (!_armedPatches.empty() && _armedPatches.back().depth == depth)
                _armedPatches.back().rule = rule;
            else
                _armedPatches.push_back(ArmedPatch{ rule, depth });
            return;
        }
    }
}

    void loadOverridesFromDirectory(Loader& loader, const std::filesystem::path& folder, const std::string& labelPrefix)
    {
        crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
        size_t loadedOverrides = 0;
        size_t loadedPatches = 0;

        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (!entry.is_regular_file())
                continue;

            bool isOverride = entry.path().extension() == ".lua";
            bool isPatch = entry.path().extension() == ".patch";
            if (!isOverride && !isPatch)
                continue;

            std::string content;
            if (!readTextFile(entry.path(), labelPrefix.c_str(), content))
                continue;

            std::string matchHint = entry.path().stem().string();
            std::string label = labelPrefix + "/" + matchHint;
            if (isOverride) {
                loader.registerLoadOverride(matchHint, std::move(content), label);
                ++loadedOverrides;
            } else {
                loader.registerChunkPatch(matchHint, std::move(content), label);
                ++loadedPatches;
            }
        }

        if (loadedOverrides > 0 || loadedPatches > 0) {
            logger.info("Loader: {} override(s) and {} patch(es) registered from {}.",
                        loadedOverrides, loadedPatches, labelPrefix);
        }
    }

    // One characters/*.lua, isolated: its own function scope and its own pcall, so
    // an error in it no longer aborts the files after it. The error is collected
    // under the file's name; the origin sits in a long bracket no name can close.
    std::string guardedCharacterSource(const std::string& origin, const std::string& content)
    {
        return "do local ok, err = pcall(function()\n" + content +
               "\nend) if not ok then _crabeCharErrors = _crabeCharErrors .. "
               "\"\\n  [[\" .. [==[" + origin + "]==] .. \"]] \" .. tostring(err) end end\n";
    }

    // Runs the guarded files in one chunk, then raises every collected error at
    // once, so the patch still fails loudly in loader.log, naming each file.
    std::string characterScript(const std::string& skuTable, const std::string& guardedFiles)
    {
        return skuTable + "local _crabeCharErrors = \"\"\n" + guardedFiles +
               "if _crabeCharErrors ~= \"\" then error(\"characters/*.lua failed:\" .. "
               "_crabeCharErrors, 0) end\n";
    }

    // Every characters/*.lua in one folder: exposeCharacter declarations go to
    // `exposed` for the registry, guarded sources are appended to `combined`.
    void loadCharactersFromDirectory(const std::filesystem::path& folder,
                                     const std::string& labelPrefix,
                                     std::vector<gateway::Entry>& exposed,
                                     std::string& combined)
    {
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".lua")
                continue;

            std::string content;
            if (!readTextFile(entry.path(), labelPrefix.c_str(), content))
                continue;

            const std::string origin = labelPrefix + "/" + entry.path().filename().string();
            for (auto& exposedEntry : gateway::parseExposedCharacters(content)) {
                exposedEntry.origin = origin;
                exposed.push_back(std::move(exposedEntry));
            }

            combined += guardedCharacterSource(origin, content);
        }
    }

void Loader::loadCharactersFromDisk()
{
    // The one window in which the catalog can still be changed: this patch runs
    // immediately after the chunk below has built VirtualReaderPC_Data.AvatarData
    // and before Presentation/pressstart.lua hands it to native code, after which
    // it is frozen. No mod runs in that window -- which is why this is here and
    // not in Lua. See characters/README.md.
    constexpr const char* kTargetName = "Presentation/VirtualReaderPC_Data.lua";

    crabe::shared::Logger& logger = crabe::shared::Logger::getInstance();
    std::vector<gateway::Entry> exposed;
    std::string combined;

    // mods/<name>/characters/ only. A <gameDir>/characters/ folder was read too
    // until this was restored; it is not any more, because a character is mod
    // content and belongs with the mod that ships it -- the same shape as
    // mods/<name>/skilltrees/, which loadOverridesFromDisk below reads.
    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";
    if (std::filesystem::exists(modsFolder)) {
        for (const auto& entry : std::filesystem::directory_iterator(modsFolder)) {
            if (!entry.is_directory())
                continue;
            std::string modName = entry.path().filename().string();
            if (modName.empty() || modName[0] == '.' || modName[0] == '_')
                continue;

            for (const std::string_view sub : crabe::domain::kCharacterDirectories) {
                auto subPath = entry.path() / sub;
                if (std::filesystem::exists(subPath)) {
                    loadCharactersFromDirectory(subPath, modName + "/" + std::string(sub),
                                                exposed, combined);
                    break;
                }
            }
        }
    }

    if (exposed.empty() && combined.empty()) {
        logger.debug("Loader: no characters/*.lua found; the character grid is left alone.");
        return;
    }

    for (const std::string& issue : gateway::resolveSkus(exposed))
        logger.error("Loader: characters: {}", issue);

    _characterInjectionScript = characterScript(gateway::buildSkuTableLua(exposed), combined);
    registerNamedPatch(kTargetName, _characterInjectionScript, "characters/");
    registerChunkPatch(gateway::containerKey(), gateway::buildInjectionLua(exposed),
                       "figure registry");
    logger.info("Loader: {} total character definition(s) registered.", exposed.size());
}

void Loader::loadOverridesFromDisk()
{
    std::filesystem::path rootFolder = std::filesystem::current_path() / "skilltrees";
    if (std::filesystem::exists(rootFolder)) {
        loadOverridesFromDirectory(*this, rootFolder, "skilltrees");
    }

    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";
    if (!std::filesystem::exists(modsFolder))
        return;

    for (const auto& entry : std::filesystem::directory_iterator(modsFolder)) {
        if (!entry.is_directory())
            continue;
        std::string modName = entry.path().filename().string();
        if (modName.empty() || modName[0] == '.' || modName[0] == '_')
            continue;

        for (const std::string_view sub : crabe::domain::kSkillTreeDirectories) {
            auto subPath = entry.path() / sub;
            if (std::filesystem::exists(subPath)) {
                loadOverridesFromDirectory(*this, subPath, modName + "/" + std::string(sub));
                break;
            }
        }
    }
}

} // namespace crabe::application

