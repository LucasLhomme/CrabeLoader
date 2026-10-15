import numpy as np

from src.core.fingers import _smooth, chain_params


def test_smooth_step_bounds():
    x = np.array([-1.0, 0.0, 2.0])
    assert list(_smooth(x, 0.0, 1.0)) == [0.0, 0.0, 1.0]


def test_chain_params_before_along_after():
    chain = np.array([[0, 0, 0], [0, -1, 0], [0, -2, 0]], float)
    pts = np.array([[0, 0.5, 0], [0, -0.5, 0], [0, -1.5, 0], [0, -3, 0.1]])
    dist, u = chain_params(pts, chain)
    assert np.allclose(u, [-0.5, 0.5, 1.5, 3.0])
    assert dist[1] == 0 and dist[3] > 1
