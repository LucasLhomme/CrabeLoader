"""Dual-quaternion blending of rigid bone transforms.

Linear blending of rotated copies (what the bake did) averages positions: where the shoulder rotates the arm down
by ~80 degrees, vertices weighted half to the chest and half to the arm collapse towards the joint, which gives the
"square shoulder" and the hollow armpit. Blending the rotations themselves (dual quaternions) keeps the volume
there. Transforms use the row-vector convention of the rig: p' = p @ m + t, m a pure rotation."""
import numpy as np


def _quat_from_matrix(r: np.ndarray) -> np.ndarray:
    """(w, x, y, z) of a column-convention 3x3 rotation matrix."""
    t = np.trace(r)
    if t > 0:
        s = np.sqrt(t + 1.0) * 2
        q = [0.25 * s, (r[2, 1] - r[1, 2]) / s, (r[0, 2] - r[2, 0]) / s, (r[1, 0] - r[0, 1]) / s]
    elif r[0, 0] > r[1, 1] and r[0, 0] > r[2, 2]:
        s = np.sqrt(1.0 + r[0, 0] - r[1, 1] - r[2, 2]) * 2
        q = [(r[2, 1] - r[1, 2]) / s, 0.25 * s, (r[0, 1] + r[1, 0]) / s, (r[0, 2] + r[2, 0]) / s]
    elif r[1, 1] > r[2, 2]:
        s = np.sqrt(1.0 + r[1, 1] - r[0, 0] - r[2, 2]) * 2
        q = [(r[0, 2] - r[2, 0]) / s, (r[0, 1] + r[1, 0]) / s, 0.25 * s, (r[1, 2] + r[2, 1]) / s]
    else:
        s = np.sqrt(1.0 + r[2, 2] - r[0, 0] - r[1, 1]) * 2
        q = [(r[1, 0] - r[0, 1]) / s, (r[0, 2] + r[2, 0]) / s, (r[1, 2] + r[2, 1]) / s, 0.25 * s]
    q = np.array(q)
    return q / np.linalg.norm(q)


def _qmul(a, b):
    """Hamilton product, broadcast over leading axes."""
    aw, ax, ay, az = np.moveaxis(a, -1, 0)
    bw, bx, by, bz = np.moveaxis(b, -1, 0)
    return np.stack([aw * bw - ax * bx - ay * by - az * bz,
                     aw * bx + ax * bw + ay * bz - az * by,
                     aw * by - ax * bz + ay * bw + az * bx,
                     aw * bz + ax * by - ay * bx + az * bw], -1)


def _conj(q):
    return q * np.array([1.0, -1.0, -1.0, -1.0])


def _rotate(q, v):
    """Rotate vectors v (N, 3) by unit quaternions q (N, 4)."""
    p = np.concatenate([np.zeros((len(v), 1)), v], axis=1)
    return _qmul(_qmul(q, p), _conj(q))[:, 1:]


def is_rigid(m: np.ndarray, tol: float = 1e-4) -> bool:
    return bool(np.allclose(m @ m.T, np.eye(3), atol=tol) and np.linalg.det(m) > 0)


def blend(points: np.ndarray, normals: np.ndarray, weights: np.ndarray, transforms: list):
    """Dual-quaternion skinning of `points`/`normals` (N, 3) with `weights` (N, B) over B rigid row-vector
    transforms [(m, t)]. Returns (points, normals)."""
    real = np.array([_quat_from_matrix(m.T) for m, _ in transforms])  # (B, 4)
    trans = np.array([np.r_[0.0, t] for _, t in transforms])
    dual = 0.5 * _qmul(trans, real)  # (B, 4)
    pivot = np.argmax(weights, axis=1)
    sign = np.sign(real @ real.T)[pivot]  # keep every rotation on the dominant bone's hemisphere
    sign[sign == 0] = 1.0
    w = weights * sign
    qr, qd = w @ real, w @ dual
    norm = np.linalg.norm(qr, axis=1, keepdims=True)
    qr, qd = qr / np.maximum(norm, 1e-12), qd / np.maximum(norm, 1e-12)
    translation = 2.0 * _qmul(qd, _conj(qr))[:, 1:]
    return _rotate(qr, points) + translation, _rotate(qr, normals)


def smooth_region(points: np.ndarray, weld: np.ndarray, faces: np.ndarray, strength: np.ndarray, n: int,
                  iterations: int = 8, lam: float = 0.5, mu: float = -0.53) -> np.ndarray:
    """Taubin smoothing (no shrinking) of the vertices where `strength` (0-1 per final vertex) is high.

    `weld` maps each final vertex to its welded position (UV seams split vertices: they must move together) and
    `faces` are the welded triangles."""
    from scipy.sparse import coo_matrix
    pos = np.zeros((n, 3))
    np.add.at(pos, weld, points)
    count = np.bincount(weld, minlength=n)[:, None]
    pos /= np.maximum(count, 1)
    s = np.zeros(n)
    np.maximum.at(s, weld, strength)
    rows = np.concatenate([faces[:, 0], faces[:, 1], faces[:, 2], faces[:, 1], faces[:, 2], faces[:, 0]])
    cols = np.concatenate([faces[:, 1], faces[:, 2], faces[:, 0], faces[:, 0], faces[:, 1], faces[:, 2]])
    adj = coo_matrix((np.ones(len(rows)), (rows, cols)), shape=(n, n)).tocsr()
    adj.data[:] = 1.0
    deg = np.asarray(adj.sum(axis=1)).ravel()
    deg[deg == 0] = 1
    k = s[:, None]
    for _ in range(iterations):
        for f in (lam, mu):
            avg = adj @ pos / deg[:, None]
            pos = pos + f * k * (avg - pos)
    return pos[weld]
