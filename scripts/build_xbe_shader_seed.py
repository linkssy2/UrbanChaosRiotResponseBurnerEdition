"""Curate observed portable shader records; never export GPU/driver pipeline blobs."""
from pathlib import Path
import argparse
import collections
import hashlib
import importlib.util
import json
import re
import stat
import struct

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('burner_shader_seed', ROOT / 'launcher/burner-xbe/shader_seed.py')
seed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(seed)
PREFIX = b'UC-D3D12-DXBC-1:'
PROFILES = {b'ps_5_0': 0, b'vs_5_0': 1, b'gs_5_0': 2}


def fnv(data, value=14695981039346656037):
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return value


def record(name, data):
    """Check structure and identity; the runtime additionally reflects the DXBC."""
    if not seed.filename(name) or not 48 < len(data) <= seed.FILE_LIMIT:
        raise ValueError('Invalid shader record filename or size.')
    header = struct.unpack_from('<10IQ', data)
    magic, version, family, flags, effect_flags, timestamp, image_size, compiler, key_size, code_size, code_hash = header
    if (magic != 0x5343564e or version != 1 or family != 0x55434431 or
            flags != 0 or effect_flags != 0 or not timestamp or not image_size or compiler != 47 or
            not 0 < key_size <= 2 * 1024 * 1024 or not 0 < code_size <= 16 * 1024 * 1024 or
            len(data) != 48 + key_size + code_size):
        raise ValueError('Unsupported shader record contract or lengths.')
    key, code = data[48:48 + key_size], data[48 + key_size:]
    if (name != f'v1-{fnv(key, fnv(data[8:32])):016x}.bin' or fnv(code) != code_hash or
            not key.startswith(PREFIX) or b'\0' in key):
        raise ValueError('Shader record identity or payload checksum failed.')
    compiler_hash = key[len(PREFIX):len(PREFIX) + 64]
    if not re.fullmatch(rb'[0-9a-f]{64}', compiler_hash):
        raise ValueError('Invalid shader compiler identity.')
    at = len(PREFIX) + 64
    parts = []
    for limit in (128, 128, 2 * 1024 * 1024 - 1024):
        end = key.find(b':', at, at + 8)
        digits = key[at:end] if end >= at else b''
        if not re.fullmatch(rb'[1-9][0-9]{0,6}', digits):
            raise ValueError('Invalid length-prefixed shader key.')
        size = int(digits)
        at = end + 1
        if size > limit or size > len(key) - at:
            raise ValueError('Shader key component is out of bounds.')
        parts.append(key[at:at + size])
        at += size
    entry, profile, source = parts
    if (at != len(key) or profile not in PROFILES or
            not re.fullmatch(rb'[A-Za-z_][A-Za-z0-9_]*', entry) or not source.strip()):
        raise ValueError('Unsupported shader entry/profile or empty source.')
    if len(code) < 36 or code[:4] != b'DXBC':
        raise ValueError('GPU-specific pipeline data is not portable shader bytecode.')
    dxbc_version, dxbc_size, chunks = struct.unpack_from('<III', code, 20)
    if dxbc_version != 1 or dxbc_size != len(code) or not 0 < chunks <= min(1024, (len(code) - 32) // 4):
        raise ValueError('Invalid DXBC container header.')
    regions = []
    programs = []
    for offset in struct.unpack_from('<' + 'I' * chunks, code, 32):
        if offset % 4 or not 32 + chunks * 4 <= offset <= len(code) - 8:
            raise ValueError('Invalid DXBC chunk offset.')
        size, = struct.unpack_from('<I', code, offset + 4)
        end = offset + 8 + size
        if end > len(code):
            raise ValueError('Truncated DXBC chunk.')
        regions.append((offset, end))
        if code[offset:offset + 4] in (b'SHEX', b'SHDR'):
            if size < 8 or size % 4:
                raise ValueError('Invalid shader token stream.')
            token, words = struct.unpack_from('<II', code, offset + 8)
            if token != ((PROFILES[profile] << 16) | 0x50) or words * 4 != size:
                raise ValueError('DXBC shader stage/version does not match its profile.')
            programs.append(offset)
    regions.sort()
    if len(programs) != 1 or any(a[1] > b[0] for a, b in zip(regions, regions[1:])):
        raise ValueError('Overlapping DXBC chunks or invalid shader program count.')
    return {'entry': entry.decode('ascii'), 'profile': profile.decode('ascii'),
            'compiler_sha256': compiler_hash.decode('ascii'),
            'source_sha256': hashlib.sha256(source).hexdigest()}


def read_record(path):
    path = Path(path).absolute()
    seed.unlinked(path)
    before = path.stat(follow_symlinks=False)
    if not stat.S_ISREG(before.st_mode) or before.st_nlink != 1 or not 48 < before.st_size <= seed.FILE_LIMIT:
        raise ValueError('Shader source must be a bounded regular file, without links.')
    with path.open('rb') as file:
        data = file.read(seed.FILE_LIMIT + 1)
    seed.unlinked(path)
    after = path.stat(follow_symlinks=False)
    identity = lambda info: (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns)
    if identity(before) != identity(after):
        raise ValueError('Shader source changed while it was being read.')
    return data, record(path.name, data)


def build(caches, output, inventory=None):
    output = Path(output).absolute()
    seed.unlinked(output)
    if output.exists() and (not output.is_dir() or any(output.iterdir())):
        raise ValueError('Shader seed output must be a fresh or empty directory.')
    if inventory is not None:
        inventory = Path(inventory).absolute()
        seed.unlinked(inventory)
        if inventory.exists() or output == inventory or output in inventory.parents:
            raise ValueError('Inventory must be a new file outside the shipped seed directory.')
    records, origins = {}, []
    total = 0
    for cache in caches:
        cache = Path(cache).absolute()
        seed.unlinked(cache)
        if not cache.is_dir():
            raise ValueError('Shader cache input must be an existing directory.')
        files = sorted(cache.iterdir())
        if len(files) > seed.ENTRY_LIMIT:
            raise ValueError('Shader cache input exceeds the entry budget.')
        added = 0
        for path in files:
            data, metadata = read_record(path)
            if path.name in records:
                if records[path.name][0] != data:
                    raise ValueError('Conflicting shader records share a filename.')
                continue
            total += len(data)
            if len(records) >= seed.ENTRY_LIMIT or total > seed.BYTE_LIMIT:
                raise ValueError('Shader seed exceeds its startup budget.')
            records[path.name] = (data, metadata)
            added += 1
        origins.append({'cache': str(cache), 'records': len(files), 'added': added})
    if not records:
        raise ValueError('No observed shader records were supplied.')
    output.mkdir(parents=True, exist_ok=True)
    manifest = {'version': 1, 'files': {}}
    for name, (data, _) in sorted(records.items()):
        with (output / name).open('xb') as file:
            file.write(data)
        manifest['files'][name] = hashlib.sha256(data).hexdigest()
    with (output / 'manifest.json').open('x', encoding='utf-8') as file:
        file.write(json.dumps(manifest, indent=2) + '\n')
    seed.validate(output, manifest['files'])
    report = {'version': 1, 'records': len(records), 'bytes': total, 'origins': origins,
              'profiles': dict(collections.Counter(m['profile'] for _, m in records.values())),
              'compilers': sorted({m['compiler_sha256'] for _, m in records.values()}),
              'coverage': 'Observed shader variants only; unseen shaders, driver pipeline work and asset loading may still cause pauses.',
              'files': {name: metadata for name, (_, metadata) in sorted(records.items())}}
    if inventory is not None:
        inventory.parent.mkdir(parents=True, exist_ok=True)
        with inventory.open('x', encoding='utf-8') as file:
            file.write(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'packaging/shader-seed')
    parser.add_argument('--inventory', type=Path, help='Optional evidence JSON outside the shipped seed directory.')
    args = parser.parse_args()
    report = build(args.cache, args.output, args.inventory)
    print(json.dumps({key: report[key] for key in ('records', 'bytes', 'profiles', 'compilers', 'coverage')}))


if __name__ == '__main__':
    main()
