-- CrabeLoader
-- File description:
-- Skill-tree and ability overrides, declared by a mod for the loader to apply.
-- Progression data loads as anonymous bytecode before mods run, so overrides are authored in files.
-- Applies nothing here: the skilltrees/ folder is read by the loader C++, before mods run.
--
-- Authors: @LucasLhomme

Crabe.SkillTree = Crabe.SkillTree or {}

-- Registers a live chunk load override matched by bytecode string hint.
function Crabe.SkillTree.override(matchHint, luaSource)
    if type(matchHint) ~= "string" or matchHint == "" then
        error("Crabe.SkillTree.override: matchHint must be a non-empty string", 2)
    end
    if type(luaSource) ~= "string" or luaSource == "" then
        error("Crabe.SkillTree.override: luaSource must be a non-empty Lua source string", 2)
    end
    Crabe._registerLoadOverride(matchHint, luaSource)
end

-- Clears all registered chunk load overrides.
function Crabe.SkillTree.clearOverrides()
    Crabe._clearLoadOverrides()
end
