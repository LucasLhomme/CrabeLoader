"""Crash and error log: nothing fails silently any more.

Writes build/creator.log (kept small), and shows a message box on a crash so the window never just vanishes."""
import logging
import sys
import traceback
from logging.handlers import RotatingFileHandler

from src.core.paths import WORK_DIR

LOG_PATH = WORK_DIR / "creator.log"
_logger = logging.getLogger("creator")


def install() -> None:
    """Start logging to build/creator.log and route uncaught exceptions (also from threads) into it."""
    try:
        WORK_DIR.mkdir(parents=True, exist_ok=True)
        handler = RotatingFileHandler(LOG_PATH, maxBytes=500_000, backupCount=2, encoding="utf-8")
        handler.setFormatter(logging.Formatter("%(asctime)s %(levelname)s %(message)s"))
        _logger.addHandler(handler)
        _logger.setLevel(logging.INFO)
    except OSError:
        pass  # a read-only folder must not stop the app
    sys.excepthook = lambda kind, value, tb: report(kind, value, tb)
    try:
        import threading
        threading.excepthook = lambda args: report(args.exc_type, args.exc_value, args.exc_traceback)
    except Exception:
        pass


def info(text: str) -> None:
    _logger.info(text)


def report(kind, value, tb, show: bool = True) -> str:
    """Log an exception and (by default) tell the user where to find it. Returns the formatted traceback."""
    text = "".join(traceback.format_exception(kind, value, tb))
    _logger.error(text)
    print(text, file=sys.stderr)
    if show:
        _show(f"{kind.__name__}: {value}\n\nFull details: {LOG_PATH}")
    return text


def _show(message: str) -> None:
    try:
        from tkinter import messagebox
        messagebox.showerror("Disney Infinity Creator", message)
    except Exception:
        pass  # no display available: the log file is enough
