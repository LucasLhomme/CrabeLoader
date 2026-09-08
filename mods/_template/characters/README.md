# Custom Characters

Drop your character registration scripts (`*.lua`) here.
CrabeLoader automatically scans this folder when your mod is installed in `mods/`.

### Example (`Hulk.lua`):
```lua
Crabe.VirtualReader.exposeCharacter({
    Name = "AVG_Hulk",
    baseCharacter = "AVG_Thor"
})
```

