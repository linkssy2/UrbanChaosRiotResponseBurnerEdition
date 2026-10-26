"""Install only the separate converted world-texture ZIP; never extract arbitrary paths."""
from pathlib import Path
import hashlib
import re
import shutil
import struct
import uuid
import zipfile
from disc import check_cancel, progress


def records(text):
    rows = []
    seen = set()
    for line in text.splitlines():
        fields = line.split()
        if len(fields) != 6 or len(line) > 256 or len(rows) >= 4096:
            raise ValueError('Invalid HD texture manifest')
        w, h, fmt, size = map(int, fields[:4])
        if (not 0 < w <= 4096 or not 0 < h <= 4096 or w & (w-1) or h & (h-1) or
                fmt not in (12, 14, 15) or not 0 < size <= 32*1024*1024 or
                any(not re.fullmatch('[0-9a-f]{64}', v) for v in fields[4:])):
            raise ValueError('Invalid HD texture identity')
        identity = tuple(fields[:5])
        if identity in seen:
            raise ValueError('Duplicate HD texture identity')
        seen.add(identity)
        rows.append((w, h, fields[5]))
    if not rows:
        raise ValueError('Empty HD texture pack')
    return rows


def dimensions(payload):
    if len(payload) < 20 or payload[:8] != b'UCWT0001':
        raise ValueError('Invalid HD texture data')
    w, h, count = struct.unpack_from('<III', payload, 8)
    if (not 0 < w <= 4096 or not 0 < h <= 4096 or w & (w-1) or h & (h-1) or
            count != max(w, h).bit_length()):
        raise ValueError('Invalid HD texture dimensions')
    width, height, size = w, h, 20
    for _ in range(count):
        size += ((w+3)//4)*((h+3)//4)*16
        w, h = max(1, w//2), max(1, h//2)
    if len(payload) != size:
        raise ValueError('Incomplete HD texture mipmaps')
    return width, height


def install(archive, data, cancel):
    from import_game import unlinked_path
    unlinked_path(data, root=data)
    runtime = data / 'xbe-runtime'
    unlinked_path(runtime, root=data)
    runtime.mkdir(parents=True, exist_ok=True)
    target = runtime / 'world-textures'
    unlinked_path(target, root=data)
    stage = runtime / ('.textures-' + uuid.uuid4().hex)
    backup = runtime / ('.textures-old-' + uuid.uuid4().hex)
    stage.mkdir()
    published = False
    try:
        check_cancel(cancel)
        with zipfile.ZipFile(archive) as z:
            files = z.infolist()
            if len(files) > 4100 or sum(f.file_size for f in files) > 2*1024**3:
                raise ValueError('HD texture archive is too large')
            names = [f.filename for f in files]
            if len(set(names)) != len(names):
                raise ValueError('Duplicate archive entries')
            for f in files:
                if ((f.external_attr >> 16) & 0o170000) == 0o120000 or f.flag_bits & 1:
                    raise ValueError('Links and encrypted files are not supported')
                if f.filename not in ('README.txt', 'world-textures/', 'world-textures/manifest.txt') and not re.fullmatch(r'world-textures/[0-9a-f]{64}\.uctex', f.filename):
                    raise ValueError('This is not a Burner Edition HD game texture download')
                limit = 24*1024*1024 if f.filename.endswith('.uctex') else 1024*1024
                if f.file_size > limit:
                    raise ValueError('Oversized HD texture entry')
            raw = z.read('world-textures/manifest.txt')
            rows = records(raw.decode('ascii'))
            expected = {digest for _, _, digest in rows}
            actual = {Path(n).stem for n in names if n.endswith('.uctex')}
            if expected != actual:
                raise ValueError('HD texture download is incomplete')
            if shutil.disk_usage(runtime).free < sum(f.file_size for f in files) + 1024*1024:
                raise ValueError('Not enough free space for HD textures')
            sizes = {}
            for i, digest in enumerate(sorted(expected)):
                check_cancel(cancel)
                payload = z.read('world-textures/' + digest + '.uctex')
                if hashlib.sha256(payload).hexdigest() != digest:
                    raise ValueError('HD texture checksum failed')
                sizes[digest] = dimensions(payload)
                (stage / (digest + '.uctex')).write_bytes(payload)
                progress('Installing HD game textures', int(98*(i+1)/len(expected)))
            for w, h, digest in rows:
                ww, hh = sizes[digest]
                if ww < w or hh < h or ww*h != hh*w:
                    raise ValueError('HD texture aspect ratio mismatch')
            (stage / 'manifest.txt').write_bytes(raw)
        check_cancel(cancel)
        if target.exists():
            target.rename(backup)
        try:
            stage.rename(target)
            published = True
        except BaseException:
            if backup.exists():
                backup.rename(target)
            raise
        progress('HD game textures installed. Enable them in Graphics after starting the game.', 100)
    finally:
        for path in ((stage, backup) if published else (stage,)):
            if path.exists():
                assert path.resolve().parent == runtime.resolve() and path.name.startswith('.textures-')
                shutil.rmtree(path)
