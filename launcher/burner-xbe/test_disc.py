"""XDVDFS format and hostile-image checks, without importing legacy ISO tools."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from disc import Disc, Entry, MAGIC, PARTITIONS, SECTOR, XisoError


def record(name, sector=64, size=4, attr=0, left=0, right=0):
    encoded = name.encode('latin-1')
    return struct.pack('<HHIIBB', left, right, sector, size, attr, len(encoded)) + encoded


def image(path, table, base=0, extra=()):
    header = bytearray(SECTOR)
    header[:20] = header[0x7EC:] = MAGIC
    struct.pack_into('<II', header, 0x14, 40, len(table))
    with path.open('wb') as stream:
        stream.seek(base + 32 * SECTOR)
        stream.write(header)
        stream.seek(base + 40 * SECTOR)
        stream.write(table)
        stream.seek(base + 64 * SECTOR)
        stream.write(b'game')
        for sector, data in extra:
            stream.seek(base + sector * SECTOR)
            stream.write(data)


class DiscChecks(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.path = Path(self.scratch.name) / 'fixture.iso'

    def entries(self):
        with Disc(self.path) as disc:
            return disc.entries()

    def test_partition_layouts_and_payload(self):
        for base in PARTITIONS:
            with self.subTest(base=hex(base)):
                image(self.path, record('Default.xbe'), base)
                with Disc(self.path) as disc:
                    self.assertEqual(disc.base, base)
                    original, entry = disc.entries()['default.xbe']
                    self.assertEqual(original, 'Default.xbe')
                    self.assertEqual(disc.read(entry), b'game')
                self.assertTrue(disc._f.closed)
        self.assertNotIn('xdvdfs', sys.modules)

    def test_tree_links_not_physical_record_order(self):
        table = bytearray(b'\xFF' * 256)
        for offset, entry in ((0, record('middle', left=32, right=16)),
                              (64, record('z-last')), (128, record('a-first'))):
            table[offset:offset + len(entry)] = entry
        image(self.path, table)
        self.assertEqual(set(self.entries()), {'middle', 'a-first', 'z-last'})

    def test_nested_and_empty_directories(self):
        table = bytearray(128)
        for offset, entry in ((0, record('Assets', 42, SECTOR, 0x10, right=8)),
                              (32, record('Empty', 0, 0, 0x10, right=16)),
                              (64, record('AlsoEmpty', 0, 0, 0x10))):
            table[offset:offset + len(entry)] = entry
        child = record('part.bin') + b'\xFF' * (SECTOR - len(record('part.bin')))
        image(self.path, table, extra=[(42, child)])
        self.assertEqual(set(self.entries()), {'assets/part.bin'})
        for empty in (b'', b'\xFF' * SECTOR):
            image(self.path, empty)
            self.assertEqual(self.entries(), {})

    def test_both_volume_signatures_and_handle_cleanup(self):
        image(self.path, record('ok'))
        with self.path.open('r+b') as stream:
            stream.seek(32 * SECTOR + 0x7EC)
            stream.write(b'BAD')
        handle = self.path.open('rb')
        with patch('disc.open', return_value=handle, create=True):
            with self.assertRaises(XisoError):
                Disc(self.path)
        self.assertTrue(handle.closed)
        self.path.write_bytes(MAGIC)
        with self.assertRaises(XisoError):
            Disc(self.path)

    def test_root_bounds(self):
        for sector, size in ((40, 16 * 1024 * 1024 + 1), (0xFFFFFFFF, SECTOR), (40, 0xFFFFFFFF)):
            image(self.path, record('ok'))
            with self.path.open('r+b') as stream:
                stream.seek(32 * SECTOR + 0x14)
                stream.write(struct.pack('<II', sector, size))
            with self.subTest(sector=sector, size=size), self.assertRaises(XisoError):
                Disc(self.path)

    def test_malformed_or_overlapping_entries(self):
        loop = bytearray(64)
        loop[:len(record('first', right=8))] = record('first', right=8)
        loop[32:32 + len(record('loop', right=8))] = record('loop', right=8)
        alias = bytearray(loop)
        struct.pack_into('<HH', alias, 0, 8, 8)
        struct.pack_into('<HH', alias, 32, 0, 0)
        cases = [b'\0' * 13, record(''), record('long')[:-1],
                 record('out-of-range', right=0xFFFE), record('overlap', right=4),
                 bytes(loop), bytes(alias), b'\xFF\xFF' + b'\0' * 30]
        for table in cases:
            with self.subTest(table=table.hex()[:60]):
                image(self.path, table)
                with self.assertRaises(XisoError):
                    self.entries()

    def test_unsafe_names_case_alias_and_file_bounds(self):
        for name in ('..', '../file', 'a\\b', 'x:y', 'NUL.txt', 'trailing.', 'a\x01'):
            with self.subTest(name=name):
                image(self.path, record(name))
                with self.assertRaises(ValueError):
                    self.entries()
        table = bytearray(64)
        for at, entry in ((0, record('Item', right=8)), (32, record('ITEM'))):
            table[at:at + len(entry)] = entry
        image(self.path, table)
        with self.assertRaisesRegex(XisoError, 'Duplicate'):
            self.entries()
        image(self.path, record('outside', 0xFFFFFFFF, 0xFFFFFFFF))
        with self.assertRaises(XisoError):
            self.entries()

    def test_directory_cycles_depth_and_entry_limit(self):
        image(self.path, record('loop', 40, 18, 0x10))
        with self.assertRaises(XisoError):
            self.entries()
        tables = [record('dir', 41 + i, SECTOR, 0x10) for i in range(18)]
        tables = [table.ljust(SECTOR, b'\xFF') for table in tables]
        image(self.path, tables[0], extra=list(enumerate(tables[1:], 41)))
        with self.assertRaisesRegex(XisoError, 'directory tree'):
            self.entries()
        table = bytearray(10001 * 20)
        for index in range(10001):
            entry = record(f'{index:04x}', 0, 0, right=(index + 1) * 5 if index < 10000 else 0)
            table[index * 20:index * 20 + len(entry)] = entry
        # This table spans sector 64, so place the payload before the table write.
        image(self.path, b'', extra=[(40, table)])
        with self.path.open('r+b') as stream:
            stream.seek(32 * SECTOR + 0x18)
            stream.write(struct.pack('<I', len(table)))
        with self.assertRaisesRegex(XisoError, 'Excessive ISO directory'):
            self.entries()

    def test_read_rejects_invalid_range_and_truncated_file(self):
        image(self.path, record('ok'))
        with Disc(self.path) as disc:
            for entry in (Entry('negative', -1, 1, False), Entry('negative', 0, -1, False),
                          Entry('outside', 65, 1, False), Entry('folder', 40, 1, True)):
                with self.subTest(entry=entry), self.assertRaises(XisoError):
                    disc.read(entry)
            with self.path.open('r+b') as stream:
                stream.truncate(64 * SECTOR)
            with self.assertRaises(XisoError):
                disc.read(Entry('truncated', 64, 4, False))


if __name__ == '__main__':
    unittest.main()
