-- Skill-tree / ability overrides. See skilltrees/README.md for the full
-- picture -- short version: the per-character gamedb/progression data loads
-- as anonymous (name=NULL) bytecode, and before mods run, so it must be
-- authored from the skilltrees/ folder (read by the loader's C++ before the
-- hooks install), not from here:
--   skilltrees/<matchHint>.patch  -- a few `ProgressionTable.Foo.Bar = value`
--                                     lines, applied after the original
--                                     chunk runs. Use this by default.
--   skilltrees/<matchHint>.lua    -- full chunk replacement, for the rare
--                                     case a patch doesn't fit.
-- Crabe.SkillTree.override() below is the live-callable form of the second
-- one (Crabe._registerLoadOverride) -- it will not reach gamedb/core files
-- in time, only whatever loads after mods run.

Crabe.SkillTree = Crabe.SkillTree or {}

-- matchHint: a string guaranteed to appear in the target chunk's own
-- compiled bytes (source or bytecode) -- there is no filename to match on.
-- luaSource: full replacement Lua source (plain text; Lua 5.1 loads source
-- and precompiled bytecode transparently, no recompilation needed).
function Crabe.SkillTree.override(matchHint, luaSource)
    if type(matchHint) ~= "string" or matchHint == "" then
        error("Crabe.SkillTree.override: matchHint must be a non-empty string", 2)
    end
    if type(luaSource) ~= "string" or luaSource == "" then
        error("Crabe.SkillTree.override: luaSource must be a non-empty Lua source string", 2)
    end
    Crabe._registerLoadOverride(matchHint, luaSource)
end

function Crabe.SkillTree.clearOverrides()
    Crabe._clearLoadOverrides()
end
