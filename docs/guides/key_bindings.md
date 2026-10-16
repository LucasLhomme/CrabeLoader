# Key Bindings

The in-game keybinds subsystem lets players rebind the game's own controls from the game's Options menu. It is built natively into `CrabeLoader.asi` without requiring any loose files in `mods/`.

## Using it

1. Open **Options**, from the main menu or the pause menu.
2. Right under **Controls** there are two entries: **Keyboard and mouse bindings** and **Controller bindings**.
3. Each page lists one row per action, with its current key drawn as the game's own key icon.
4. Choose a row, then press the new key, mouse button or controller button.
   - The action moves to that input, and the input's old action moves to the key you left, so no action is ever lost.
   - **Escape** or **Backspace** cancels; after eight seconds the row gives up by itself.
5. The last row of each page resets that page to the game's defaults.

The bindings are saved to `Crabe/keybinds.json`, next to `crabe.toml`, and restored at every launch. Deleting the file resets everything.

## How it works

- CrabeLoader remaps inputs below the game: `Crabe.Input.remapKey`, `remapMouseButton` and `remapPadButton` change what the game reads from DirectInput and XInput. The game's own control tables are never edited.
- Keyboard codes are DirectInput scan codes, so a binding follows the key's position whatever the layout. The icons follow the layout's own name for letter keys (AZERTY shows Z where QWERTY shows W).
- While a game menu has the input, the remaps step aside, so menus keep their usual keys.
- The pages reuse the game's `SettingsExtras` list screen, which the PC build never opens otherwise.

## Limits

- A key cannot be bound to a mouse button or the other way round; each action stays on its own device.
- Triggers and sticks are not remapped.
- The list of actions and their defaults is the game's PC default layout; an action the game binds elsewhere is not listed.

## Native C++23 Architecture

Key bindings are implemented entirely in compiled C++23 (`src/infrastructure/patch/patch_keybind.cpp`), exactly like resolution presets (`patch_resolution.cpp`) and borderless window mode (`patch_borderless.cpp`):
- Action definitions, DirectInput/XInput scan codes, and glyph markup formatting are computed in C++.
- Configurations are read and written to `Crabe/keybinds.json` using `nlohmann::json`.
- Conflicting actions are automatically swapped directly in the C++ remapping table (`InputRemap::get()`).
- The in-game UI is injected into the vanilla `SettingsMenu` and `SettingsExtras` screens via native C callbacks without requiring any Lua files in `mods/`.
