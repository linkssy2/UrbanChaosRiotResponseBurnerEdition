"""Bounded access to the supported PAL PS2 disc and its mono Sony ADPCM voices."""
from array import array
from pathlib import Path
import hashlib
import re
import struct
import sys

from shader_seed import unlinked

READ_LIMIT = 64 * 1024 * 1024
DIRECTORY_SHA256 = 'bd8a24393dfef7c0d6d4b2a1b81510651fc36d35e881a685972a6646c3eb537d'
EXECUTABLE_SHA256 = '9b8209299cf7bb1043d9a369f15a0a7c7ee16f0e13b747935f2ddca706de6193'
PARTS = ('GAME.WAD', 'GAME.000', 'GAME.001', 'GAME.002')


def _name(raw):
    name = raw.decode('ascii').split(';', 1)[0]
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]{0,127}', name):
        raise ValueError('Invalid PS2 disc filename.')
    return name.lower()


def parse_directory(data, total):
    if len(data) < 8:
        raise ValueError('Truncated PS2 game directory.')
    version, count = struct.unpack_from('<II', data)
    if version != 2 or not 0 < count <= 4096 or len(data) != 8 + count * 72:
        raise ValueError('Unsupported PS2 game directory.')
    entries, previous = {}, 0
    for index in range(count):
        raw, size, sector = struct.unpack_from('<64sII', data, 8 + index * 72)
        if b'\0' not in raw:
            raise ValueError('Unterminated PS2 filename.')
        name = _name(raw.split(b'\0', 1)[0])
        offset = sector * 2048
        if (name in entries or not size or offset != (previous + 2047) // 2048 * 2048 or
                offset + size > total):
            raise ValueError('Invalid PS2 game file extent.')
        entries[name] = (offset, size)
        previous = offset + size
    if previous != total:
        raise ValueError('PS2 game directory does not cover its containers.')
    return entries


