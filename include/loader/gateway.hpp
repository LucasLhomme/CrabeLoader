/*
** CrabeLoader
** File description:
** gateway -- figure registry slots, built in memory
*/

#ifndef GATEWAY_HPP_
#define GATEWAY_HPP_

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
namespace Gateway {

// One character needing a registry slot.
struct Entry {
    std::string name;   // catalog Name, and the slot's `name` field
    std::string sku;    // empty -> derived from `name` by allocateSku
};

// Deterministic sku_id for a character name, inside the range reserved for
// mods. Arithmetic only, no bitwise ops, so src/api/12_virtualreader.lua can
// mirror it in Lua 5.1. Stable across installs, which is what lets a shared
// characters/*.lua keep working.
std::string allocateSku(const std::string& name);

// Byte string identifying the gateway chunk, which doubles as the _G key its
// slot table lives under. Serves as the patch's match hint.
const std::string& containerKey();

// Characters declared by exposeCharacter{...} in one characters/*.lua source.
// addCharacter rows are skipped on purpose: they reuse an existing character's
// sku_id, which already owns a slot.
std::vector<Entry> parseExposedCharacters(const std::string& luaSource);

// Lua source inserting one AVATAR slot per entry into the live registry, empty
// when `entries` is. Calls no standard library function: gateway chunks run in
// a Lua state with no base library loaded.
std::string buildInjectionLua(const std::vector<Entry>& entries);

// AES-128-CBC + PKCS7 under the gateway's fixed key and IV, behind the game's
// type tag. Exposed for tests and diagnostics.
std::string encryptString(const std::string& value);
std::string encryptNumber(double value);

// Lua string literal for arbitrary bytes. Decimal escapes are padded to three
// digits ("\011", never "\11"): Lua consumes up to three, so an unpadded escape
// merges with a digit that follows it.
std::string luaLiteral(const std::string& bytes);

}

#endif /* !GATEWAY_HPP_ */
