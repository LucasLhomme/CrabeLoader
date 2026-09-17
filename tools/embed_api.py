#!/usr/bin/env python3
"""
Embed Lua API modules into a C++ header for CrabeLoader.
Scans src/api/*.lua in alphabetical order, removes BOMs, and outputs
include/application/embedded_api.hpp as static constexpr byte arrays.
"""

import sys
from pathlib import Path
import re

def sanitize_identifier(filename: str) -> str:
    # Replace non-alphanumeric chars with underscore
    clean = re.sub(r'[^a-zA-Z0-9_]', '_', filename)
    if clean and clean[0].isdigit():
        clean = "mod_" + clean
    return clean

def generate_header(api_dir: Path, output_file: Path):
    if not api_dir.exists():
        print(f"Error: API directory '{api_dir}' does not exist.", file=sys.stderr)
        sys.exit(1)

    lua_files = sorted(api_dir.glob("*.lua"))
    if not lua_files:
        print(f"Error: No .lua files found in '{api_dir}'.", file=sys.stderr)
        sys.exit(1)

    lines = []
    lines.append("/*")
    lines.append("** CrabeLoader")
    lines.append("** File description:")
    lines.append("** Auto-generated embedded Lua API modules. DO NOT EDIT DIRECTLY.")
    lines.append("*/\n")
    lines.append("#ifndef CRABELOADER_APPLICATION_EMBEDDED_API_HPP_")
    lines.append("#define CRABELOADER_APPLICATION_EMBEDDED_API_HPP_\n")
    lines.append("#include <cstddef>")
    lines.append("#include <string_view>\n")
    lines.append("namespace Crabe::EmbeddedApi {\n")
    lines.append("    namespace Modules {")

    modules_meta = []

    for path in lua_files:
        data = path.read_bytes()
        # Strip UTF-8 BOM if present
        if data.startswith(b'\xef\xbb\xbf'):
            data = data[3:]

        var_name = sanitize_identifier(path.name)
        modules_meta.append((path.name, var_name, len(data)))

        lines.append(f"\n        // {path.name} ({len(data)} bytes)")
        lines.append(f"        inline constexpr unsigned char {var_name}[] = {{")

        # Format hex in chunks of 16
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_bytes = ", ".join(f"0x{b:02x}" for b in chunk)
            lines.append(f"            {hex_bytes},")
        lines.append("        };")

    lines.append("\n    } // namespace Modules\n")
    lines.append("    struct Module {")
    lines.append("        std::string_view name;")
    lines.append("        const char* data;")
    lines.append("        size_t size;")
    lines.append("    };\n")
    lines.append("    inline const Module kModules[] = {")

    for name, var_name, length in modules_meta:
        lines.append(f'        {{ "{name}", reinterpret_cast<const char*>(Modules::{var_name}), sizeof(Modules::{var_name}) }},')

    lines.append("    };\n")
    lines.append(f"    inline constexpr size_t kModuleCount = {len(modules_meta)};\n")
    lines.append("} // namespace Crabe::EmbeddedApi\n")
    lines.append("#endif /* !CRABELOADER_APPLICATION_EMBEDDED_API_HPP_ */\n")

    output_content = "\n".join(lines)

    # Only write if content actually changed to avoid triggering unnecessary rebuilds
    if output_file.exists():
        existing = output_file.read_text(encoding='utf-8')
        if existing == output_content:
            print(f"[embed_api] {output_file.name} is up to date ({len(modules_meta)} modules).")
            return

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(output_content, encoding='utf-8')
    print(f"[embed_api] Generated {output_file.name} with {len(modules_meta)} modules.")

def main():
    repo_root = Path(__file__).resolve().parent.parent
    api_dir = repo_root / "src" / "api"
    output_file = repo_root / "include" / "application" / "embedded_api.hpp"

    if len(sys.argv) >= 3:
        api_dir = Path(sys.argv[1])
        output_file = Path(sys.argv[2])

    generate_header(api_dir, output_file)

if __name__ == "__main__":
    main()

