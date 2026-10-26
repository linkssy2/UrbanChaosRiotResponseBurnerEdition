"""Offline original-asset and malformed-input checks; no game or GPU launch."""
from pathlib import Path
import argparse, datetime, hashlib, json, struct, subprocess, sys, zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from analyze_frontend import parse_archive, unpack_entry

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    args = parser.parse_args()
    exe, assets = args.exe.resolve(), args.assets.resolve()
    output = ROOT / 'build/coop-preview-assets-evidence' / datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S-%f')
    output.mkdir(parents=True)
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    originals = [assets / name for name in ('coopcharacters.lst', '40-01.ent', 'Default.xbe') if (assets / name).is_file()]
    before = {str(p): sha(p) for p in originals}
    report = {'scope': __doc__, 'executable_sha256': sha(exe), 'originals_before': before, 'checks': []}

    def run(name, folder, reject=False, expected=None):
        result = subprocess.run([str(exe), str(folder), *(['--reject'] if reject else [])], capture_output=True, text=True, timeout=30)
        text = result.stdout + result.stderr
        (output / (name + '.log')).write_text(text)
        report['checks'].append({'name': name, 'returncode': result.returncode, 'output': text})
        assert result.returncode == 0 and (expected is None or expected in text), (name, text)

    run('five-original-bodies', assets)
    raw, entries = parse_archive(assets / 'coopcharacters.lst')
    by_name = {e['name'].lower(): e for e in entries}
    prefix = 'assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_skin.amf'

    def changed(name, edit):
        entry = by_name[name]
        payload = edit(bytearray(unpack_entry(raw, entry)))
        packed = zlib.compress(payload)
        result = bytearray(raw)
        marker = entry['name'].encode('latin1') + b'\0'
        position = result.find(marker, 0xc8, 0xc4 + struct.unpack_from('<I', result, 0xc0)[0])
        assert position >= 0
        struct.pack_into('<III', result, position + len(marker), len(result), len(payload), len(packed))
        result.extend(packed)
        return result

    def flip_program(payload):
        payload[-4] ^= 1
        return payload

    def bad_texture(payload):
        struct.pack_into('<I', payload, 4, 8192)
        return payload

    cases = [
        ('truncated-archive', raw[:100], 'Invalid co-op character archive size'),
        ('truncated-model', changed(prefix, lambda p: p[:-1]), 'Unsupported co-op preview model'),
        ('unknown-material-program', changed('$n:\\' + prefix + '$0.skcsf', flip_program), 'Unsupported co-op preview material recipe'),
        ('oversized-texture', changed('characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_mp_tzero_riot_helmet.tga', bad_texture), 'Unsupported co-op preview texture'),
        ('truncated-texture-sequence', changed('characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_mp_tzero_eyes.ifl', lambda p: p[:-1]), 'Unsupported preview texture sequence'),
    ]
    for name, data, expected in cases:
        folder = output / name
        folder.mkdir()
        (folder / 'coopcharacters.lst').write_bytes(data)
        run(name, folder, True, expected)
    report['originals_after'] = {str(p): sha(p) for p in originals}
    assert report['originals_after'] == before
    report['passed'] = True
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(output / 'results.json')

if __name__ == '__main__':
    main()
