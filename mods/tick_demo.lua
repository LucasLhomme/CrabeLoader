-- CrabeLoader demo mod: proves Game.onTick fires every frame.
-- Drop any .lua file in this folder and the loader runs it once the game's
-- Lua state is up, with the Game API already available.

local frames = 0

-- Game.onTick(function()
--     frames = frames + 1

--     -- Roughly once per second at 60 fps. Printing every frame would flood the
--     -- console and cost a string build per frame for nothing.
--     if frames % 60 == 0 then
--         print("tick_demo: " .. frames .. " frames")
--     end
-- end)

print("tick_demo loaded")
