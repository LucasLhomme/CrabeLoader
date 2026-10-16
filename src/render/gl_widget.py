"""Offscreen OpenGL viewport shown in a Tk label: textured model, comparison model, skeleton, floor grid."""
import customtkinter as ctk
import moderngl
import numpy as np
from PIL import Image

from src.core.rig import vertex_normals

VERTEX_SHADER = """
#version 330
in vec3 in_position;
in vec3 in_normal;
in vec2 in_texcoord;
uniform mat4 mvp;
uniform vec3 offset;
out vec3 v_normal;
out vec2 v_texcoord;
void main() {
    gl_Position = mvp * vec4(in_position + offset, 1.0);
    v_normal = in_normal;
    v_texcoord = in_texcoord;
}
"""

FRAGMENT_SHADER = """
#version 330
in vec3 v_normal;
in vec2 v_texcoord;
out vec4 f_color;
uniform sampler2D texture0;
uniform bool use_texture;
uniform bool flat_color;
uniform vec4 color;
void main() {
    if (flat_color) { f_color = color; return; }
    vec3 n = normalize(v_normal);
    float lum = abs(dot(n, normalize(vec3(0.4, 0.8, 0.6)))) * 0.75 + 0.35;
    vec4 base = use_texture ? texture(texture0, v_texcoord) : vec4(0.75, 0.75, 0.78, 1.0);
    f_color = vec4(base.rgb * lum * color.rgb, color.a);
}
"""


def perspective(fov, aspect, near, far):
    f = 1.0 / np.tan(np.radians(fov) / 2.0)
    m = np.zeros((4, 4), dtype=np.float32)
    m[0, 0], m[1, 1] = f / aspect, f
    m[2, 2] = (far + near) / (near - far)
    m[2, 3] = -1.0
    m[3, 2] = (2.0 * far * near) / (near - far)
    return m


def look_at(eye, target, up):
    f = target - eye
    f = f / np.linalg.norm(f)
    s = np.cross(f, up)
    s = s / np.linalg.norm(s)
    u = np.cross(s, f)
    m = np.eye(4, dtype=np.float32)
    m[0, :3], m[1, :3], m[2, :3] = s, u, -f
    m[:3, 3] = [-np.dot(s, eye), -np.dot(u, eye), np.dot(f, eye)]
    return m.T


class _Mesh:
    def __init__(self, ctx, prog, verts, faces, uvs, texture: Image.Image | None):
        self.ctx, self.prog = ctx, prog
        self.faces = np.asarray(faces, np.int32)
        self.uvs = np.asarray(uvs, np.float32)
        self.vbo = ctx.buffer(reserve=len(verts) * 32)
        self.ibo = ctx.buffer(self.faces.tobytes())
        self.vao = ctx.vertex_array(prog, [(self.vbo, "3f 3f 2f", "in_position", "in_normal", "in_texcoord")],
                                    index_buffer=self.ibo)
        self.texture = None
        if texture is not None:
            img = texture.convert("RGBA")
            self.texture = ctx.texture(img.size, 4, img.tobytes())
            self.texture.build_mipmaps()
        self.update(verts)

    def update(self, verts):
        verts = np.asarray(verts, np.float32)
        self.verts = verts
        self.bounds = (verts.min(0), verts.max(0))
        normals = vertex_normals(verts.astype(float), self.faces).astype(np.float32)
        self.vbo.write(np.hstack([verts, normals, self.uvs]).astype("f4").tobytes())

    def draw(self, offset=(0, 0, 0), color=(1, 1, 1, 1)):
        self.prog["offset"].value = tuple(float(v) for v in offset)
        self.prog["flat_color"].value = False
        self.prog["color"].value = color
        self.prog["use_texture"].value = self.texture is not None
        if self.texture is not None:
            self.texture.use(0)
            self.prog["texture0"].value = 0
        self.vao.render(moderngl.TRIANGLES)

    def release(self):
        for obj in (self.vao, self.vbo, self.ibo, self.texture):
            if obj is not None:
                obj.release()


