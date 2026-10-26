"""Validate and atomically publish optional Spanish voice assets."""
import json
import re
import uuid

from disc import check_cancel, sha
from shader_seed import unlinked


def asset_name(name):
    return isinstance(name, str) and bool(re.fullmatch(
        r'(?:[1-9][0-9]?-01\.ent|[1-9][0-9]?\.saf|movies/[a-z0-9_]+\.xmv)', name))


def write_manifest(folder):
    files = {}
    for path in sorted(folder.rglob('*')):
        unlinked(path, root=folder)
        if path.is_dir():
            if path != folder / 'movies':
                raise ValueError('Unexpected Spanish voice directory.')
            continue
        name = path.relative_to(folder).as_posix()
        if not asset_name(name) or not 0 < path.stat().st_size <= 2 * 1024**3:
            raise ValueError('Invalid generated Spanish voice file: ' + name)
        files[name] = {'size': path.stat().st_size, 'sha256': sha(path)}
    from spanish_text import FILES
    if ({name for name in files if name.endswith('.ent')} != set(FILES) or
            not any(name.endswith('.saf') for name in files) or not 28 <= len(files) <= 128):
        raise ValueError('Spanish voice conversion is incomplete.')
    text = f'UCSPANISHVOICES 1 {len(files)}\n'
    text += ''.join(f"{name} {row['size']}\n" for name, row in files.items())
    (folder / 'manifest.txt').write_text(text, encoding='ascii', newline='\n')
    return files


def validate(data, text_sha):
    """Fast startup admission; full output hashes are checked during conversion."""
    folder = data / 'xbe-runtime/translations/es-voices'
    unlinked(folder, root=data)
    if not folder.exists():
        return
    marker = folder / 'install.json'
    unlinked(marker, root=data)
    if not marker.is_file() or marker.stat().st_size > 128 * 1024:
        raise ValueError('Spanish voices are incomplete. Import them again from your PS2 ISO in Setup.')
    info = json.loads(marker.read_text(encoding='utf-8'))
    if info.get('version') != 1 or info.get('text_sha256') != text_sha:
        raise ValueError('Spanish voices need updating. Import them again from your PS2 ISO in Setup; your saves are kept.')
    files = info.get('files')
    if not isinstance(files, dict) or not 28 <= len(files) <= 128:
        raise ValueError('Invalid installed Spanish voice list.')
    text = f'UCSPANISHVOICES 1 {len(files)}\n'
    for name, row in files.items():
        if (not asset_name(name) or not isinstance(row, dict) or
                type(row.get('size')) is not int or not 0 < row['size'] <= 2 * 1024**3 or
                not isinstance(row.get('sha256'), str) or not re.fullmatch('[0-9a-f]{64}', row['sha256'])):
            raise ValueError('Invalid installed Spanish voice identity.')
        path = folder / name
        unlinked(path, root=data)
        if not path.is_file() or path.stat().st_size != row['size']:
            raise ValueError('An imported Spanish voice file is missing or damaged: ' + name)
        text += f"{name} {row['size']}\n"
    index = folder / 'manifest.txt'
    unlinked(index, root=data)
    if (not index.is_file() or index.stat().st_size > 16384 or
            index.read_bytes() != text.encode('ascii')):
        raise ValueError('The installed Spanish voice manifest is damaged. Import voices again from your PS2 ISO.')


def publish(stage, destination, cancel):
    backup = destination.parent / ('.previous-es-voices-' + uuid.uuid4().hex)
    previous = installed = False
    try:
        check_cancel(cancel)
        if destination.exists():
            destination.rename(backup)
            previous = True
        stage.rename(destination)
        installed = True
        check_cancel(cancel)
    except BaseException:
        if installed:
            destination.rename(stage)
        if previous:
            backup.rename(destination)
        raise

