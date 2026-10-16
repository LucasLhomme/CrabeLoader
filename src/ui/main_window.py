import queue
import threading
import traceback
from pathlib import Path
import tkinter as tk
from tkinter import filedialog

import customtkinter as ctk
import numpy as np

from src.core.base_character import BaseCharacter
from src.core.base_pack import has_pack, preview_texture, status
from src.core import errorlog
from src.core.config import normalize_blender_path
from src.core.accessories import ZONE_LIMIT, detect
from src.core.cleaner import NEEDS_BLENDER, clean_model
from src.core.config import load_settings, save_settings
from src.core.exporter import install
from src.core.mod_writer import default_actor, make_identity
from src.core.model import CharacterModel, auto_placement, load_source
from src.core.paths import WORK_DIR
from src.core.quality import deformation_report, stretched_segments
from src.core.ranking import rank_bases
from src.core.rig import base_tpose, build_rig
from src.core.roster import find_entry, load_roster
from src.core.skeleton import (
    fit_advice,
    fit_issues,
    apply_apose_to_proportions,
    apply_tpose_to_proportions,
    base_joint_points,
    base_skeleton_segments,
    build_anchors,
    joint_points,
    measure,
    skeleton_segments,
)
from src.core.blender_rig import load_rig_file, RigParsingError, UnsupportedRigError, ParsedRig
from src.core.animation_player import AnimationPlayer, AnimationClip, get_character_animation_clips

from src.core.npc_config import EntityConfig, EntityType, Faction
from src.core.abilities import (
    CharacterAbilities,
    CombatArchetype,
    ARCHETYPE_ACTOR_MAP,
    ARCHETYPE_PRESETS,
    ANIM_ACTION_SLOTS,
    CustomAnimation,
)
from src.render.gl_widget import GLRenderer
from src.ui.timeline import TimelineWidget

GREEN, RED, YELLOW, GREY = "#4ade80", "#f87171", "#fbbf24", "#a1a1aa"
POSES = {"T-pose": "tpose", "In-game pose": "bind", "Arms up": "arms", "Walk": "walk", "Overhead": "overhead",
         "Forward": "reach", "Elbows": "elbow", "Twist": "twist", "Fist": "fist"}
DEFAULT_BASE = "EMP_Luke"
GROW_MAX_PIECE = 3000
""""Grow" only absorbs loose pieces this small (in vertices): never the body or the head."""

# Bones a user can lock a loose piece to (names of the canonical skeleton) and how they read in the menu.
PIECE_BONES = {
    "Head": "Head", "Neck": "Neck", "Chest": "Spine3", "Belly": "Spine2", "Hips / pelvis": "Hips",
    "Left shoulder": "Shoulder_Left", "Right shoulder": "Shoulder_Right",
    "Left hand": "Hand_Left", "Right hand": "Hand_Right",
    "Left thigh": "Thigh_Left", "Right thigh": "Thigh_Right",
    "Left ankle": "Ankle_Left", "Right ankle": "Ankle_Right",
}

# (Placement field, label, +/- range): metres for offsets, degrees for the fine yaw.
PLACEMENT_SLIDERS = (
    ("offset_x", "Left / right (m)", 0.4),
    ("offset_z", "Back / forward (m)", 0.5),
    ("offset_y", "Down / up (m)", 0.4),
    ("yaw_deg", "Fine rotation (°)", 45.0),
)

# (field, label, low, high, relative) — relative bounds are multiples of the model height, others are 0..1 fractions.
SKELETON_SLIDERS = (
    ("arm_y", "Arm height", 0.3, 0.95, True),
    ("shoulder_x", "Shoulder width", 0.02, 0.45, True),
    ("wrist", "Wrist position", 0.3, 0.9, False),
    ("neck_y", "Neck", 0.35, 0.98, True),
    ("head_y", "Head pivot", 0.4, 0.99, True),
    ("crotch_y", "Hips (crotch)", 0.12, 0.8, True),
    ("leg_x", "Leg spacing", 0.0, 0.3, True),
    ("knee", "Knees", 0.2, 0.8, False),
    ("ankle_y", "Ankles", 0.01, 0.25, True),
)
SKELETON_SLIDERS = SKELETON_SLIDERS[:3] + (("elbow", "Elbow position", 0.3, 0.7, False),) + SKELETON_SLIDERS[3:]

# What each slider should line up with (the green dots in the viewport), and which auto-fit estimate it belongs to.
SLIDER_HINTS = {
    "arm_y": "Height of the arm line: the shoulder dots go on the middle of the arms in the T-pose.",
    "shoulder_x": "Distance from the body centre to the shoulder joint: the dot goes where the arm leaves the torso.",
    "wrist": "Along the arm: the hand dot goes on the wrist, where the hand starts (not the fingertips).",
    "elbow": "Along the arm: the elbow dot goes on the elbow crease. 0.5 = halfway between shoulder and wrist.",
    "neck_y": "Height of the neck base. Too low and the chin stretches, too high and the shoulders pull the neck.",
    "head_y": "Pivot of the head: the middle of the skull base, roughly the jaw line.",
    "crotch_y": "Where the legs start. For a long coat or skirt, set it to the real crotch height.",
    "leg_x": "Distance between the hip joints and the body centre.",
    "knee": "Knee position between ankle and hip (0.5 = halfway).",
    "ankle_y": "Height of the ankle joint, just above the foot.",
}
ESTIMATE_SLIDERS = {"shoulders": ("arm_y", "shoulder_x"), "wrists": ("wrist", "elbow"), "neck": ("neck_y", "head_y"),
                    "hips": ("crotch_y",), "legs": ("leg_x", "knee"), "ankles": ("ankle_y",)}


class Tooltip:
    """A small text box that follows the mouse over a widget (shown after the pointer rests on it)."""

    def __init__(self, widget, text=lambda: ""):
        self.widget, self.text, self.window = widget, text, None
        widget.bind("<Enter>", self._show, add="+")
        widget.bind("<Leave>", self._hide, add="+")

    def _show(self, event):
        text = self.text()
        if not text:
            return
        self._hide()
        self.window = tk.Toplevel(self.widget)
        self.window.wm_overrideredirect(True)
        self.window.wm_geometry(f"+{event.x_root + 12}+{event.y_root + 14}")
        tk.Label(self.window, text=text, justify="left", wraplength=300, background="#27272a", foreground="#e4e4e7",
                 relief="solid", borderwidth=1, font=("Segoe UI", 9), padx=6, pady=4).pack()

    def _hide(self, _event=None):
        if self.window is not None:
            self.window.destroy()
            self.window = None


