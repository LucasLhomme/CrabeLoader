import json
import tempfile
import unittest
from pathlib import Path

from src.core.mod_writer import Identity, write_text_files
from src.core.npc_config import EntityConfig, EntityType, Faction, generate_lua_spawner_code


class DummyEntry:
    def __init__(self):
        self.progression_tree = "tree_luke"
        self.dnax_stem = "emp_luke"


class DummyBase:
    def __init__(self):
        self.actor = "EMP_Luke"
        self.actor_lower = "emp_luke"
        self.model = "emp_luke"
        self.model_dir = "emp_luke"
        self.entry = DummyEntry()


class TestNPCConfig(unittest.TestCase):
    def test_entity_config_defaults(self):
        cfg = EntityConfig()
        self.assertTrue(cfg.is_player)
        self.assertFalse(cfg.is_enemy)
        self.assertFalse(cfg.is_npc_or_ally)
        self.assertEqual(cfg.faction, Faction.HEROES)
        self.assertEqual(cfg.max_health, 100)
        self.assertIn("idle", cfg.animations)
        self.assertIn("walk", cfg.animations)
        self.assertIn("attack", cfg.animations)

    def test_enemy_configuration(self):
        cfg = EntityConfig(
            entity_type=EntityType.ENEMY,
            faction=Faction.ENEMIES,
            max_health=250,
            base_damage=45,
            behavior="Aggressive",
            animations={"idle": "CombatIdle", "walk": "Run", "attack": "HeavySlash"},
        )
        self.assertFalse(cfg.is_player)
        self.assertTrue(cfg.is_enemy)
        self.assertIn("Enemy", cfg.summary())
        self.assertEqual(cfg.max_health, 250)
        self.assertEqual(cfg.animations["attack"], "HeavySlash")

    def test_export_enemy_files(self):
        cfg = EntityConfig(
            entity_type=EntityType.ENEMY,
            faction=Faction.ENEMIES,
            max_health=300,
            base_damage=50,
        )
        ident = Identity(slug="stormtrooper_elite", display_name="Stormtrooper Elite", actor="EMP_Strm")
        base = DummyBase()

        with tempfile.TemporaryDirectory() as tmp_dir:
            mod_dir = Path(tmp_dir)
            (mod_dir / "characters").mkdir()
            write_text_files(mod_dir, ident, base, cfg)

            # Check mod.json
            mod_json = json.loads((mod_dir / "mod.json").read_text(encoding="utf-8"))
            self.assertEqual(mod_json["entityType"], "Enemy")
            self.assertEqual(mod_json["faction"], "Enemies")

            # Check Lua entry
            lua_code = (mod_dir / f"{ident.slug}.lua").read_text(encoding="utf-8")
            self.assertIn("creator.stormtrooper_elite", lua_code)
            self.assertIn('health = 300', lua_code)
            self.assertIn('damage = 50', lua_code)
            self.assertIn('Game.SpawnActor', lua_code)

            # Check ActorList.patch
            patch_code = (mod_dir / "skilltrees" / "ActorList.patch").read_text(encoding="utf-8")
            self.assertIn('Type = "Enemy"', patch_code)
            self.assertIn('Team=Enemy', patch_code)
            self.assertIn('Health=300', patch_code)

    def test_export_ally_npc_files(self):
        cfg = EntityConfig(
            entity_type=EntityType.ALLY,
            faction=Faction.TOWNSPERSON,
            max_health=120,
        )
        ident = Identity(slug="town_guard", display_name="Town Guard", actor="TWN_Grd")
        base = DummyBase()

        with tempfile.TemporaryDirectory() as tmp_dir:
            mod_dir = Path(tmp_dir)
            (mod_dir / "characters").mkdir()
            write_text_files(mod_dir, ident, base, cfg)

            mod_json = json.loads((mod_dir / "mod.json").read_text(encoding="utf-8"))
            self.assertEqual(mod_json["entityType"], "Ally")

            lua_code = (mod_dir / f"{ident.slug}.lua").read_text(encoding="utf-8")
            self.assertIn("creator.town_guard", lua_code)

            patch_code = (mod_dir / "skilltrees" / "ActorList.patch").read_text(encoding="utf-8")
            self.assertIn('Type = "NPC"', patch_code)
            self.assertIn('Team=Townsperson', patch_code)


if __name__ == "__main__":
    unittest.main()

