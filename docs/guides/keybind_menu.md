# Binding pages in the native Controls screen

Keyboard/mouse and controller binding entries are added beside Controls in both
SettingsMenu and InGameSettingsMenu. They open SettingsControls with a separate
binding-page mode. Its original screen definition, layout, enter/movie/exit
lifecycle and normal Controls behavior are retained. Only instances explicitly
opened as binding pages replace their rows and header. Capture and reset use
the existing native remapping bridge and Crabe/keybinds.json persistence.

Earlier local previews reused SettingsExtras. Those previews opened off-screen
with blank rows despite returning correct Lua data. Supplying a data provider
alone did not fix the visual failure. That approach has been replaced.

Live verification in the F: game confirmed centered keyboard bindings, readable
action labels and key icons, scrolling, and a native Cancel transition back to
the complete Options menu. Windows injected keyboard/mouse input did not produce
observable navigation in this session; the final Back check invoked the game's
native Cancel handler through the loader console. End-to-end physical rebinding
and persistence still need user testing. 121 Lua tests pass, including capture,
reset, native lifecycle preservation and ordinary Controls selection.

PC assets supplied the working layout. Wii U assets/executable may be useful
comparative references, but have not been used to claim PC input compatibility.
S9 and source sound tables are not changed.
