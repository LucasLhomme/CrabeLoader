# toml++ — vendored single header

| | |
|---|---|
| Library | [toml++](https://github.com/marzer/tomlplusplus) |
| Version | **3.4.0** (tag `v3.4.0`, released 2023-10-13) |
| File | `toml.hpp`, the repository's own pre-assembled single-include artefact (root of the tag, not a release asset -- v3.4.0 attaches none) |
| Source | <https://raw.githubusercontent.com/marzer/tomlplusplus/v3.4.0/toml.hpp> |
| SHA256 | `6b5172ad4dd6519aec67b919181fa7a38a2234131e5b2afa232dfe444819783e` |
| Size | 485931 bytes |
| License | MIT — see `LICENSE` (SHA256 `529bc3900a9571e49db285b0df432397e70b881cc3bf48de6667ae74ff4b06d8`) |

Verify with:

```bash
sha256sum include/third_party/tomlplusplus/toml.hpp
```

## Why vendored rather than fetched

`tests/CMakeLists.txt` pulls doctest and Lua with `FetchContent`, but that
subdirectory is opt-in (`CRABELOADER_BUILD_TESTS`, off by default). The shipped
`bink2w32.dll` is built by the *default* configuration, which downloads nothing
— the CI build job and the release job both depend on that. `crabe.toml`
parsing (T11) lives in the DLL, so its TOML library must be in the tree, not on
the network. Same reasoning, same precedent as `include/third_party/nlohmann/`.

## Do not edit

The file is upstream's, byte for byte; the checksum above is the only thing
that makes it auditable. Include it through `include/third_party/toml.hpp`,
which wraps it in a warning-level pragma so upstream's warnings stay out of the
project's zero-warning budget. Updating means replacing the file wholesale and
updating this note.
