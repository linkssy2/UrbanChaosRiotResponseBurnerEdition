"""Derive co-op helicopter render variants from the user's original archives.

Only the isolated built-in gun draw suffix is removed. Bones, vertices, indices,
cabin/FX draws, original resource names and animation data remain unchanged.
The aliases are lazy resources: construct them after the campaign archive is
mounted, so their original material/animation dependencies are available.
"""
from pathlib import Path
import hashlib
import struct
import tempfile
import zlib

from analyze_frontend import parse_archive, unpack_entry

SPECS = {
    '3-01': {
        'source': r'assets_scene_items\03-01\sil_interiorhelicopter\sil_interiorhelicopter_skin.amf',
        'alias': r'assets_coop\helicopter\03-01\sil_interiorhelicopter_skin.amf',
        'sha256': '0757559fd9ba233a6a3d2c06b872344dbbf4a9317fad57087e77a5c435c707c5',
        'gun_bone': 23, 'gun_materials': (20, 21, 22), 'keep_draws': 25,
        'companions': {
            '.mdlhk': '3d3d8206f873dd54a750574955fbe5446ccf0e78219f152578fd0fee1df686e2',
            '.entsupp': '30834e37832c94dde0d56c1e63ed76c47bcba14b5921ac1103436ac1e34a852b',
        },
    },
    '8-01': {
        'source': r'assets_scene_items\08-01\sil_interiorhelicopter\sil_interiorhelicopter_skin.amf',
        'alias': r'assets_coop\helicopter\08-01\sil_interiorhelicopter_skin.amf',
        'sha256': 'f7edb9a6358495ef5ceb138d325b618aeb3d0630281d1372e1d4851ccaf1c944',
        'gun_bone': 18, 'gun_materials': (22, 23), 'keep_draws': 28,
        'companions': {
            '.mdlhk': '34f101ca4e5246d228700cfe9c68453e2916deec400edb154c2a451cb490af5d',
            '.entsupp': 'dddf4b759c3bc829608d09017fe5541dd2e9fe84e3853cdc7382991de645795b',
        },
    },
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def _layout(data):
    """Bounded original AMF/A6220 cursor for these two pinned single-node models."""
    at = 0
    def take(size):
        nonlocal at
        if size < 0 or at + size > len(data):
            raise ValueError('Truncated helicopter AMF')
        start = at
        at += size
        return data[start:at]
    def word():
        return struct.unpack('<I', take(4))[0]
    def name():
        return take(struct.unpack('<H', take(2))[0])
    if take(4) != b'AMF\n':
        raise ValueError('Not an AMF model')
    header = take(72)
    materials, groups, bones = struct.unpack_from('<3I', header)
    if groups != 1 or not 0 < bones <= 256 or not 0 < materials <= 256:
        raise ValueError('Unexpected helicopter model layout')
    take(bones * 64)
    metadata = take(bones * 160)
    for _ in range(materials):
        name()
    if struct.unpack_from('<I', take(56))[0] != 1:
        raise ValueError('Unexpected helicopter group count')
    node = at
    _, children, vertices, draws, indices, extras, arrays, flags = struct.unpack_from('<8I', take(112))
    if children or arrays or flags != 17 or max(vertices, draws, indices, extras) > 100000:
        raise ValueError('Unsupported helicopter geometry layout')
    take(vertices * 30)  # compressed position, colour, UV0 and UV1
    descriptors_at = at
    descriptors = [take(20) for _ in range(draws)]
    indices_at = at
    index_data = take(indices * 2)
    for _ in range(extras):
        take(struct.unpack_from('<H', take(56), 50)[0] * 4)
    palette_at = at
    palettes = []
    for _ in range(draws):
        prefix = take(8)
        count = struct.unpack_from('<I', prefix)[0]
        palettes.append(prefix + take(count * 4))
    skin_at = at
    skin = take(vertices * 8)
    if word() != 0:
        raise ValueError('Unexpected helicopter child terminator')
    name()  # original animation-template path, followed by retained source suffix
    return dict(node=node, vertices=vertices, descriptors_at=descriptors_at,
                descriptors=descriptors, indices_at=indices_at, index_data=index_data,
                palette_at=palette_at, palettes=palettes, skin_at=skin_at,
                skin=skin, metadata=metadata, materials=materials, bones=bones)


def derive_model(data, level):
    spec = SPECS[level]
    if digest(data) != spec['sha256']:
        raise ValueError(f'{level}: unsupported original helicopter model identity')
    layout = _layout(data)
    keep, gun = spec['keep_draws'], spec['gun_bone']
    if layout['metadata'][gun * 160 + 4:gun * 160 + 22].split(b'\0')[0] != b'gun':
        raise ValueError('Original gun bone identity changed')
    palette = {}
    for index, (draw, updates) in enumerate(zip(layout['descriptors'], layout['palettes'])):
        first, material, alternate, _, _, triangles, topology = struct.unpack_from('<IHHHHHB', draw)
        removed = material in spec['gun_materials']
        if removed != (index >= keep) or alternate != 0xffff or not draw[16] or topology not in (4, 5):
            raise ValueError('Gun draws are no longer an isolated base-pass suffix')
        for bone, row, _ in struct.iter_unpack('<HbB', updates[8:]):
            palette[row] = bone
        count = triangles * 3 if topology == 4 else triangles + 2
        if first + count > len(layout['index_data']) // 2:
            raise ValueError('Invalid helicopter draw indices')
        for vertex in struct.unpack_from('<' + 'H' * count, layout['index_data'], first * 2):
            if vertex >= layout['vertices']:
                raise ValueError('Invalid helicopter vertex')
            r0, r1, r2, w0, w1 = struct.unpack_from('<3h2B', layout['skin'], vertex * 8)
            if w0 + w1 > 255:
                raise ValueError('Unsupported helicopter skin weights')
            for row, weight in zip((r0, r1, r2), (w0, w1, 255 - w0 - w1)):
                if weight and ((palette.get(row) == gun) != removed):
                    raise ValueError('Gun bone influences cabin geometry, or gun has shared weights')
    prefix = bytearray(data[:layout['descriptors_at']])
    struct.pack_into('<I', prefix, layout['node'] + 12, keep)
    result = (bytes(prefix) + b''.join(layout['descriptors'][:keep]) +
              data[layout['indices_at']:layout['palette_at']] +
              b''.join(layout['palettes'][:keep]) + data[layout['skin_at']:])
    checked = _layout(result)
    if (checked['descriptors'] != layout['descriptors'][:keep] or
            checked['palettes'] != layout['palettes'][:keep] or
            result[checked['skin_at']:] != data[layout['skin_at']:] or
            result[checked['indices_at']:checked['palette_at']] != data[layout['indices_at']:layout['palette_at']]):
        raise ValueError('Derived helicopter changed retained geometry')
    return result


def build_resources(game_dir, *, level=None, cancel=None):
    """Return six lazy alias payloads plus a source/output identity report.

    Original collision/support payloads are copied under alias names unchanged;
    no collision geometry or physics behavior is rewritten by this render fix.
    """
    check = cancel or (lambda: None)
    if level is not None and level not in SPECS:
        raise ValueError('Unsupported co-op helicopter level')
    resources, report = {}, {}
    for level, spec in SPECS.items() if level is None else ((level, SPECS[level]),):
        check()
        path = Path(game_dir) / (level + '.ent')
        raw, entries = parse_archive(path)
        # Original resource-name map uses its final registration. These pinned
        # model/support payload identities must still match the retail source.
        by_name = {e['name'].lower(): e for e in entries if e['bank'] == 0}
        source = unpack_entry(raw, by_name[spec['source']])
        alias = spec['alias']
        derived = derive_model(source, level)
        resources[alias] = derived
        for extension, expected in spec['companions'].items():
            check()
            payload = unpack_entry(raw, by_name[spec['source'][:-4] + extension])
            if digest(payload) != expected:
                raise ValueError(f'{level}: unsupported helicopter {extension} identity')
            resources[alias[:-4] + extension] = payload
        report[level] = {'archive_sha256': digest(raw), 'source_sha256': digest(source),
                         'alias': alias, 'derived_sha256': digest(derived),
                         'source_bytes': len(source), 'derived_bytes': len(derived),
                         'retained_draws': spec['keep_draws'], 'removed_materials': list(spec['gun_materials']),
                         'unchanged_support_payloads': dict(spec['companions'])}
    return resources, report


def build_archive(game_dir, output, *, level, cancel=None):
    """Write one three-record LST, opened only after its campaign is mounted.

    Retail 54B70 resolves every record eagerly. Keep 03 and 08 separate so the
    other campaign's material/template dependencies are never resolved early.
    A failed/cancelled conversion leaves any previous output intact.
    """
    check = cancel or (lambda: None)
    check()
    output = Path(output)
    if output.resolve() in {(Path(game_dir) / (key + '.ent')).resolve() for key in SPECS}:
        raise ValueError('The source game archives must remain unchanged')
    resources, report = build_resources(game_dir, level=level, cancel=check)
    names = [name.encode('latin1') + b'\0' for name in resources]
    directory_size = 4 + sum(2 + len(name) + 16 for name in names)
    result = bytearray(0xc8)
    struct.pack_into('<I', result, 0, 0x1c)
    struct.pack_into('<HHH', result, 0x12, len(names), 1, 0)
    struct.pack_into('<II', result, 0xc0, directory_size, len(names))
    position = 0xc4 + directory_size
    compressed = []
    for name, payload in zip(names, resources.values()):
        check()
        packed = zlib.compress(payload, 9)
        result.extend(struct.pack('<H', len(name)) + name)
        result.extend(struct.pack('<4I', position, len(payload), len(packed), 0))
        compressed.append(packed)
        position += len(packed)
    for packed in compressed:
        result.extend(packed)
    check()
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(prefix=output.name + '.', suffix='.tmp', dir=output.parent, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(result)
        checked, entries = parse_archive(temporary)
        if len(entries) != 3:
            raise ValueError('Incomplete co-op helicopter archive')
        for entry, (name, payload) in zip(entries, resources.items()):
            check()
            if entry['name'] != name or entry['bank'] != 0 or unpack_entry(checked, entry) != payload:
                raise ValueError('Co-op helicopter archive roundtrip failed')
        check()
        temporary.replace(output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {'size': len(result), 'sha256': digest(result), 'entries': 3,
            'level': level, 'names': list(resources), 'models': report,
            'scope': 'Co-op-only helicopter render alias and unchanged original support; no campaign entities or audio.'}
