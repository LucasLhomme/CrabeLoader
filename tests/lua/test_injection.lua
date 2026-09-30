local harness = ...

local assertEquals = harness.assertEquals
local assertNil = harness.assertNil
local assertTrue = harness.assertTrue
local assertFalse = harness.assertFalse
local assertNotNil = harness.assertNotNil

local kApostrophePath = [[C:/mods/Bob's "cool" mod\main.lua]]

local kExploitPath = [[x'); os.exit() --]]

local suite = { name = "Mod-loading chunk injection (T5)", cases = {} }

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

local function arm(state)
    local seen = { exited = false }

    state.os = setmetatable({ exit = function() seen.exited = true end },
                            { __index = os })

    state.loadfile = function(path)
        seen.path = path
        return nil, "test loadfile: no such file"
    end

    local realCreate = state.Crabe.Sandbox.create
    state.Crabe.Sandbox.create = function(modName)
        seen.modName = modName
        return realCreate(modName)
    end

    return seen
end

suite.cases[#suite.cases + 1] = {
    "a path with quotes and backslashes round-trips through the package-path chunk",
    function(state)
        local chunk = assert(loadstring(readChunk("kSetPackagePathChunk"), "=kSetPackagePathChunk"))
        local seen = arm(state)

        state.package = { path = "ORIGINAL" }
        setfenv(chunk, state)
        chunk(kApostrophePath)

        assertFalse(seen.exited, "nothing was executed")
        assertEquals(state.package.path,
                     kApostrophePath .. "/?.lua;" .. kApostrophePath .. "/modules/?.lua;ORIGINAL",
                     "the path is concatenated verbatim, apostrophes and backslashes intact")
    end,
}

suite.cases[#suite.cases + 1] = {
    "an os.exit payload through the package-path chunk is inert data",
    function(state)
        local chunk = assert(loadstring(readChunk("kSetPackagePathChunk"), "=kSetPackagePathChunk"))
        local seen = arm(state)

        state.package = { path = "ORIGINAL" }
        setfenv(chunk, state)
        chunk(kExploitPath)

        assertFalse(seen.exited, "os.exit was not called")
        assertEquals(state.package.path,
                     kExploitPath .. "/?.lua;" .. kExploitPath .. "/modules/?.lua;ORIGINAL",
                     "the payload landed in package.path as a plain string")
    end,
}

suite.cases[#suite.cases + 1] = {
    "a path with quotes and backslashes reaches the mod-loading chunk unchanged",
    function(state)
        local chunk = assert(loadstring(readChunk("kLoadModChunk"), "=kLoadModChunk"))
        local seen = arm(state)

        setfenv(chunk, state)
        local ok = pcall(chunk, kApostrophePath, "Bob's mod")

        assertFalse(ok, "the chunk propagates the loadfile failure")
        assertEquals(seen.path, kApostrophePath, "the path arrived byte-identical")
        assertEquals(seen.modName, "Bob's mod", "the mod name arrived byte-identical")
        assertFalse(seen.exited, "nothing was executed")
        assertNil(state.Crabe.Registry._current, "the owner is cleared on the failure path too")
    end,
}

suite.cases[#suite.cases + 1] = {
    "an os.exit payload through the mod-loading chunk is inert data",
    function(state)
        local chunk = assert(loadstring(readChunk("kLoadModChunk"), "=kLoadModChunk"))
        local seen = arm(state)

        setfenv(chunk, state)
        pcall(chunk, kExploitPath, kExploitPath)

        assertFalse(seen.exited, "os.exit was not called")
        assertEquals(seen.path, kExploitPath, "the payload arrived as a path, not as code")
        assertEquals(seen.modName, kExploitPath, "and as a name, not as code")
    end,
}


suite.cases[#suite.cases + 1] = {
    "NEGATIVE CONTROL: the pre-T5 interpolated mod loader executes the payload",
    function(state)
        local seen = arm(state)

        local preT5 = string.format(
            "if Crabe and Crabe.Sandbox and Crabe.Sandbox.create then\n" ..
            "    local env = Crabe.Sandbox.create('%s')\n" ..
            "    local chunk, err = loadfile('%s')\n" ..
            "    if chunk then\n" ..
            "        setfenv(chunk, env)\n" ..
            "        local ok, runErr = pcall(chunk)\n" ..
            "        if not ok then error(runErr) end\n" ..
            "    else\n" ..
            "        error(err)\n" ..
            "    end\n" ..
            "end\n",
            "mod", kExploitPath)

        local chunk = loadstring(preT5, "=pre-T5 interpolation")
        assertNotNil(chunk, "the payload was crafted to keep the source compiling")

        setfenv(chunk, state)
        pcall(chunk)

        assertTrue(seen.exited,
                   "the pre-T5 construction must execute os.exit -- if it does not, " ..
                   "the passing cases above prove nothing")
    end,
}

suite.cases[#suite.cases + 1] = {
    "NEGATIVE CONTROL: the pre-T5 interpolated package path breaks on an apostrophe",
    function(state)
        local name = "mods/Bob's mod"

        local preT5 = string.format(
            "package.path = '%s/?.lua;%s/modules/?.lua;' .. package.path", name, name)

        local chunk, err = loadstring(preT5, "=pre-T5 interpolation")
        assertNil(chunk, "the interpolated source must not compile")
        assertTrue(err ~= nil, "and Lua must say why")

        local shipped = assert(loadstring(readChunk("kSetPackagePathChunk"), "=kSetPackagePathChunk"))
        state.package = { path = "ORIGINAL" }
        setfenv(shipped, state)
        shipped(name)

        assertEquals(state.package.path,
                     name .. "/?.lua;" .. name .. "/modules/?.lua;ORIGINAL",
                     "Bob's mod loads rather than erroring")
    end,
}

return suite
