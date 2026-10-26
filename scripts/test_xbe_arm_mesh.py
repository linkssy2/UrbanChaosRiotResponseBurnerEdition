"""Exact atomic arm edits, native joint ownership and original animation poses.

No game launch; source archives remain unchanged. The optional visual exporter
uses these same corrected streams, not a different model implementation.
"""
from pathlib import Path
import hashlib
import math
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_frontend import parse_archive, unpack_entry
from arm_mesh_assets import read_mesh, corrected_v, posed
from coop_injection_assets import ARMS, decode_clip


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def bend_alignment(points, world, mesh, sign):
    ring = [i for i in mesh['ring'] if (mesh['positions'][i][0] < 0) == (sign < 0)]
    center = [sum(points[i][k] for i in ring)/len(ring) for k in range(3)]
    upper, elbow = (32, 33) if sign < 0 else (10, 11)
    actual = [center[k]-world[elbow][3][k] for k in range(3)]
    expected = [world[upper][3][k]-world[elbow][3][k] for k in range(3)]
    return dot(actual, expected)/math.sqrt(dot(actual, actual)*dot(expected, expected))


def main():
    exe, game = map(Path, sys.argv[1:])
    sources, meshes, clips = {}, [], []
    for archive, terms in [('2-01.ent', ('eqp_ladder_anim_climb_up.kfa', 'eqp_ladder_anim_exit_ladder_top.kfa')),
                           ('1-01.ent', ('eqp_shield_anim_melee.kfa', 'eqp_shield_anim_idle.kfa'))]:
        path = game/archive
        raw, entries = parse_archive(path)
        sources[path] = hashlib.sha256(raw).digest()
        for term in terms:
            entry = next(e for e in entries if e['name'].endswith(term))
            clips.append((term, decode_clip(unpack_entry(raw, entry))))
    with tempfile.TemporaryDirectory(prefix='burner-arm-mesh-') as temporary:
        fixtures = []
        for archive, name, digest in ARMS:
            path = game/archive
            raw, entries = parse_archive(path)
            sources[path] = hashlib.sha256(raw).digest()
            payload = unpack_entry(raw, next(e for e in entries if e['name'] == name))
            assert hashlib.sha256(payload).hexdigest() == digest
            mesh = read_mesh(payload)
            count = mesh['node']['vertices']
            fixture = Path(temporary)/(archive + '.positions')
            for suffix, at, stride in [('.positions', mesh['position_at'], 10), ('.uv', mesh['uv_at'], 8), ('.skin', mesh['skin_at'], 8)]:
                fixture.with_suffix(suffix).write_bytes(payload[at:at+count*stride])
            fixtures.append(fixture)
            meshes.append(mesh)
        subprocess.run([str(exe.resolve()), *map(str, fixtures)], check=True)
        cases = failed_old_bend = 0
        for fixture, original in zip(fixtures, meshes):
            payload = bytearray(original['data'])
            count = original['node']['vertices']
            for suffix, at, stride in [('.positions', original['position_at'], 10), ('.uv', original['uv_at'], 8), ('.skin', original['skin_at'], 8)]:
                payload[at:at+count*stride] = fixture.with_suffix('.patched'+suffix).read_bytes()
            changed = read_mesh(bytes(payload))
            shoulder = 8000 if count == 3103 else 5000
            for vertex in original['ring']:
                assert original['bones'][vertex][1] in (10, 32)
                v, neighbor = corrected_v(original, vertex, shoulder)
                assert abs(changed['uv'][vertex][1]-v) < 1e-6
                old_dx = abs(original['positions'][neighbor][0])-abs(original['positions'][vertex][0])
                new_dx = abs(changed['positions'][neighbor][0])-abs(changed['positions'][vertex][0])
                old_density = abs(original['uv'][vertex][1]-original['uv'][neighbor][1])/old_dx
                density = abs(changed['uv'][vertex][1]-changed['uv'][neighbor][1])/new_dx
                assert abs(density/old_density-1) < 2e-5
            # Same new positions/UV but original forearm-heavy weights is the
            # negative witness for the old stretched, unbending sleeve repair.
            wrong_payload = bytearray(payload)
            at = original['skin_at']
            wrong_payload[at:at+count*8] = original['data'][at:at+count*8]
            wrong = read_mesh(bytes(wrong_payload))
            for name, clip in clips:
                for frame in sorted({0, clip['frames']//4, clip['frames']//2, 3*clip['frames']//4, clip['frames']-1}):
                    before, world = posed(original, clip, frame)
                    after, after_world = posed(changed, clip, frame)
                    bad, _ = posed(wrong, clip, frame)
                    assert world == after_world  # all authored joint/contact poses survive
                    for i in range(count):
                        if i not in original['ring']:
                            assert before[i] == after[i]  # includes every glove, finger and hand vertex
                    for sign in (-1, 1):
                        # Upper-arm surface follows the original shoulder/elbow
                        # segment throughout ladder and shield animations.
                        assert bend_alignment(after, world, original, sign) > .99
                        if bend_alignment(bad, world, original, sign) < .97:
                            failed_old_bend += 1
                    cases += 1
            print(f'{fixture.stem}: exact upper-arm skin influence, original texel density and unchanged contact geometry passed')
        assert failed_old_bend > 0
        print(f'{cases} original animation/model poses checked; old forearm-heavy edge fails {failed_old_bend} bend checks')
    assert all(hashlib.sha256(path.read_bytes()).digest() == digest for path, digest in sources.items())


if __name__ == '__main__':
    main()
