# Third-Party Dependencies

This directory provides isolated entry points for external libraries used by CrabeLoader:

- **nlohmann/json (v3.11.3)**: JSON parsing and serialization for mod manifests (`mod.json`).
- **toml++ (v3.4.0)**: TOML configuration parsing and formatting for game configuration (`crabe.toml`).

## Management via CMake FetchContent

To keep the repository clean and ensure Git contains strictly project code, third-party source archives are not vendored directly in Git. Instead, they are resolved at configure-time via CMake's `FetchContent` module using verified SHA-256 checksums.

## Warning Isolation

The local wrapper headers (`json.hpp` and `toml.hpp`) encapsulate third-party inclusions within compiler warning suppression pragmas (`#pragma warning(push, 0)` under MSVC), ensuring that external code does not interfere with the project's zero-warning (`/W4`) build policy.
