import json

from src.core import shared
from src.core.material_bundle import MaterialBundle, character_bundle


def make_game(tmp_path):
    game = tmp_path / "game"
    for rel in ("assets/textures/17.zip", "assets/textures/a7.zip", "assets/ui/icons/icons.zip"):
        (game / rel).parent.mkdir(parents=True, exist_ok=True)
        (game / rel).write_bytes(b"vanilla")
    return game


def make_mod(game, slug, ids, icon=False, old_zip=None):
    mod = game / "mods" / slug
    (mod / "characters" / slug).mkdir(parents=True)
    (mod / "mod.json").write_text(json.dumps({"id": f"creator.{slug}", "character": {"name": slug.upper(),
                                                                                       "displayName": slug.title()}}))
    (mod / "characters" / slug / f"{slug}.mtb").write_bytes(
        character_bundle(1, ids[0], ids[1], b"\1\2\3\4", ["00000000-0000-0000-0000-000000000000"]))
    (mod / "textures").mkdir()
    for i in ids:
        (mod / "textures" / f"{i}.tbody").write_bytes(b"DDS " + i.encode())
    if icon:
        (mod / "ui" / "icons").mkdir(parents=True)
        (mod / "ui" / "icons" / f"hud_playericons_{slug}.mtb").write_bytes(b"BNDL")
    if old_zip:
        (mod / "textures" / f"{old_zip}.zip").write_bytes(b"old")
    return mod


def test_plan_groups_sources_by_archive(tmp_path):
    game = make_game(tmp_path)
    make_mod(game, "new", ["17aaaaaaaaaaaaaa", "17bbbbbbbbbbbbbb"], icon=True)
    make_mod(game, "old", ["a7cccccccccccccc", "a7dddddddddddddd"], old_zip="a7")
    (game / "mods" / "_off").mkdir()
    make_mod(game, "gone", ["17eeeeeeeeeeeeee", "17ffffffffffffff"]).rename(game / "mods" / "_gone")
    (game / "mods" / "crabemenu").mkdir()
    mods = shared.creator_mods(game)
    assert [(m.slug, m.enabled) for m in mods] == [("gone", False), ("new", True), ("old", True)]
    plan = shared.plan(game, mods)
    assert set(plan) == {"textures/17", "textures/a7", "ui/icons"}
    assert set(plan["textures/17"][2]) == {"17/textures/17aaaaaaaaaaaaaa.tbody", "17/textures/17bbbbbbbbbbbbbb.tbody"}
    assert set(plan["ui/icons"][2]) == {"icons/hud_playericons_new.mtb"}
    stale = shared.out_of_date(game)
    assert "textures/17" in stale and "old: own archive" in stale


def test_migrate_prefix_rewrites_bundle_and_files(tmp_path):
    game = make_game(tmp_path)
    mod_dir = make_mod(game, "old", ["a7cccccccccccccc", "a7dddddddddddddd"])
    mod = shared.creator_mods(game)[0]
    assert shared.migrate_prefix(mod) == 2
    bundle = MaterialBundle.from_bytes((mod_dir / "characters" / "old" / "old.mtb").read_bytes())
    assert [t.hex() for t in bundle.textures] == ["17cccccccccccccc", "17dddddddddddddd"]
    assert sorted(p.name for p in (mod_dir / "textures").glob("*.tbody")) == ["17cccccccccccccc.tbody",
                                                                              "17dddddddddddddd.tbody"]
    assert shared.migrate_prefix(shared.creator_mods(game)[0]) == 0


def test_rebuild_removes_archives_left_by_migration(tmp_path, monkeypatch):
    game = make_game(tmp_path)
    mod_dir = make_mod(game, "old", ["a7cccccccccccccc", "a7dddddddddddddd"], old_zip="a7")
    shared.migrate_prefix(shared.creator_mods(game)[0])
    built = []
    monkeypatch.setattr(shared, "game_running", lambda: False)
    monkeypatch.setattr(shared, "_build", lambda v, stem, files, out, work, log: (built.append(stem), out.parent.mkdir(
        parents=True, exist_ok=True), out.write_bytes(b"zip"), len(files))[-1])
    report = shared.rebuild(game, lambda s: None)
    assert built == ["17"] and not (mod_dir / "textures" / "a7.zip").exists()
    assert "old/textures/a7.zip" in report.removed
    assert shared.out_of_date(game) == []
    assert shared.rebuild(game, lambda s: None).kept == ["textures/17"]
