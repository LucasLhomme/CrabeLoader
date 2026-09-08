-- MyCustomMod - Main Entry Point
-- Built on CrabeLoader for Disney Infinity 3.0

local Mod = {
    name = "MyCustomMod",
    version = "1.0.0",
    active = false
}

-- 1. Initialize UI under CrabeMenu (F5 key in-game)
Crabe.Menu.registerCategory("MyCustomMod")

Crabe.Menu.addToggle("MyCustomMod", "Enable Mod Features", function()
    return Mod.active
end, function(val)
    Mod.active = val
    Crabe.Menu.setStatus(Mod.active and "MyCustomMod Enabled!" or "MyCustomMod Disabled.")
end)

Crabe.Menu.addItem("MyCustomMod", {
    label = "Trigger Test Action",
    action = function()
        Crabe.write("[MyCustomMod] Action button clicked!")
    end
})

-- 2. Hook into the Unified Event Bus
Crabe.Events.on("init", function()
    Crabe.write("[MyCustomMod] Initialized successfully via Crabe.Events!")
end)

Crabe.Events.on("update", function(dt)
    if not Mod.active then return end
    -- Put your per-frame mod logic here
end)

return Mod

