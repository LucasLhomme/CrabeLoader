"""Timeline and animation playback control widget."""
from __future__ import annotations

from typing import Callable

import customtkinter as ctk

from src.core.animation_player import AnimationPlayer, BUILTIN_CLIPS, AnimationClip


class TimelineWidget(ctk.CTkFrame):
    def __init__(self, master, player: AnimationPlayer, on_frame_changed: Callable[[], None], **kwargs):
        super().__init__(master, fg_color="#121316", corner_radius=6, **kwargs)
        self.player = player
        self.on_frame_changed = on_frame_changed
        self._scrubbing = False
        self.clips: dict[str, AnimationClip] = dict(BUILTIN_CLIPS)

        self.grid_columnconfigure(3, weight=1)

        # 1. Animation choice
        self.clip_menu = ctk.CTkOptionMenu(
            self,
            values=list(self.clips.keys()),
            command=self._on_clip_selected,
            width=210,
            height=26,
            fg_color="#27272a",
            button_color="#3f3f46",
            text_color="#e4e4e7",
            font=ctk.CTkFont(size=11),
        )
        self.clip_menu.set(list(self.clips.keys())[0] if self.clips else "Roll / Dive Roll (Rouler)")
        self.clip_menu.grid(row=0, column=0, padx=(8, 4), pady=4)

        # 2. Play / Pause button
        self.play_btn = ctk.CTkButton(
            self,
            text="▶",
            width=32,
            height=26,
            fg_color="#3b82f6",
            hover_color="#2563eb",
            font=ctk.CTkFont(size=12, weight="bold"),
            command=self._toggle_play,
        )
        self.play_btn.grid(row=0, column=1, padx=2, pady=4)

        # 3. Stop button
        self.stop_btn = ctk.CTkButton(
            self,
            text="⏹",
            width=32,
            height=26,
            fg_color="#27272a",
            hover_color="#3f3f46",
            font=ctk.CTkFont(size=12),
            command=self._stop,
        )
        self.stop_btn.grid(row=0, column=2, padx=(2, 6), pady=4)

        # 4. Scrubber slider
        self.slider = ctk.CTkSlider(
            self,
            from_=0,
            to=max(1, self.player.total_frames - 1),
            number_of_steps=max(1, self.player.total_frames - 1),
            height=14,
            progress_color="#3b82f6",
            button_color="#60a5fa",
            button_hover_color="#93c5fd",
            command=self._on_slider_moved,
        )
        self.slider.set(0)
        self.slider.grid(row=0, column=3, sticky="ew", padx=6, pady=4)
        self.slider.bind("<ButtonPress-1>", lambda e: setattr(self, "_scrubbing", True))
        self.slider.bind("<ButtonRelease-1>", lambda e: setattr(self, "_scrubbing", False))

        # 5. Frame & time label
        self.time_label = ctk.CTkLabel(
            self,
            text="0 / 60 (0.00s)",
            font=ctk.CTkFont(family="Consolas", size=11),
            text_color="#a1a1aa",
            width=115,
            anchor="e",
        )
        self.time_label.grid(row=0, column=4, padx=(4, 6), pady=4)

        # 6. Loop checkbox
        self.loop_var = ctk.BooleanVar(value=True)
        self.loop_check = ctk.CTkCheckBox(
            self,
            text="Loop",
            variable=self.loop_var,
            width=50,
            height=20,
            font=ctk.CTkFont(size=11),
            command=self._on_loop_toggle,
        )
        self.loop_check.grid(row=0, column=5, padx=(2, 6), pady=4)

        # 7. Sync Ghost comparison checkbox
        self.compare_var = ctk.BooleanVar(value=True)
        self.compare_check = ctk.CTkCheckBox(
            self,
            text="Sync Ghost",
            variable=self.compare_var,
            width=80,
            height=20,
            font=ctk.CTkFont(size=11),
            text_color="#38bdf8",
        )
        self.compare_check.grid(row=0, column=6, padx=(2, 6), pady=4)

        # 8. Rig status badge / hint
        self.status_label = ctk.CTkLabel(
            self,
            text="⚠️ Build rig (Tab 4)",
            font=ctk.CTkFont(size=11),
            text_color="#fbbf24",
            width=115,
            anchor="w",
        )
        self.status_label.grid(row=0, column=7, padx=(2, 8), pady=4)

        self.rig_ready = False
        self._update_label()
        self.set_rig_ready(False)

    @property
    def sync_compare(self) -> bool:
        return self.compare_var.get()

    def set_rig_ready(self, ready: bool):
        """Enable or disable timeline playback based on whether the rig has been built."""
        self.rig_ready = ready
        if ready:
            self.play_btn.configure(state="normal", fg_color="#3b82f6" if not self.player.is_playing else "#f59e0b")
            self.stop_btn.configure(state="normal")
            self.slider.configure(state="normal")
            self.status_label.configure(text="✓ Rig built (Active)", text_color="#4ade80")
        else:
            if self.player.is_playing:
                self._stop()
            self.play_btn.configure(state="disabled", fg_color="#27272a")
            self.stop_btn.configure(state="disabled")
            self.slider.configure(state="disabled")
            self.status_label.configure(text="⚠️ Build rig (Tab 4)", text_color="#fbbf24")

    def _toggle_play(self):
        if not self.rig_ready:
            return
        self.player.toggle_play()
        self.play_btn.configure(text="⏸" if self.player.is_playing else "▶")
        if self.player.is_playing:
            self.play_btn.configure(fg_color="#f59e0b", hover_color="#d97706")
        else:
            self.play_btn.configure(fg_color="#3b82f6", hover_color="#2563eb")

    def _stop(self):
        self.player.stop()
        self.play_btn.configure(text="▶", fg_color="#3b82f6", hover_color="#2563eb")
        self.slider.set(0)
        self._update_label()
        self.on_frame_changed()

    def _on_slider_moved(self, value):
        frame = int(round(value))
        self.player.seek_frame(frame)
        self._update_label()
        self.on_frame_changed()

    def _on_loop_toggle(self):
        self.player.loop = self.loop_var.get()

    def _on_clip_selected(self, clip_name: str):
        if clip_name in self.clips:
            self.player.set_clip(self.clips[clip_name])
            total = max(1, self.player.total_frames - 1)
            self.slider.configure(to=total, number_of_steps=total)
            self.slider.set(0)
            self._update_label()
            self.on_frame_changed()

    def set_clips(self, clips: dict[str, AnimationClip]):
        """Replace active clips with the character-specific moveset."""
        self.clips = dict(clips)
        vals = list(self.clips.keys())
        self.clip_menu.configure(values=vals)
        if vals:
            first_clip = vals[0]
            self.clip_menu.set(first_clip)
            self.player.set_clip(self.clips[first_clip])
            total = max(1, self.player.total_frames - 1)
            self.slider.configure(to=total, number_of_steps=total)
            self.slider.set(0)
            self._update_label()
            self.on_frame_changed()

    def add_custom_clip(self, clip: AnimationClip):
        self.clips[clip.name] = clip
        current_vals = list(self.clips.keys())
        self.clip_menu.configure(values=current_vals)
        self.clip_menu.set(clip.name)
        self.player.set_clip(clip)
        total = max(1, self.player.total_frames - 1)
        self.slider.configure(to=total, number_of_steps=total)
        self.slider.set(0)
        self._update_label()
        self.on_frame_changed()

    def update_playback_state(self):
        """Called by the main tick loop when playing."""
        if not self._scrubbing:
            self.slider.set(self.player.current_frame)
        self._update_label()
        if not self.player.is_playing:
            self.play_btn.configure(text="▶", fg_color="#3b82f6", hover_color="#2563eb")

    def _update_label(self):
        cur_f = self.player.current_frame
        tot_f = self.player.total_frames
        cur_t = self.player.current_time
        self.time_label.configure(text=f"{cur_f:2d}/{tot_f:2d} ({cur_t:.2f}s)")

