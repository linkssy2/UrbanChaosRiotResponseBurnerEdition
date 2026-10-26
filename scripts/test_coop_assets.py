"""Resource admission, dependency recursion, byte preservation and source immutability."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
import coop_assets
from analyze_frontend import parse_archive, unpack_entry


def archive(path, resources):
    names = [name.encode() + b'\0' for name, _, _ in resources]
    packed = [zlib.compress(payload, 1) for _, payload, _ in resources]
    size = 4 + sum(2 + len(name) + 16 for name in names)
    data = bytearray(0xc8)
    struct.pack_into('<I', data, 0, 0x1c)
    struct.pack_into('<II', data, 0xc0, size, len(resources))
    offset = 0xc4 + size
    for (name, payload, bank), encoded, block in zip(resources, names, packed):
        data.extend(struct.pack('<H', len(encoded)) + encoded + struct.pack('<IIII', offset, len(payload), len(block), bank))
        offset += len(block)
    for block in packed:
        data.extend(block)
    path.write_bytes(data)


class CoopAssets(unittest.TestCase):
    def test_admission_and_roundtrip(self):
        required = list(coop_assets.ROOTS) + list(coop_assets.ANIMATIONS) + sorted(coop_assets.TEMPLATES)
        required += [root[:-4] + ext for root in coop_assets.ROOTS for ext in ('.mdlhk', '.entsupp')]
        required += list(coop_assets.LIFECYCLE_RESOURCES)
        material = '$n:\\' + coop_assets.ROOTS[0] + '$0.skcsf'
        texture = 'characters\\cha_multiplayer\\cha_multiplayer_burners\\test.tga'
        spawn_texture, death_texture = 'particles\\gasexplosion01.tga', 'particles\\shapes\\ring.tga'
        resources = [(name, b'original\0', 0) for name in required]
        resources[0] = (required[0], material.upper().encode(), 0)
        for effect, image in zip(coop_assets.LIFECYCLE_RESOURCES, (spawn_texture, death_texture)):
            resources[required.index(effect)] = (effect, b'Particle_Texture ' + image.upper().encode(), 0)
        resources += [(material, texture.upper().encode(), 0), (texture, b'original pixels', 0),
                      (spawn_texture, b'spawn pixels', 0), (death_texture, b'earlier ring pixels', 0),
                      (death_texture, b'final ring pixels', 0),
                      ('40-01.ent', b'world', 0), ('hud\\menu.tga', b'ui', 0), ('dialogue.wav', b'audio', 0),
                      ('characters\\cha_multiplayer\\cha_multiplayer_burners\\unused.tga', b'unreferenced', 0)]
        with tempfile.TemporaryDirectory() as temp:
            source, output = Path(temp)/'source.ent', Path(temp)/'coopcharacters.lst'
            archive(source, resources)
            original = source.read_bytes()
            report = coop_assets.build(source, output)
            first = output.read_bytes()
            self.assertEqual(source.read_bytes(), original)
            self.assertEqual(report['names'], required + [material, texture, spawn_texture, death_texture, death_texture])
            raw, entries = parse_archive(output)
            self.assertEqual(struct.unpack_from('<HHH', raw, 0x12), (len(entries), 1, 0))
            self.assertEqual([unpack_entry(raw, e) for e in entries], [r[1] for r in resources[:-4]])
            self.assertEqual(report['dependencies'][coop_assets.LIFECYCLE_RESOURCES[0]], [spawn_texture])
            self.assertEqual(report['dependencies'][coop_assets.LIFECYCLE_RESOURCES[1]], [death_texture])
            coop_assets.build(source, output)
            self.assertEqual(first, output.read_bytes())
            # The earlier duplicate is emitted too: it cannot bypass payload
            # validation just because dependency traversal uses the final name.
            external = bytearray(original)
            ring_name = death_texture.encode() + b'\0'
            struct.pack_into('<I', external, external.index(ring_name) + len(ring_name) + 8, 0)
            source.write_bytes(external)
            with self.assertRaises(ValueError):
                coop_assets.build(source, output)
            self.assertEqual(first, output.read_bytes())
            # Old body-only archives cannot silently pass as complete, and the
            # particle references remain required even if their target is absent.
            for missing in (*coop_assets.LIFECYCLE_RESOURCES, *coop_assets.WEAPON_ROOTS, spawn_texture, death_texture):
                with self.subTest(missing=missing):
                    archive(source, [r for r in resources if r[0] != missing])
                    with self.assertRaises(ValueError):
                        coop_assets.build(source, output)
                    self.assertEqual(first, output.read_bytes())
            with self.assertRaises(ValueError):
                coop_assets.build(source, source)
            # A missing required animation must not publish a partial replacement.
            archive(source, [r for r in resources if r[0] != coop_assets.ANIMATIONS[0]])
            with self.assertRaises(ValueError):
                coop_assets.build(source, output)
            self.assertEqual(first, output.read_bytes())
            archive(source, [r for r in resources if r[0] != texture])
            with self.assertRaises(ValueError):
                coop_assets.build(source, output)
            self.assertEqual(first, output.read_bytes())
            archive(source, resources + [resources[0]])
            with self.assertRaises(ValueError):
                coop_assets.build(source, output)
            self.assertEqual(first, output.read_bytes())
            unsafe = resources.copy()
            unsafe[0] = (required[0], b'hud\\menu.tga', 0)
            archive(source, unsafe)
            with self.assertRaises(ValueError):
                coop_assets.build(source, output)
            self.assertEqual(first, output.read_bytes())
            # Admission is restricted to these effects, not the whole particle
            # family. A new referenced effect texture requires explicit review.
            unsafe = resources.copy()
            effect = coop_assets.LIFECYCLE_RESOURCES[0]
            unsafe[required.index(effect)] = (effect, b'Particle_Texture Particles\\unreviewed.tga', 0)
            unsafe.append(('particles\\unreviewed.tga', b'other pixels', 0))
            archive(source, unsafe)
            with self.assertRaises(ValueError):
                coop_assets.build(source, output)
            self.assertEqual(first, output.read_bytes())
            self.assertFalse(coop_assets.admitted('multiplayer\\unreviewed.simpleparticlesystem'))


if __name__ == '__main__':
    unittest.main()
