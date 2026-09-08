/*
** CrabeLoader
** File description:
** loader_content -- load overrides, chunk patches, and the folders they load from
*/

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "application/loader.hpp"
#include "application/gateway.hpp"
#include "shared/logger.hpp"

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
        Logger& logger = Logger::getInstance();
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

void Loader::armPatchIfMatched(const char* buff, size_t size)
{
    if (const ChunkRule* rule = findByContent(_chunkPatches, buff, size))
        _armedPatch = *rule;
}

bool Loader::hasArmedPatch() const
{
    return !_armedPatch.source.empty();
}

Loader::ChunkRule Loader::takeArmedPatch()
{
    ChunkRule result = std::move(_armedPatch);
    _armedPatch = {};
    return result;
}

void Loader::registerNamedPatch(std::string exactName, std::string patchSource,
                                std::string label)
{
    upsert(_namedPatches, std::move(exactName), std::move(patchSource), std::move(label));
}

void Loader::armPatchIfNameMatched(const char* name)
{
    if (!name || _namedPatches.empty())
        return;

    for (const ChunkRule& rule : _namedPatches) {
        if (rule.key == name) {
            _armedPatch = rule;
            return;
        }
    }
}

    void loadOverridesFromDirectory(Loader& loader, const std::filesystem::path& folder, const std::string& labelPrefix)
    {
        Logger& logger = Logger::getInstance();
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

    void loadCharactersFromDirectory(const std::filesystem::path& folder,
                                     const std::string& labelPrefix,
                                     std::vector<Gateway::Entry>& exposed,
                                     std::string& combined)
    {
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".lua")
                continue;

            std::string content;
            if (!readTextFile(entry.path(), labelPrefix.c_str(), content))
                continue;

            for (auto& exposedEntry : Gateway::parseExposedCharacters(content))
                exposed.push_back(std::move(exposedEntry));

            combined += "do\n" + content + "\nend\n";
        }
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

        for (const char* sub : { "skilltrees", "Skillstree", "skilltree" }) {
            auto subPath = entry.path() / sub;
            if (std::filesystem::exists(subPath)) {
                loadOverridesFromDirectory(*this, subPath, modName + "/" + sub);
                break;
            }
        }
    }
}

void Loader::loadCharactersFromDisk()
{
    constexpr const char* kTargetName = "Presentation/VirtualReaderPC_Data.lua";
    Logger& logger = Logger::getInstance();
    std::vector<Gateway::Entry> exposed;
    std::string combined;

    std::filesystem::path rootFolder = std::filesystem::current_path() / "characters";
    if (std::filesystem::exists(rootFolder)) {
        loadCharactersFromDirectory(rootFolder, "characters", exposed, combined);
    }

    std::filesystem::path modsFolder = std::filesystem::current_path() / "mods";
    if (std::filesystem::exists(modsFolder)) {
        for (const auto& entry : std::filesystem::directory_iterator(modsFolder)) {
            if (!entry.is_directory())
                continue;
            std::string modName = entry.path().filename().string();
            if (modName.empty() || modName[0] == '.' || modName[0] == '_')
                continue;

            for (const char* sub : { "characters", "Character", "character" }) {
                auto subPath = entry.path() / sub;
                if (std::filesystem::exists(subPath)) {
                    loadCharactersFromDirectory(subPath, modName + "/" + sub, exposed, combined);
                    break;
                }
            }
        }
    }

    if (!exposed.empty()) {
        registerNamedPatch(kTargetName, Gateway::buildSkuTableLua(exposed) + combined, "characters/");
        registerChunkPatch(Gateway::containerKey(), Gateway::buildInjectionLua(exposed), "figure registry");
        logger.info("Loader: {} total character definition(s) registered.", exposed.size());
    }
}
