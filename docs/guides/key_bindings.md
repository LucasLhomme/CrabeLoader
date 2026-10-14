# Key Bindings

`mods/keybinds/` lets players rebind the game's own controls from the game's Options menu. It ships with CrabeLoader.

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

## Files

| File | Role |
|---|---|
| `main.lua` | Loads the saved bindings, adds the Options entries, runs the mod each tick |
| `bindings.lua` | The remap list, the swap rule, saving and loading |
| `controls_page.lua` | The two pages on the game's `SettingsExtras` screen, and the key capture |
| `options_rows.lua` | The two entries under Controls in the Options list |
| `menu_focus.lua` | Tells when a game menu has the input |
| `input.lua` | Reads the key, mouse button or pad button pressed during a capture, and the mouse wheel |
| `glyphs.lua` | Maps an input to the game's `cbi_*` key icon |

The offline tests are in `tests/lua/test_keybinds.lua`.
