"""Exercise seed curation and malformed records without launching the game."""
from pathlib import Path
import hashlib
import importlib.util
import json
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('build_xbe_shader_seed', ROOT / 'scripts/build_xbe_shader_seed.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


def dxbc(stage=0):
    program = struct.pack('<II', stage << 16 | 0x50, 2)
    return b'DXBC' + bytes(16) + struct.pack('<4I', 1, 52, 1, 36) + b'SHEX' + struct.pack('<I', len(program)) + program


def fixture(*, source=b'float4 main():SV_Target{return 1;}', profile=b'ps_5_0', code=None, key=None, contract=None):
    if code is None:
        code = dxbc(builder.PROFILES.get(profile, 0))
    if key is None:
        key = builder.PREFIX + b'1' * 64
        for part in (b'main', profile, source):
            key += str(len(part)).encode() + b':' + part
    contract = contract or (0x55434431, 0, 0, 123, 4714496, 47)
    data = struct.pack('<10IQ', 0x5343564e, 1, *contract, len(key), len(code), builder.fnv(code)) + key + code
    name = f'v1-{builder.fnv(key, builder.fnv(data[8:32])):016x}.bin'
    return name, data


class ShaderSeedTests(unittest.TestCase):
    def test_all_supported_stages(self):
        for profile in builder.PROFILES:
            name, data = fixture(profile=profile)
            self.assertEqual(builder.record(name, data)['profile'], profile.decode())

    def test_payload_and_filename_identity(self):
        name, data = fixture()
        for bad_name, bad_data in [('v1-0000000000000000.bin', data), ('../' + name, data),
                                   (name, data[:-1] + bytes([data[-1] ^ 1])), (name, data + b'x')]:
            with self.subTest(name=bad_name), self.assertRaises(ValueError):
                builder.record(bad_name, bad_data)

    def test_driver_and_contract_are_rejected(self):
        for kwargs in ({'code': b'driver blob'}, {'profile': b'driver'},
                       {'contract': (0x55434431, 1, 0, 123, 4714496, 47)},
                       {'contract': (0x55434431, 0, 0, 123, 4714496, 46)},
                       {'contract': (0x12345678, 0, 0, 123, 4714496, 47)}):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                builder.record(*fixture(**kwargs))

    def test_key_must_be_complete_and_unambiguous(self):
        good = builder.PREFIX + b'1' * 64 + b'4:main6:ps_5_01:x'
        invalid = [good + b'x', good.replace(b'4:main', b'04:main'), good[:-3] + b'0:',
                   good.replace(b'1' * 64, b'z' * 64), good[:-1] + b'\0',
                   good.replace(b'4:main', b'9999999:main')]
        for key in invalid:
            with self.subTest(key=key), self.assertRaises(ValueError):
                builder.record(*fixture(key=key))

    def test_empty_source_and_bad_entry(self):
        with self.assertRaises(ValueError):
            builder.record(*fixture(source=b' \n'))
        key = builder.PREFIX + b'1' * 64 + b'4:m/in6:ps_5_01:x'
        with self.assertRaises(ValueError):
            builder.record(*fixture(key=key))

    def test_dxbc_container_and_program_bounds(self):
        base = dxbc()
        variants = [base[:-1], dxbc(1)]
        for offset, value in [(20, 2), (24, 999), (28, 0), (28, 9999), (32, 0),
                              (32, 37), (40, 999), (48, 999)]:
            altered = bytearray(base)
            struct.pack_into('<I', altered, offset, value)
            variants.append(bytes(altered))
        variants.append(b'DXBC' + bytes(16) + struct.pack('<5I', 1, 56, 2, 40, 40) + base[36:])
        for code in variants:
            with self.subTest(code=code), self.assertRaises(ValueError):
                builder.record(*fixture(code=code))

    def test_union_manifest_and_inventory_separation(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'build') as directory:
            root = Path(directory)
            a, b, output = root / 'a', root / 'b', root / 'output'
            a.mkdir(); b.mkdir()
            first = fixture()
            second = fixture(source=b'float4 main():SV_Target{return 0;}')
            for cache, item in ((a, first), (b, first), (b, second)):
                (cache / item[0]).write_bytes(item[1])
            report = builder.build([a, b], output, root / 'inventory.json')
            self.assertEqual(report['records'], 2)
            self.assertEqual([row['added'] for row in report['origins']], [1, 1])
            manifest = json.loads((output / 'manifest.json').read_text())
            self.assertEqual(manifest, {'version': 1, 'files': {
                name: hashlib.sha256(data).hexdigest() for name, data in (first, second)}})
            self.assertEqual({p.name for p in output.iterdir()}, {'manifest.json', first[0], second[0]})
            self.assertNotIn(str(root), (output / 'manifest.json').read_text())
            with self.assertRaises(ValueError):
                builder.build([a], output)
            self.assertEqual((output / first[0]).read_bytes(), first[1])

    def test_invalid_input_never_publishes_output(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'build') as directory:
            root = Path(directory)
            cache = root / 'input'; cache.mkdir()
            name, data = fixture()
            (cache / name).write_bytes(data)
            (cache / 'z.bin').write_bytes(data)
            output = root / 'output'
            with self.assertRaises(ValueError):
                builder.build([cache], output)
            self.assertFalse(output.exists())

    def test_empty_inputs_and_internal_inventory_rejected(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'build') as directory:
            root = Path(directory)
            cache = root / 'input'; cache.mkdir()
            with self.assertRaises(ValueError):
                builder.build([cache], root / 'output')
            name, data = fixture(); (cache / name).write_bytes(data)
            with self.assertRaises(ValueError):
                builder.build([cache], root / 'output', root / 'output/inventory.json')

    def test_hard_links_rejected(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'build') as directory:
            root = Path(directory)
            name, data = fixture(); original = root / name; original.write_bytes(data)
            link = root / ('other-' + name)
            link.hardlink_to(original)
            with self.assertRaises(ValueError):
                builder.read_record(original)


if __name__ == '__main__':
    unittest.main()
