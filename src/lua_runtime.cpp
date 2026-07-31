/*
** CrabeLoader
** File description:
** lua_runtime
*/

#include "loader/lua_runtime.hpp"
#include "loader/luacall.hpp"
#include "logger/logger.hpp"

namespace {

    // Output bridge. The loader cannot register a C function as `print` (that
    // needs lua_pushcclosure, which is not resolved), so the capture is done in
    // Lua: print appends to a buffer, and the C++ side periodically asks for
    // it with flush(). Everything a mod prints ends up in the overlay console.
    constexpr const char* kBridgeSource = R"lua(
CrabeBridge = CrabeBridge or {}
CrabeBridge._lines = {}
CrabeBridge._maxLines = 200

function CrabeBridge.write(line)
    local lines = CrabeBridge._lines
    lines[#lines + 1] = tostring(line)

    -- The game can print faster than the loader drains; dropping the oldest
    -- line bounds the buffer instead of growing it until the next flush.
    while #lines > CrabeBridge._maxLines do
        table.remove(lines, 1)
    end
end

function CrabeBridge.flush()
    local lines = CrabeBridge._lines
    if #lines == 0 then return "" end

    local joined = table.concat(lines, "\n")
    CrabeBridge._lines = {}
    return joined
end

-- Guarded: the runtime is injected once per captured lua_State, but chaining
-- the wrapper onto itself twice would double every line.
if not CrabeBridge._printHooked then
    CrabeBridge._printHooked = true
    local originalPrint = print

    print = function(...)
        local parts = {}
        for i = 1, select("#", ...) do
            parts[i] = tostring((select(i, ...)))
        end
        CrabeBridge.write(table.concat(parts, "\t"))

        if originalPrint then originalPrint(...) end
    end
end
)lua";

    // The modding API itself. Wraps the engine natives behind stable names, so
    // a mod says what it wants rather than which native happens to do it.
    constexpr const char* kGameSource = R"lua(
Game = Game or {}
Game._itemRegistry = Game._itemRegistry or {}

-- An item id is a dotted string ("Items.money"). Two kinds exist so far:
--   { kind = "currency", apply = function(amount) end }
--   { kind = "spawn", rrofile = "<INV_ name without the INV_ prefix>" }
function Game.registerItem(id, entry)
    if type(id) ~= "string" or type(entry) ~= "table" then
        error("Game.registerItem: expected (string, table)", 2)
    end
    Game._itemRegistry[id] = entry
end

Game.registerItem("Items.money", {
    kind = "currency",
    apply = function(amount) UI_IncrementSparks(amount) end,
})

function Game.GetSparks()
    return UI_GetSparks()
end

function Game.ShowMessage(text, body)
    UI_DisplayTextBox(tostring(text), body or "", false, 0,
                    Players_GetHostPlayerID(), "HelpBubble", 3)
end

-- Placing an object only works from the Toy Box editor's context, hence the
-- SetEditorState prefix: without it CalculateSpawnPosition yields nothing and
-- the ghost is created but never placed.
local function spawnOnce(player, rrofile)
    Place_SetEditorState(player, "Editor::ObjectMode")
    Place_CalculateSpawnPosition(player)
    Place_PlaceObject(player, Place_CreateGhost(player, 0, rrofile))
end

function Game.AddToInventory(id, amount)
    local entry = Game._itemRegistry[id]
    if not entry then
        error("Game.AddToInventory: unknown item id '" .. tostring(id) .. "'", 2)
    end
    amount = amount or 1

    if entry.kind == "currency" then
        entry.apply(amount)
        return amount
    end

    if entry.kind == "spawn" then
        local player = Players_GetHostPlayerID()
        local ok, err = true, nil

        for _ = 1, amount do
            ok, err = pcall(spawnOnce, player, entry.rrofile)
            if not ok then break end
        end

        -- StopPlaceMode has to run even when a spawn above failed, or the
        -- editor keeps the player's input locked.
        pcall(Place_StopPlaceMode, player, 0, false)

        if not ok then error(err, 2) end
        return amount
    end

    error("Game.AddToInventory: item '" .. tostring(id) .. "' has an unknown kind", 2)
end
)lua";

    struct Module {
        const char* name;
        const char* source;
    };

    // Injection order is dependency order: game.lua reports through the bridge.
    constexpr Module kModules[] = {
        { "bridge", kBridgeSource },
        { "game",   kGameSource },
    };

} // namespace

size_t LuaRuntime::moduleCount()
{
    return sizeof(kModules) / sizeof(kModules[0]);
}

const char* LuaRuntime::moduleName(size_t index)
{
    return index < moduleCount() ? kModules[index].name : "";
}

const char* LuaRuntime::moduleSource(size_t index)
{
    return index < moduleCount() ? kModules[index].source : "";
}

bool LuaRuntime::injectAll(void* L)
{
    Logger& logger = Logger::getInstance();
    bool allOk = true;

    for (size_t i = 0; i < moduleCount(); ++i) {
        std::string error;

        if (LuaCall::get().runSnippet(L, kModules[i].source, error)) continue;

        // Keep going: a broken module costs its own features, not the API.
        logger.error("LuaRuntime: module '{}' failed to inject: {}", kModules[i].name, error);
        allOk = false;
    }

    if (allOk) logger.info("LuaRuntime: API injected ({} modules).", moduleCount());
    return allOk;
}
