local harness = ...

local assertEquals = harness.assertEquals
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertContains = harness.assertContains
local assertNotContains = harness.assertNotContains

-- What a mod sees through Crabe.Sandbox must be what the API sees. Lua 5.1 has
-- no __len, and ipairs/pairs/next ignore __index, so a proxy standing in for a
-- table reads as empty to all four. A deep proxy did exactly that to every
-- table under Crabe: `#Crabe.Menu.stack` read 0 inside CrabeMenu and the menu
-- drew "No mod has registered a menu entry" while the API held nine entries.
-- Every test before this one called Crabe.* from the API's own environment,
-- never from a mod's, which is how that shipped with the suite green.

local suite = { name = "Mod sandbox: what a mod sees (01_sandbox.lua)", cases = {} }

local kFixtureModName = "sandbox_menu_fixture"

local function readChunk(constantName)
    local source = harness.readFile(harness.path("src", "domain", "mod_manager.cpp"))
    local marker = constantName .. ' = R"LUA('
    local from = source:find(marker, 1, true)
    if not from then
        error("could not find " .. constantName ..
              " in src/domain/mod_manager.cpp -- has the constant been renamed?", 0)
    end
    local bodyStart = from + #marker
    local bodyEnd = source:find(')LUA"', bodyStart, true)
    if not bodyEnd then
        error("unterminated R\"LUA(...)LUA\" literal for " .. constantName, 0)
    end
    return source:sub(bodyStart, bodyEnd - 1)
end

-- In the game Crabe lives in the real _G; in a harness state it lives in the
-- state's own environment. Pointing the state's _G at itself gives the sandbox
-- the same view it has in game, so `_G.Crabe` from a mod resolves to Crabe.
local function gameLikeState(state)
    state._G = state
    return state
end

-- Runs `source` as a mod would run: in a fresh Crabe.Sandbox env.
local function runAsMod(state, source, modName)
    local env = state.Crabe.Sandbox.create(modName or "test_mod")
    local chunk = assert(loadstring(source, "=" .. (modName or "test_mod")))
    setfenv(chunk, env)
    return chunk()
end

local function countIpairs(t)
    local n = 0
    for _ in ipairs(t) do n = n + 1 end
    return n
end

local function countPairs(t)
    local n = 0
    for _ in pairs(t) do n = n + 1 end
    return n
end

local function seedMenu(state, count)
    for i = 1, count do
        state.Crabe.Menu.register({ label = "entry " .. i, action = function() end })
    end
end

