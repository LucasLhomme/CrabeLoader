#!/usr/bin/env python3
"""Convenience trampoline to tests/run_lua_tests.py"""

import sys
from pathlib import Path

# Add tests/ directory to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from run_lua_tests import main

if __name__ == "__main__":
    main()
