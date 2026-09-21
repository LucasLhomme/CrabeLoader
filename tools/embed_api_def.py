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


# MSVC refuses a single string literal over 16380 bytes (C2026), and the
# definition passed that the first time an API section was added to it. C++
# concatenates adjacent literals, so the text is emitted in pieces instead.
#
# The split is by source line, not by byte count, for two reasons: a line never
# straddles two literals, so no escape sequence can be cut in half; and a diff
# of the generated header stays readable.
MSVC_LITERAL_LIMIT = 16000


def as_cpp_literal(text: str) -> str:
    """One or more adjacent C++ string literals holding `text`.

    json.dumps escapes exactly the characters ("\\", '"', and the common
    control characters) that also need escaping in a C++ string literal, and
    crabe_api.def.lua is plain ASCII, so each piece is directly usable.
    """
    pieces: list[str] = []
    current = ""

    for line in text.splitlines(keepends=True):
        # A single line longer than the limit would still overflow; no line in
        # the definition is close, and splitting mid-line risks cutting an
        # escape sequence, so this is left to fail loudly rather than silently.
        if current and len(json.dumps(current + line)) > MSVC_LITERAL_LIMIT:
            pieces.append(json.dumps(current))
            current = line
        else:
            current += line

    pieces.append(json.dumps(current))

    if len(pieces) == 1:
        return pieces[0]
    separator = "\n        "
    return separator + separator.join(pieces)


def generate_header(source: Path, output: Path) -> None:
    if not source.exists():
        print(f"Error: '{source}' does not exist.", file=sys.stderr)
        sys.exit(1)

    text = source.read_text(encoding="utf-8")
    literal = as_cpp_literal(text)

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

    # A multi-piece literal starts on its own line, which leaves "= " dangling
    # with a trailing space. Generated files should not carry whitespace a
    # linter would flag.
    content = "\n".join(line.rstrip() for line in content.split("\n"))

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
