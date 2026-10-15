"""Character gameplay capabilities, moveset archetypes, special powers, and custom animations."""
from __future__ import annotations

import json
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Any

from src.core.base_character import BaseCharacter
from src.core.blender_rig import AnimationTrack
from src.core.roster import find_entry


class CombatArchetype(str, Enum):
    INHERIT_BASE = "Inherit Base Moveset"
    MELEE_BRAWLER = "Melee Brawler (Poings / Corps-à-corps)"
    LIGHTSABER_SWORD = "Lightsaber & Swords (Sabre laser & Épées)"
    BLASTER_RANGED = "Blaster & Shooter (Tir laser à distance)"
    ENERGY_REPULSOR = "Flight & Repulsors (Répulseurs & Rayons d'énergie)"
    MAGIC_SPELL = "Magic & Spells (Magie des glaces & Sortilèges)"
    WEB_ACROBAT = "Web-Slinger & Acrobat (Agilité & Toiles d'araignée)"
    HEAVY_SLAM = "Heavy Brute & Slam (Super force & Écrasement au sol)"
    CUSTOM_ROSTER = "Borrow from Roster Character (Choisir un personnage)"


# Archetype default character pairings
ARCHETYPE_ACTOR_MAP: dict[CombatArchetype, str] = {
    CombatArchetype.INHERIT_BASE: "",
    CombatArchetype.MELEE_BRAWLER: "AVG_CaptainAmerica",
    CombatArchetype.LIGHTSABER_SWORD: "EMP_Luke",
    CombatArchetype.BLASTER_RANGED: "EMP_HanSolo",
    CombatArchetype.ENERGY_REPULSOR: "AVG_IronMan",
    CombatArchetype.MAGIC_SPELL: "FRO_Elsa",
    CombatArchetype.WEB_ACROBAT: "SPD_Spiderman",
    CombatArchetype.HEAVY_SLAM: "AVG_Hulk",
    CombatArchetype.CUSTOM_ROSTER: "AVG_IronMan",
}

# Archetype default ability presets
ARCHETYPE_PRESETS: dict[CombatArchetype, dict[str, Any]] = {
    CombatArchetype.INHERIT_BASE: {
        "can_fly": False, "super_jump": False, "wall_crawl": False, "glide": False,
        "damage_multiplier": 1.0, "speed_multiplier": 1.0,
    },
    CombatArchetype.MELEE_BRAWLER: {
        "can_fly": False, "super_jump": False, "wall_crawl": False, "glide": False,
        "damage_multiplier": 1.15, "speed_multiplier": 1.05,
    },
    CombatArchetype.LIGHTSABER_SWORD: {
        "can_fly": False, "super_jump": True, "wall_crawl": False, "glide": False,
        "damage_multiplier": 1.25, "speed_multiplier": 1.1,
    },
    CombatArchetype.BLASTER_RANGED: {
        "can_fly": False, "super_jump": False, "wall_crawl": False, "glide": False,
        "damage_multiplier": 1.15, "speed_multiplier": 1.0,
    },
    CombatArchetype.ENERGY_REPULSOR: {
        "can_fly": True, "super_jump": True, "wall_crawl": False, "glide": True,
        "damage_multiplier": 1.3, "speed_multiplier": 1.2,
    },
    CombatArchetype.MAGIC_SPELL: {
        "can_fly": False, "super_jump": False, "wall_crawl": False, "glide": True,
        "damage_multiplier": 1.2, "speed_multiplier": 1.0,
    },
    CombatArchetype.WEB_ACROBAT: {
        "can_fly": False, "super_jump": True, "wall_crawl": True, "glide": False,
        "damage_multiplier": 1.1, "speed_multiplier": 1.2,
    },
    CombatArchetype.HEAVY_SLAM: {
        "can_fly": False, "super_jump": True, "wall_crawl": False, "glide": False,
        "damage_multiplier": 1.5, "speed_multiplier": 0.9,
    },
    CombatArchetype.CUSTOM_ROSTER: {
        "can_fly": False, "super_jump": False, "wall_crawl": False, "glide": False,
        "damage_multiplier": 1.0, "speed_multiplier": 1.0,
    },
}

