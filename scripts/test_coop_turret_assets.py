"""Owned original AMF gun isolation and byte-exact cabin preservation."""
from pathlib import Path
import hashlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import coop_turret_assets as assets
from analyze_frontend import parse_archive, unpack_entry


class TurretAssets(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.game = Path(__file__).resolve().parents[1] / 'game_files'
        if not all((cls.game / (level + '.ent')).is_file() for level in assets.SPECS):
            raise unittest.SkipTest('Owned retail campaign archives are required')

    def test_original_gun_suffix_and_unchanged_cabin(self):
        resources, report = assets.build_resources(self.game)
        self.assertEqual(len(resources), 6)
        for level, spec in assets.SPECS.items():
            path = self.game / (level + '.ent')
            before = hashlib.sha256(path.read_bytes()).hexdigest()
            raw, entries = parse_archive(path)
            source = unpack_entry(raw, next(e for e in entries if e['name'].lower() == spec['source']))
            old = assets._layout(source)
            derived = resources[spec['alias']]
            new = assets._layout(derived)
            keep = spec['keep_draws']
            self.assertGreater(len(old['descriptors']), keep)  # actual visible-gun negative witness
            self.assertTrue(any(struct.unpack_from('<H', d, 4)[0] in spec['gun_materials'] for d in old['descriptors']))
            self.assertEqual(new['descriptors'], old['descriptors'][:keep])
            self.assertEqual(new['palettes'], old['palettes'][:keep])
            self.assertTrue(all(struct.unpack_from('<H', d, 4)[0] not in spec['gun_materials'] for d in new['descriptors']))
            prefix = bytearray(derived[:new['descriptors_at']])
            struct.pack_into('<I', prefix, new['node'] + 12, len(old['descriptors']))
            self.assertEqual(bytes(prefix), source[:old['descriptors_at']])
            self.assertEqual(derived[new['indices_at']:new['palette_at']], source[old['indices_at']:old['palette_at']])
            self.assertEqual(derived[new['skin_at']:], source[old['skin_at']:])
            self.assertEqual(assets.derive_model(source, level), derived)
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), before)
            self.assertEqual(report[level]['source_sha256'], spec['sha256'])
            # The unchanged source path is never emitted as an override.
            self.assertNotIn(spec['source'], resources)

    def test_unknown_or_damaged_input_fails_closed(self):
        for level, spec in assets.SPECS.items():
            raw, entries = parse_archive(self.game / (level + '.ent'))
            source = unpack_entry(raw, next(e for e in entries if e['name'].lower() == spec['source']))
            for damaged in (source[:-1], source + b'\0', source[:4] + b'\0' * (len(source) - 4)):
                with self.assertRaisesRegex(ValueError, 'identity'):
                    assets.derive_model(damaged, level)

    def test_per_campaign_archive_roundtrip_and_cancelled_publish(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'coopturrets.lst'
            for level in assets.SPECS:
                report = assets.build_archive(self.game, output, level=level)
                expected, _ = assets.build_resources(self.game, level=level)
                raw, entries = parse_archive(output)
                self.assertEqual(len(entries), 3)
                self.assertEqual({entry['name']: unpack_entry(raw, entry) for entry in entries}, expected)
                self.assertTrue(all(entry['bank'] == 0 for entry in entries))
                self.assertEqual(report['sha256'], assets.digest(raw))
                self.assertEqual(report['size'], len(raw))
                first = output.read_bytes()
                self.assertEqual(assets.build_archive(self.game, output, level=level)['sha256'], assets.digest(first))
                for cancel_at in range(1, 13):
                    count = 0
                    def cancel():
                        nonlocal count
                        count += 1
                        if count == cancel_at:
                            raise InterruptedError('cancelled')
                    with self.assertRaises(InterruptedError):
                        assets.build_archive(self.game, output, level=level, cancel=cancel)
                    self.assertEqual(output.read_bytes(), first)
                    self.assertEqual(list(Path(directory).glob('*.tmp')), [])
            for level in assets.SPECS:
                with self.assertRaisesRegex(ValueError, 'remain unchanged'):
                    assets.build_archive(self.game, self.game / (level + '.ent'), level=level)


if __name__ == '__main__':
    unittest.main()
