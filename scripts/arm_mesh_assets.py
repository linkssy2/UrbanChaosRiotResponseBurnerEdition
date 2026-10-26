"""Read-only arm mesh oracle using the original A6220 stream/palette layout."""
import math
import struct

from native_scene_checks.inspect_scene import Reader
from coop_injection_assets import model_rig, pose_frame, _multiply


def read_mesh(data):
    reader = Reader(data)
    reader.model()
    assert reader.pos == len(data) and len(reader.nodes) == 1
    node = reader.nodes[0]
    assert node['flags'] == 17
    count = node['vertices']
    position, _, _, uv, uv1 = node['streams']
    at = uv1 + count * 8
    draws = [struct.unpack_from('<IHHHHHB', data, at + i*20) for i in range(node['draws'])]
    at += (node['draws'] + node['arrays']) * 20
    indices = struct.unpack_from('<' + 'H'*node['indices'], data, at)
    at += node['indices'] * 2
    for _ in range(node['extra']):
        at += 56 + struct.unpack_from('<H', data, at + 50)[0] * 4
    updates = []
    for _ in draws:
        n = struct.unpack_from('<I', data, at)[0]
        at += 8
        updates.append([struct.unpack_from('<HbB', data, at + i*4) for i in range(n)])
        at += n * 4
    skin = at
    positions = [struct.unpack_from('<hhh', data, position+i*10) for i in range(count)]
    weights = [struct.unpack_from('<hhhBB', data, skin+i*8) for i in range(count)]
    uv_values = [struct.unpack_from('<2f', data, uv+i*8) for i in range(count)]
    palette, triangles, bones = {}, [], [None]*count
    for draw, changes in zip(draws, updates):
        for bone, row, _ in changes:
            palette[row] = bone
        first, material, _, _, _, primitives, topology = draw
        for i in range(primitives):
            start = first + i*(3 if topology == 4 else 1)
            flip = i & 1 if topology == 5 else 0
            triangle = (indices[start+flip], indices[start+1-flip], indices[start+2])
            triangles.append((triangle, material))
            for vertex in triangle:
                resolved = tuple(palette.get(row) for row in weights[vertex][:3])
                w = (*weights[vertex][3:], 255-sum(weights[vertex][3:]))
                assert all(b is not None or not v for b, v in zip(resolved, w))
                if bones[vertex] is not None:
                    assert all(not v or x == y for v, x, y in zip(w, bones[vertex], resolved))
                bones[vertex] = resolved
    ring = [i for i, p in enumerate(positions) if 15980 <= abs(p[0]) <= 16000]
    return dict(data=data, node=node, position_at=position, uv_at=uv, skin_at=skin,
                positions=positions, uv=uv_values, weights=weights, bones=bones,
                triangles=triangles, ring=ring)


def corrected_v(mesh, vertex, shoulder):
    """Continue the original texel spacing, mirrored at the last forearm ring.

    Skin atlas continuation runs back into the same skin island; sleeves already
    run in that direction. No sampler change, texture resize or glove UV change.
    """
    p = mesh['positions'][vertex]
    neighbors = {j for tri, _ in mesh['triangles'] if vertex in tri for j in tri
                 if j not in mesh['ring'] and (mesh['positions'][j][0] < 0) == (p[0] < 0)}
    neighbor = min(neighbors, key=lambda j: sum((mesh['positions'][j][k]-p[k])**2 for k in (1, 2)))
    dx = abs(mesh['positions'][neighbor][0])-abs(p[0])
    assert dx > 0
    v = mesh['uv'][neighbor][1]
    result = v - abs(mesh['uv'][vertex][1]-v)*(abs(mesh['positions'][neighbor][0])-shoulder)/dx
    assert 0 <= result <= 1
    return result, neighbor


def posed(mesh, clip, frame):
    world, _ = pose_frame(clip, model_rig(mesh['data']), frame)
    scale = struct.unpack_from('<f', mesh['data'], 68)[0]
    scaling = [[scale if r == c and r < 3 else 1. if r == c else 0. for c in range(4)] for r in range(4)]
    palette = []
    for bone, pose in enumerate(world):
        values = struct.unpack_from('<16f', mesh['data'], 76+len(world)*64+bone*160+96)
        bind = [list(values[r*4:r*4+4]) for r in range(4)]
        palette.append(_multiply(_multiply(scaling, bind), pose))
    result = []
    for position, weight, bones in zip(mesh['positions'], mesh['weights'], mesh['bones']):
        p = [max(-1., x/32767.) for x in position] + [1.]
        weights = [weight[3]/255., weight[4]/255., (255-weight[3]-weight[4])/255.]
        out = [0., 0., 0.]
        for b, w in zip(bones or (0, 0, 0), weights):
            if w:
                for c in range(3):
                    out[c] += w * sum(p[r]*palette[b][r][c] for r in range(4))
        result.append(out)
    return result, world
