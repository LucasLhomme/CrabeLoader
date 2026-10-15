import json
import tempfile
import unittest
from pathlib import Path

from src.core.abilities import (
    ANIM_ACTION_SLOTS,
    ARCHETYPE_ACTOR_MAP,
    ARCHETYPE_PRESETS,
    CharacterAbilities,
    CombatArchetype,
    CustomAnimation,
)
from src.core.mod_writer import Identity, write_text_files
from src.core.npc_config import EntityConfig, EntityType
from src.core.roster import RosterEntry


class DummyBase:
    def __init__(self, actor="EMP_Luke", display_name="Luke Skywalker", progression_tree="IN3_EMP_Luke"):
        self.actor = actor
        self.actor_lower = actor.lower()
        self.display_name = display_name
        self.model = actor.lower()
        self.model_dir = actor.lower()
        self.entry = RosterEntry(
            actor=actor,
            display_name=display_name,
            sku="1000206",
            icon="HUD_PlayerIcons_Luke",
            progression_tree=progression_tree,
            franchise="Star Wars",
            dnax=f"characters/{actor}.dnax",
        )
        self.animtree_file = None


class TestAbilities(unittest.TestCase):
    def test_default_abilities(self):
        abils = CharacterAbilities()
        self.assertEqual(abils.combat_archetype, CombatArchetype.INHERIT_BASE)
        self.assertFalse(abils.can_fly)
        self.assertFalse(abils.super_jump)
        self.assertTrue(abils.double_jump)
        self.assertFalse(abils.wall_crawl)
        self.assertFalse(abils.glide)
        self.assertEqual(abils.speed_multiplier, 1.0)
        self.assertEqual(abils.damage_multiplier, 1.0)

    def test_archetype_presets(self):
        # Repulsor flight preset
        iron_man_preset = ARCHETYPE_PRESETS[CombatArchetype.ENERGY_REPULSOR]
        self.assertTrue(iron_man_preset["can_fly"])
        self.assertTrue(iron_man_preset["super_jump"])
        self.assertTrue(iron_man_preset["glide"])
        self.assertGreater(iron_man_preset["damage_multiplier"], 1.0)

        # Web acrobat preset
        spidey_preset = ARCHETYPE_PRESETS[CombatArchetype.WEB_ACROBAT]
        self.assertTrue(spidey_preset["wall_crawl"])
        self.assertTrue(spidey_preset["super_jump"])

    def test_effective_combat_actor_and_progression_tree(self):
        base = DummyBase()
        abils = CharacterAbilities(combat_archetype=CombatArchetype.ENERGY_REPULSOR)
        eff_actor = abils.effective_combat_actor(base)
        self.assertEqual(eff_actor, "AVG_IronMan")

        tree = abils.get_effective_progression_tree(base)
        self.assertTrue(tree.startswith("IN") and "IronMan" in tree)

        # Explicit borrow from custom roster
        abils_custom = CharacterAbilities(
            combat_archetype=CombatArchetype.CUSTOM_ROSTER,
            combat_source_actor="SPD_Spiderman"
        )
        self.assertEqual(abils_custom.effective_combat_actor(base), "SPD_Spiderman")
        spidey_tree = abils_custom.get_effective_progression_tree(base)
        self.assertTrue("Spiderman" in spidey_tree)

    def test_generate_parms_string(self):
        base = DummyBase()
        abils = CharacterAbilities(
            can_fly=True,
            super_jump=True,
            wall_crawl=True,
            glide=True,
            speed_multiplier=1.4,
            damage_multiplier=1.5,
            combat_source_actor="AVG_IronMan"
        )
        parms = abils.generate_parms_string("MAR_Mari", base)
        self.assertIn("CanFly=1", parms)
        self.assertIn("Flight=1", parms)
        self.assertIn("SuperJump=1", parms)
        self.assertIn("WallClimb=1", parms)
        self.assertIn("CanGlide=1", parms)
        self.assertIn("SpeedMultiplier=1.40", parms)
        self.assertIn("DamageMultiplier=1.50", parms)
        self.assertIn("CombatStyle=AVG_IronMan", parms)

    def test_custom_animation_slot_mapping(self):
        abils = CharacterAbilities()
        anim = CustomAnimation(
            slot="attack_1",
            name="SuperSlash",
            source_path="c:/test/slash.glb",
            duration=1.25,
            fps=30.0,
        )
        abils.custom_animations["attack_1"] = anim

        d = abils.to_dict()
        restored = CharacterAbilities.from_dict(d)
        self.assertIn("attack_1", restored.custom_animations)
        self.assertEqual(restored.custom_animations["attack_1"].name, "SuperSlash")
        self.assertEqual(restored.custom_animations["attack_1"].duration, 1.25)

    def test_write_text_files_with_abilities(self):
        base = DummyBase()
        ident = Identity(slug="super_mario", display_name="Super Mario", actor="MAR_Mari")
        abils = CharacterAbilities(
            combat_archetype=CombatArchetype.ENERGY_REPULSOR,
            can_fly=True,
            super_jump=True,
            speed_multiplier=1.3,
            damage_multiplier=1.2,
        )
        anim = CustomAnimation(
            slot="special",
            name="FireballBlast",
            source_path="c:/test/fireball.glb",
            duration=2.0,
            fps=30.0,
        )
        abils.custom_animations["special"] = anim

        with tempfile.TemporaryDirectory() as tmp_dir:
            mod_dir = Path(tmp_dir)
            write_text_files(mod_dir, ident, base, entity_config=EntityConfig(), abilities=abils)

            # Check mod.json
            mod_json = json.loads((mod_dir / "mod.json").read_text(encoding="utf-8"))
            self.assertIn("abilities", mod_json)
            self.assertTrue(mod_json["abilities"]["canFly"])
            self.assertTrue(mod_json["abilities"]["superJump"])
            self.assertEqual(mod_json["abilities"]["combatSource"], "AVG_IronMan")
            self.assertIn("special", mod_json["abilities"]["customAnimationSlots"])

            # Check Lua registration
            lua_code = (mod_dir / f"{ident.slug}.lua").read_text(encoding="utf-8")
            self.assertIn('combatSource = "AVG_IronMan"', lua_code)
            self.assertIn('canFly = true', lua_code)
            self.assertIn('superJump = true', lua_code)
            self.assertIn('speedMultiplier = 1.30', lua_code)

            # Check ActorList.patch
            patch_code = (mod_dir / "skilltrees" / "ActorList.patch").read_text(encoding="utf-8")
            self.assertIn("CanFly=1", patch_code)
            self.assertIn("SuperJump=1", patch_code)
            self.assertIn("CombatStyle=AVG_IronMan", patch_code)

            # Check Virtual Reader character registration
            char_lua = (mod_dir / "characters" / f"{ident.actor}.lua").read_text(encoding="utf-8")
            self.assertIn("ProgressionTree", char_lua)
            self.assertIn("IronMan", char_lua)


if __name__ == "__main__":
    unittest.main()

