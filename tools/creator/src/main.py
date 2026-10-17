import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from src.core import errorlog


def main():
    errorlog.install()
    try:
        from src.ui.main_window import MainWindow
        app = MainWindow()
        # Errors inside Tk callbacks (button clicks, timers) are caught by Tk, not by sys.excepthook.
        app.report_callback_exception = lambda kind, value, tb: errorlog.report(kind, value, tb)
        app.mainloop()
    except Exception:
        errorlog.report(*sys.exc_info())
        sys.exit(1)


if __name__ == "__main__":
    main()
