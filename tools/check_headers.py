#!/usr/bin/env python3
"""Check the file header on every non-vendored, non-generated source file.

    python tools/check_headers.py            # check, exit 1 on any offence
    python tools/check_headers.py --list     # print every file in scope and exit 0

The header is the first thing in the file:

    /*
    ** CrabeLoader
    ** File description:
    ** <what this file is responsible for>
    ** <the constraint or mechanism a reader must know>
    ** <what it deliberately does not do, or what it defers to>
    **
    ** Authors: @LucasLhomme
    */

Lua files in src/api/ carry the same content with `--` comments.

What is enforced here, and why only this much: a missing header, a brief that is
not exactly three lines, a brief line that merely restates the filename, and an
empty or malformed `Authors:` line. Whether a brief is *worth reading* is a
review question and cannot be checked by a script -- what a script can do is
stop the two failure modes that reliably creep back in, an absent header and a
description that says nothing.

The three-line rule is three *physical* lines, which is stricter than the
example in the work order, whose second line wraps. A wrap-tolerant check
cannot tell a three-line brief from a five-line one, so the line is the unit
and the budget below is what keeps that workable.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

# Vendored: upstream's code, never edited here (see include/third_party/*/VENDORED.md
# and the MinHook and Dear ImGui trees).
VENDORED_DIRS = (
    "include/imgui",
    "include/minhook",
    "include/third_party",
    "src/minhook",
)

# Multiplayer subsystem: its headers are not yet aligned with the format and it is
# left untouched for now. Drop this tuple once they are, so the gate covers it.
DEFERRED_DIRS = (
    "include/application/multiplayer",
    "include/domain/multiplayer",
    "include/infrastructure/multiplayer",
    "src/application/multiplayer",
    "src/infrastructure/multiplayer",
)

# Generated at build time. Editing the output instead of the template is the
# mistake this list exists to prevent; the templates themselves are in scope.
GENERATED_FILES = (
    "include/application/embedded_api.hpp",
    "include/cli/embedded_api_def.hpp",
    "include/shared/version.hpp",
    "src/version.rc",
)

ROOTS = ("include", "src")
SUFFIXES = (".cpp", ".hpp", ".h", ".lua", ".in")

BRIEF_LINES = 3
MAX_LINE = 100  # the whole comment line, prefix included

HANDLE = re.compile(r"^@[A-Za-z0-9](?:[A-Za-z0-9-]*[A-Za-z0-9])?$")


def in_scope(path: pathlib.Path) -> bool:
    posix = path.as_posix()
    if any(posix.startswith(d + "/") for d in VENDORED_DIRS + DEFERRED_DIRS):
        return False
    if posix in GENERATED_FILES:
        return False
    return path.suffix in SUFFIXES


def files_in_scope(repo: pathlib.Path) -> list[pathlib.Path]:
    found: list[pathlib.Path] = []
    for root in ROOTS:
        base = repo / root
        if not base.is_dir():
            continue
        found += [p.relative_to(repo) for p in base.rglob("*") if p.is_file() and in_scope(p.relative_to(repo))]
    return sorted(found, key=lambda p: p.as_posix())


def strip_prefix(line: str) -> str | None:
    """The comment payload of one header line, or None if it is not one."""
    stripped = line.strip()
    for prefix in ("** ", "**", "-- ", "--"):
        if stripped.startswith(prefix):
            return stripped[len(prefix):].strip()
    return None


def read_header(text: str) -> list[str] | None:
    """Payload lines of the leading header block, or None if there is none."""
    lines = text.split("\n")
    if not lines:
        return None

    start = 0
    if lines[0].strip() == "/*":
        start = 1
    elif strip_prefix(lines[0]) is None:
        return None

    payload: list[str] = []
    for line in lines[start:start + 20]:
        if line.strip() in ("*/", ""):
            break
        got = strip_prefix(line)
        if got is None:
            break
        payload.append(got)

    return payload or None


def restates_filename(brief_line: str, stem: str) -> bool:
    """True for the old empty form: the filename, optionally plus a noise word."""
    words = ("implementation", "header", "impl", "definitions", "helpers",
             "declarations", "class", "module", "file", "source")
    flat = re.sub(r"[^a-z0-9]", "", brief_line.lower())
    target = re.sub(r"[^a-z0-9]", "", stem.lower())
    if flat == target:
        return True
    for word in words:
        if flat in (target + word, word + target):
            return True
    return False


def offences(path: pathlib.Path, text: str) -> list[str]:
    payload = read_header(text)
    if payload is None:
        return ["no header"]

    if not payload or payload[0] != "CrabeLoader":
        return ['first header line is not "CrabeLoader"']
    if len(payload) < 2 or payload[1] != "File description:":
        return ['second header line is not "File description:"']

    authors_at = next((i for i, l in enumerate(payload) if l.startswith("Authors:")), None)
    if authors_at is None:
        return ["no Authors: line"]

    brief = [l for l in payload[2:authors_at] if l]
    found: list[str] = []

    if len(brief) != BRIEF_LINES:
        found.append(f"brief is {len(brief)} line(s), must be exactly {BRIEF_LINES}")

    stem = path.name.split(".")[0]
    for line in brief:
        if restates_filename(line, stem):
            found.append(f'brief line restates the filename: "{line}"')

    handles = [h.strip() for h in payload[authors_at][len("Authors:"):].split(",") if h.strip()]
    if not handles:
        found.append("Authors: line is empty")
    for handle in handles:
        if not HANDLE.match(handle):
            found.append(f'Authors: entry is not an @handle: "{handle}"')

    for raw in text.split("\n")[:20]:
        if strip_prefix(raw) is not None and len(raw.rstrip()) > MAX_LINE:
            found.append(f"header line is {len(raw.rstrip())} chars, over the {MAX_LINE} budget")

    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="print the files in scope and exit 0")
    parser.add_argument("--repo", default=".", help="repository root (default: .)")
    args = parser.parse_args()

    repo = pathlib.Path(args.repo).resolve()
    scope = files_in_scope(repo)

    if args.list:
        for path in scope:
            print(path.as_posix())
        print(f"\n{len(scope)} file(s) in scope")
        return 0

    if not scope:
        print(f"check_headers: no files in scope under {repo}", file=sys.stderr)
        return 1

    failures = 0
    for path in scope:
        text = (repo / path).read_text(encoding="utf-8", errors="replace")
        found = offences(path, text)
        if found:
            failures += 1
            print(f"{path.as_posix()}:")
            for offence in found:
                print(f"    {offence}")

    if failures:
        print(f"\ncheck_headers: {failures} of {len(scope)} file(s) have a header offence")
        print("The format is documented in CONTRIBUTING.md.")
        return 1

    print(f"check_headers: {len(scope)} file(s) checked, all headers well-formed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
