#!/usr/bin/env python3
"""
Embed docs/crabe_api.def.lua into a C++ header for crabe-cli's `check-api`
command.

Mirrors what embed_api.py already does for src/api/*.lua -> embedded_api.hpp,
kept as a separate script because that one feeds the shipped DLL and this one
feeds an opt-in developer tool: crabe-cli carries its own copy of the ground
truth so `check-api` works against a mod living anywhere, without needing the
CrabeLoader source tree nearby. `--api-def <path>` on the command line can
still point it at a different file; this is only the default.
"""

import json
import sys
from pathlib import Path


def generate_header(source: Path, output: Path) -> None:
    if not source.exists():
        print(f"Error: '{source}' does not exist.", file=sys.stderr)
        sys.exit(1)

    text = source.read_text(encoding="utf-8")
    # json.dumps escapes exactly the characters ("\\", '"', and the common
    # control characters) that also need escaping in a C++ string literal,
    # and crabe_api.def.lua is plain ASCII, so the result is a valid,
    # directly-usable C++ string literal.
    literal = json.dumps(text)

    content = (
        "/*\n"
        "** CrabeLoader\n"
        "** File description:\n"
        "** Auto-generated embedded copy of docs/crabe_api.def.lua. DO NOT EDIT DIRECTLY.\n"
        "*/\n\n"
        "#ifndef CRABELOADER_CLI_EMBEDDED_API_DEF_HPP_\n"
        "#define CRABELOADER_CLI_EMBEDDED_API_DEF_HPP_\n\n"
        "#include <string_view>\n\n"
        "namespace crabe::cli {\n\n"
        f"    inline constexpr std::string_view kEmbeddedApiDef = {literal};\n\n"
        "} // namespace crabe::cli\n\n"
        "#endif /* !CRABELOADER_CLI_EMBEDDED_API_DEF_HPP_ */\n"
    )

    if output.exists() and output.read_text(encoding="utf-8") == content:
        print(f"[embed_api_def] {output.name} is up to date.")
        return

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(content, encoding="utf-8")
    print(f"[embed_api_def] Generated {output.name} ({len(text)} bytes embedded).")


def main() -> None:
    repo_root = Path(__file__).resolve().parent.parent
    source = repo_root / "docs" / "crabe_api.def.lua"
    output = repo_root / "include" / "cli" / "embedded_api_def.hpp"

    if len(sys.argv) >= 3:
        source = Path(sys.argv[1])
        output = Path(sys.argv[2])

    generate_header(source, output)


if __name__ == "__main__":
    main()