# Standard Animation Action Slots
ANIM_ACTION_SLOTS: list[tuple[str, str]] = [
    ("attack_1", "Attack 1 (Light / Combo Start)"),
    ("attack_2", "Attack 2 (Combo Continuation)"),
    ("attack_heavy", "Heavy Attack / Smash Finisher"),
    ("special", "Special Power / Finisher"),
    ("fly", "Flight / Hover State"),
    ("idle", "Idle Pose / Breathing"),
    ("walk", "Walk Locomotion"),
    ("run", "Sprint / Dash"),
    ("jump", "Jump Leap & Fall"),
    ("victory", "Victory / Celebration Emote"),
]


@dataclass
class CustomAnimation:
    slot: str
    name: str
    source_path: str
    duration: float = 1.0
    fps: float = 30.0
    track: AnimationTrack | None = None

    def to_dict(self) -> dict:
        return {
            "slot": self.slot,
            "name": self.name,
            "source_path": self.source_path,
            "duration": self.duration,
            "fps": self.fps,
        }

    @classmethod
    def from_dict(cls, data: dict) -> CustomAnimation:
        return cls(
            slot=data["slot"],
            name=data["name"],
            source_path=data.get("source_path", ""),
            duration=float(data.get("duration", 1.0)),
            fps=float(data.get("fps", 30.0)),
        )