class Ps2Disc:
    """Read GAME.WAD and numbered continuations without extracting a second disc."""
    def __init__(self, source):
        self._handles = []
        self._parts = []
        try:
            source = Path(source).absolute()
            unlinked(source, root=source if source.is_dir() else source.parent)
            if source.is_dir():
                folder = source if (source / 'GAME.DIR').exists() else source / 'URBAN'
                unlinked(folder, root=source)
                files = {}
                for path in folder.iterdir():
                    unlinked(path, root=source)
                    if path.is_file():
                        key = path.name.lower()
                        if key in files:
                            raise ValueError('Duplicate PS2 source filename.')
                        files[key] = path
                def open_file(name):
                    path = files[name.lower()]
                    handle = path.open('rb')
                    self._handles.append(handle)
                    return handle, 0, path.stat().st_size
                config = folder.parent / 'SYSTEM.CNF'
                unlinked(config, root=folder.parent)
                if config.stat().st_size > 4096:
                    raise ValueError('Invalid PS2 boot configuration.')
                boot = config.read_bytes()
            else:
                handle = source.open('rb')
                self._handles.append(handle)
                self._iso_size = source.stat().st_size
                files, boot = self._iso_files(handle)
                def open_file(name):
                    offset, size = files[name.lower()]
                    return handle, offset, size
            if (not re.search(rb'BOOT2\s*=\s*cdrom0:\\SLES_539\.91;1', boot, re.I) or
                    not re.search(rb'VMODE\s*=\s*PAL', boot, re.I)):
                raise ValueError('Spanish voices require the supported PAL SLES-53991 PS2 disc.')
            directory = self._slice(*open_file('GAME.DIR'))
            executable = self._slice(*open_file('URBAN.ELF'))
            if (hashlib.sha256(directory).hexdigest() != DIRECTORY_SHA256 or
                    hashlib.sha256(executable).hexdigest() != EXECUTABLE_SHA256):
                raise ValueError('Unsupported or damaged PAL PS2 disc revision.')
            self._parts = [open_file(name) for name in PARTS]
            self.entries = parse_directory(directory, sum(part[2] for part in self._parts))
        except BaseException:
            self.close()
            raise

    @staticmethod
    def _slice(handle, offset, size):
        if not 0 <= size <= READ_LIMIT or offset < 0:
            raise ValueError('Excessive PS2 read size.')
        handle.seek(offset)
        data = handle.read(size)
        if len(data) != size:
            raise ValueError('Truncated PS2 source file.')
        return data

    def _iso_files(self, handle):
        def read(offset, size):
            if offset < 0 or offset + size > self._iso_size:
                raise ValueError('PS2 ISO extent is outside the image.')
            return self._slice(handle, offset, size)
        def both(record, offset, width):
            a = int.from_bytes(record[offset:offset + width], 'little')
            b = int.from_bytes(record[offset + width:offset + 2 * width], 'big')
            if a != b:
                raise ValueError('Inconsistent PS2 ISO extent.')
            return a
        def extent(record):
            if len(record) < 34 or record[1] or record[26] or record[27] or record[25] & ~2:
                raise ValueError('Unsupported PS2 ISO record.')
            offset, size = both(record, 2, 4) * 2048, both(record, 10, 4)
            if offset + size > self._iso_size or both(record, 28, 2) != 1:
                raise ValueError('Invalid PS2 ISO file extent.')
            return offset, size
        def directory(record):
            offset, size = extent(record)
            if not record[25] & 2 or not 0 < size <= 4 * 1024 * 1024:
                raise ValueError('Invalid PS2 ISO directory.')
            data, result, at = read(offset, size), {}, 0
            while at < size:
                length = data[at]
                if not length:
                    at = (at // 2048 + 1) * 2048
                    continue
                if length < 34 or at + length > size or at % 2048 + length > 2048:
                    raise ValueError('Truncated PS2 ISO directory record.')
                row = data[at:at + length]
                at += length
                if 33 + row[32] > length:
                    raise ValueError('Invalid PS2 ISO filename length.')
                raw = row[33:33 + row[32]]
                if raw in (b'\0', b'\1'):
                    continue
                key = _name(raw)
                if key in result or len(result) >= 4096:
                    raise ValueError('Duplicate or excessive PS2 ISO directory.')
                extent(row)
                result[key] = row
            return result
        primary = None
        for sector in range(16, 48):
            descriptor = read(sector * 2048, 2048)
            if descriptor[1:7] != b'CD001\1':
                raise ValueError('The selected file is not a supported PS2 ISO.')
            if descriptor[0] == 1:
                primary = descriptor
                break
            if descriptor[0] == 255:
                break
        if primary is None or both(primary, 128, 2) != 2048:
            raise ValueError('Missing PS2 ISO primary volume.')
        if both(primary, 80, 4) * 2048 > self._iso_size:
            raise ValueError('Truncated PS2 ISO volume.')
        root = directory(primary[156:156 + primary[156]])
        urban = directory(root['urban'])
        boot_offset, boot_size = extent(root['system.cnf'])
        if boot_size > 4096 or root['system.cnf'][25] & 2:
            raise ValueError('Invalid PS2 boot configuration.')
        return ({name: extent(row) for name, row in urban.items() if not row[25] & 2},
                read(boot_offset, boot_size))

    def size(self, name):
        return self.entries[name.lower()][1]

    def read(self, name, offset=0, size=None):
        start, length = self.entries[name.lower()]
        size = length - offset if size is None else size
        if not 0 <= offset <= length or not 0 <= size <= min(READ_LIMIT, length - offset):
            raise ValueError('PS2 read is outside its named file.')
        offset += start
        chunks = []
        for handle, base, part_size in self._parts:
            if offset >= part_size:
                offset -= part_size
                continue
            count = min(size, part_size - offset)
            chunks.append(self._slice(handle, base + offset, count))
            size -= count
            offset = 0
            if not size:
                return b''.join(chunks)
        if size:
            raise ValueError('Truncated PS2 game containers.')
        return b''

    def close(self):
        for handle in self._handles:
            handle.close()
        self._handles.clear()

    def __enter__(self):
        return self

    def __exit__(self, *unused):
        self.close()


def decode_adpcm(data, cancel=None):
    """Decode one mono clip; coefficients and truncation verified against FFmpeg."""
    if len(data) % 16 or len(data) > READ_LIMIT:
        raise ValueError('Invalid Sony ADPCM clip length.')
    coefficients = ((0, 0), (60, 0), (115, -52), (98, -55), (122, -60))
    pcm = array('h')
    first = second = 0
    for offset in range(0, len(data), 16):
        if cancel and offset % (16 * 1024) == 0:
            cancel()
        header, flags = data[offset:offset + 2]
        predictor, shift = header >> 4, header & 15
        if predictor >= len(coefficients) or flags > 7:
            raise ValueError('Invalid Sony ADPCM frame.')
        a, b = coefficients[predictor]
        for byte in data[offset + 2:offset + 16]:
            for nibble in (byte & 15, byte >> 4):
                value = 0
                if flags != 7:
                    prediction = first * a + second * b
                    prediction = prediction // 64 if prediction >= 0 else -(-prediction // 64)
                    value = (((nibble if nibble < 8 else nibble - 16) << 12) >> shift) + prediction
                pcm.append(max(-32768, min(32767, value)))
                second, first = first, value
    if sys.byteorder != 'little':
        pcm.byteswap()
    if cancel:
        cancel()
    return pcm.tobytes()


def trim_adpcm(data):
    """Remove sector padding after a speech clip's terminating frame."""
    if not data or len(data) % 16 or len(data) > READ_LIMIT:
        raise ValueError('Invalid Sony ADPCM speech span.')
    for offset in range(0, len(data), 16):
        predictor, flags = data[offset] >> 4, data[offset + 1]
        if predictor > 4 or flags not in (0, 1, 2):
            raise ValueError('Unsupported Sony ADPCM speech frame.')
        if flags == 1:
            end = offset + 16
            if any(data[end:]):
                raise ValueError('Nonzero data follows the Sony ADPCM speech terminator.')
            return data[:end]
    raise ValueError('Missing Sony ADPCM speech terminator.')
