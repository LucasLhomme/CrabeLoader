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

// The gateway (assets/gateway*.lua) is the game's figure registry: it maps an
// encrypted sku_id to a character record, and a character with no slot is
// refused at selection time -- "Figurine Disney Infinity manquante" -- however
// correct its catalog row and actor data are.
//
// This module builds the missing slots as Lua source, for the patch mechanism
// to run right after the gateway chunk loads. The registry is therefore only
// ever changed in memory, and a modder declares nothing but a character name.
namespace crabe::application::gateway {

    // One character needing a registry slot.
    struct Entry {
        std::string name;   // catalog Name, and the slot's `name` field
        std::string sku;    // empty -> derived from `name` by allocateSku
        std::string origin; // file that declared it, for diagnostics only
    };

    // Deterministic sku_id for a character name, inside the range reserved for
    // mods. Arithmetic only, no bitwise ops, so src/api/12_virtualreader.lua can
    // mirror it in Lua 5.1. Stable across installs, which is what lets a shared
    // characters/*.lua keep working.
    [[nodiscard]] std::string allocateSku(const std::string& name);

    // Gives every entry a unique sku_id among mod entries and returns one message per
    // problem: duplicate Names are dropped, a taken explicit id drops its entry, a
    // taken derived id moves up by one (in Name order) until free.
    [[nodiscard]] std::vector<std::string> resolveSkus(std::vector<Entry>& entries);

    // Byte string identifying the gateway chunk, which doubles as the _G key its
    // slot table lives under. Serves as the patch's match hint.
    [[nodiscard]] const std::string& containerKey();

    // Characters declared by exposeCharacter{...} in one characters/*.lua source.
    // addCharacter rows are skipped on purpose: they reuse an existing character's
    // sku_id, which already owns a slot.
    [[nodiscard]] std::vector<Entry> parseExposedCharacters(const std::string& luaSource);

    // Lua source inserting one AVATAR slot per entry into the live registry, never
    // overwriting an existing slot. Calls no standard library function: gateway
    // chunks run in a Lua state with no base library loaded.
    [[nodiscard]] std::string buildInjectionLua(const std::vector<Entry>& entries);

    // Lua source defining the Name -> sku_id table for these entries, so the Lua
    // API can look ids up instead of re-deriving them. Keeping the hash on one
    // side only is what stops a row and its registry slot from disagreeing.
    [[nodiscard]] std::string buildSkuTableLua(const std::vector<Entry>& entries);

    // AES-128-CBC + PKCS7 under the gateway's fixed key and IV, behind the game's
    // type tag. Exposed for tests and diagnostics.
    [[nodiscard]] std::string encryptString(const std::string& value);
    [[nodiscard]] std::string encryptNumber(double value);

    // Lua string literal for arbitrary bytes. Decimal escapes are padded to three
    // digits ("\011", never "\11"): Lua consumes up to three, so an unpadded escape
    // merges with a digit that follows it.
    [[nodiscard]] std::string luaLiteral(const std::string& bytes);

} // namespace crabe::application::gateway

#endif /* !CRABELOADER_APPLICATION_GATEWAY_HPP_ */
