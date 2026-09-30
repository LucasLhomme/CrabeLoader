#!/usr/bin/env python3
"""
Standalone runner for the CrabeLoader Lua API test suite.

Can be run directly via:
    python tests/run_lua_tests.py
    python tests/lua/run_tests.py
Or invoked as part of a unittest suite:
    python -m unittest tests/run_lua_tests.py

It locates a suitable Lua interpreter (crabe_lua, lua5.1, luajit, or lua)
and executes tests/lua/run_tests.lua with repo root as the working context.
"""

import os
import sys
import subprocess
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
TESTS_LUA_RUNNER = REPO_ROOT / "tests" / "lua" / "run_tests.lua"


def find_lua_interpreter(explicit_path=None):
    """Finds a compatible Lua interpreter."""
    if explicit_path and Path(explicit_path).is_file():
        return str(Path(explicit_path).resolve())

    env_lua = os.environ.get("CRABE_LUA") or os.environ.get("LUA_EXE")
    if env_lua and Path(env_lua).is_file():
        return str(Path(env_lua).resolve())

    # Check build output directories
    candidate_rel_paths = [
        "build/Release/crabe_lua.exe",
        "build/Release/crabe_lua",
        "build/Debug/crabe_lua.exe",
        "build/Debug/crabe_lua",
        "build/tests/Release/crabe_lua.exe",
        "build/tests/Release/crabe_lua",
        "build/tests/Debug/crabe_lua.exe",
        "build/tests/Debug/crabe_lua",
        "build/tests/RelWithDebInfo/crabe_lua.exe",
        "build/tests/RelWithDebInfo/crabe_lua",
        "build/bin/crabe_lua.exe",
        "build/bin/crabe_lua",
        "build/crabe_lua.exe",
        "build/crabe_lua",
    ]
    for rel in candidate_rel_paths:
        candidate = REPO_ROOT / rel
        if candidate.is_file():
            return str(candidate.resolve())

    # Check system PATH
    for name in ["crabe_lua", "lua5.1", "lua51", "luajit", "lua"]:
        found = shutil_which(name)
        if found:
            return found

    return None


def shutil_which(cmd):
    import shutil
    return shutil.which(cmd)


def run_lua_tests(lua_bin=None, extra_args=None):
    """
    Executes tests/lua/run_tests.lua with the discovered or specified Lua binary.
    Returns the process exit code.
    """
    lua_exe = find_lua_interpreter(lua_bin)
    if not lua_exe:
        sys.stderr.write(
            "ERROR: No compatible Lua interpreter found.\n"
            "Checked build directories (e.g. build/tests/Release/crabe_lua.exe) and system PATH.\n"
            "Please provide a path using --lua <path> or set CRABE_LUA / LUA_EXE.\n"
        )
        return 127

    cmd = [lua_exe, str(TESTS_LUA_RUNNER), str(REPO_ROOT)]
    if extra_args:
        cmd.extend(extra_args)

    print(f"[runner] Running: {' '.join(cmd)}")
    proc = subprocess.run(cmd, cwd=str(REPO_ROOT))
    return proc.returncode


class TestCrabeLuaApi(unittest.TestCase):
    """unittest integration so standard python test discovery finds the suite."""

    def test_all_lua_api_suites(self):
        ret = run_lua_tests()
        self.assertEqual(ret, 0, f"Lua API test suite exited with error code {ret}")


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Run CrabeLoader Lua API tests")
    parser.add_argument("--lua", help="Path to Lua interpreter binary")
    args, unknown = parser.parse_known_args()

    code = run_lua_tests(lua_bin=args.lua, extra_args=unknown)
    sys.exit(code)


if __name__ == "__main__":
    main()