class GLRenderer:
    def __init__(self, label: ctk.CTkLabel):
        self.label = label
        self.ctx = moderngl.create_context(standalone=True)
        self.prog = self.ctx.program(vertex_shader=VERTEX_SHADER, fragment_shader=FRAGMENT_SHADER)
        self.width, self.height = 800, 600
        self.fbo = None
        self.model: _Mesh | None = None
        self.ghost: list[_Mesh] = []
        self.show_ghost = False
        self.lines = np.zeros((0, 3), np.float32)
        self.points = np.zeros((0, 3), np.float32)
        self.show_skeleton = True
        self.base_lines = np.zeros((0, 3), np.float32)
        self.base_points = np.zeros((0, 3), np.float32)
        self.show_base_skeleton = True
        self.overlay_base_skeleton = False
        self.heat_lines = np.zeros((0, 3), np.float32)
        self.select_lines = np.zeros((0, 3), np.float32)
        self.show_heat = True
        self.multi_view = False
        self._bounds = None
        self.yaw, self.pitch, self.distance = 25.0, 10.0, 4.0
        self.target = np.array([0.0, 1.0, 0.0], np.float32)
        self._last = (0, 0)
        self._pending = False
        label.bind("<ButtonPress-1>", self._press)
        label.bind("<ButtonPress-3>", self._press)
        label.bind("<B1-Motion>", self._orbit)
        label.bind("<B3-Motion>", self._pan)
        label.bind("<MouseWheel>", self._zoom)
        label.bind("<Double-Button-1>", lambda e: self.frame_all())
        self._make_fbo()

    # --- scene content -------------------------------------------------------
    def set_model(self, verts, faces, uvs, texture):
        if self.model:
            self.model.release()
        self.model = _Mesh(self.ctx, self.prog, verts, faces, uvs, texture) if verts is not None else None

    def update_model_vertices(self, verts):
        if self.model:
            self.model.update(verts)

    def set_ghost(self, parts):
        """The base figure, as (verts, faces, uvs, texture) parts (one per primitive)."""
        for mesh in self.ghost:
            mesh.release()
        self.ghost = [_Mesh(self.ctx, self.prog, v, f, uv, tex) for v, f, uv, tex in parts]

    def update_ghost_vertices(self, list_of_verts):
        for mesh, verts in zip(self.ghost, list_of_verts):
            mesh.update(verts)

    def reset_ghost_vertices(self, original_parts):
        for mesh, part in zip(self.ghost, original_parts):
            mesh.update(part[0])

    def _ghost_bounds(self):
        return (np.min([m.bounds[0] for m in self.ghost], axis=0), np.max([m.bounds[1] for m in self.ghost], axis=0))

    def set_skeleton(self, lines, points):
        self.lines = np.zeros((0, 3), np.float32) if lines is None else np.asarray(lines, np.float32)
        self.points = np.zeros((0, 3), np.float32) if points is None else np.asarray(points, np.float32)

    def set_base_skeleton(self, lines, points):
        self.base_lines = np.zeros((0, 3), np.float32) if lines is None else np.asarray(lines, np.float32)
        self.base_points = np.zeros((0, 3), np.float32) if points is None else np.asarray(points, np.float32)

    def set_heat_lines(self, lines):
        """Edges to draw in red on top of the model (the stretched ones), in posed positions."""
        self.heat_lines = np.zeros((0, 3), np.float32) if lines is None else np.asarray(lines, np.float32)

    def set_select_lines(self, lines):
        """Edges of the piece picked by the user, drawn in magenta."""
        self.select_lines = np.zeros((0, 3), np.float32) if lines is None else np.asarray(lines, np.float32)

    def pick_vertex(self, x, y, radius=14.0):
        """Index of the model vertex under the pixel (x, y) of the single-view camera, or None.

        Projects every vertex to the screen; among those within `radius` pixels, the one nearest the camera wins
        (so the front of the body is picked rather than what is behind it)."""
        if self.model is None or self.multi_view:
            return None
        verts = self.model.verts.astype(np.float64)
        proj = perspective(40.0, self.width / self.height, 0.02, 200.0)
        view = look_at(self._eye(), self.target, np.array([0, 1, 0], np.float32))
        clip = np.c_[verts, np.ones(len(verts))] @ (view @ proj).astype(np.float64)
        w = clip[:, 3]
        ahead = w > 1e-6
        sx = (clip[:, 0] / np.where(ahead, w, 1) * 0.5 + 0.5) * self.width
        sy = (1 - (clip[:, 1] / np.where(ahead, w, 1) * 0.5 + 0.5)) * self.height
        scale = ctk.ScalingTracker.get_widget_scaling(self.label)
        near = ahead & (np.hypot(sx - x * scale, sy - y * scale) <= radius * scale)
        if not near.any():
            return None
        candidates = np.nonzero(near)[0]
        return int(candidates[np.argmin(w[candidates])])

    def ghost_offset(self):
        if not (self.ghost and self.model):
            return np.zeros(3)
        glo, ghi = self._ghost_bounds()
        gap = (self.model.bounds[1][0] - self.model.bounds[0][0]) / 2 + (ghi[0] - glo[0]) / 2 + 0.25
        return np.array([-gap, 0.0, 0.0])

    def frame_all(self):
        if self.model is None and not self.ghost:
            return
        lo, hi = self.model.bounds if self.model else self._ghost_bounds()
        if self.show_ghost and self.model and self.ghost:
            off = self.ghost_offset()
            glo, ghi = self._ghost_bounds()
            lo = np.minimum(lo, glo + off)
            hi = np.maximum(hi, ghi + off)
        self.target = ((lo + hi) / 2).astype(np.float32)
        self.distance = float(max(np.linalg.norm(hi - lo), 0.5) * 1.6)
        self._bounds = (np.asarray(lo, float), np.asarray(hi, float))
        self.render()

    # --- camera --------------------------------------------------------------
    def _press(self, e):
        self._last = (e.x, e.y)

    def _orbit(self, e):
        dx, dy = e.x - self._last[0], e.y - self._last[1]
        self._last = (e.x, e.y)
        self.yaw -= dx * 0.5
        self.pitch = float(np.clip(self.pitch + dy * 0.5, -89, 89))
        self.request_render()

    def _pan(self, e):
        dx, dy = e.x - self._last[0], e.y - self._last[1]
        self._last = (e.x, e.y)
        eye = self._eye()
        f = self.target - eye
        f /= np.linalg.norm(f)
        right = np.cross(f, [0, 1, 0])
        right /= np.linalg.norm(right)
        up = np.cross(right, f)
        k = self.distance * 0.0015
        self.target = (self.target - right * dx * k + up * dy * k).astype(np.float32)
        self.request_render()

    def _zoom(self, e):
        self.distance = float(np.clip(self.distance * (0.9 if e.delta > 0 else 1.1), 0.2, 50))
        self.request_render()

    def _eye(self):
        p, y = np.radians(self.pitch), np.radians(self.yaw)
        return self.target + self.distance * np.array([np.cos(p) * np.sin(y), np.sin(p), np.cos(p) * np.cos(y)], np.float32)

    # --- drawing -------------------------------------------------------------
    def resize(self, w, h):
        if w > 10 and h > 10 and (w, h) != (self.width, self.height):
            self.width, self.height = w, h
            self._make_fbo()
            self.request_render()

    def _make_fbo(self):
        if self.fbo:
            self.fbo.release()
        self.fbo = self.ctx.simple_framebuffer((self.width, self.height), components=3)

    def request_render(self):
        """Coalesce bursts of mouse events into one frame."""
        if not self._pending:
            self._pending = True
            self.label.after(15, self.render)

    def _draw_lines(self, pts, color, mode=moderngl.LINES, offset=(0.0, 0.0, 0.0)):
        if len(pts) == 0:
            return
        data = np.hstack([pts, np.zeros_like(pts), np.zeros((len(pts), 2), np.float32)]).astype("f4")
        vbo = self.ctx.buffer(data.tobytes())
        vao = self.ctx.vertex_array(self.prog, [(vbo, "3f 3f 2f", "in_position", "in_normal", "in_texcoord")])
        self.prog["offset"].value = tuple(float(v) for v in offset)
        self.prog["flat_color"].value = True
        self.prog["color"].value = color
        vao.render(mode)
        vao.release()
        vbo.release()

    def _grid(self):
        n, step = 10, 0.25
        r = n * step
        pts = []
        for i in range(-n, n + 1):
            pts += [(i * step, 0, -r), (i * step, 0, r), (-r, 0, i * step), (r, 0, i * step)]
        return np.array(pts, np.float32)

    # Fixed cameras of the "3 views" layout: (yaw, pitch, title).
    VIEWS = ((0.0, 0.0, "front"), (90.0, 0.0, "side"), (0.0, 89.0, "top"))

    def _view_distance(self, index, aspect, fallback):
        """Camera distance that fits the whole scene in one narrow cell of the 3-views layout."""
        if self._bounds is None:
            return fallback
        span = self._bounds[1] - self._bounds[0]
        horizontal, vertical, depth = ((span[0], span[1], span[2]), (span[2], span[1], span[0]),
                                       (span[0], span[2], span[1]))[index]
        half_fov = np.tan(np.radians(20.0))
        return float(max(horizontal / aspect, vertical) * 0.5 / half_fov * 1.15 + depth * 0.5)

    def render(self):
        self._pending = False
        image = self.snapshot()
        scale = ctk.ScalingTracker.get_widget_scaling(self.label)
        ctk_image = ctk.CTkImage(light_image=image, dark_image=image,
                                 size=(int(self.width / scale), int(self.height / scale)))
        self.label.configure(image=ctk_image, text="")
        self.label.image = ctk_image

    def snapshot(self) -> Image.Image:
        """Draw the scene (one view or the 3-views layout) and return it as an image; no window needed."""
        self.fbo.use()
        self.ctx.viewport = (0, 0, self.width, self.height)
        self.ctx.clear(0.075, 0.075, 0.085)
        if not self.multi_view:
            self._draw_scene(self._eye(), self.target, self.width / self.height)
        else:
            cell = self.width // 3
            saved = (self.yaw, self.pitch)
            saved_distance = self.distance
            for i, (yaw, pitch, _) in enumerate(self.VIEWS):
                self.yaw, self.pitch = yaw, pitch
                self.distance = self._view_distance(i, cell / self.height, saved_distance)
                self.ctx.viewport = (i * cell, 0, cell, self.height)
                self.ctx.scissor = (i * cell, 0, cell, self.height)
                self.ctx.clear(0.075 + 0.012 * (i % 2), 0.075 + 0.012 * (i % 2), 0.085 + 0.012 * (i % 2),
                               viewport=(i * cell, 0, cell, self.height))
                self._draw_scene(self._eye(), self.target, cell / self.height)
            self.yaw, self.pitch = saved
            self.distance = saved_distance
            self.ctx.scissor = None
            self.ctx.viewport = (0, 0, self.width, self.height)

        data = self.fbo.read(components=3, alignment=1)
        return Image.frombytes("RGB", (self.width, self.height), data).transpose(Image.FLIP_TOP_BOTTOM)

    def render_icon(self, center, radius: float, size: int = 256, yaw: float = 0.0) -> Image.Image:
        """The model alone, seen from the front and framed on a sphere (the head), on a transparent background."""
        fbo = self.ctx.simple_framebuffer((size, size), components=4)
        fbo.use()
        self.ctx.viewport = (0, 0, size, size)
        self.ctx.clear(0.0, 0.0, 0.0, 0.0)
        self.ctx.enable(moderngl.DEPTH_TEST)
        self.ctx.enable(moderngl.BLEND)
        self.ctx.blend_func = moderngl.SRC_ALPHA, moderngl.ONE_MINUS_SRC_ALPHA
        fov = 40.0
        distance = radius / np.tan(np.radians(fov / 2))
        target = np.asarray(center, np.float32)
        y = np.radians(yaw)
        eye = target + distance * np.array([np.sin(y), 0.0, np.cos(y)], np.float32)
        view = look_at(eye, target, np.array([0, 1, 0], np.float32))
        self.prog["mvp"].write((view @ perspective(fov, 1.0, 0.02, 200.0)).astype("f4").tobytes())
        if self.model:
            self.model.draw()
        data = fbo.read(components=4, alignment=1)
        fbo.release()
        self.fbo.use()
        return Image.frombytes("RGBA", (size, size), data).transpose(Image.FLIP_TOP_BOTTOM)

    def _draw_scene(self, eye, target, aspect):
        self.ctx.enable(moderngl.DEPTH_TEST)
        self.ctx.enable(moderngl.BLEND)
        self.ctx.blend_func = moderngl.SRC_ALPHA, moderngl.ONE_MINUS_SRC_ALPHA
        proj = perspective(40.0, aspect, 0.02, 200.0)
        view = look_at(eye, target, np.array([0, 1, 0], np.float32))
        self.prog["mvp"].write((view @ proj).astype("f4").tobytes())

        self._draw_lines(self._grid(), (0.3, 0.3, 0.34, 1.0))
        if self.model:
            self.model.draw()
            if self.show_ghost and self.ghost:
                off = self.ghost_offset()
                for mesh in self.ghost:
                    mesh.draw(offset=off, color=(0.8, 0.85, 1.0, 1.0))
        else:
            for mesh in self.ghost:
                mesh.draw()

        # Draw base character reference skeleton (cyan lines & points) on the ghost base character
        if self.show_base_skeleton and len(self.base_lines) > 0:
            base_off = self.ghost_offset() if (self.model and self.show_ghost and self.ghost and not self.overlay_base_skeleton) else (0.0, 0.0, 0.0)
            self.ctx.disable(moderngl.DEPTH_TEST)
            self._draw_lines(self.base_lines, (0.05, 0.75, 1.0, 0.85), offset=base_off)
            self.ctx.point_size = 8.0
            self._draw_lines(self.base_points, (0.2, 0.9, 1.0, 1.0), moderngl.POINTS, offset=base_off)

        # Draw custom model fitted skeleton (green lines, yellow points) on the custom model (at origin)
        if self.show_skeleton and self.model and len(self.lines) > 0:
            self.ctx.disable(moderngl.DEPTH_TEST)
            self._draw_lines(self.lines, (0.2, 1.0, 0.35, 1.0), offset=(0.0, 0.0, 0.0))
            self.ctx.point_size = 7.0
            self._draw_lines(self.points, (1.0, 0.85, 0.2, 1.0), moderngl.POINTS, offset=(0.0, 0.0, 0.0))

        if len(self.select_lines) > 0:
            self.ctx.disable(moderngl.DEPTH_TEST)
            self._draw_lines(self.select_lines, (1.0, 0.2, 0.9, 1.0))

        # Stretched edges of the current pose, in red, always on top.
        if self.show_heat and len(self.heat_lines) > 0:
            self.ctx.disable(moderngl.DEPTH_TEST)
            self._draw_lines(self.heat_lines, (1.0, 0.15, 0.1, 1.0))
