-- Stand-ins for the Disney Infinity engine natives the API modules reach for.
--
-- These are the globals the game's Lua state exposes, not the API's own
-- wrappers: stubbing Players_IsCharacterDead leaves Game.IsCharacterDead --
-- the code actually under test -- running for real. Overriding the wrapper
-- instead would replace the thing being tested with the test's own answer.
--
-- Deliberately minimal: a stub exists here only because a module under test
-- calls it, and no test asserts on a stub's own behaviour. Behaviour that can
-- only be observed inside the game belongs in
-- docs/testing/manual_checklist.md instead.

local stubs = {}

-- Installs the natives into `env` and returns the table tests drive them with.
-- Each API state gets its own.
function stubs.install(env)
    local natives = {
        -- playerId -> boolean. Game.onDeath polls once per tick, so a test
        -- flips this between Game._runTicks calls to produce a death.
        dead = {},

        -- Every poll, in order, so a test can tell "the death pump never ran"
        -- apart from "it ran and saw nothing".
        deadPolls = {},

        hostPlayerId = 0,
    }

    function env.Players_IsCharacterDead(playerId, characterIndex)
        local id = playerId or 0
        natives.deadPolls[#natives.deadPolls + 1] = { id, characterIndex }
        return natives.dead[id] == true
    end

    function env.Players_GetHostPlayerID()
        return natives.hostPlayerId
    end

    return natives
end

return stubs