@dataclass
class CharacterAbilities:
    # Combat moveset & style
    combat_archetype: CombatArchetype = CombatArchetype.INHERIT_BASE
    combat_source_actor: str | None = None  # Specific actor to borrow attacks from
    damage_multiplier: float = 1.0
    attack_speed_multiplier: float = 1.0

    # Locomotion & powers
    can_fly: bool = False
    super_jump: bool = False
    double_jump: bool = True
    wall_crawl: bool = False
    glide: bool = False
    speed_multiplier: float = 1.0

    # Custom imported animations mapped by slot
    custom_animations: dict[str, CustomAnimation] = field(default_factory=dict)

    def effective_combat_actor(self, base: BaseCharacter) -> str:
        """Actor name whose attacks/moveset this character executes."""
        if self.combat_source_actor:
            return self.combat_source_actor
        if self.combat_archetype == CombatArchetype.INHERIT_BASE:
            return base.actor
        mapped = ARCHETYPE_ACTOR_MAP.get(self.combat_archetype)
        return mapped if mapped else base.actor

    def get_effective_progression_tree(self, base: BaseCharacter) -> str:
        """Returns the progression/skill tree for the character."""
        eff_actor = self.effective_combat_actor(base)
        if eff_actor == base.actor:
            return base.entry.progression_tree
        try:
            entry = find_entry(eff_actor)
            return entry.progression_tree
        except KeyError:
            return base.entry.progression_tree

    def get_effective_animtree_source(self, base: BaseCharacter) -> BaseCharacter | None:
        """Loads the BaseCharacter to borrow animtree overrides from (if any)."""
        eff_actor = self.effective_combat_actor(base)
        if eff_actor == base.actor:
            return base
        try:
            entry = find_entry(eff_actor)
            return BaseCharacter.from_pack(entry)
        except Exception:
            return base

    def generate_parms_string(self, ident_name: str, base: BaseCharacter, entity_cfg: Any | None = None) -> str:
        """Generates the ActorList Parms string with ability and combat overrides."""
        eff_actor = self.effective_combat_actor(base)
        parms_list = [f"DNAFile=characters/{ident_name}/{ident_name}.dnax"]

        # Health / damage from entity_cfg if provided
        if entity_cfg and not getattr(entity_cfg, "is_player", True):
            if getattr(entity_cfg, "is_enemy", False):
                parms_list.append("Team=Enemy")
                parms_list.append(f"Aggro={getattr(entity_cfg, 'aggro_range', 15.0)}")
                parms_list.append(f"Health={getattr(entity_cfg, 'max_health', 100)}")
                parms_list.append(f"Damage={int(round(getattr(entity_cfg, 'base_damage', 25) * self.damage_multiplier))}")
            else:
                parms_list.append(f"Team={getattr(entity_cfg.faction, 'value', 'Townsperson')}")
                parms_list.append(f"Health={getattr(entity_cfg, 'max_health', 100)}")
        else:
            if abs(self.damage_multiplier - 1.0) > 0.01:
                parms_list.append(f"DamageMultiplier={self.damage_multiplier:.2f}")

        # Locomotion & Special abilities
        if self.can_fly:
            parms_list.append("CanFly=1")
            parms_list.append("Flight=1")
        if self.super_jump:
            parms_list.append("SuperJump=1")
        if self.wall_crawl:
            parms_list.append("WallClimb=1")
            parms_list.append("WallCrawl=1")
        if self.double_jump:
            parms_list.append("DoubleJump=1")
        if self.glide:
            parms_list.append("CanGlide=1")
        if abs(self.speed_multiplier - 1.0) > 0.01:
            parms_list.append(f"SpeedMultiplier={self.speed_multiplier:.2f}")

        if eff_actor != base.actor:
            parms_list.append(f"CombatStyle={eff_actor}")

        return ";".join(parms_list)

    def apply_to_manifest(self, manifest: dict, base: BaseCharacter) -> dict:
        """Augment mod.json manifest with ability tags and archetype data."""
        eff_actor = self.effective_combat_actor(base)
        manifest["abilities"] = {
            "canFly": self.can_fly,
            "superJump": self.super_jump,
            "doubleJump": self.double_jump,
            "wallCrawl": self.wall_crawl,
            "glide": self.glide,
            "speedMultiplier": round(self.speed_multiplier, 2),
            "damageMultiplier": round(self.damage_multiplier, 2),
            "attackSpeedMultiplier": round(self.attack_speed_multiplier, 2),
            "combatArchetype": self.combat_archetype.value,
            "combatSource": eff_actor,
            "customAnimationSlots": list(self.custom_animations.keys()),
        }
        return manifest

    def summary(self, base: BaseCharacter | None = None) -> str:
        """Human-readable overview of active capabilities."""
        parts = []
        eff_actor = self.effective_combat_actor(base) if base else (self.combat_source_actor or "Default")
        if self.combat_archetype != CombatArchetype.INHERIT_BASE:
            parts.append(f"Combat: {self.combat_archetype.value} ({eff_actor})")
        else:
            parts.append(f"Combat: Inherit ({eff_actor})")

        powers = []
        if self.can_fly:
            powers.append("Flight")
        if self.super_jump:
            powers.append("Super Jump")
        if self.double_jump:
            powers.append("Double Jump")
        if self.wall_crawl:
            powers.append("Wall Crawl")
        if self.glide:
            powers.append("Glide")
        if powers:
            parts.append(f"Powers: {', '.join(powers)}")

        if abs(self.speed_multiplier - 1.0) > 0.01:
            parts.append(f"Speed: {self.speed_multiplier:.1f}x")
        if abs(self.damage_multiplier - 1.0) > 0.01:
            parts.append(f"Damage: {self.damage_multiplier:.1f}x")
        if self.custom_animations:
            parts.append(f"Custom Anims: {len(self.custom_animations)} slot(s)")

        return " | ".join(parts)

    def to_dict(self) -> dict:
        return {
            "combat_archetype": self.combat_archetype.value,
            "combat_source_actor": self.combat_source_actor,
            "damage_multiplier": self.damage_multiplier,
            "attack_speed_multiplier": self.attack_speed_multiplier,
            "can_fly": self.can_fly,
            "super_jump": self.super_jump,
            "double_jump": self.double_jump,
            "wall_crawl": self.wall_crawl,
            "glide": self.glide,
            "speed_multiplier": self.speed_multiplier,
            "custom_animations": {k: v.to_dict() for k, v in self.custom_animations.items()},
        }

    @classmethod
    def from_dict(cls, data: dict) -> CharacterAbilities:
        arch_val = data.get("combat_archetype", CombatArchetype.INHERIT_BASE.value)
        try:
            arch = CombatArchetype(arch_val)
        except ValueError:
            arch = CombatArchetype.INHERIT_BASE

        anims = {}
        for k, v in data.get("custom_animations", {}).items():
            try:
                anims[k] = CustomAnimation.from_dict(v)
            except Exception:
                pass

        return cls(
            combat_archetype=arch,
            combat_source_actor=data.get("combat_source_actor"),
            damage_multiplier=float(data.get("damage_multiplier", 1.0)),
            attack_speed_multiplier=float(data.get("attack_speed_multiplier", 1.0)),
            can_fly=bool(data.get("can_fly", False)),
            super_jump=bool(data.get("super_jump", False)),
            double_jump=bool(data.get("double_jump", True)),
            wall_crawl=bool(data.get("wall_crawl", False)),
            glide=bool(data.get("glide", False)),
            speed_multiplier=float(data.get("speed_multiplier", 1.0)),
            custom_animations=anims,
        )