class MainWindow(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.settings = load_settings()
        self.base: BaseCharacter | None = None
        self.model: CharacterModel | None = None
        self.props = None
        self.auto_props = None
        self.fit_report = None
        self.anchors = None
        self.rig = None
        self.pose = "tpose"
        self.busy = False
        self._actor_edited = False
        self._events: queue.Queue = queue.Queue()
        self._generation = 0
        self.base_buttons = {}
        self.animation_player = AnimationPlayer()
        self.custom_rig: ParsedRig | None = None
        self.entity_config = EntityConfig()
        self.abilities = CharacterAbilities()
        self.piece_overrides: list[tuple[tuple[int, ...], str]] = []
        self._selected_pieces: set[int] = set()
        self._piece_labels = None

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("dark-blue")
        self.title("Disney Infinity Creator")
        self.geometry("1440x900")
        self.configure(fg_color="#09090b")
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(1, weight=1)

        side = ctk.CTkFrame(self, width=380, corner_radius=0, fg_color="#121316")
        side.grid(row=0, column=0, sticky="nsew")
        side.grid_propagate(False)
        side.grid_rowconfigure(1, weight=1)
        side.grid_columnconfigure(0, weight=1)
        ctk.CTkLabel(side, text="Disney Infinity Creator", font=ctk.CTkFont(size=20, weight="bold")).grid(
            row=0, column=0, padx=16, pady=(16, 4), sticky="w")
        self.tabs = ctk.CTkTabview(side, fg_color="#121316")
        self.tabs.grid(row=1, column=0, padx=8, pady=0, sticky="nsew")
        self._build_base_tab(self.tabs.add("0. Base"))
        model_tab = self.tabs.add("1. Model")
        model_tab.grid_rowconfigure(0, weight=1)
        model_tab.grid_columnconfigure(0, weight=1)
        model_scroll = ctk.CTkScrollableFrame(model_tab, fg_color="transparent")
        model_scroll.grid(row=0, column=0, sticky="nsew")
        self._build_model_tab(model_scroll)
        self._build_skeleton_tab(self.tabs.add("2. Skeleton"))
        self._build_abilities_tab(self.tabs.add("3. Abilities & Moveset"))
        self._build_export_tab(self.tabs.add("4. Rig & Export"))

        view = ctk.CTkFrame(self, fg_color="#09090b", corner_radius=0)
        view.grid(row=0, column=1, sticky="nsew")
        view.grid_rowconfigure(0, weight=1)
        view.grid_columnconfigure(0, weight=1)
        self.view_label = ctk.CTkLabel(view, text="")
        self.view_label.grid(row=0, column=0, sticky="nsew")

        self.timeline = TimelineWidget(view, self.animation_player, self._on_anim_frame_changed)
        self.timeline.grid(row=1, column=0, sticky="ew", padx=12, pady=(0, 2))

        bar = ctk.CTkFrame(view, fg_color="transparent")
        bar.grid(row=2, column=0, pady=(2, 4))
        hint = "Left click: orbit  •  right click: pan  •  wheel: zoom  •  double click: frame"
        ctk.CTkLabel(bar, text=hint, text_color=GREY, font=ctk.CTkFont(size=11)).grid(row=0, column=0, padx=(0, 16))
        self.views_var = ctk.BooleanVar(value=False)
        ctk.CTkCheckBox(bar, text="3 views (front / side / top)", variable=self.views_var, command=self.on_views_toggle,
                        font=ctk.CTkFont(size=11), checkbox_width=18, checkbox_height=18).grid(row=0, column=1)
        view.grid_propagate(False)
        self.renderer = GLRenderer(self.view_label)
        self.view_label.bind("<Control-Button-1>", self.on_pick_piece)
        view.bind("<Configure>", lambda e: self.renderer.resize(e.width, max(50, e.height - 75)))
        self.after(50, self._drain_events)
        self.after(100, self._start)
        self.after(200, self._anim_tick)

    def _update_anim_render(self):
        if self.rig:
            posed_verts, posed_world = self.animation_player.evaluate_pose_and_world(self.rig)
            self.renderer.update_model_vertices(posed_verts)
            self.renderer.set_heat_lines(None)  # the red overlay only describes the static test poses
            if getattr(self.timeline, "sync_compare", True) and self.base and self.renderer.show_ghost:
                ghost_verts = self.base.deform_preview_parts(posed_world)
                self.renderer.update_ghost_vertices(ghost_verts)
            self.renderer.set_skeleton(None, None)
            self.renderer.set_base_skeleton(None, None)
            self.renderer.request_render()

    def _on_anim_frame_changed(self):
        self._update_anim_render()

    def _anim_tick(self):
        if self.animation_player.tick():
            self.timeline.update_playback_state()
            self._update_anim_render()
        self.after(33, self._anim_tick)


    def _start(self):
        if not has_pack():
            self.base_info.configure(text="Creator's base pack is missing (src/assets/bases): "
                                          "run tools/build_base_pack.py.", text_color=RED)
            return
        self.on_pick_base(self.settings.get("base_actor", DEFAULT_BASE))

    # ------------------------------------------------------------------ tabs
    def _build_base_tab(self, tab):
        tab.grid_columnconfigure(0, weight=1)
        tab.grid_rowconfigure(2, weight=1)
        ctk.CTkLabel(tab, text="Pick the figure your character is built on: it keeps its skeleton, animations "
                               "and moveset, only the mesh changes.",
                     text_color=GREY, wraplength=330, justify="left", font=ctk.CTkFont(size=11)).grid(
            row=0, column=0, sticky="w", pady=(0, 6))
        # No textvariable: CTkEntry hides its placeholder when one is bound.
        self.search_entry = ctk.CTkEntry(tab, placeholder_text="Search a figure (name, franchise)…",
                                         fg_color="#27272a", border_width=0)
        self.search_entry.grid(row=1, column=0, sticky="new", pady=(0, 4))
        self.search_entry.bind("<KeyRelease>", lambda e: self._filter_bases())
        self.base_list = ctk.CTkScrollableFrame(tab, fg_color="#0c0c0e", height=400)
        self.base_list.grid(row=2, column=0, sticky="nsew")
        self.base_list.grid_columnconfigure(0, weight=1)
        for entry in sorted(load_roster(), key=lambda e: e.display_name.lower()):
            button = ctk.CTkButton(self.base_list, text=self._base_label(entry), anchor="w", height=26,
                                   fg_color="transparent", hover_color="#27272a", text_color="#e4e4e7",
                                   command=lambda a=entry.actor: self.on_pick_base(a))
            self.base_buttons[entry.actor] = (entry, button)
        self._filter_bases()
        ctk.CTkLabel(tab, text="● standard skeleton   ◐ adapted (some bones stand in for others)   ✕ not humanoid",
                     text_color=GREY, wraplength=330, justify="left", font=ctk.CTkFont(size=10)).grid(
            row=3, column=0, sticky="w", pady=(4, 2))
        self.base_info = ctk.CTkLabel(tab, text="Loading…", wraplength=330, justify="left",
                                      font=ctk.CTkFont(size=11))
        self.base_info.grid(row=4, column=0, sticky="w", pady=(2, 4))
        self.suggest_button = ctk.CTkButton(tab, text="Suggest the best bases for my model", fg_color="#27272a",
                                            hover_color="#3f3f46", font=ctk.CTkFont(size=11, weight="bold"),
                                            command=self.on_suggest_bases)
        self.suggest_button.grid(row=5, column=0, sticky="ew", pady=(2, 2))
        self.suggest_frame = ctk.CTkFrame(tab, fg_color="transparent")
        self.suggest_frame.grid(row=6, column=0, sticky="ew")
        self.suggest_frame.grid_columnconfigure(0, weight=1)

    def _build_model_tab(self, tab):
        tab.grid_columnconfigure(0, weight=1)
        import_box = ctk.CTkFrame(tab, fg_color="transparent")
        import_box.grid(row=0, column=0, sticky="ew", pady=(4, 2))
        import_box.grid_columnconfigure(0, weight=1)
        ctk.CTkButton(import_box, text="Import a 3D model (.zip .obj .glb .gltf .fbx ...)", fg_color="#ffffff",
                      text_color="#09090b", hover_color="#e4e4e7", font=ctk.CTkFont(weight="bold"),
                      command=self.on_import).grid(row=0, column=0, sticky="ew")
        self.clean_var = ctk.BooleanVar(value=bool(self.settings.get("clean_on_import", True)))
        ctk.CTkCheckBox(import_box, text="Clean with Blender while importing (needed for .fbx, fixes scale, "
                                         "pieces and lost textures)", variable=self.clean_var, font=ctk.CTkFont(size=11),
                        command=lambda: self.settings.update(clean_on_import=bool(self.clean_var.get()))
                        ).grid(row=1, column=0, sticky="w", pady=(4, 0))
        budget_row = ctk.CTkFrame(import_box, fg_color="transparent")
        budget_row.grid(row=2, column=0, sticky="w", pady=(2, 0))
        ctk.CTkLabel(budget_row, text="Triangle budget (cleaning):", font=ctk.CTkFont(size=11)).grid(row=0, column=0)
        self.budget_var = ctk.StringVar(value=str(self.settings.get("triangle_budget", "8000")))
        ctk.CTkOptionMenu(budget_row, values=["No limit", "12000", "8000", "5000", "3000"], variable=self.budget_var,
                          width=100, height=22, font=ctk.CTkFont(size=11),
                          command=lambda v: self.settings.update(triangle_budget=v)).grid(row=0, column=1, padx=6)
        self.transfer_var = ctk.BooleanVar(value=bool(self.settings.get("robust_transfer", False)))
        ctk.CTkCheckBox(import_box, text="Robust weight transfer: also try copying the weights of the base body, "
                                         "keep the best (skirts, capes, long coats)", variable=self.transfer_var,
                        font=ctk.CTkFont(size=11),
                        command=lambda: self.settings.update(robust_transfer=bool(self.transfer_var.get()))
                        ).grid(row=3, column=0, sticky="w", pady=(4, 0))
        self.fingers_var = ctk.BooleanVar(value=bool(self.settings.get("finger_weights", True)))
        ctk.CTkCheckBox(import_box, text="Finger weights: hands follow the base's finger bones (closing fists, grips)",
                        variable=self.fingers_var, font=ctk.CTkFont(size=11),
                        command=lambda: self.settings.update(finger_weights=bool(self.fingers_var.get()))
                        ).grid(row=4, column=0, sticky="w", pady=(4, 0))
        ctk.CTkButton(tab, text="Import Blender Rig & Tracks (.glb .gltf .json)", fg_color="#27272a",
                      hover_color="#3f3f46", text_color="#e4e4e7",
                      command=self.on_import_rig).grid(row=1, column=0, sticky="ew", pady=(0, 6))
        self.model_info = ctk.CTkLabel(tab, text="No model loaded: showing the base figure.",
                                       text_color=GREY, wraplength=330, justify="left", font=ctk.CTkFont(size=11))
        self.model_info.grid(row=2, column=0, sticky="w", pady=(0, 10))

        self.base_label = ctk.CTkLabel(tab, text="Base character: —", font=ctk.CTkFont(size=12, weight="bold"))
        self.base_label.grid(row=3, column=0, sticky="w")
        ctk.CTkLabel(tab, text="Your character reuses its skeleton, animations and moveset (tab 0 to change it).",
                     text_color=GREY, font=ctk.CTkFont(size=11), wraplength=330, justify="left").grid(
            row=3, column=0, sticky="w", pady=(0, 10))

        ctk.CTkLabel(tab, text="Orientation (T-pose, facing you):").grid(row=4, column=0, sticky="w")
        row = ctk.CTkFrame(tab, fg_color="transparent")
        row.grid(row=5, column=0, sticky="ew", pady=(2, 10))
        for i, (text, cmd) in enumerate((("↻ 90°", lambda: self.on_turn(1)), ("↻ 180°", lambda: self.on_turn(2)),
                                         ("Stand up", self.on_upright))):
            row.grid_columnconfigure(i, weight=1)
            ctk.CTkButton(row, text=text, width=90, fg_color="#27272a", hover_color="#3f3f46", command=cmd).grid(
                row=0, column=i, padx=2, sticky="ew")

        self.height_label = ctk.CTkLabel(tab, text="Model height")
        self.height_label.grid(row=6, column=0, sticky="w")
        self.height_slider = ctk.CTkSlider(tab, from_=0.5, to=3.5, command=self.on_height)
        self.height_slider.set(2.0)
        self.height_slider.grid(row=7, column=0, sticky="ew", pady=(0, 10))
        self._value_entry(tab, 6, self.height_slider, self.on_height)
        self._update_height_label()

        ctk.CTkLabel(tab, text="Position (move the model if the skeleton is not centred in the body):",
                     wraplength=330, justify="left").grid(row=8, column=0, sticky="w")
        self.place_sliders = {}
        for i, (key, text, span) in enumerate(PLACEMENT_SLIDERS):
            r = 9 + 2 * i
            label = ctk.CTkLabel(tab, text=text, font=ctk.CTkFont(size=11), text_color=GREY)
            label.grid(row=r, column=0, sticky="w")
            slider = ctk.CTkSlider(tab, from_=-span, to=span, command=lambda v, k=key: self.on_place_slider(k, v))
            slider.set(0)
            slider.grid(row=r + 1, column=0, sticky="ew", pady=(0, 4))
            self.place_sliders[key] = (slider, label, text, span)
            self._value_entry(tab, r, slider, lambda v, k=key: self.on_place_slider(k, v))
        nudge = ctk.CTkFrame(tab, fg_color="transparent")
        nudge.grid(row=9 + 2 * len(PLACEMENT_SLIDERS), column=0, sticky="ew", pady=(2, 8))
        for i, (text, cmd) in enumerate((("◀ 1 cm", lambda: self.on_nudge("offset_x", -0.01)),
                                         ("1 cm ▶", lambda: self.on_nudge("offset_x", 0.01)),
                                         ("▼ back", lambda: self.on_nudge("offset_z", -0.01)),
                                         ("▲ forward", lambda: self.on_nudge("offset_z", 0.01)),
                                         ("Recentre", self.on_recentre))):
            nudge.grid_columnconfigure(i, weight=1)
            ctk.CTkButton(nudge, text=text, width=56, height=26, fg_color="#27272a", hover_color="#3f3f46",
                          font=ctk.CTkFont(size=11), command=cmd).grid(row=0, column=i, padx=1, sticky="ew")

        self.ghost_var = ctk.BooleanVar(value=True)
        ghost_row = ctk.CTkFrame(tab, fg_color="transparent")
        ghost_row.grid(row=10 + 2 * len(PLACEMENT_SLIDERS), column=0, sticky="w", pady=4)
        self.ghost_box = ctk.CTkCheckBox(ghost_row, text="Show the base figure alongside (comparison)",
                                         variable=self.ghost_var, command=self.on_ghost)
        self.ghost_box.grid(row=0, column=0, sticky="w")
        self.ghost_tpose_var = ctk.BooleanVar(value=bool(self.settings.get("ghost_tpose", True)))
        ctk.CTkCheckBox(ghost_row, text="in T-pose", variable=self.ghost_tpose_var, width=60,
                        command=self.on_ghost_tpose).grid(row=0, column=1, padx=(8, 0))

        r0 = 11 + 2 * len(PLACEMENT_SLIDERS)
        ctk.CTkLabel(tab, text="Pieces: Ctrl+click a part in the view (hair, cape, weapon…), \"Grow\" to take the "
                               "loose pieces touching it, then \"Lock\" to make it follow one bone rigidly.", wraplength=330, justify="left", text_color=GREY,
                     font=ctk.CTkFont(size=11)).grid(row=r0, column=0, sticky="w", pady=(10, 2))
        self.piece_label = ctk.CTkLabel(tab, text="No piece selected.", font=ctk.CTkFont(size=11), anchor="w")
        self.piece_label.grid(row=r0 + 1, column=0, sticky="w")
        piece_row = ctk.CTkFrame(tab, fg_color="transparent")
        piece_row.grid(row=r0 + 2, column=0, sticky="ew", pady=(2, 2))
        piece_row.grid_columnconfigure(0, weight=1)
        self.piece_bone = ctk.CTkOptionMenu(piece_row, values=list(PIECE_BONES), height=26, fg_color="#27272a",
                                            button_color="#3f3f46")
        self.piece_bone.set("Head")
        self.piece_bone.grid(row=0, column=0, sticky="ew", padx=(0, 4))
        self.piece_grow = ctk.CTkButton(piece_row, text="Grow", width=56, height=26, fg_color="#27272a",
                                        hover_color="#3f3f46", command=self.on_piece_grow, state="disabled")
        self.piece_grow.grid(row=0, column=1, padx=(0, 4))
        self.piece_apply = ctk.CTkButton(piece_row, text="Lock", width=56, height=26, fg_color="#27272a",
                                         hover_color="#3f3f46", command=self.on_piece_apply, state="disabled")
        self.piece_apply.grid(row=0, column=2)
        self.piece_list = ctk.CTkFrame(tab, fg_color="transparent")
        self.piece_list.grid(row=r0 + 3, column=0, sticky="ew")
        self.piece_list.grid_columnconfigure(0, weight=1)

        acc = ctk.CTkFrame(tab, fg_color="transparent")
        acc.grid(row=r0 + 4, column=0, sticky="ew", pady=(10, 0))
        acc.grid_columnconfigure(0, weight=1)
        ctk.CTkLabel(acc, text="Accessories (cape, tail, wings, weapon): they fake the body measurements. Find them, "
                               "then ignore them (kept, but skinned to follow the torso) or remove them.",
                     wraplength=330, justify="left", text_color=GREY, font=ctk.CTkFont(size=11)).grid(
            row=0, column=0, columnspan=2, sticky="w")
        self.acc_limit = ctk.CTkSlider(acc, from_=0.08, to=0.4, command=self.on_acc_limit)
        self.acc_limit.set(ZONE_LIMIT)
        self.acc_limit.grid(row=2, column=0, sticky="ew", pady=(0, 2))
        self.acc_limit_label = ctk.CTkLabel(acc, text="", font=ctk.CTkFont(size=11), text_color=GREY)
        self._value_entry(acc, 1, self.acc_limit, self.on_acc_limit, "{:.2f}")
        self.acc_limit_label.grid(row=1, column=0, sticky="w")
        self.on_acc_limit(ZONE_LIMIT, detect=False)
        acc_row = ctk.CTkFrame(acc, fg_color="transparent")
        acc_row.grid(row=3, column=0, columnspan=2, sticky="ew")
        acc_row.grid_columnconfigure((0, 1, 2, 3), weight=1)
        self.acc_buttons = {}
        for i, (key, text, cmd) in enumerate((("find", "Find", self.on_acc_find), ("ignore", "Ignore", self.on_acc_ignore),
                                              ("remove", "Remove", self.on_acc_remove),
                                              ("clear", "Clear", self.on_acc_clear))):
            self.acc_buttons[key] = ctk.CTkButton(acc_row, text=text, height=26, width=60, fg_color="#27272a",
                                                  hover_color="#3f3f46", command=cmd,
                                                  state="normal" if key == "find" else "disabled")
            self.acc_buttons[key].grid(row=0, column=i, padx=1, sticky="ew")
        self.acc_label = ctk.CTkLabel(acc, text="", font=ctk.CTkFont(size=11), anchor="w", wraplength=330,
                                      justify="left")
        self.acc_label.grid(row=4, column=0, columnspan=2, sticky="w")
        self.acc_list = ctk.CTkFrame(acc, fg_color="transparent")
        self.acc_list.grid(row=5, column=0, columnspan=2, sticky="ew")
        self.acc_list.grid_columnconfigure(0, weight=1)
        self._zone_mask = None

        look = ctk.CTkFrame(tab, fg_color="transparent")
        look.grid(row=r0 + 5, column=0, sticky="ew", pady=(10, 0))
        look.grid_columnconfigure(0, weight=1)
        self.look_label = ctk.CTkLabel(look, text="", wraplength=330, justify="left", text_color=GREY,
                                       font=ctk.CTkFont(size=11))
        self.look_label.grid(row=0, column=0, columnspan=2, sticky="w")
        self.look_slider = ctk.CTkSlider(look, from_=0, to=1, command=self._on_look_drag)
        self.look_slider.set(float(self.settings.get("game_look", 0.0)))
        self.look_slider.grid(row=1, column=0, sticky="ew")
        self.look_slider.bind("<ButtonRelease-1>", lambda _e: self.on_look(self.look_slider.get()))
        self._value_entry(look, 1, self.look_slider, self.on_look, "{:.2f}")
        self._on_look_drag(self.look_slider.get())

        ctk.CTkButton(tab, text="Preview the texture atlas", fg_color="#27272a", hover_color="#3f3f46",
                      command=self.on_preview_texture).grid(row=r0 + 6, column=0, sticky="ew", pady=(10, 4))

    def _build_skeleton_tab(self, tab):
        tab.grid_columnconfigure(0, weight=1)
        top = ctk.CTkFrame(tab, fg_color="transparent")
        top.grid(row=0, column=0, sticky="ew", pady=(0, 4))
        top.grid_columnconfigure((0, 1), weight=1)
        self.skel_var = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(top, text="Custom Skeleton (Green)", variable=self.skel_var,
                        command=self.on_skeleton_toggle, font=ctk.CTkFont(size=11)).grid(row=0, column=0, sticky="w")
        self.base_skel_var = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(top, text="Base Skeleton (Cyan)", variable=self.base_skel_var,
                        command=self.on_base_skeleton_toggle, font=ctk.CTkFont(size=11),
                        text_color="#38bdf8").grid(row=0, column=1, sticky="w")

        presets = ctk.CTkFrame(tab, fg_color="transparent")
        presets.grid(row=1, column=0, sticky="ew", pady=(2, 4))
        presets.grid_columnconfigure((0, 1), weight=1)
        ctk.CTkButton(presets, text="Auto-Fit Skeleton", fg_color="#27272a", hover_color="#3f3f46",
                      font=ctk.CTkFont(size=11, weight="bold"), command=self.on_auto_fit).grid(row=0, column=0, padx=2, sticky="ew")
        ctk.CTkButton(presets, text="Reset to Auto-Fit", fg_color="#27272a", hover_color="#3f3f46",
                      font=ctk.CTkFont(size=11), command=self.on_reset_fit).grid(row=0, column=1, padx=2, sticky="ew")

        self.overlay_var = ctk.BooleanVar(value=False)
        ctk.CTkCheckBox(tab, text="Superimpose base skeleton over model",
                        variable=self.overlay_var, command=self.on_overlay_toggle,
                        font=ctk.CTkFont(size=11), text_color=GREY).grid(row=2, column=0, sticky="w", pady=(0, 2))

        self.fit_label = ctk.CTkLabel(tab, text="Import a model to place the skeleton.", text_color=GREY,
                                      wraplength=330, justify="left", font=ctk.CTkFont(size=11))
        self.fit_label.grid(row=3, column=0, sticky="w", pady=(2, 4))
        self.sliders = {}
        self._status_icons, self._status_text = {}, {}
        scroll = ctk.CTkScrollableFrame(tab, fg_color="transparent", height=380)
        scroll.grid(row=4, column=0, sticky="nsew")
        scroll.grid_columnconfigure(0, weight=1)
        for i, (name, text, lo, hi, rel) in enumerate(SKELETON_SLIDERS):
            label = ctk.CTkLabel(scroll, text=text, font=ctk.CTkFont(size=12))
            label.grid(row=3 * i, column=0, sticky="w")
            slider = ctk.CTkSlider(scroll, from_=lo, to=hi, command=lambda v, n=name: self.on_slider(n, v))
            slider.grid(row=3 * i + 1, column=0, sticky="ew")
            slider.configure(state="disabled")
            ctk.CTkLabel(scroll, text=SLIDER_HINTS.get(name, ""), font=ctk.CTkFont(size=10), text_color=GREY,
                         wraplength=300, justify="left").grid(row=3 * i + 2, column=0, sticky="w", pady=(0, 6))
            self.sliders[name] = (slider, label, text, rel)
            self._value_entry(scroll, 3 * i, slider, lambda v, n=name: self.on_slider(n, v))
            icon = ctk.CTkLabel(scroll, text="", width=18, font=ctk.CTkFont(size=13))
            icon.grid(row=3 * i, column=2, sticky="e", padx=(4, 0))
            self._status_icons[name] = icon
            Tooltip(icon, lambda n=name: self._status_text.get(n, ""))
        legend = ("• Green skeleton: Fitted to your 3D model (arms, legs, head).\n"
                  "• Cyan skeleton: Base character reference (on the ghost figure).\n"
                  "• Note: The builder automatically retargets your model to the game's bind pose.")
        ctk.CTkLabel(tab, text=legend, text_color=GREY, wraplength=330, justify="left", font=ctk.CTkFont(size=10)).grid(
            row=5, column=0, sticky="w", pady=4)

    # -------------------------------------------------------- abilities & moveset
    def _build_abilities_tab(self, tab):
        tab.grid_columnconfigure(0, weight=1)
        tab.grid_rowconfigure(0, weight=1)

        scroll = ctk.CTkScrollableFrame(tab, fg_color="#0c0c0e", corner_radius=6)
        scroll.grid(row=0, column=0, sticky="nsew", padx=2, pady=2)
        scroll.grid_columnconfigure(0, weight=1)

        ctk.CTkLabel(scroll, text="Gameplay & Moveset Customization", font=ctk.CTkFont(size=14, weight="bold"),
                     text_color="#e4e4e7").grid(row=0, column=0, sticky="w", padx=8, pady=(8, 2))
        ctk.CTkLabel(scroll, text="Give your character unique attack movesets from other figures, "
                                  "flight, super powers, and custom imported animations.",
                     text_color=GREY, wraplength=330, justify="left", font=ctk.CTkFont(size=11)).grid(
            row=1, column=0, sticky="w", padx=8, pady=(0, 10))

        ctk.CTkLabel(scroll, text="Combat Archetype & Moveset:", font=ctk.CTkFont(size=12, weight="bold"),
                     text_color="#e4e4e7").grid(row=2, column=0, sticky="w", padx=8, pady=(4, 2))

        archetypes = [a.value for a in CombatArchetype]
        self.archetype_menu = ctk.CTkOptionMenu(
            scroll, values=archetypes, command=self._on_archetype_selected,
            height=28, fg_color="#27272a", button_color="#3f3f46", text_color="#e4e4e7"
        )
        self.archetype_menu.set(CombatArchetype.INHERIT_BASE.value)
        self.archetype_menu.grid(row=3, column=0, sticky="ew", padx=8, pady=(0, 6))

        self.borrow_frame = ctk.CTkFrame(scroll, fg_color="#18181b", corner_radius=6)
        self.borrow_frame.grid(row=4, column=0, sticky="ew", padx=8, pady=(0, 10))
        self.borrow_frame.grid_columnconfigure(1, weight=1)

        ctk.CTkLabel(self.borrow_frame, text="Borrow attacks from:", font=ctk.CTkFont(size=11)).grid(
            row=0, column=0, padx=8, pady=6, sticky="w")
        roster_names = [f"{e.display_name} ({e.actor})" for e in sorted(load_roster(), key=lambda x: x.display_name.lower())]
        self.combat_source_menu = ctk.CTkOptionMenu(
            self.borrow_frame, values=["Inherit Base Figure"] + roster_names,
            command=self._on_combat_source_selected,
            height=26, fg_color="#27272a", button_color="#3f3f46"
        )
        self.combat_source_menu.set("Inherit Base Figure")
        self.combat_source_menu.grid(row=0, column=1, padx=8, pady=6, sticky="ew")

        c_stats = ctk.CTkFrame(scroll, fg_color="transparent")
        c_stats.grid(row=5, column=0, sticky="ew", padx=8, pady=(0, 10))
        c_stats.grid_columnconfigure((0, 1), weight=1)

        self.dmg_label = ctk.CTkLabel(c_stats, text="Damage: 1.0x", font=ctk.CTkFont(size=11))
        self.dmg_label.grid(row=0, column=0, sticky="w")
        self.dmg_slider = ctk.CTkSlider(c_stats, from_=0.5, to=3.0, number_of_steps=25, command=self._on_dmg_slider)
        self.dmg_slider.set(1.0)
        self.dmg_slider.grid(row=1, column=0, sticky="ew", padx=(0, 4), pady=(2, 0))

        self.atk_spd_label = ctk.CTkLabel(c_stats, text="Atk Speed: 1.0x", font=ctk.CTkFont(size=11))
        self.atk_spd_label.grid(row=0, column=1, sticky="w")
        self.atk_spd_slider = ctk.CTkSlider(c_stats, from_=0.5, to=2.0, number_of_steps=15, command=self._on_atk_spd_slider)
        self.atk_spd_slider.set(1.0)
        self.atk_spd_slider.grid(row=1, column=1, padx=(4, 0), sticky="ew", pady=(2, 0))

        ctk.CTkLabel(scroll, text="Special Powers & Locomotion:", font=ctk.CTkFont(size=12, weight="bold"),
                     text_color="#e4e4e7").grid(row=6, column=0, sticky="w", padx=8, pady=(6, 2))

        powers_frame = ctk.CTkFrame(scroll, fg_color="#18181b", corner_radius=6)
        powers_frame.grid(row=7, column=0, sticky="ew", padx=8, pady=(0, 10))
        powers_frame.grid_columnconfigure(0, weight=1)

        self.fly_var = ctk.BooleanVar(value=False)
        self.fly_check = ctk.CTkCheckBox(powers_frame, text="Flight / Hover (Iron Man / Thor / Falcon)",
                                         variable=self.fly_var, command=self._on_power_toggled, font=ctk.CTkFont(size=11))
        self.fly_check.grid(row=0, column=0, sticky="w", padx=8, pady=(6, 4))

        self.super_jump_var = ctk.BooleanVar(value=False)
        self.super_jump_check = ctk.CTkCheckBox(powers_frame, text="Super Jump (Hulk / Heavy Leap)",
                                                variable=self.super_jump_var, command=self._on_power_toggled, font=ctk.CTkFont(size=11))
        self.super_jump_check.grid(row=1, column=0, sticky="w", padx=8, pady=4)

        self.double_jump_var = ctk.BooleanVar(value=True)
        self.double_jump_check = ctk.CTkCheckBox(powers_frame, text="Double Jump (Air Hop)",
                                                 variable=self.double_jump_var, command=self._on_power_toggled, font=ctk.CTkFont(size=11))
        self.double_jump_check.grid(row=2, column=0, sticky="w", padx=8, pady=4)

        self.wall_crawl_var = ctk.BooleanVar(value=False)
        self.wall_crawl_check = ctk.CTkCheckBox(powers_frame, text="Wall Crawling (Spider-Man / Venom)",
                                                variable=self.wall_crawl_var, command=self._on_power_toggled, font=ctk.CTkFont(size=11))
        self.wall_crawl_check.grid(row=3, column=0, sticky="w", padx=8, pady=4)

        self.glide_var = ctk.BooleanVar(value=False)
        self.glide_check = ctk.CTkCheckBox(powers_frame, text="Gliding (Descend smoothly)",
                                           variable=self.glide_var, command=self._on_power_toggled, font=ctk.CTkFont(size=11))
        self.glide_check.grid(row=4, column=0, sticky="w", padx=8, pady=(4, 6))

        self.speed_label = ctk.CTkLabel(scroll, text="Movement Speed: 1.0x", font=ctk.CTkFont(size=11))
        self.speed_label.grid(row=8, column=0, sticky="w", padx=8)
        self.speed_slider = ctk.CTkSlider(scroll, from_=0.5, to=2.5, number_of_steps=20, command=self._on_speed_slider)
        self.speed_slider.set(1.0)
        self.speed_slider.grid(row=9, column=0, sticky="ew", padx=8, pady=(2, 12))

        ctk.CTkLabel(scroll, text="Custom Animation Importer:", font=ctk.CTkFont(size=12, weight="bold"),
                     text_color="#e4e4e7").grid(row=10, column=0, sticky="w", padx=8, pady=(4, 2))
        ctk.CTkLabel(scroll, text="Import .glb, .gltf, .fbx, or .json animations and map them to actions.",
                     text_color=GREY, font=ctk.CTkFont(size=11)).grid(row=11, column=0, sticky="w", padx=8, pady=(0, 6))

        anim_box = ctk.CTkFrame(scroll, fg_color="#18181b", corner_radius=6)
        anim_box.grid(row=12, column=0, sticky="ew", padx=8, pady=(0, 10))
        anim_box.grid_columnconfigure(1, weight=1)

        ctk.CTkLabel(anim_box, text="Action Slot:", font=ctk.CTkFont(size=11)).grid(
            row=0, column=0, padx=8, pady=6, sticky="w")
        slot_names = [label for _, label in ANIM_ACTION_SLOTS]
        self.anim_slot_menu = ctk.CTkOptionMenu(
            anim_box, values=slot_names, height=26, fg_color="#27272a", button_color="#3f3f46",
            command=self._on_anim_slot_changed
        )
        self.anim_slot_menu.set(slot_names[0])
        self.anim_slot_menu.grid(row=0, column=1, padx=8, pady=6, sticky="ew")

        self.anim_status_label = ctk.CTkLabel(anim_box, text="No custom animation mapped for this slot.",
                                              text_color=GREY, font=ctk.CTkFont(size=11), wraplength=310, justify="left")
        self.anim_status_label.grid(row=1, column=0, columnspan=2, padx=8, pady=(0, 6), sticky="w")

        btn_row = ctk.CTkFrame(anim_box, fg_color="transparent")
        btn_row.grid(row=2, column=0, columnspan=2, padx=8, pady=(0, 8), sticky="ew")
        btn_row.grid_columnconfigure((0, 1), weight=1)

        self.import_anim_btn = ctk.CTkButton(
            btn_row, text="Import File...", fg_color="#ffffff", text_color="#09090b",
            hover_color="#e4e4e7", font=ctk.CTkFont(size=11, weight="bold"),
            command=self.on_import_custom_animation
        )
        self.import_anim_btn.grid(row=0, column=0, padx=(0, 4), sticky="ew")

        self.preview_anim_btn = ctk.CTkButton(
            btn_row, text="Preview Track", fg_color="#27272a", text_color="#e4e4e7",
            hover_color="#3f3f46", font=ctk.CTkFont(size=11),
            command=self.on_preview_custom_animation
        )
        self.preview_anim_btn.grid(row=0, column=1, padx=(4, 0), sticky="ew")

        self.clear_anim_btn = ctk.CTkButton(
            anim_box, text="Clear Slot", fg_color="transparent", text_color="#f87171",
            hover_color="#27272a", font=ctk.CTkFont(size=11),
            command=self.on_clear_custom_animation
        )
        self.clear_anim_btn.grid(row=3, column=0, columnspan=2, padx=8, pady=(0, 6), sticky="e")

        self.abilities_summary_label = ctk.CTkLabel(scroll, text="", text_color=GREEN,
                                                    wraplength=330, justify="left", font=ctk.CTkFont(size=11))
        self.abilities_summary_label.grid(row=13, column=0, sticky="w", padx=8, pady=(4, 10))
        self._update_abilities_summary()

    def _current_action_slot_key(self) -> str:
        current_label = self.anim_slot_menu.get()
        for key, label in ANIM_ACTION_SLOTS:
            if label == current_label:
                return key
        return "attack_1"

    def _sync_abilities_from_ui(self):
        self.abilities.can_fly = self.fly_var.get()
        self.abilities.super_jump = self.super_jump_var.get()
        self.abilities.double_jump = self.double_jump_var.get()
        self.abilities.wall_crawl = self.wall_crawl_var.get()
        self.abilities.glide = self.glide_var.get()
        self.abilities.speed_multiplier = float(self.speed_slider.get())
        self.abilities.damage_multiplier = float(self.dmg_slider.get())
        self.abilities.attack_speed_multiplier = float(self.atk_spd_slider.get())
        self._update_abilities_summary()

    def _update_abilities_summary(self):
        if hasattr(self, "abilities_summary_label"):
            summary_txt = self.abilities.summary(self.base)
            self.abilities_summary_label.configure(text=f"Summary: {summary_txt}")

    def _on_archetype_selected(self, choice: str):
        try:
            arch = CombatArchetype(choice)
        except ValueError:
            arch = CombatArchetype.INHERIT_BASE
        self.abilities.combat_archetype = arch

        if arch == CombatArchetype.INHERIT_BASE:
            self.abilities.combat_source_actor = None
            self.combat_source_menu.set("Inherit Base Figure")
        elif arch != CombatArchetype.CUSTOM_ROSTER:
            preset = ARCHETYPE_PRESETS.get(arch, {})
            self.fly_var.set(preset.get("can_fly", False))
            self.super_jump_var.set(preset.get("super_jump", False))
            self.wall_crawl_var.set(preset.get("wall_crawl", False))
            self.glide_var.set(preset.get("glide", False))
            dmg = preset.get("damage_multiplier", 1.0)
            spd = preset.get("speed_multiplier", 1.0)
            self.dmg_slider.set(dmg)
            self.dmg_label.configure(text=f"Damage: {dmg:.1f}x")
            self.speed_slider.set(spd)
            self.speed_label.configure(text=f"Movement Speed: {spd:.1f}x")

            actor = ARCHETYPE_ACTOR_MAP.get(arch)
            self.abilities.combat_source_actor = actor
            if actor:
                try:
                    entry = find_entry(actor)
                    self.combat_source_menu.set(f"{entry.display_name} ({entry.actor})")
                except KeyError:
                    pass

        self._sync_abilities_from_ui()
        self._refresh_moveset_clips()

    def _on_combat_source_selected(self, choice: str):
        if choice == "Inherit Base Figure":
            self.abilities.combat_source_actor = None
        else:
            import re
            m = re.search(r"\(([^)]+)\)", choice)
            if m:
                self.abilities.combat_source_actor = m.group(1).strip()
                self.archetype_menu.set(CombatArchetype.CUSTOM_ROSTER.value)
                self.abilities.combat_archetype = CombatArchetype.CUSTOM_ROSTER
        self._sync_abilities_from_ui()
        self._refresh_moveset_clips()

    def _on_dmg_slider(self, val):
        self.abilities.damage_multiplier = float(val)
        self.dmg_label.configure(text=f"Damage: {float(val):.1f}x")
        self._update_abilities_summary()

    def _on_atk_spd_slider(self, val):
        self.abilities.attack_speed_multiplier = float(val)
        self.atk_spd_label.configure(text=f"Atk Speed: {float(val):.1f}x")
        self._update_abilities_summary()

    def _on_power_toggled(self):
        self._sync_abilities_from_ui()

    def _on_speed_slider(self, val):
        self.abilities.speed_multiplier = float(val)
        self.speed_label.configure(text=f"Movement Speed: {float(val):.1f}x")
        self._update_abilities_summary()

    def _on_anim_slot_changed(self, choice: str):
        slot = self._current_action_slot_key()
        if slot in self.abilities.custom_animations:
            ca = self.abilities.custom_animations[slot]
            self.anim_status_label.configure(
                text=f"✓ Mapped: {ca.name} ({ca.duration:.2f}s, {int(round(ca.duration*ca.fps))} frames)",
                text_color=GREEN
            )
        else:
            self.anim_status_label.configure(
                text="No custom animation mapped for this slot.",
                text_color=GREY
            )

    def on_import_custom_animation(self):
        path = filedialog.askopenfilename(
            title="Import Animation Track",
            filetypes=[("Animation Files", "*.glb *.gltf *.fbx *.bvh *.json"), ("All Files", "*.*")]
        )
        if not path:
            return
        slot = self._current_action_slot_key()
        slot_label = self.anim_slot_menu.get()
        try:
            parsed = load_rig_file(path)
            if not parsed.animations:
                self.anim_status_label.configure(
                    text=f"⚠️ {Path(path).name} parsed, but contains 0 animation tracks.",
                    text_color=YELLOW
                )
                return
            anim = parsed.animations[0]
            ca = CustomAnimation(
                slot=slot,
                name=f"{Path(path).stem} ({anim.name})",
                source_path=path,
                duration=anim.duration,
                fps=anim.fps,
                track=anim,
            )
            self.abilities.custom_animations[slot] = ca

            clip = AnimationClip(
                name=f"[Custom] {slot_label.split('(')[0].strip()}: {anim.name}",
                duration=anim.duration,
                fps=anim.fps,
                custom_track=anim,
            )
            self.timeline.add_custom_clip(clip)
            self.anim_status_label.configure(
                text=f"✓ Loaded {ca.name} ({ca.duration:.2f}s, {int(round(ca.duration*ca.fps))} frames)",
                text_color=GREEN
            )
            self.log(f"Custom animation track imported for {slot}: {anim.name} ({anim.duration:.2f}s)")
            self._update_abilities_summary()
        except Exception as exc:
            self.anim_status_label.configure(
                text=f"Import failed: {exc}",
                text_color=RED
            )
            self.log(f"Custom animation import error: {exc}")

    def on_preview_custom_animation(self):
        slot = self._current_action_slot_key()
        if slot not in self.abilities.custom_animations:
            self.anim_status_label.configure(text="Please import an animation file first.", text_color=YELLOW)
            return
        ca = self.abilities.custom_animations[slot]
        slot_label = self.anim_slot_menu.get()
        target_clip = None
        for name in self.timeline.clips:
            if slot in name.lower() or ca.name in name or (ca.track and ca.track.name in name):
                target_clip = name
                break
        if target_clip:
            self.timeline._on_clip_selected(target_clip)

    def on_clear_custom_animation(self):
        slot = self._current_action_slot_key()
        if slot in self.abilities.custom_animations:
            del self.abilities.custom_animations[slot]
            self.anim_status_label.configure(text="Slot cleared.", text_color=GREY)
            self._update_abilities_summary()
            self._refresh_moveset_clips()

    def _refresh_moveset_clips(self):
        if not hasattr(self, "timeline") or not self.timeline:
            return
        game_dir = Path(self.game_entry.get().strip()) if hasattr(self, "game_entry") and self.game_entry.get() else None
        eff_actor = self.abilities.effective_combat_actor(self.base) if self.base else None

        target_base = self.base
        if eff_actor and self.base and eff_actor != self.base.actor:
            try:
                entry = find_entry(eff_actor)
                target_base = BaseCharacter.from_pack(entry)
            except Exception:
                target_base = self.base

        if target_base:
            clips = get_character_animation_clips(target_base, game_dir)
            # Re-add custom animations
            for slot, ca in self.abilities.custom_animations.items():
                if ca.track:
                    c_name = f"[Custom] {slot.upper()}: {ca.name}"
                    clips[c_name] = AnimationClip(name=c_name, duration=ca.duration, fps=ca.fps, custom_track=ca.track)
            self.timeline.set_clips(clips)

    def _build_export_tab(self, tab):
        tab.grid_columnconfigure(0, weight=1)

        def entry(row, label, value, browse=None):
            ctk.CTkLabel(tab, text=label, font=ctk.CTkFont(size=12)).grid(row=row, column=0, sticky="w")
            frame = ctk.CTkFrame(tab, fg_color="transparent")
            frame.grid(row=row + 1, column=0, sticky="ew", pady=(0, 6))
            frame.grid_columnconfigure(0, weight=1)
            e = ctk.CTkEntry(frame, fg_color="#27272a", border_width=0)
            e.insert(0, value)
            e.grid(row=0, column=0, sticky="ew")
            if browse:
                ctk.CTkButton(frame, text="…", width=32, fg_color="#27272a", hover_color="#3f3f46",
                              command=lambda: browse(e)).grid(row=0, column=1, padx=(4, 0))
            return e

        self.name_entry = entry(0, "Character name", self.settings.get("mod_name", "mario"))
        self.actor_entry = entry(2, "Internal actor name (3 to 32 characters, e.g. MAR_Mari)",
                                 default_actor(self.name_entry.get()))
        self.name_entry.bind("<KeyRelease>", self._sync_actor)
        self.actor_entry.bind("<KeyRelease>", lambda e: setattr(self, "_actor_edited", True))
        self.game_entry = entry(4, "Game folder", self.settings.get("game_path", ""), self._browse_dir)
        self.blender_entry = entry(6, "Blender (for high-quality skinning weights)",
                                   self.settings.get("blender_path", ""), self._browse_exe)

        self.rig_button = ctk.CTkButton(tab, text="1. Build the rig", fg_color="#3b82f6", hover_color="#2563eb",
                                        font=ctk.CTkFont(weight="bold"), command=self.on_rig)
        self.rig_button.grid(row=8, column=0, sticky="ew", pady=(8, 4))
        ctk.CTkLabel(tab, text="Rig preview:", font=ctk.CTkFont(size=12)).grid(row=9, column=0, sticky="w")
        pose_box = ctk.CTkFrame(tab, fg_color="transparent")
        pose_box.grid(row=10, column=0, sticky="ew", pady=(0, 6))
        pose_box.grid_columnconfigure(0, weight=1)
        self.pose_buttons = ctk.CTkSegmentedButton(pose_box, values=list(POSES), command=self.on_pose)
        self.pose_buttons.set("T-pose")
        self.pose_buttons.grid(row=0, column=0, sticky="ew")
        self.pose_buttons.configure(state="disabled")
        self.pose_amount_label = ctk.CTkLabel(pose_box, text="Pose amount: 100%", font=ctk.CTkFont(size=11),
                                              text_color=GREY)
        self.pose_amount_label.grid(row=1, column=0, sticky="w")
        self.pose_amount = ctk.CTkSlider(pose_box, from_=0.2, to=1.5, command=self.on_pose_amount)
        self.pose_amount.set(1.0)
        self.pose_amount.grid(row=2, column=0, sticky="ew")
        self._value_entry(pose_box, 1, self.pose_amount, self.on_pose_amount, "{:.2f}")
        self.quality_label = ctk.CTkLabel(pose_box, text="Deformation: build the rig to measure it.",
                                          font=ctk.CTkFont(size=11), text_color=GREY, justify="left", anchor="w")
        self.quality_label.grid(row=3, column=0, sticky="w", pady=(4, 0))
        self.heat_var = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(pose_box, text="Show stretched edges in red", variable=self.heat_var,
                        command=self.on_heat_toggle, font=ctk.CTkFont(size=11), checkbox_width=18,
                        checkbox_height=18).grid(row=4, column=0, sticky="w", pady=(2, 0))

        ctk.CTkLabel(tab, text="Entity Type (Character, Enemy, Ally/NPC):", font=ctk.CTkFont(size=12)).grid(
            row=11, column=0, sticky="w")
        self.entity_type_btn = ctk.CTkSegmentedButton(tab, values=["Avatar", "Enemy", "Ally / NPC"],
                                                      command=self._on_entity_type_changed)
        self.entity_type_btn.set("Avatar")
        self.entity_type_btn.grid(row=12, column=0, sticky="ew", pady=(0, 6))

        # Enemy / NPC parameters frame
        self.npc_frame = ctk.CTkFrame(tab, fg_color="#18181b", corner_radius=6)
        self.npc_frame.grid_columnconfigure(1, weight=1)

        ctk.CTkLabel(self.npc_frame, text="Faction:", font=ctk.CTkFont(size=11)).grid(row=0, column=0, padx=6, pady=4, sticky="w")
        self.faction_menu = ctk.CTkOptionMenu(self.npc_frame, values=["Enemies", "Villains", "Townsperson", "Neutral"],
                                              height=24, fg_color="#27272a", button_color="#3f3f46")
        self.faction_menu.set("Enemies")
        self.faction_menu.grid(row=0, column=1, padx=6, pady=4, sticky="ew")

        ctk.CTkLabel(self.npc_frame, text="HP / Damage:", font=ctk.CTkFont(size=11)).grid(row=1, column=0, padx=6, pady=4, sticky="w")
        hd_row = ctk.CTkFrame(self.npc_frame, fg_color="transparent")
        hd_row.grid(row=1, column=1, padx=6, pady=4, sticky="ew")
        hd_row.grid_columnconfigure((0, 1), weight=1)
        self.health_entry = ctk.CTkEntry(hd_row, height=24, placeholder_text="HP", fg_color="#27272a", border_width=0)
        self.health_entry.insert(0, "250")
        self.health_entry.grid(row=0, column=0, padx=(0, 2), sticky="ew")
        self.damage_entry = ctk.CTkEntry(hd_row, height=24, placeholder_text="DMG", fg_color="#27272a", border_width=0)
        self.damage_entry.insert(0, "35")
        self.damage_entry.grid(row=0, column=1, padx=(2, 0), sticky="ew")

        ctk.CTkLabel(self.npc_frame, text="Animations:", font=ctk.CTkFont(size=11)).grid(row=2, column=0, padx=6, pady=4, sticky="w")
        anim_row = ctk.CTkFrame(self.npc_frame, fg_color="transparent")
        anim_row.grid(row=2, column=1, padx=6, pady=4, sticky="ew")
        anim_row.grid_columnconfigure((0, 1), weight=1)
        self.anim_walk_menu = ctk.CTkOptionMenu(anim_row, values=["Walk", "Run", "Patrol"], height=24,
                                                fg_color="#27272a", button_color="#3f3f46")
        self.anim_walk_menu.set("Walk")
        self.anim_walk_menu.grid(row=0, column=0, padx=(0, 2), sticky="ew")
        self.anim_attack_menu = ctk.CTkOptionMenu(anim_row, values=["Attack", "Slash", "Shoot"], height=24,
                                                  fg_color="#27272a", button_color="#3f3f46")
        self.anim_attack_menu.set("Attack")
        self.anim_attack_menu.grid(row=0, column=1, padx=(2, 0), sticky="ew")

        self.install_button = ctk.CTkButton(tab, text="2. Install into the game", fg_color="#fbbf24",
                                            text_color="#09090b", hover_color="#f59e0b",
                                            font=ctk.CTkFont(weight="bold"), command=self.on_install)
        self.install_button.grid(row=15, column=0, sticky="ew", pady=(4, 8))
        self.zip_var = ctk.BooleanVar(value=bool(self.settings.get("legacy_zip", True)))
        ctk.CTkCheckBox(tab, text="Ship the texture archive (.zip) - required for the textures to show in game",
                        variable=self.zip_var, font=ctk.CTkFont(size=11),
                        command=lambda: self.settings.update(legacy_zip=bool(self.zip_var.get()))
                        ).grid(row=14, column=0, sticky="w", pady=(4, 0))
        self.log_box = ctk.CTkTextbox(tab, height=240, fg_color="#0c0c0e", font=ctk.CTkFont(family="Consolas", size=11),
                                      wrap="word")
        self.log_box.grid(row=16, column=0, sticky="nsew")
        tab.grid_rowconfigure(16, weight=1)

    def _on_entity_type_changed(self, choice: str):
        if choice == "Avatar":
            self.npc_frame.grid_remove()
            self.install_button.configure(text="2. Install into the game")
        else:
            self.npc_frame.grid(row=13, column=0, sticky="ew", pady=(0, 6))
            if choice == "Enemy":
                self.faction_menu.set("Enemies")
                self.health_entry.delete(0, "end")
                self.health_entry.insert(0, "250")
                self.damage_entry.delete(0, "end")
                self.damage_entry.insert(0, "40")
                self.install_button.configure(text="2. Install Enemy into game")
            else:
                self.faction_menu.set("Townsperson")
                self.health_entry.delete(0, "end")
                self.health_entry.insert(0, "150")
                self.damage_entry.delete(0, "end")
                self.damage_entry.insert(0, "20")
                self.install_button.configure(text="2. Install Ally/NPC into game")

    # ------------------------------------------------------------- helpers
    def _browse_dir(self, e):
        path = filedialog.askdirectory(title="Disney Infinity 3.0 folder", initialdir=e.get() or None)
        if path:
            e.delete(0, "end")
            e.insert(0, path)
            self._save_settings()

    def _browse_exe(self, e):
        path = filedialog.askopenfilename(title="blender.exe", filetypes=[("blender.exe", "blender.exe"), ("*", "*.*")])
        if path:
            e.delete(0, "end")
            e.insert(0, path)

    def _sync_actor(self, _=None):
        if not self._actor_edited:
            self.actor_entry.delete(0, "end")
            self.actor_entry.insert(0, default_actor(self.name_entry.get()))

    def _save_settings(self):
        self.settings.update(game_path=self.game_entry.get().strip(), blender_path=normalize_blender_path(self.blender_entry.get()),
                             mod_name=self.name_entry.get().strip())
        if self.base:
            self.settings["base_actor"] = self.base.actor
        save_settings(self.settings)

    def _drain_events(self):
        """Worker threads never touch Tk: they queue callables that run here, on the UI thread."""
        while True:
            try:
                self._events.get_nowait()()
            except queue.Empty:
                break
        self.after(50, self._drain_events)

    def log(self, text):
        def write():
            self.log_box.insert("end", text + "\n")
            self.log_box.see("end")
        self._events.put(write)

    def _set_busy(self, busy):
        self.busy = busy
        state = "disabled" if busy else "normal"
        self.rig_button.configure(state=state)
        self.install_button.configure(state=state)

    def _update_height_label(self):
        ref = f" ({self.base.display_name} = {self.base.fit_height:.2f})" if self.base else ""
        self.height_label.configure(text=f"Model height: {self.height_slider.get():.2f}{ref}")
        self._sync_entry(self.height_slider)

    def _value_entry(self, parent, row, slider, apply, fmt="{:.3f}"):
        """A box next to a slider to type the exact value (Enter or leaving the box applies it)."""
        if not hasattr(self, "_entries"):
            self._entries = {}
        var = ctk.StringVar(value=fmt.format(slider.get()))
        entry = ctk.CTkEntry(parent, width=64, height=22, textvariable=var, justify="right", font=ctk.CTkFont(size=11))
        entry.grid(row=row, column=1, sticky="e", padx=(6, 0))

        def commit(_event=None):
            try:
                value = float(var.get().replace(",", "."))
            except ValueError:
                var.set(fmt.format(slider.get()))
                return
            low, high = sorted((slider._from_, slider._to))
            value = min(max(value, low), high)
            slider.set(value)
            var.set(fmt.format(value))
            apply(value)
        entry.bind("<Return>", commit)
        entry.bind("<FocusOut>", commit)
        self._entries[slider] = (var, fmt, commit)

    def _sync_entry(self, slider):
        var, fmt, _ = getattr(self, "_entries", {}).get(slider, (None, None, None))
        if var is not None:
            var.set(fmt.format(slider.get()))

    # ---------------------------------------------------------------- bases
    def _base_label(self, entry) -> str:
        mark = status(entry.actor).mark
        franchise = f"   ·  {entry.franchise}" if entry.franchise else ""
        return f"{mark}  {entry.display_name}{franchise}"

    def _filter_bases(self):
        needle = self.search_entry.get().strip().lower()
        row = 0
        for actor, (entry, button) in self.base_buttons.items():
            hay = f"{entry.display_name} {entry.actor} {entry.franchise}".lower()
            if needle in hay:
                button.grid(row=row, column=0, sticky="ew")
                row += 1
            else:
                button.grid_remove()

    def _highlight_base(self):
        for actor, (_, button) in self.base_buttons.items():
            selected = self.base is not None and actor == self.base.actor
            button.configure(fg_color="#1e3a8a" if selected else "transparent")

    def on_pick_base(self, actor: str):
        if self.busy:
            return
        try:
            entry = find_entry(actor)
        except KeyError:
            entry = find_entry(DEFAULT_BASE)
        self.base_info.configure(text=f"Loading {entry.display_name}…", text_color=GREY)

        def job():
            base = BaseCharacter.from_pack(entry)
            parts = base.preview_parts(preview_texture)
            _ = base.skeleton, base.body_mesh, base.winding_sign
            return base, parts

        def done(result, error):
            self._set_busy(False)
            if error:
                self.base_info.configure(text=f"Could not load {entry.display_name}: {error[0]}", text_color=RED)
                print(error[1])
                return
            base, parts = result
            self.base = base
            self.base_parts = parts
            self.timeline.set_rig_ready(self.rig is not None)
            self._save_settings()
            self._highlight_base()
            self.renderer.set_ghost(parts)
            self.renderer.show_ghost = self.ghost_var.get()
            self._show_base_rest(self.pose == "tpose")
            game_dir = Path(self.game_entry.get().strip()) if hasattr(self, "game_entry") and self.game_entry.get() else None
            self._refresh_moveset_clips()
            self._update_abilities_summary()
            skeleton = base.skeleton

            colour = GREEN if not skeleton.fallbacks else YELLOW
            if not base.supported:
                colour = RED
            mismatch = ""
            if self.model and self.props:
                m_leg = self.props.crotch_y / max(self.props.height, 0.1)
                b_leg = base.joints["Hips"][1] / max(base.height, 0.1)
                if abs(m_leg - b_leg) > 0.20:
                    colour = YELLOW
                    mismatch = (f"\n⚠️ Mismatch: {base.display_name} has {int(b_leg*100)}% legs vs {int(m_leg*100)}% on model. "
                                f"Prefer Donald Duck, Stitch, or Syndrome.")
            self.base_info.configure(
                text=f"{base.display_name}  ({base.actor})\n"
                     f"model {base.model}, {len(base.meshes)} mesh(es), {len(base.bone_names)} skinned bones, "
                     f"height {base.height:.2f}\n{skeleton.summary()}"
                     + mismatch
                     + ("" if base.supported else "\nThis figure cannot take a humanoid mesh: pick another one."),
                text_color=colour)
            self.base_label.configure(text=f"Base character: {base.display_name} ({base.actor})")
            self.ghost_box.configure(text=f"Show {base.display_name} alongside (comparison)")
            if self.model:
                self.height_slider.set(base.fit_height)
                self.on_height(base.fit_height)
            else:
                self.height_slider.set(base.fit_height)
                self._update_height_label()
            self.renderer.frame_all()
        self._run(job, done)

    # -------------------------------------------------------------- model
    def on_preview_texture(self):
        """Show the texture the game will receive (after atlas packing) with a few sanity checks."""
        if not self.model:
            self.model_info.configure(text="Import a model first.", text_color=YELLOW)
            return
        import numpy as np
        image = self.model.source.texture.convert("RGB")
        pixels = np.asarray(image, dtype=np.float32)
        luminance = pixels @ np.array([0.299, 0.587, 0.114], np.float32)
        dark = float((luminance < 12).mean())
        notes = [f"{image.width}×{image.height} px, average brightness {luminance.mean():.0f}/255, "
                 f"{dark * 100:.0f}% of pixels almost black."]
        if dark > 0.6 or luminance.mean() < 25:
            notes.append("⚠ Mostly black: the model probably had no colour texture (or it was lost on import). "
                         "It will look black in game too.")
        if image.width != image.height:
            notes.append("Not square: the game packs it to a power of two.")
        window = ctk.CTkToplevel(self)
        window.title("Texture atlas")
        window.geometry("700x780")
        shown = image.copy()
        shown.thumbnail((660, 660))
        picture = ctk.CTkImage(light_image=shown, dark_image=shown, size=shown.size)
        label = ctk.CTkLabel(window, image=picture, text="")
        label.image = picture
        label.pack(padx=12, pady=12)
        ctk.CTkLabel(window, text="\n".join(notes), wraplength=650, justify="left",
                     text_color=RED if len(notes) > 1 and notes[1].startswith("⚠") else GREY).pack(padx=12)
        window.after(100, window.lift)

    def _model_pieces(self):
        """Connected-piece label of every vertex of the imported model (cached per model)."""
        if self._piece_labels is None and self.model:
            from scipy.sparse import coo_matrix
            from scipy.sparse.csgraph import connected_components
            positions = self.model.vertices()
            # UV seams duplicate vertices: weld by position first, or every seam would cut a piece in two.
            _, weld = np.unique(np.round(positions * 1e5).astype(np.int64), axis=0, return_inverse=True)
            weld = weld.reshape(-1)
            f, n = weld[self.model.faces], int(weld.max()) + 1
            graph = coo_matrix((np.ones(len(f) * 2), (np.r_[f[:, 0], f[:, 1]], np.r_[f[:, 1], f[:, 2]])), shape=(n, n))
            self._piece_labels = connected_components(graph, directed=False)[1][weld]
        return self._piece_labels

    def on_pick_piece(self, event):
        if not self.model:
            return
        index = self.renderer.pick_vertex(event.x, event.y)
        if index is None:
            self._selected_pieces = set()
        else:
            self._selected_pieces = {int(self._model_pieces()[index])}
        self._show_piece_selection()

    # ------------------------------------------------------------ accessories
    def _selection_mask(self):
        """Vertices selected by piece and/or by the accessory finder."""
        if not self.model:
            return None
        mask = np.zeros(len(self.model.source.vertices), bool)
        if self._selected_pieces:
            mask |= np.isin(self._model_pieces(), list(self._selected_pieces))
        if self._zone_mask is not None:
            mask |= self._zone_mask
        return mask if mask.any() else None

    def _on_look_drag(self, value):
        self._sync_entry(self.look_slider)
        self.look_label.configure(text=f"Game look (texture filter): {float(value):.2f}. Softens baked shading and "
                                       "noise, lifts shadows, boosts colours like the base characters. 0 = original.")

    def on_look(self, value):
        """Apply the game-look filter to the texture shown and exported."""
        value = float(value)
        self._on_look_drag(value)
        self.settings.update(game_look=value)
        if not self.model:
            return
        self.model.look = value

        def job():
            self.model.texture  # computed (and cached) off the UI thread
        def done(_result, error):
            self._set_busy(False)
            if error:
                return self._failed(error)
            if self.rig is not None:
                self.renderer.set_model(self.rig.tpose, self.rig.faces, self.rig.uvs, self.model.texture)
            else:
                self.renderer.set_model(self.model.vertices(), self.model.faces, self.model.uvs, self.model.texture)
            self.log(f"Game look {value:.2f}: texture updated" if value > 0 else "Game look off: original texture.")
        self._run(job, done)

    def on_acc_limit(self, value, detect=True):
        self.acc_limit_label.configure(text=f"Depth behind the body to count as accessory: {float(value):.2f} × height")
        if detect and self.model and self._zone_mask is not None:
            self.on_acc_find()

    def on_acc_find(self):
        if not self.model:
            self.acc_label.configure(text="Import a model first.")
            return
        findings = detect(self.model.vertices(), self._model_pieces(), float(self.acc_limit.get()))
        self._zone_mask = np.logical_or.reduce([f.mask for f in findings]) if findings else None
        self.acc_label.configure(text="\n".join(f"• {f.text}" for f in findings) if findings else
                                 "Nothing found at this depth: lower the limit to catch less deep parts.")
        self._show_piece_selection()

    def _apply_accessories(self, kind):
        mask = self._selection_mask()
        if mask is None or not self.model:
            return
        attr = "ignored" if kind == "ignore" else "removed"
        old = getattr(self.model, attr)
        setattr(self.model, attr, mask if old is None else (old | mask))
        self._selected_pieces, self._zone_mask = set(), None
        self.acc_label.configure(text="")
        self._refresh_accessory_list()
        if kind == "remove":
            self.piece_overrides = []
            self._refresh_piece_list()
            self.renderer.set_model(self.model.vertices(), self.model.faces, self.model.uvs, self.model.texture)
        self._show_piece_selection()
        self._refit()
        self.log(f"{int(mask.sum())} vertices {'ignored for the measurements and following the torso' if kind == 'ignore' else 'removed from the model'}. Build the rig again to apply it.")

    def on_acc_ignore(self):
        self._apply_accessories("ignore")

    def on_acc_remove(self):
        self._apply_accessories("remove")

    def on_acc_clear(self):
        self._selected_pieces, self._zone_mask = set(), None
        self.acc_label.configure(text="")
        self._show_piece_selection()

    def _restore_accessories(self, attr):
        setattr(self.model, attr, None)
        self.renderer.set_model(self.model.vertices(), self.model.faces, self.model.uvs, self.model.texture)
        self._refresh_accessory_list()
        self._refit()

    def _refresh_accessory_list(self):
        for child in self.acc_list.winfo_children():
            child.destroy()
        if not self.model:
            return
        row = 0
        for attr, title in (("ignored", "ignored (follow the torso)"), ("removed", "removed")):
            mask = getattr(self.model, attr)
            if mask is not None and mask.any():
                ctk.CTkLabel(self.acc_list, text=f"• {int(mask.sum())} vertices {title}", font=ctk.CTkFont(size=11),
                             anchor="w").grid(row=row, column=0, sticky="w")
                ctk.CTkButton(self.acc_list, text="✕", width=26, height=22, fg_color="#27272a", hover_color="#3f3f46",
                              command=lambda a=attr: self._restore_accessories(a)).grid(row=row, column=1, padx=2)
                row += 1

    def on_piece_grow(self):
        """Add the small loose pieces that touch the selection (hair strips, cloth cards, jewels...)."""
        if not self._selected_pieces or not self.model:
            return
        from scipy.spatial import cKDTree
        labels, positions = self._model_pieces(), self.model.vertices()
        sizes = np.bincount(labels)
        reach = 0.03 * float(np.ptp(positions[:, 1]))
        chosen = np.isin(labels, list(self._selected_pieces))
        near = cKDTree(positions[chosen]).query(positions, distance_upper_bound=reach)[0] < reach
        for label in np.unique(labels[near & ~chosen]):
            if sizes[label] <= GROW_MAX_PIECE:
                self._selected_pieces.add(int(label))
        self._show_piece_selection()

    def _show_piece_selection(self):
        mask = self._selection_mask()
        has_pieces = bool(self._selected_pieces)
        for key in ("ignore", "remove", "clear"):
            self.acc_buttons[key].configure(state="normal" if mask is not None else "disabled")
        if mask is None:
            self.piece_label.configure(text="No piece selected.")
            self.piece_apply.configure(state="disabled")
            self.piece_grow.configure(state="disabled")
            self.renderer.set_select_lines(None)
            self.renderer.request_render()
            return
        faces = self.model.faces[mask[self.model.faces].all(axis=1)]
        edges = np.unique(np.sort(np.concatenate([faces[:, [0, 1]], faces[:, [1, 2]], faces[:, [2, 0]]]), axis=1), axis=0)
        self.piece_label.configure(text=f"Selected: {int(mask.sum())} vertices (magenta in the view)"
                                        + (f", {len(self._selected_pieces)} piece(s)" if has_pieces else ""))
        self.piece_apply.configure(state="normal" if has_pieces else "disabled")
        self.piece_grow.configure(state="normal" if has_pieces else "disabled")
        self.renderer.set_select_lines(self.model.vertices()[edges].reshape(-1, 3))
        self.renderer.request_render()

    def on_piece_apply(self):
        if not self._selected_pieces or not self.model:
            return
        labels = self._model_pieces()
        bone = PIECE_BONES[self.piece_bone.get()]
        reps = tuple(int(np.nonzero(labels == lab)[0][0]) for lab in sorted(self._selected_pieces))
        taken = set(self._selected_pieces)
        # A piece can only follow one bone: drop it from older locks.
        updated = []
        for idxs, b in self.piece_overrides:
            keep = tuple(i for i in idxs if int(labels[i]) not in taken)
            if keep:
                updated.append((keep, b))
        updated.append((reps, bone))
        self.piece_overrides = updated
        self._refresh_piece_list()
        self._geometry_changed()  # the rig is stale until rebuilt
        self.log(f"{len(reps)} piece(s) locked to {self.piece_bone.get()}. Build the rig again to apply it.")

    def _refresh_piece_list(self):
        for child in self.piece_list.winfo_children():
            child.destroy()
        names = {v: k for k, v in PIECE_BONES.items()}
        labels = self._model_pieces() if self.model else None
        for row, (indices, bone) in enumerate(self.piece_overrides):
            size = int(np.isin(labels, [labels[i] for i in indices]).sum()) if labels is not None else 0
            ctk.CTkLabel(self.piece_list, text=f"• {len(indices)} piece(s), {size} vertices → {names.get(bone, bone)}",
                         font=ctk.CTkFont(size=11), anchor="w").grid(row=row, column=0, sticky="w")
            ctk.CTkButton(self.piece_list, text="✕", width=26, height=22, fg_color="#27272a", hover_color="#3f3f46",
                          command=lambda r=row: self._remove_piece_override(r)).grid(row=row, column=1, padx=2)

    def _remove_piece_override(self, row):
        if 0 <= row < len(self.piece_overrides):
            del self.piece_overrides[row]
            self._refresh_piece_list()
            self._geometry_changed()

    def on_suggest_bases(self, more: bool = False):
        """Compare every figure with the imported model; the best ones get the full rig test, `more` tests the next
        batch (the moveset comes with the figure, so the whole roster stays pickable)."""
        if self.busy:
            return
        if not self.model:
            self.base_info.configure(text="Import a model first (tab 1), then ask for suggestions.", text_color=YELLOW)
            return
        source, blender = self.model.source, normalize_blender_path(self.blender_entry.get()) if hasattr(self, "blender_entry") else ""
        previous = {c.actor: c for c in getattr(self, "_candidates", []) if c.tested} if more else {}
        self.suggest_button.configure(state="disabled", text="Testing the figures… (1 to 2 minutes)")
        transfer = self._weight_transfer()
        fingers = bool(self.fingers_var.get())
        ignored, removed = self.model.ignored, self.model.removed

        def job():
            return rank_bases(source, load_roster(), WORK_DIR / "rank", lambda text: self._events.put(
                lambda: self.suggest_button.configure(text=text)),
                blender=Path(blender) if blender else None, weight_transfer=transfer, skip=set(previous),
                fingers=fingers, ignored=ignored, removed=removed)

        def done(result, error):
            self._set_busy(False)
            self.suggest_button.configure(state="normal", text="Suggest the best bases for my model")
            if error:
                self._failed(error)
                return
            tested = sorted([c for c in result if c.tested] + list(previous.values()),
                            key=lambda c: (-c.score, c.distortion))
            self._candidates = tested + [c for c in result if not c.tested]
            self._show_candidates()
        self._run(job, done)

    def _show_candidates(self):
        """The whole ranked roster in a scrollable list, filterable by name or franchise."""
        for child in self.suggest_frame.winfo_children():
            child.destroy()
        candidates = getattr(self, "_candidates", [])
        if not candidates:
            return
        self.suggest_filter = ctk.StringVar()
        entry = ctk.CTkEntry(self.suggest_frame, textvariable=self.suggest_filter, height=24,
                             placeholder_text="Filter by name or franchise (Avengers, Star Wars…)")
        entry.grid(row=0, column=0, sticky="ew", pady=(0, 2))
        entry.bind("<KeyRelease>", lambda _e: self._fill_candidates())
        self.suggest_list = ctk.CTkScrollableFrame(self.suggest_frame, height=230, fg_color="transparent")
        self.suggest_list.grid(row=1, column=0, sticky="ew")
        self.suggest_list.grid_columnconfigure(0, weight=1)
        self.suggest_more = ctk.CTkButton(self.suggest_frame, height=24, fg_color="#27272a",
                                          text="Test the next 12 figures on this model",
                                          command=lambda: self.on_suggest_bases(more=True))
        self.suggest_more.grid(row=2, column=0, sticky="ew", pady=2)
        ctk.CTkLabel(self.suggest_frame, text="Tested figures: score = body shape match, little stretching in 6 test "
                                              "poses (shoulders, elbows, torso), little distortion when baking your "
                                              "T-pose. Untested ones are ranked by silhouette only. The moveset comes "
                                              "with the figure: check the comparison view.", text_color=GREY,
                     wraplength=330, justify="left", font=ctk.CTkFont(size=10)).grid(row=3, column=0, sticky="w")
        self._fill_candidates()

    def _fill_candidates(self):
        for child in self.suggest_list.winfo_children():
            child.destroy()
        needle = self.suggest_filter.get().strip().lower()
        shown = 0
        for i, c in enumerate(self._candidates, 1):
            if needle and needle not in c.display_name.lower() and needle not in c.franchise.lower():
                continue
            colour = {"good": GREEN, "warn": YELLOW, "bad": RED}[c.level] if c.tested else GREY
            franchise = f" [{c.franchise}]" if c.franchise else ""
            ctk.CTkButton(self.suggest_list, anchor="w", height=24, fg_color="#18181b", hover_color="#27272a",
                          text=f"{i}. {c.display_name}{franchise} — {c.score}/100  ({c.note})", text_color=colour,
                          command=lambda a=c.actor: self.on_pick_base(a)).grid(row=shown, column=0, sticky="ew", pady=1)
            shown += 1
            if shown >= 120:
                break

    def _weight_transfer(self) -> bool:
        """True when the robust weight transfer option is on and usable (the reason is logged otherwise)."""
        if not self.transfer_var.get():
            return False
        from src.core.weight_transfer import availability

        ok, message = availability()
        if not ok:
            self.log(f"Robust weight transfer skipped: {message}")
        return ok

    def _triangle_budget(self) -> int:
        value = getattr(self, "budget_var", None)
        text = value.get() if value is not None else "No limit"
        return int(text) if text.isdigit() else 0

    def on_import(self):
        if self.base is None:
            self.model_info.configure(text="Pick a base figure first (tab 0).", text_color=RED)
            return
        path = filedialog.askopenfilename(
            title="Import a 3D model or a ZIP archive",
            filetypes=[("3D models and ZIP", "*.zip *.obj *.glb *.gltf *.fbx *.dae *.ply *.stl *.off"), ("All", "*.*")])
        if not path:
            return
        blender = normalize_blender_path(self.blender_entry.get()) if hasattr(self, "blender_entry") else ""
        if Path(path).suffix.lower() in NEEDS_BLENDER or self.clean_var.get():
            if not blender or not Path(blender).is_file():
                if Path(path).suffix.lower() in NEEDS_BLENDER:
                    self.model_info.configure(text="This format needs Blender: set blender.exe in the Rig & Export "
                                                   "tab.", text_color=RED)
                    return
                errorlog.info(f"import without cleaning: blender path {blender!r} is not a file")
                self._finish_import(path, path, [f"NOT cleaned: blender.exe not found ('{blender or 'empty'}'). "
                                                  "Set it in the Rig & Export tab, then import again."])
                return
            self.model_info.configure(text=f"Cleaning {Path(path).name} with Blender...", text_color=YELLOW)

            def done(result, error):
                self._set_busy(False)
                if error:
                    self.model_info.configure(text=f"Cleaning failed: {error[0]}", text_color=RED)
                    return
                self._finish_import(str(result[0]), path, result[1])
            self._run(lambda: clean_model(path, blender, max_triangles=self._triangle_budget()), done)
            return
        self._finish_import(path, path, [])

    def _finish_import(self, load_path, shown_path, notes):
        path = shown_path
        try:
            source = load_source(load_path)
        except Exception as exc:
            self.model_info.configure(text=f"Error: {exc}", text_color=RED)
            return
        source.report = list(notes) + list(source.report)
        placement = auto_placement(source, self.height_slider.get())
        self.model = CharacterModel(source, placement, look=float(self.look_slider.get()))
        self.model_info.configure(
            text=f"{Path(path).name}\n{len(source.vertices)} vertices, {len(source.faces)} triangles, "
                 f"texture {source.texture.width}×{source.texture.height}\n"
                 f"Auto orientation: {placement.quarter_turns % 4 * 90}°"
                 f"{', stood up (Z-up file)' if placement.upright_fix else ''}"
                 + ("".join(f"\n⚠ {line}" for line in source.report[:6]) if source.report else ""),
            text_color=YELLOW if source.report else GREEN)
        self.renderer.set_model(self.model.vertices(), self.model.faces, self.model.uvs, self.model.texture)
        self.piece_overrides, self._piece_labels, self._selected_pieces = [], None, set()
        self._zone_mask = None
        self.acc_label.configure(text="")
        self._refresh_accessory_list()
        self._refresh_piece_list()
        self._show_piece_selection()
        self._sync_placement_controls()
        self._refit()
        self.renderer.frame_all()

    def on_import_rig(self):
        path = filedialog.askopenfilename(
            title="Import Blender Rig or Animation Tracks",
            filetypes=[("Blender Rig & Animations", "*.glb *.gltf *.json"), ("All Files", "*.*")])
        if not path:
            return
        try:
            rig = load_rig_file(path)
            self.custom_rig = rig
            self.log(f"Blender Rig imported: {rig.summary()}")
            for anim in rig.animations:
                clip = AnimationClip(name=anim.name, duration=anim.duration, fps=anim.fps, custom_track=anim)
                self.timeline.add_custom_clip(clip)
            self.model_info.configure(
                text=f"{self.model_info.cget('text')}\n✓ Rig: {rig.summary()}",
                text_color=GREEN)
        except (RigParsingError, UnsupportedRigError, Exception) as exc:
            self.model_info.configure(text=f"Rig Import Error: {exc}", text_color=RED)
            self.log(f"Rig import failed: {exc}")

    def _refit(self):
        self.auto_props, self.fit_report = measure(self.model.vertices(), self.model.faces, self.base, self.model.measure_mask())
        self.props = self.auto_props
        self._checked = set()
        summary = self.fit_report.summary()
        if self.base:
            for line in fit_advice(self.props, self.base):
                summary += f"\n⚠️ {line}"
        self.fit_label.configure(text=summary,
                                 text_color=YELLOW if (self.fit_report.estimated or "⚠️" in summary) else GREEN)
        self._sync_sliders()
        self._geometry_changed()

    def _update_status_icons(self):
        """One small icon per skeleton slider: ✓ found on the mesh, ⚠ guessed or far from the base figure, ✎ yours."""
        issues = {}
        if self.props and self.base:
            for names, text in fit_issues(self.props, self.base):
                for n in names:
                    issues[n] = text
        flagged = self._flagged_sliders()
        for name, icon in self._status_icons.items():
            if not self.props:
                icon.configure(text="")
                self._status_text[name] = ""
            elif name in getattr(self, "_checked", set()):
                icon.configure(text="✎", text_color="#60a5fa")
                self._status_text[name] = "Adjusted by you."
            elif name in flagged:
                icon.configure(text="⚠", text_color=YELLOW)
                self._status_text[name] = "Guessed by the auto-fit, not found on the mesh: check it against the model."
            elif name in issues:
                icon.configure(text="⚠", text_color="#fb923c")
                self._status_text[name] = issues[name]
            else:
                icon.configure(text="✓", text_color=GREEN)
                self._status_text[name] = "Found on the mesh, close to the base figure's proportions."

    def _flagged_sliders(self) -> set:
        """Sliders whose value the auto-fit only guessed, until the user has touched them."""
        flagged = set()
        for item in (self.fit_report.estimated if self.fit_report else []):
            for key, names in ESTIMATE_SLIDERS.items():
                if item.startswith(key):
                    flagged.update(names)
        return flagged - getattr(self, "_checked", set())

    def _sync_sliders(self):
        h = self.props.height
        for name, (slider, label, text, rel) in self.sliders.items():
            lo, hi = next((s[2], s[3]) for s in SKELETON_SLIDERS if s[0] == name)
            if rel:
                slider.configure(from_=lo * h, to=hi * h, state="normal")
            else:
                slider.configure(from_=lo, to=hi, state="normal")
            value = getattr(self.props, name)
            slider.set(value)
            flagged = name in self._flagged_sliders()
            label.configure(text=f"{text}: {value:.2f}" + ("  ⚠ check" if flagged else ""),
                            text_color=YELLOW if flagged else "#e4e4e7")
            self._sync_entry(slider)
        self._update_status_icons()

    def _geometry_changed(self):
        """Placement, joints or base changed: the rig is stale, redraw the T-pose model and its skeleton."""
        showing_rig = self.rig is not None
        self._generation += 1
        self.rig = None
        if hasattr(self, "timeline"):
            self.timeline.set_rig_ready(False)
        self.pose = "tpose"
        self._show_base_rest(True)
        self.pose_buttons.set("T-pose")
        self.pose_buttons.configure(state="disabled")
        self.renderer.set_heat_lines(None)
        self.quality_label.configure(text="Deformation: build the rig to measure it.", text_color=GREY)
        self.anchors = build_anchors(self.props, self.base)
        if showing_rig:
            self.renderer.set_model(self.model.vertices(), self.model.faces, self.model.uvs, self.model.texture)
        else:
            self.renderer.update_model_vertices(self.model.vertices())
        self.renderer.set_skeleton(skeleton_segments(self.anchors), joint_points(self.anchors))
        self._show_base_rest(True)
        self.renderer.request_render()

    def _sync_placement_controls(self):
        if not self.model:
            return
        for key, (slider, label, text, span) in self.place_sliders.items():
            value = float(getattr(self.model.placement, key))
            slider.set(max(-span, min(span, value)))
            label.configure(text=f"{text}: {value:+.2f}")
            self._sync_entry(slider)

    def on_place_slider(self, key, value):
        """Slider moved: apply it at once, but re-measure only once the hand stops (measuring is the slow part)."""
        if not self.model:
            return
        setattr(self.model.placement, key, float(value))
        slider, label, text, _ = self.place_sliders[key]
        label.configure(text=f"{text}: {float(value):+.2f}")
        self._sync_entry(slider)
        self.renderer.update_model_vertices(self.model.vertices())
        self.renderer.request_render()
        if getattr(self, "_place_job", None):
            self.after_cancel(self._place_job)
        self._place_job = self.after(250, self._placement_settled)

    def _placement_settled(self):
        self._place_job = None
        if self.model:
            self._refit()

    def on_nudge(self, key, delta):
        if not self.model:
            return
        placement = self.model.placement
        setattr(placement, key, round(getattr(placement, key) + delta, 4))
        self._sync_placement_controls()
        self._refit()

    def on_recentre(self):
        """Back to the automatic position and straight orientation (keeps the 90° steps and the height)."""
        if not self.model:
            return
        placement = self.model.placement
        placement.offset_x = placement.offset_y = placement.offset_z = 0.0
        placement.yaw_deg = 0.0
        self._sync_placement_controls()
        self._refit()

    def on_turn(self, quarters):
        if self.model:
            self.model.placement.quarter_turns += quarters
            self._refit()

    def on_upright(self):
        if self.model:
            self.model.placement.upright_fix = not self.model.placement.upright_fix
            self._refit()
            self.renderer.frame_all()

    def on_height(self, value):
        self._update_height_label()
        if not self.model:
            return
        ratio = value / self.model.placement.height
        self.model.placement.height = value

        def scale(props):
            return props.adjusted(**{k: (v if k in ("wrist", "knee", "elbow") else v * ratio) for k, v in vars(props).items()})
        self.props, self.auto_props = scale(self.props), scale(self.auto_props)
        self._sync_sliders()
        self._geometry_changed()

    def _show_base_rest(self, tpose: bool):
        """Comparison figure and its skeleton: raised to T-pose when the model is shown in T-pose (and the option is
        on), in the native bind pose otherwise."""
        if not (self.base and getattr(self, "base_parts", None)):
            return
        if tpose and self.ghost_tpose_var.get():
            world, joints = base_tpose(self.base)
            self.renderer.update_ghost_vertices(self.base.deform_preview_parts(world))
        else:
            joints = self.base.joints
            self.renderer.reset_ghost_vertices(self.base_parts)
        if tpose:
            self.renderer.set_base_skeleton(base_skeleton_segments(joints), base_joint_points(joints))

    def on_ghost_tpose(self):
        self.settings.update(ghost_tpose=bool(self.ghost_tpose_var.get()))
        self._show_base_rest(self.pose == "tpose")
        self.renderer.request_render()

    def on_ghost(self):
        self.renderer.show_ghost = self.ghost_var.get()
        self.renderer.request_render()

    # ------------------------------------------------------------ skeleton
    def on_skeleton_toggle(self):
        self.renderer.show_skeleton = self.skel_var.get()
        self.renderer.request_render()

    def on_base_skeleton_toggle(self):
        self.renderer.show_base_skeleton = self.base_skel_var.get()
        self.renderer.request_render()

    def on_overlay_toggle(self):
        self.renderer.overlay_base_skeleton = self.overlay_var.get()
        self.renderer.request_render()

    def on_apply_apose(self):
        if self.model and self.props and self.base:
            self.props = apply_apose_to_proportions(self.props, self.base)
            self._sync_sliders()
            self._geometry_changed()

    def on_apply_tpose(self):
        if self.model and self.props:
            self.props = apply_tpose_to_proportions(self.props)
            self._sync_sliders()
            self._geometry_changed()

    def on_auto_fit(self):
        if self.model:
            self._refit()

    def on_reset_fit(self):
        if self.auto_props:
            self.props = self.auto_props
            self._sync_sliders()
            self._geometry_changed()

    def on_slider(self, name, value):
        if not self.props:
            return
        changes = {name: float(value)}
        if name == "arm_y":
            changes["tip_y"] = self.props.tip_y + (value - self.props.arm_y)
        self.props = self.props.adjusted(**changes)
        slider, label, text, _ = self.sliders[name]
        self._checked = getattr(self, "_checked", set()) | {name}
        label.configure(text=f"{text}: {value:.2f}", text_color="#e4e4e7")
        self._sync_entry(slider)
        self._update_status_icons()
        self._geometry_changed()

    # --------------------------------------------------------- rig & export
    def on_pose(self, choice):
        if not self.rig:
            return
        self.pose = POSES[choice]
        amount = float(self.pose_amount.get())
        self.renderer.update_model_vertices(self.rig.posed(self.pose, amount))
        self._show_base_rest(self.pose == "tpose")
        if self.pose == "tpose":
            self.renderer.set_skeleton(skeleton_segments(self.anchors), joint_points(self.anchors))
        else:
            self.renderer.set_skeleton(None, None)
            self.renderer.set_base_skeleton(None, None)
        self.renderer.set_heat_lines(stretched_segments(self.rig, self.pose, amount))
        self.renderer.request_render()

    def on_pose_amount(self, value):
        self.pose_amount_label.configure(text=f"Pose amount: {int(round(float(value) * 100))}%")
        self._sync_entry(self.pose_amount)
        if not self.rig or self.pose in ("tpose", "bind"):
            return
        if getattr(self, "_amount_job", None):
            self.after_cancel(self._amount_job)
        self._amount_job = self.after(120, lambda: self.on_pose(self.pose_buttons.get()))

    def on_heat_toggle(self):
        self.renderer.show_heat = self.heat_var.get()
        self.renderer.request_render()

    def on_views_toggle(self):
        self.renderer.multi_view = self.views_var.get()
        self.renderer.request_render()

    def _show_quality(self):
        """Measure how much the fresh rig stretches in the test poses and show it (and log it)."""
        try:
            report = deformation_report(self.rig)
        except Exception as exc:  # never block the rig on a diagnostic
            self.quality_label.configure(text=f"Deformation: could not be measured ({exc})", text_color=GREY)
            return
        colour = {"good": GREEN, "warn": YELLOW, "bad": RED}[report.level]
        self.quality_label.configure(text=report.summary(), text_color=colour)
        self.log(report.summary())

    def _run(self, job, done):
        def worker():
            try:
                result = job()
                self._events.put(lambda: done(result, None))
            except Exception as exc:
                detail = traceback.format_exc()
                errorlog.info(detail)
                # Python deletes `exc` when the except block ends: bind it now, or the lambda raises NameError later.
                self._events.put(lambda exc=exc, detail=detail: done(None, (exc, detail)))
        self._set_busy(True)
        threading.Thread(target=worker, daemon=True).start()

    def _failed(self, error):
        self.log(f"FAILED: {error[0]}")
        print(error[1])

    def _ready_to_rig(self) -> bool:
        if not self.model:
            self.log("Import a model first (tab 1).")
            return False
        if self.base is None or not self.base.supported:
            self.log("Pick a base figure with a humanoid skeleton first (tab 0).")
            return False
        return True

    def on_rig(self, then=None):
        if not self._ready_to_rig():
            return
        self._save_settings()
        self.log(f"— Building the rig on {self.base.display_name} —")
        generation = self._generation

        def done(rig, error):
            self._set_busy(False)
            if error:
                self._failed(error)
                return
            if generation != self._generation:
                self.log("The model, skeleton or base changed while the rig was building: build it again.")
                return
            self.rig = rig
            self.timeline.set_rig_ready(True)
            # The rig has its own vertex set (seams welded, UV splits rebuilt): show that mesh from now on.
            self.renderer.set_model(rig.tpose, rig.faces, rig.uvs, self.model.texture)
            self.log(f"Rig ready ({rig.method} weights). Try the “In-game pose”, “Arms up” and “Walk” previews.")
            self.pose_buttons.configure(state="normal")
            self.pose_buttons.set("In-game pose")
            self.on_pose("In-game pose")
            self._show_quality()
            if then:
                then()
        blender = normalize_blender_path(self.blender_entry.get())
        model, anchors, base = self.model, self.anchors, self.base
        overrides = list(self.piece_overrides)
        transfer = self._weight_transfer()
        fingers = bool(self.fingers_var.get())
        self._run(lambda: build_rig(model, anchors, base, Path(blender) if blender else None,
                                    WORK_DIR / "rig", self.log, overrides, transfer, fingers), done)

    def on_install(self):
        if not self._ready_to_rig():
            return
        if self.rig is None:
            self.on_rig(then=self.on_install)
            return
        self._save_settings()
        game_dir = Path(self.game_entry.get().strip())
        try:
            ident = make_identity(self.name_entry.get(), self.actor_entry.get().strip(), self.base, game_dir)
        except ValueError as exc:
            self.log(f"FAILED: {exc}")
            return
        self.log(f"— Installing “{ident.display_name}” (actor {ident.actor}, based on {self.base.display_name}) —")
        ent_choice = self.entity_type_btn.get()
        if ent_choice == "Enemy":
            ent_type = EntityType.ENEMY
            faction = Faction.ENEMIES
        elif ent_choice == "Ally / NPC":
            ent_type = EntityType.ALLY
            try:
                faction = Faction(self.faction_menu.get())
            except (ValueError, AttributeError):
                faction = Faction.TOWNSPERSON
        else:
            ent_type = EntityType.AVATAR
            faction = Faction.HEROES

        try:
            hp = int(self.health_entry.get().strip())
        except (ValueError, AttributeError):
            hp = 100
        try:
            dmg = int(self.damage_entry.get().strip())
        except (ValueError, AttributeError):
            dmg = 25

        entity_cfg = EntityConfig(
            entity_type=ent_type,
            faction=faction,
            max_health=hp,
            base_damage=dmg,
            animations={
                "idle": "Idle",
                "walk": self.anim_walk_menu.get() if hasattr(self, "anim_walk_menu") else "Walk",
                "attack": self.anim_attack_menu.get() if hasattr(self, "anim_attack_menu") else "Attack",
            }
        )

        def done(report, error):
            self._set_busy(False)
            if error:
                self._failed(error)
                return
            self.log("Checks:")
            for passed, msg in report.checks:
                self.log(f"  {'OK    ' if passed else 'FAILED'}  {msg}")
            if report.ok:
                self.log(f"Installed in {report.mod_dir}")
                if entity_cfg.is_enemy:
                    self.log(f"Restart the game, then F5 > Spawners > Enemies > {ident.display_name} (or Modded Characters).")
                elif entity_cfg.is_npc_or_ally:
                    self.log(f"Restart the game, then F5 > Spawners > NPCs > {ident.display_name} (or Modded Characters).")
                else:
                    self.log(f"Restart the game, then F5 > Heroes > ★ Modded Characters > {ident.display_name} (or pick it in the Virtual Reader).")
            else:
                self.log("Some checks failed: the entity may not show up in game.")
        rig = self.rig
        texture = self.model.texture
        abilities = self.abilities
        self._run(lambda: install(rig, texture, ident, game_dir, self.log, entity_cfg, abilities=abilities,
                                    use_legacy_zip=bool(self.zip_var.get())), done)

