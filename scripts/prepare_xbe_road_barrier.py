"""Make only two exact retail Canals road-barrier draws visible from behind."""
from pathlib import Path
import hashlib
import struct
from prepare_xbe_perf import function

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = '#include "xbe_road_barrier.h"\n'
CALL = '    if (k.cull && uc_road_barrier::TwoSided(s, a)) k.cull = 0;\n'
ANCHOR = '    k.ccw = cull == kCullCw;  // culling clockwise leaves counter clockwise as the front\n'


def transform(source):
    original = source.replace(INCLUDE, '').replace(CALL, '')
    assert original.count(ANCHOR) == 1
    assert original.count('#include "gpu/vertex.h"\n') == 1
    result = original.replace('#include "gpu/vertex.h"\n', '#include "gpu/vertex.h"\n' + INCLUDE, 1)
    return result.replace(ANCHOR, ANCHOR + CALL, 1)


def fixture_data():
    # Private fixture bytes are generated locally; original asset bytes are never
    # copied into maintained source or shipped as part of this repair.
    import sys
    sys.path.insert(0, str(ROOT / 'scripts/native_scene_checks'))
    from inspect_scene import load
    from analyze_frontend import parse_archive, unpack_entry
    from convert_native_materials import parse_material
    path = ROOT / 'game_files/1-01.geo'
    data, entries = parse_archive(path)
    assert hashlib.sha256(data).hexdigest() == '7eb10c773c61f227b5669c8ee5eb255ab8fccdedc7db7418aed21ee05e522161'
    raw = unpack_entry(data, next(e for e in entries if e['name'].endswith('$244.csf')))
    assert hashlib.sha256(raw).hexdigest() == 'ff2db82a56cfbd6563c0a407e46cb14d95aa6ed82dce4f67e2e30c2330fa0dfb'
    _, _, _, passes = parse_material(raw)
    vertex, declaration, pixel = passes[0][1]
    assert len(vertex) == 132 and len(declaration) == 32 and len(pixel) == 244
    scene = load(path)
    scene.scene()
    assert len(scene.nodes) == 48 and len(scene.draws) == 3403
    retail = (ROOT / 'game_files/Default.xbe').read_bytes()
    assert hashlib.sha256(retail).hexdigest() == 'b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac'
    base = struct.unpack_from('<I', retail, 0x104)[0]
    count, table = struct.unpack_from('<II', retail, 0x11c)
    sections = [struct.unpack_from('<6I', retail, table - base + i * 56) for i in range(count)]
    def original(address, size):
        section = next(s for s in sections if s[1] <= address and address + size <= s[1] + s[4])
        offset = section[3] + address - section[1]
        return retail[offset:offset + size]
    pre_vertex, pre_decl, pre_pixel = original(0x4EB450, 100), original(0x4F7C24, 12), original(0x4EAC98, 240)
    assert hashlib.sha256(pre_vertex[4:]).hexdigest() == 'cb77f10155ca286b59ded767748bbbf1b5ec718ce97cd553a6fadebb30556c81'
    assert hashlib.sha256(pre_pixel).hexdigest() == 'ac4445efa2005f2f249e0044276b47f33d600abee238e7e2982653753490b1c2'
    texture = unpack_entry(data, next(e for e in entries if e['name'].lower() == '01-01\\map_01_barrier.tga'))
    assert struct.unpack_from('<3I', texture) == (0, 128, 16)
    result = bytearray(b'UCROAD02' + vertex + declaration + pixel + pre_vertex + pre_decl + pre_pixel +
                       texture[28:28 + 1024] + struct.pack('<II', 48, 3403))
    slots = {}
    for i, node in enumerate(scene.nodes):
        assert node['flags'] == 0
        slots[node['offset']] = i
        count = node['vertices']
        result += struct.pack('<I', count)
        for slot, stride in ((0, 16), (2, 4), (3, 8)):
            start = node['streams'][slot]
            result += scene.data[start:start + count*stride]
    for draw in scene.draws:
        node = scene.nodes[slots[draw['node']]]
        assert draw['topology'] == 5
        count = draw['primitives'] + 2
        at = node['offset'] + 112 + node['vertices']*44 + node['draws']*20 + node['arrays']*20 + draw['first']*2
        result += struct.pack('<II', slots[draw['node']], count) + scene.data[at:at + count*2]
    return result


def prepare(area):
    compat = Path(area) / 'src/compat'
    path = compat / 'd3d12.cpp'
    source = transform(path.read_text())
    assert transform(source) == source
    path.write_text(source)
    (compat / 'xbe_road_barrier.h').write_text(Path(__file__).with_name('xbe_road_barrier.h').read_text())
    key_at = source.index('struct PsoKey {')
    key = source[key_at:source.index('\n};', key_at) + 3]
    raster = function(source, 'void KeyRasterizer(')
    before = raster.replace(CALL, '').replace('KeyRasterizer(', 'BeforeRasterizer(')
    depth = function(source, 'void D3D12Backend::KeyDepthStencil(')
    # This fixture exercises depth/stencil state, with polygon offset disabled.
    depth = depth[:depth.index('    // polygon offset:')] + '    assert(!SimpleState(s, kMSolidOffsetEnable));\n}\n'
    depth = depth.replace('D3D12Backend::KeyDepthStencil', 'FixtureDepthStencil').replace('const Assembled& a, ', '')
    fixture = Path(__file__).with_name('test_xbe_road_barrier.cpp').read_text()
    fixture = fixture.replace('/* PRODUCTION_VERTEX */', (compat / 'vertex.cpp').read_text())
    fixture = fixture.replace('/* PRODUCTION_SHADERS */', '#include "shaders.cpp"')
    texture = (compat / 'texture.cpp').read_text()
    fixture = fixture.replace('/* PRODUCTION_DECLARATION */', 'namespace gpu {\n' +
        function(texture, 'static uint32_t TypeSize(') + '\n' +
        function(texture, 'void ParseDeclaration(') + '\n}\n')
    fixture = fixture.replace('/* PRODUCTION_RASTER */',
        'namespace gpu {\nconstexpr uint32_t kCullCw=0x900;\n' +
        function(source, 'uint32_t SimpleState(') + '\n' + key + '\n' +
        function(source, 'void KeyBlend(') + '\n' + depth + '\n' +
        before + '\n' + raster + '\n}\n')
    (compat / 'test_xbe_road_barrier.cpp').write_text(fixture)
    (compat / 'road_barrier_fixture.bin').write_bytes(fixture_data())
