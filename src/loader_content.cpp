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

#include "loader/loader.hpp"
#include "loader/gateway.hpp"
#include "logger/logger.hpp"

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

void Loader::registerLoadOverride(std::string matchSubstring, std::string replacement)
{
    if (matchSubstring.empty())
        return;

    for (auto& entry : _loadOverrides) {
        if (entry.first == matchSubstring) {
            entry.second = std::move(replacement);
            return;
        }
    }
    _loadOverrides.emplace_back(std::move(matchSubstring), std::move(replacement));
}

void Loader::clearLoadOverrides()
{
    _loadOverrides.clear();
}

const std::string* Loader::findLoadOverride(const char* buff, size_t size) const
{
    if (!buff || size == 0 || _loadOverrides.empty())
        return nullptr;

    std::string_view haystack(buff, size);
    for (const auto& [match, replacement] : _loadOverrides) {
        if (haystack.find(match) != std::string_view::npos)
            return &replacement;
    }
    return nullptr;
}

void Loader::registerSkillTreePatch(std::string matchSubstring, std::string patchSource)
{
    if (matchSubstring.empty())
        return;

    for (auto& entry : _skillTreePatches) {
        if (entry.first == matchSubstring) {
            entry.second = std::move(patchSource);
            return;
        }
    }
    _skillTreePatches.emplace_back(std::move(matchSubstring), std::move(patchSource));
}

void Loader::armPatchIfMatched(const char* buff, size_t size)
{
    if (!buff || size == 0 || _skillTreePatches.empty())
        return;

    std::string_view haystack(buff, size);
    for (const auto& [match, patchSource] : _skillTreePatches) {
        if (haystack.find(match) != std::string_view::npos) {
            _armedPatchSource = patchSource;
            return;
        }
    }
}

bool Loader::hasArmedPatch() const
{
    return !_armedPatchSource.empty();
}

std::string Loader::takeArmedPatch()
{
    std::string result = _armedPatchSource;
    _armedPatchSource.clear();
    return result;
}

void Loader::registerNamedPatch(std::string exactName, std::string patchSource)
{
    if (exactName.empty())
        return;

    for (auto& entry : _namedPatches) {
        if (entry.first == exactName) {
            entry.second = std::move(patchSource);
            return;
        }
    }
    _namedPatches.emplace_back(std::move(exactName), std::move(patchSource));
}

void Loader::armPatchIfNameMatched(const char* name)
{
    if (!name || _namedPatches.empty())
        return;

    for (const auto& [exactName, patchSource] : _namedPatches) {
        if (exactName == name) {
            _armedPatchSource = patchSource;
            return;
        }
    }
}

void Loader::loadOverridesFromDisk()
{
    std::filesystem::path folder = std::filesystem::current_path() / "skilltrees";
    Logger& logger = Logger::getInstance();

    if (!std::filesystem::exists(folder)) {
        std::filesystem::create_directory(folder);
        return;
    }

    size_t loadedOverrides = 0;
    size_t loadedPatches = 0;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (!entry.is_regular_file())
            continue;

        bool isOverride = entry.path().extension() == ".lua";
        bool isPatch = entry.path().extension() == ".patch";
        if (!isOverride && !isPatch)
            continue;

        std::string matchHint = entry.path().stem().string();
        std::ifstream file(entry.path(), std::ios::binary);
        if (!file) {
            logger.warning("Loader: skilltrees/{} could not be opened, skipping.", entry.path().filename().string());
            continue;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (content.empty()) {
            logger.warning("Loader: skilltrees/{} is empty, skipping.", entry.path().filename().string());
            continue;
        }

        if (isOverride) {
            registerLoadOverride(matchHint, std::move(content));
            ++loadedOverrides;
        } else {
            registerSkillTreePatch(matchHint, std::move(content));
            ++loadedPatches;
        }
    }

    if (loadedOverrides > 0 || loadedPatches > 0) {
        logger.info("Loader: {} load override(s) and {} patch(es) registered from skilltrees/.",
                    loadedOverrides, loadedPatches);
    }
}

void Loader::loadCharactersFromDisk()
{
    constexpr const char* kTargetName = "Presentation/VirtualReaderPC_Data.lua";

    std::filesystem::path folder = std::filesystem::current_path() / "characters";
    Logger& logger = Logger::getInstance();

    if (!std::filesystem::exists(folder)) {
        std::filesystem::create_directory(folder);
        return;
    }

    std::string combined;
    std::vector<Gateway::Entry> exposed;
    size_t loadedFiles = 0;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua")
            continue;

        std::ifstream file(entry.path(), std::ios::binary);
        if (!file) {
            logger.warning("Loader: characters/{} could not be opened, skipping.", entry.path().filename().string());
            continue;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (content.empty()) {
            logger.warning("Loader: characters/{} is empty, skipping.", entry.path().filename().string());
            continue;
        }

        for (auto& exposedEntry : Gateway::parseExposedCharacters(content))
            exposed.push_back(std::move(exposedEntry));

        combined += "do\n" + content + "\nend\n";
        ++loadedFiles;
    }

    if (loadedFiles > 0) {
        registerNamedPatch(kTargetName, std::move(combined));
        logger.info("Loader: {} character definition(s) registered from characters/.", loadedFiles);
    }

    // Every exposeCharacter row needs a slot in the figure registry, or the game
    // refuses to play it ("Figurine Disney Infinity manquante"). Build those
    // slots here and hand them to the patch mechanism keyed on the gateway
    // chunk's own container key, so the registry is only ever changed in memory
    // -- the player's gateway*.lua files are left untouched.
    if (!exposed.empty()) {
        registerSkillTreePatch(Gateway::containerKey(), Gateway::buildInjectionLua(exposed));
        for (const auto& entry : exposed) {
            logger.info("Loader: figure registry slot for '{}' -> sku {}.",
                        entry.name,
                        entry.sku.empty() ? Gateway::allocateSku(entry.name) : entry.sku);
        }
    }
}
