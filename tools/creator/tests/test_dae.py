import numpy as np

from src.core.dae import load

DAE = """<?xml version="1.0"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1">
  <asset><unit name="centimeter" meter="0.01"/><up_axis>Z_UP</up_axis></asset>
  <library_geometries><geometry id="g"><mesh>
    <source id="pos"><float_array id="pa" count="12">0 0 0 100 0 0 100 0 200 0 0 200</float_array>
      <technique_common><accessor source="#pa" count="4" stride="3"/></technique_common></source>
    <source id="uv"><float_array id="ua" count="8">0 0 1 0 1 1 0 1</float_array>
      <technique_common><accessor source="#ua" count="4" stride="2"/></technique_common></source>
    <vertices id="vtx"><input semantic="POSITION" source="#pos"/></vertices>
    <polylist material="m" count="1"><input semantic="VERTEX" source="#vtx" offset="0"/>
      <input semantic="TEXCOORD" source="#uv" offset="1" set="0"/>
      <vcount>4</vcount><p>0 0 1 1 2 2 3 3</p></polylist>
  </mesh></geometry></library_geometries>
  <library_visual_scenes><visual_scene id="s"><node><translate>0 0 0</translate>
    <instance_geometry url="#g"/></node></visual_scene></library_visual_scenes>
</COLLADA>"""


def test_quad_polylist_units_and_up_axis(tmp_path):
    f = tmp_path / "quad.dae"
    f.write_text(DAE)
    (mesh,) = load(f)
    assert len(mesh.faces) == 2 and len(mesh.vertices) == 4
    v = np.asarray(mesh.vertices)
    # Z-up centimetres -> Y-up metres: the 200 cm height ends up on Y as 2 m
    assert np.isclose(v[:, 1].max(), 2.0) and np.isclose(v[:, 0].max(), 1.0)
