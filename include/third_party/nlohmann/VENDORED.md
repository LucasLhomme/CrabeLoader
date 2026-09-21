# nlohmann/json — vendored single header

| | |
|---|---|
| Library | [JSON for Modern C++](https://github.com/nlohmann/json) |
| Version | **3.11.3** (tag `v3.11.3`, released 2023-11-28) |
| File | `json.hpp`, the official single-include release artefact |
| Source | <https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp> |
| SHA256 | `9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6` |
| Size | 919975 bytes |
| License | MIT — see `LICENSE.MIT` (SHA256 `86b998c792894ccb911a1cb7994f7a9652894e7a094c0b5e45be2f553f45cf14`) |

Verify with:

```bash
sha256sum include/third_party/nlohmann/json.hpp
```

## Why vendored rather than fetched

`tests/CMakeLists.txt` pulls doctest and Lua with `FetchContent`, but that
subdirectory is opt-in (`CRABELOADER_BUILD_TESTS`, off by default). The shipped
`bink2w32.dll` is built by the *default* configuration, which downloads nothing
— the CI build job and the release job both depend on that. `mod.json` parsing
lives in the DLL, so its JSON library must be in the tree, not on the network.

## Do not edit

The file is upstream's, byte for byte; the checksum above is the only thing
that makes it auditable. Include it through `include/third_party/json.hpp`,
which wraps it in a warning-level pragma so upstream's warnings stay out of the
project's zero-warning budget. Updating means replacing the file wholesale and
updating this note.
