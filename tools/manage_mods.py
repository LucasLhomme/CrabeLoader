"""Gerer_Mods.bat: list, disable, delete and clean the characters made with Creator (separate from Creator).

Deleting goes to the Windows recycle bin. After every change the shared archives (zz_creator_shared: textures and
icons of all characters) are rebuilt, so a removed character leaves nothing behind in the game.

Usage: python tools/manage_mods.py            (menu)
       python tools/manage_mods.py --list | --rebuild | --clean    (one action, no menu)
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from src.core import shared  # noqa: E402
from src.core.paths import CONFIG_PATH  # noqa: E402
from src.core.mod_writer import BACKUP_DIR  # noqa: E402


def game_dir() -> Path:
    try:
        path = json.loads(CONFIG_PATH.read_text(encoding="utf-8")).get("game_path", "")
    except (OSError, ValueError):
        path = ""
    while not (Path(path) / "assets").is_dir():
        path = input("Dossier du jeu (celui qui contient assets et mods) : ").strip().strip('"')
    return Path(path)


def mb(n: int) -> str:
    return f"{n / 1e6:.1f} Mo"


def show(game: Path) -> list[shared.CreatorMod]:
    mods = shared.creator_mods(game)
    print(f"\nPersonnages Creator dans {game / 'mods'} :")
    if not mods:
        print("  (aucun)")
    for i, m in enumerate(mods, 1):
        state = "actif    " if m.enabled else "DESACTIVE"
        flags = []
        if m.ui:
            flags.append("icone")
        if m.own_archives:
            flags.append(f"ancienne archive {mb(sum(a.stat().st_size for a in m.own_archives))}")
        if m.prefixes - {shared.SHARED_PREFIX}:
            flags.append("textures a migrer")
        print(f"  {i:2d}. {state} {m.name:24.24s} {m.slug:16.16s} {mb(m.size):>9s}  {', '.join(flags)}")
    sh = game / "mods" / shared.SHARED_SLUG
    if sh.is_dir():
        size = sum(f.stat().st_size for f in sh.rglob("*") if f.is_file())
        print(f"  Archives partagees ({shared.SHARED_SLUG}) : {mb(size)}")
    stale = shared.out_of_date(game)
    if stale:
        print(f"  A reconstruire : {', '.join(stale)}")
    else:
        print("  Archives partagees a jour.")
    return mods


def pick(mods, prompt):
    raw = input(prompt).strip()
    out = []
    for part in raw.replace(",", " ").split():
        if part.isdigit() and 1 <= int(part) <= len(mods):
            out.append(mods[int(part) - 1])
    return out


def rebuild(game: Path) -> bool:
    try:
        shared.rebuild(game, lambda s: print("  " + s))
        return True
    except Exception as exc:  # noqa: BLE001
        print(f"  ERREUR : {exc}")
        return False


def clean(game: Path) -> None:
    mods = shared.creator_mods(game)
    moved = sum(shared.migrate_prefix(m) for m in mods if m.enabled)
    if moved:
        print(f"  {moved} texture(s) d'anciens personnages deplacee(s) dans l'archive partagee {shared.SHARED_PREFIX}.zip")
    rebuild(game)
    if BACKUP_DIR.is_dir():
        backups = sorted(p for p in BACKUP_DIR.iterdir() if p.is_dir())
        size = sum(f.stat().st_size for b in backups for f in b.rglob("*") if f.is_file())
        if backups and sys.stdin.isatty() and input(f"  Envoyer les {len(backups)} sauvegardes d'anciennes versions ({mb(size)}) "
                             "a la corbeille ? (o/N) ").strip().lower() == "o":
            for b in backups:
                shared.move_to_recycle_bin(b)
            print("  Sauvegardes envoyees a la corbeille.")


def menu(game: Path) -> None:
    while True:
        mods = show(game)
        print("\n  1. Supprimer (corbeille)   2. Desactiver / reactiver   3. Nettoyer (fichiers residuels, "
              "anciennes archives)\n  4. Reconstruire les archives partagees   0. Quitter")
        choice = input("Choix : ").strip()
        if choice == "0" or choice == "":
            return
        if shared.game_running() and choice in "1234":
            print("  Ferme le jeu d'abord (il garde ses archives ouvertes).")
            continue
        if choice == "1":
            targets = pick(mods, "Numero(s) a supprimer : ")
            if targets and input(f"  Supprimer {', '.join(m.name for m in targets)} ? (o/N) ").strip().lower() == "o":
                for m in targets:
                    shared.move_to_recycle_bin(m.path)
                    print(f"  {m.name} envoye a la corbeille.")
                rebuild(game)
        elif choice == "2":
            for m in pick(mods, "Numero(s) a desactiver / reactiver : "):
                target = m.path.with_name(m.slug if not m.enabled else "_" + m.slug)
                if target.exists():
                    print(f"  {target.name} existe deja : ignore.")
                    continue
                m.path.rename(target)
                print(f"  {m.name} {'reactive' if not m.enabled else 'desactive'}.")
            rebuild(game)
        elif choice == "3":
            clean(game)
        elif choice == "4":
            shared.rebuild(game, lambda s: print("  " + s), force=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--rebuild", action="store_true")
    ap.add_argument("--clean", action="store_true")
    ap.add_argument("--game", type=Path)
    args = ap.parse_args()
    game = args.game or game_dir()
    if args.list:
        show(game)
    elif args.rebuild:
        sys.exit(0 if rebuild(game) else 1)
    elif args.clean:
        clean(game)
    else:
        menu(game)


if __name__ == "__main__":
    main()
