from pathlib import Path

APP_DIR = Path(__file__).resolve().parents[2]
SRC_DIR = APP_DIR / "src"
ASSETS_DIR = SRC_DIR / "assets"
TOOLS_DIR = SRC_DIR / "tools"
SCRIPTS_DIR = SRC_DIR / "scripts"
CONFIG_PATH = SRC_DIR / "config" / "settings.json"
BACKUP_DIR = APP_DIR / "backups"
WORK_DIR = APP_DIR / "build"

QUICKBMS = TOOLS_DIR / "quickbms.exe"
BMS_SCRIPT = TOOLS_DIR / "disney_infinity_new.bms"
REENCRYPTER = TOOLS_DIR / "offsetting.exe"
HEAT_SCRIPT = SCRIPTS_DIR / "blender_heat_weights.py"
