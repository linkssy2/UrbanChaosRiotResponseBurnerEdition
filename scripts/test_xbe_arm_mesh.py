"""Test exact arm-stream edits against the user's archives; never modify disc assets."""
from pathlib import Path
import hashlib
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_frontend import parse_archive, unpack_entry
from native_scene_checks.inspect_scene import Reader

exe, game = map(Path, sys.argv[1:])
with tempfile.TemporaryDirectory(prefix='burner-arm-mesh-') as temporary:
    fixtures = []
    sources = {}
    for name in ('armssleeved.lst', 'armsbare.lst'):
        path = game / name
        sources[path] = hashlib.sha256(path.read_bytes()).digest()
        data, entries = parse_archive(path)
        models = [entry for entry in entries if entry['extension'] == '.amf']
        assert len(models) == 1
        payload = unpack_entry(data, models[0])
        reader = Reader(payload)
        reader.model()
        assert reader.pos == len(payload) and len(reader.nodes) == 1
        node = reader.nodes[0]
        assert node['flags'] == 17
        start = node['streams'][0]
        fixture = Path(temporary) / (name + '.positions')
        fixture.write_bytes(payload[start:start + node['vertices'] * 10])
        uv = node['streams'][3]
        fixture.with_suffix('.uv').write_bytes(payload[uv:uv + node['vertices'] * 8])
        fixtures.append(fixture)
    subprocess.run([str(exe.resolve()), *map(str, fixtures)], check=True)
    assert all(hashlib.sha256(path.read_bytes()).digest() == digest for path, digest in sources.items())
