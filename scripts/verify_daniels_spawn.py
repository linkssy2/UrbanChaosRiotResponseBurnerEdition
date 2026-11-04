"""Verify the authored spawn, wall overlap and candidate clearance; not a live physics test."""
from pathlib import Path
import hashlib
import json
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from coop_progression_assets import read_level
from native_collision_checks.inspect_collision import load

def verify(folder):
    folder = Path(folder)
    hashes = {'9-01.ent': '5a3c62345bebe80c0f2d84f23410c09217b0b10fe5d7c02d8203d5d64d7bb391',
              '9-01.geo': 'd8da62a940889bd63f731b404342dce536f70bbd8bf7a06bde30084a574e28eb'}
    for name, expected in hashes.items():
        assert hashlib.sha256((folder / name).read_bytes()).hexdigest() == expected, name
    reader, _ = read_level(folder / '9-01.ent')
    actor = reader.entities[153]
    assert (actor['kind'], actor['name']) == ('CharacterCivilian', 'z1 Cop Rescued Curtis')
    xyz = actor['matrix'][12:15]
    assert struct.unpack('<3I', struct.pack('<3f', *xyz)) == (0x45900b6d, 0x42f4bef0, 0xc4204a85)
    matches = []
    levels = sorted(folder.glob('*-01.ent'))
    for path in levels:
        level, _ = read_level(path)
        matches += [(path.name, i) for i, e in enumerate(level.entities)
                    if e['kind'].startswith('Character') and e.get('matrix', [])[12:15] == xyz]
    assert len(levels) == 27 and matches == [('9-01.ent', 153)], matches
    collision = load(folder / '9-01.geo')
    assert struct.unpack('<4f', collision.scaling) == (1, 1, 1, 1)
    part = collision.parts[11]
    vertices = list(struct.iter_unpack('<3f', part['arrays'][0][2]))
    indices = list(struct.iter_unpack('<3H', part['arrays'][1][2]))
    triangles = {i: [vertices[v] for v in indices[i]] for i in (4, 15, 80, 81, 432, 433, 398, 399)}
    outer = sum(p[0] for p in triangles[4]) / 3
    inner = sum(p[0] for p in triangles[432]) / 3
    old = [xyz[0] / 100, xyz[2] / 100, xyz[1] / 100]  # Havok swaps Y/Z and uses meters.
    new = [4558 / 100, old[1], old[2]]
    assert inner < old[0] < outer and outer - inner < .014
    for i in (4, 15, 80, 81, 432, 433):
        tri = triangles[i]
        assert max(p[0] for p in tri) - min(p[0] for p in tri) < .00001
        # Both wall layers are safely beyond the .35m capsule plus .1m recovery margin.
        assert min(p[0] for p in tri) - new[0] > .50
    assert old[2] + 1.7 > min(p[2] for p in triangles[432])
    floor = triangles[398] + triangles[399]
    assert all(p[2] == 1 for p in floor)
    for axis in (0, 1):
        assert min(p[axis] for p in floor) + .45 < new[axis] < max(p[axis] for p in floor) - .45
    assert 0 < new[2] - 1 < .25  # Preserve the original short settling drop onto the floor.
    return {'scope': __doc__, 'assets': hashes, 'levels_scanned': len(levels), 'matching_characters': matches,
            'original_game_position': xyz, 'corrected_game_position': [4558, xyz[1], xyz[2]],
            'wall_x_meters': [inner, outer], 'corrected_wall_clearance_meters': inner - new[0],
            'collision_triangles': triangles, 'live_acceptance': False}

if __name__ == '__main__':
    result = verify(sys.argv[1])
    if len(sys.argv) > 2:
        Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + '\n')
    print('Daniels geometry: unique character spawn, two intersecting wall layers, >0.50m corrected clearance and floor support verified.')