suite.cases[#suite.cases + 1] = {
    "a mod reads the menu stack and its items with # and ipairs",
    function(state)
        gameLikeState(state)
        seedMenu(state, 3)

        local depth, items, walked = runAsMod(state, [[
            local stack = Crabe.Menu.stack
            local menu = stack[#stack].menu
            local n = 0
            for _ in ipairs(menu.items) do n = n + 1 end
            return #stack, #menu.items, n
        ]])

        assertEquals(depth, 1, "#Crabe.Menu.stack from a mod")
        assertEquals(items, 3, "#items of the top menu from a mod")
        assertEquals(walked, 3, "ipairs over the top menu's items from a mod")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a mod gets the real nested tables, so its reads stay live",
    function(state)
        gameLikeState(state)

        local root = runAsMod(state, "return Crabe.Menu.root")
        assertTrue(root == state.Crabe.Menu.root, "Crabe.Menu.root from a mod is the API's own table")

        -- Registered after the mod took its reference: a snapshot would miss it.
        seedMenu(state, 2)
        assertEquals(#root.items, 2, "#items through the reference the mod kept")
    end,
}

-- The generic net: every table reachable under Crabe, compared through the
-- sandbox and outside it. A module added later is covered without a new case.
suite.cases[#suite.cases + 1] = {
    "every table under Crabe reads the same from a mod as from the API",
    function(state)
        gameLikeState(state)
        seedMenu(state, 4)

        local env = state.Crabe.Sandbox.create("walker")
        local mismatches = {}
        local seen = {}

        local function compare(real, view, path, depth)
            if seen[real] or depth > 4 then return end
            seen[real] = true

            -- Namespaces stay proxied on purpose; # is meaningless on them
            -- (string keys only), but iteration must still see their keys.
            if view ~= real then
                local n = 0
                for _ in env.pairs(view) do n = n + 1 end
                if n ~= countPairs(real) then
                    mismatches[#mismatches + 1] = path .. ": pairs saw " .. n .. ", API has " .. countPairs(real)
                end
            else
                if #view ~= #real then
                    mismatches[#mismatches + 1] = path .. ": # read " .. #view .. ", API has " .. #real
                end
                if countIpairs(view) ~= countIpairs(real) then
                    mismatches[#mismatches + 1] = path .. ": ipairs saw " .. countIpairs(view)
                end
            end

            for key, value in pairs(real) do
                if type(value) == "table" then
                    compare(value, view[key], path .. "." .. tostring(key), depth + 1)
                end
            end
        end

        compare(state.Crabe, env.Crabe, "Crabe", 0)
        if state.Game then compare(state.Game, env.Game, "Game", 0) end

        if #mismatches > 0 then
            error("a mod sees different tables than the API:\n  " .. table.concat(mismatches, "\n  "), 0)
        end
    end,
}

suite.cases[#suite.cases + 1] = {
    "pairs, ipairs and next over a protected namespace see its real keys",
    function(state)
        gameLikeState(state)

        local pairsCount, nextKey = runAsMod(state, [[
            local n = 0
            for _ in pairs(Crabe.Menu) do n = n + 1 end
            return n, next(Crabe.Menu) ~= nil
        ]])

        assertEquals(pairsCount, countPairs(state.Crabe.Menu), "pairs(Crabe.Menu) from a mod")
        assertTrue(nextKey, "next(Crabe.Menu) from a mod finds a key")

        local crabeKeys = runAsMod(state, [[
            local n = 0
            for _ in pairs(Crabe) do n = n + 1 end
            return n
        ]])
        assertEquals(crabeKeys, countPairs(state.Crabe), "pairs(Crabe) from a mod")
    end,
}

suite.cases[#suite.cases + 1] = {
    "replacing a namespace is blocked, directly and through _G",
    function(state)
        gameLikeState(state)
        local realMenu = state.Crabe.Menu

        local okDirect, errDirect = pcall(runAsMod, state, "Crabe.Menu = {}", "rogue")
        assertFalse(okDirect, "Crabe.Menu = {} from a mod")
        assertContains(tostring(errDirect), "Security Violation", "the error names the violation")
        assertContains(tostring(errDirect), "rogue", "the error names the mod")

        local okGlobal = pcall(runAsMod, state, "_G.Crabe.Menu = {}", "rogue")
        assertFalse(okGlobal, "_G.Crabe.Menu = {} from a mod")

        local okNew = pcall(runAsMod, state, "Crabe.Injected = true", "rogue")
        assertFalse(okNew, "adding a key to Crabe from a mod")

        assertTrue(state.Crabe.Menu == realMenu, "Crabe.Menu is still the API's table")
        assertTrue(state.Crabe.Injected == nil, "Crabe gained no key")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a mod's own globals stay in its environment",
    function(state)
        gameLikeState(state)

        runAsMod(state, "myHelper = 42; function buildSomething() return 1 end", "tidy")
        assertTrue(rawget(state, "myHelper") == nil, "a mod global does not reach the shared state")
        assertTrue(rawget(state, "buildSomething") == nil, "a mod function does not reach the shared state")
    end,
}

-- End to end: the fixture goes through mod_manager.cpp's own kLoadModChunk and
-- the real lifecycle, and draws into a recording ImGui.
suite.cases[#suite.cases + 1] = {
    "a menu mod loaded by ModManager's chunk draws its entries, not the empty-menu text",
    function(state)
        gameLikeState(state)

        local drawn = { selectables = {}, disabled = {} }
        state.ImGui = {
            Selectable = function(label) drawn.selectables[#drawn.selectables + 1] = label end,
            TextDisabled = function(text) drawn.disabled[#drawn.disabled + 1] = text end,
        }

        local loadMod = assert(loadstring(readChunk("kLoadModChunk"), "=kLoadModChunk"))
        setfenv(loadMod, state)
        loadMod(harness.path("tests", "lua", "fixtures", "sandbox_menu_mod.lua"), kFixtureModName)

        state.Crabe.Mod.dispatchInit()
        state.Crabe.Mod.dispatchDraw()

        local disabledText = table.concat(drawn.disabled, "\n")
        assertNotContains(disabledText, "No mod has registered", "the menu drew its empty-state text")
        assertEquals(#drawn.selectables, 2, "top-level rows drawn (Cheats, About)")
        assertContains(drawn.selectables[1], "Cheats", "first row")
        assertContains(drawn.selectables[2], "About", "second row")
    end,
}

return suite
