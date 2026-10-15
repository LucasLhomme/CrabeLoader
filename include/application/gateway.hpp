/*
** CrabeLoader
** File description:
** Builds the game's figure registry slots as Lua source, for injection in memory at boot.
** Without a slot the game refuses the character -- "Figurine Disney Infinity manquante".
** Writes no game file and reads no slot back; encryption only, the registry is never decrypted.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_APPLICATION_GATEWAY_HPP_
#define CRABELOADER_APPLICATION_GATEWAY_HPP_

#include <string>
#include <vector>

namespace crabe::application::gateway {

    struct Entry {
        std::string name;
        std::string sku;
        std::string baseCharacter;
        std::string displayName;
        std::string icon;
        std::string progressionTree;
        std::string origin;
    };

    // Generates deterministic SKU ID for a character name.
    [[nodiscard]] std::string allocateSku(const std::string& name);

    // Resolves unique SKU IDs among mod entries and reports conflicts.
    [[nodiscard]] std::vector<std::string> resolveSkus(std::vector<Entry>& entries);

    // Returns gateway chunk identifier key.
    [[nodiscard]] const std::string& containerKey();

    // Parses characters declared via exposeCharacter in Lua source.
    [[nodiscard]] std::vector<Entry> parseExposedCharacters(const std::string& luaSource);

    // Parses character definitions from manifest JSON source.
    [[nodiscard]] std::vector<Entry> parseManifestCharacters(const std::string& jsonSource,
                                                             const std::string& origin);

    // Builds Lua code to inject avatar slots into live registry.
    [[nodiscard]] std::string buildInjectionLua(const std::vector<Entry>& entries);

    // Builds Lua source defining name-to-SKU mapping table.
    [[nodiscard]] std::string buildSkuTableLua(const std::vector<Entry>& entries);

    // Builds Lua call to expose a manifest character.
    [[nodiscard]] std::string buildExposeCallLua(const Entry& entry);

    // Builds ActorList patch Lua source for baseCharacter entries.
    [[nodiscard]] std::string buildActorListPatchLua(const std::vector<Entry>& entries);

    // Builds DataMap patch Lua source for baseCharacter entries.
    [[nodiscard]] std::string buildDataMapPatchLua(const std::vector<Entry>& entries);

    // Encrypts string using AES-128-CBC with gateway key and IV.
    [[nodiscard]] std::string encryptString(const std::string& value);

    // Encrypts number using AES-128-CBC with gateway key and IV.
    [[nodiscard]] std::string encryptNumber(double value);

    // Formats arbitrary bytes as a padded Lua string literal.
    [[nodiscard]] std::string luaLiteral(const std::string& bytes);

} // namespace crabe::application::gateway

#endif /* !CRABELOADER_APPLICATION_GATEWAY_HPP_ */
