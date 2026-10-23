"""Bounds and identities for the bundled, read-only Direct3D shader seed."""
from pathlib import Path
import hashlib
import re
import stat
import struct

ENTRY_LIMIT = 2048
BYTE_LIMIT = 64 * 1024 * 1024
FILE_LIMIT = 18 * 1024 * 1024 + 48


def filename(name):
    return isinstance(name, str) and re.fullmatch(r'v1-[0-9a-f]{16}\.bin', name)


def unlinked(path):
    for parent in (path, *path.parents):
        try:
            info = parent.lstat()
        except FileNotFoundError:
            continue
        if (stat.S_ISLNK(info.st_mode) or
                getattr(info, 'st_file_attributes', 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT):
            raise ValueError('Shader seed paths must not contain links or junctions.')


def validate(directory, files):
    """Only hashed flat cache records are copied; the runtime validates their DXBC."""
    if not isinstance(files, dict) or len(files) > ENTRY_LIMIT:
        raise ValueError('Invalid bundled shader seed list.')
    directory = Path(directory).absolute()
    unlinked(directory)
    total = 0
    for name, digest in files.items():
        if (not filename(name) or not isinstance(digest, str) or
                not re.fullmatch(r'[0-9a-f]{64}', digest)):
            raise ValueError('Invalid bundled shader seed identity.')
        path = directory / name
        unlinked(path)
        if not path.is_file() or not 48 < path.stat().st_size <= FILE_LIMIT:
            raise ValueError('The shader seed is incomplete or damaged. Extract the entire ZIP again.')
        total += path.stat().st_size
        if total > BYTE_LIMIT:
            raise ValueError('Bundled shader seed exceeds the startup budget.')
        with path.open('rb') as file:
            header = struct.unpack('<10IQ', file.read(48))
            magic, version, contract, source_bytes, code_bytes = header[0], header[1], header[2], header[8], header[9]
            if (magic != 0x5343564e or version != 1 or contract != 0x55434431 or
                    not 0 < source_bytes <= 2 * 1024 * 1024 or
                    not 0 < code_bytes <= 16 * 1024 * 1024 or
                    path.stat().st_size != 48 + source_bytes + code_bytes or
                    file.read(16) != b'UC-D3D12-DXBC-1:'):
                raise ValueError('Invalid portable shader seed record.')
            file.seek(48 + source_bytes)
            if file.read(4) != b'DXBC':
                raise ValueError('The shader seed must not contain GPU-specific pipeline data.')
            file.seek(0)
            if hashlib.file_digest(file, 'sha256').hexdigest() != digest:
                raise ValueError('The shader seed is incomplete or damaged. Extract the entire ZIP again.')
    return files
