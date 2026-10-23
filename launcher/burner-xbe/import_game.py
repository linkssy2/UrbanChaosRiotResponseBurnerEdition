"""Verified local ISO import and isolated D3D12 launch; release has no disc payloads."""
from pathlib import Path
from collections import deque
import argparse
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import uuid
import tomllib
import shader_seed
import spanish_text

from disc import Disc, XisoError, BLOCK, sha, supported_hash, progress, check_cancel, safe_name

ROOT = Path(__file__).resolve().parent.parent
RUNTIME_NAME = 'UrbanChaosXbeTrial.exe'
RUNTIME_FILES = (RUNTIME_NAME, 'zlib1.dll', 'libusb-1.0.dll', 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')
ARTWORK_LIMIT = 32 * 1024 * 1024


def unlinked_path(path):
    """Reject redirected parents too, before reading or publishing portable state."""
    path = Path(path).absolute()
    for part in (path, *path.parents):
        try:
            info = part.lstat()
        except FileNotFoundError:
            continue
        if (stat.S_ISLNK(info.st_mode) or
                getattr(info, 'st_file_attributes', 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT):
            raise ValueError('Game data paths must not contain links or junctions: ' + str(part))


def copy_state(source, dest, cancel):
    """Copy the allowlisted save/config tree without following filesystem links."""
    check_cancel(cancel)
    unlinked_path(source)
    unlinked_path(dest)
    info = source.stat(follow_symlinks=False)
    if stat.S_ISDIR(info.st_mode):
        dest.mkdir()
        for entry in source.iterdir():
            safe_name(entry.name)
            copy_state(entry, dest / entry.name, cancel)
    elif stat.S_ISREG(info.st_mode) and info.st_nlink == 1:
        shutil.copy2(source, dest, follow_symlinks=False)
        unlinked_path(source)
        unlinked_path(dest)
        after = source.stat(follow_symlinks=False)
        copied = dest.stat(follow_symlinks=False)
        if (not stat.S_ISREG(copied.st_mode) or copied.st_size != info.st_size or
                (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns) !=
                (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns)):
            raise ValueError('Previous game data changed during import. Close the game and try again.')
    else:
        raise ValueError('Previous game data contains an unsupported linked or special file: ' + str(source))
    check_cancel(cancel)


def stage_legacy_state(data, stage, cancel):
    """First ISO import may copy old saves; launch/status never consult AppData."""
    if data.resolve() != (ROOT / 'GameData').resolve():
        return None
    runtime = data / 'xbe-runtime'
    unlinked_path(runtime)
    if runtime.exists():
        return None
    local = os.environ.get('LOCALAPPDATA')
    if not local or not Path(local).is_absolute():
        return None
    legacy = Path(local) / 'UrbanChaosBurnerEdition'
    source = legacy / 'xbe-runtime'
    unlinked_path(source)
    if not source.exists():
        return None
    if not source.is_dir() or not ready(legacy):
        raise ValueError('The previous game data is incomplete or incompatible. Your old saves were kept; repair the previous install before importing them.')
    check_cancel(cancel)
    staged = stage / '.portable-runtime'
    staged.mkdir()
    progress('Copying previous saves and settings into GameData', 99)
    for entry in source.iterdir():
        is_directory = (entry.name == 'shader-cache' or
                        re.fullmatch(r'hdd(?:[1-9][0-9]*)?', entry.name))
        if not is_directory and entry.name not in ('settings.toml', 'xboxrecomp.ini'):
            continue
        unlinked_path(entry)
        if entry.is_dir() != bool(is_directory):
            raise ValueError('Invalid previous game state: ' + entry.name)
        copy_state(entry, staged / entry.name, cancel)
    return staged


def publish_import(data, stage, staged_runtime, cancel):
    """Commit assets and optional first-import state together, restoring on failure."""
    install = data / 'installed'
    runtime = data / 'xbe-runtime'
    backup = data / ('.previous-' + uuid.uuid4().hex)
    unlinked_path(install)
    unlinked_path(runtime)
    if staged_runtime is not None and runtime.exists():
        raise ValueError('Portable saves appeared during import. Retry to keep that existing state.')
    previous = copied_runtime = installed = False
    try:
        check_cancel(cancel)
        if install.exists():
            install.rename(backup)
            previous = True
        if staged_runtime is not None:
            staged_runtime.rename(runtime)
            copied_runtime = True
        check_cancel(cancel)
        stage.rename(install)
        installed = True
        check_cancel(cancel)
    except BaseException:
        try:
            if installed:
                install.rename(stage)
            if copied_runtime:
                runtime.rename(staged_runtime)
        finally:
            if previous and not install.exists():
                backup.rename(install)
        raise


def manifest():
    data = json.loads((ROOT / 'tools/disc-manifest.json').read_text())
    # The bundled manifest is a trust boundary too; never allow it to escape the stage.
    seen = set()
    for name, row in data['files'].items():
        parts = name.split('/')
        for part in parts:
            safe_name(part)
        if name.lower() in seen or not isinstance(row['size'], int) or row['size'] < 0:
            raise ValueError('Invalid bundled disc manifest')
        if len(row['sha256']) != 64 or any(c not in '0123456789abcdef' for c in row['sha256']):
            raise ValueError('Invalid bundled disc hash')
        seen.add(name.lower())
    if 'Default.xbe' not in data['files']:
        raise ValueError('The bundled disc manifest has no executable')
    return data


def import_iso(iso_path, data, cancel):
    """Verify the disc, then publish assets while preserving existing portable saves."""
    check_cancel(cancel)
    required = manifest()['files']
    unlinked_path(data)
    data.mkdir(parents=True, exist_ok=True)
    stage = data / ('.import-' + uuid.uuid4().hex)
    stage.mkdir()
    game = stage / 'game_files'
    game.mkdir()
    try:
        progress('Checking Xbox disc', 1)
        with Disc(iso_path) as disc:
            entries = disc.entries()
            for name, row in required.items():
                if name.lower() not in entries or entries[name.lower()][1].size != row['size']:
                    raise ValueError('This ISO is not a supported Urban Chaos disc revision.')
            entry = entries['default.xbe'][1]
            import hashlib
            if not supported_hash('Default.xbe', hashlib.sha256(disc.read(entry)).hexdigest(),
                                  required['Default.xbe']['sha256']):
                raise ValueError('Unsupported Xbox executable revision')
            total = sum(row['size'] for row in required.values())
            if shutil.disk_usage(data).free < total + 256 * BLOCK:
                raise ValueError('Not enough free space to import the game while keeping your current install.')
            done = 0
            for name, row in required.items():
                check_cancel(cancel)
                entry = entries[name.lower()][1]
                dest = game.joinpath(*name.split('/'))
                dest.parent.mkdir(parents=True, exist_ok=True)
                disc._f.seek(disc.base + entry.sector * 2048)
                digest = hashlib.sha256()
                left = entry.size
                with dest.open('xb') as output:
                    while left:
                        check_cancel(cancel)
                        chunk = disc._f.read(min(BLOCK, left))
                        if not chunk:
                            raise ValueError('Truncated ISO')
                        output.write(chunk)
                        digest.update(chunk)
                        left -= len(chunk)
                        done += len(chunk)
                if not supported_hash(name, digest.hexdigest(), row['sha256']):
                    raise ValueError('Disc verification failed: ' + name)
                progress('Importing game files', 2 + int(done * 96 / total))
        check_cancel(cancel)
        (stage / 'installed.json').write_text(json.dumps({
            'version': 1, 'disc_manifest': sha(ROOT / 'tools/disc-manifest.json'),
            'files': len(required), 'importer': 'burner-xbe-0.07'}))
        staged_runtime = stage_legacy_state(data, stage, cancel)
        publish_import(data, stage, staged_runtime, cancel)
        progress('Ready to play', 100)
    finally:
        if stage.exists():
            # Only remove this operation's private staging directory.
            assert stage.resolve().parent == data.resolve() and stage.name.startswith('.import-')
            shutil.rmtree(stage)


def ready(data):
    """Reuse verified 0.02 imports; no obsolete generated movie/material dependency."""
    install = data / 'installed'
    try:
        marker = json.loads((install / 'installed.json').read_text())
        if marker['disc_manifest'] != sha(ROOT / 'tools/disc-manifest.json'):
            return False
        required = manifest()['files']
        for name, row in required.items():
            path = install / 'game_files' / name
            if not path.is_file() or path.stat().st_size != row['size']:
                return False
        return supported_hash('Default.xbe', sha(install / 'game_files/Default.xbe'),
                              required['Default.xbe']['sha256'])
    except (OSError, ValueError, KeyError, TypeError):
        return False


def linked(path):
    return path.is_symlink() or path.is_junction()


def artwork_files(info):
    """Old releases have no artwork field; new ones contain only approved PNGs."""
    artwork = info.get('artwork', {})
    if not isinstance(artwork, dict) or len(artwork) > 512:
        raise ValueError('Invalid runtime artwork list')
    seen = set()
    for name, digest in artwork.items():
        if not isinstance(name, str) or not name.startswith('hd-ui/'):
            raise ValueError('Invalid runtime artwork path')
        leaf = name[6:]
        if (not 0 < len(leaf) < 128 or '..' in leaf or
                not re.fullmatch(r'[A-Za-z0-9_.-]+\.png', leaf) or name.lower() in seen):
            raise ValueError('Invalid runtime artwork filename')
        safe_name(leaf)
        if not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest):
            raise ValueError('Invalid runtime artwork hash')
        path = ROOT / 'runtime' / name
        if (linked(path.parent) or linked(path) or not path.is_file() or
                not 0 < path.stat().st_size <= ARTWORK_LIMIT or sha(path) != digest):
            raise ValueError('The release artwork is incomplete or damaged. Extract the entire ZIP again.')
        seen.add(name.lower())
    return artwork


def runtime_files():
    info = json.loads((ROOT / 'tools/runtime-manifest.json').read_text())
    if not isinstance(info, dict) or not isinstance(info.get('files'), dict) or set(info['files']) != set(RUNTIME_FILES):
        raise ValueError('Invalid runtime package file list')
    for name, digest in info['files'].items():
        if (not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest) or
                not (ROOT / 'runtime' / name).is_file() or sha(ROOT / 'runtime' / name) != digest):
            raise ValueError('The release runtime is incomplete or damaged. Extract the entire ZIP again.')
    artwork_files(info)
    shader_seed.validate(ROOT / 'runtime/shader-seed', info.get('shader_seed', {}))
    spanish_bundle(info)
    return info


def spanish_bundle(info=None):
    if info is None:
        info = json.loads((ROOT / 'tools/runtime-manifest.json').read_text())
    files = info.get('spanish', {})
    if not files:
        return None
    if not isinstance(files, dict) or set(files) != {'spanish/text.json', 'spanish/credits.txt'}:
        raise ValueError('Invalid bundled Spanish file list.')
    for name, digest in files.items():
        path = ROOT / 'runtime' / name
        unlinked_path(path)
        if (not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest) or
                not path.is_file() or not 0 < path.stat().st_size <= 8 * BLOCK or sha(path) != digest):
            raise ValueError('The bundled Spanish translation is damaged. Extract the entire release again.')
    path = ROOT / 'runtime/spanish/text.json'
    return path, spanish_text.load_bundle(path)


def ensure_spanish(data, info=None, cancel=None):
    """Generate only the managed translation; keep imported originals and saves untouched."""
    bundled = spanish_bundle(info)
    if bundled is None:
        return
    path, bundle = bundled
    original = data / 'installed/game_files'
    parent = data / 'xbe-runtime/translations'
    dest = parent / 'es-zarf'
    unlinked_path(original)
    unlinked_path(dest)
    check = lambda: check_cancel(cancel) if cancel is not None else None
    check()
    expected = ''.join(f"{row['file']} {row['size']}\n" for row in bundle['files'])
    paths = [dest / row['file'] for row in bundle['files']]
    for target in [dest / 'manifest.txt', *paths]:
        unlinked_path(target)
    if ((dest / 'manifest.txt').is_file() and
            (dest / 'manifest.txt').stat().st_size <= 8192 and
            (dest / 'manifest.txt').read_bytes().replace(b'\r\n', b'\n') == expected.encode('ascii') and
            all(p.is_file() and p.stat().st_size == row['size'] and sha(p) == row['sha256']
                for p, row in zip(paths, bundle['files']))):
        return
    check()
    parent.mkdir(parents=True, exist_ok=True)
    stage = parent / ('.spanish-' + uuid.uuid4().hex)
    backup = parent / ('.previous-es-zarf-' + uuid.uuid4().hex)
    moved = installed = False
    try:
        spanish_text.build(original, path, stage, cancel=check)
        check()
        if dest.exists():
            dest.rename(backup)
            moved = True
        stage.rename(dest)
        installed = True
        check()
    except BaseException:
        if installed:
            dest.rename(stage)
        if moved:
            backup.rename(dest)
        raise
    finally:
        if stage.exists():
            assert stage.resolve().parent == parent.resolve() and stage.name.startswith('.spanish-')
            shutil.rmtree(stage)


def language_settings(data, selected=None):
    """Read/write only the portable language key; preserve every other setting."""
    runtime=Path(data)/'xbe-runtime'
    config=runtime/'settings.toml'
    unlinked_path(config)
    text=config.read_text(encoding='utf-8') if config.exists() else ''
    settings=tomllib.loads(text)
    current=settings.get('language',0)
    if type(current) is not int or current not in range(5): current=0
    bundled = spanish_bundle()
    pack=runtime/'translations/es-zarf'
    available=False
    if bundled is None and (pack/'manifest.txt').is_file():
        unlinked_path(pack/'manifest.txt')
        rows=(pack/'manifest.txt').read_text().splitlines()
        names=set()
        for row in rows:
            fields=row.split()
            if len(fields)!=2 or not re.fullmatch(r'[0-9]{1,2}-01\.ent',fields[0]) or not fields[1].isdigit():break
            name,size=fields[0],int(fields[1]);path=pack/name
            unlinked_path(path)
            if name in names or not 200<=size<=64*1024*1024 or not path.is_file() or path.stat().st_size!=size:break
            names.add(name)
        else: available=len(names)==27
    if bundled is not None:
        available = ready(data)
    if selected is not None:
        if type(selected) is not int or selected not in range(5):raise ValueError('Invalid language selection.')
        if selected==4 and not available:raise ValueError('Import your supported Xbox ISO before selecting Spanish.')
        if selected==4 and bundled is not None:ensure_spanish(data)
        # Insert a root key before any TOML table; never change a similarly named table key.
        lines=text.splitlines(keepends=True);end=next((i for i,s in enumerate(lines) if s.lstrip().startswith('[')),len(lines))
        matches=[i for i in range(end) if re.match(r'\s*language\s*=',lines[i])]
        if len(matches)>1:raise ValueError('Duplicate language setting.')
        if matches:lines[matches[0]]='language = '+str(selected)+'\n'
        else:lines.insert(0,'language = '+str(selected)+'\n')
        updated=''.join(lines)
        check=tomllib.loads(updated)
        assert check==dict(settings,language=selected)
        runtime.mkdir(parents=True,exist_ok=True)
        temp=runtime/('.language-'+uuid.uuid4().hex)
        try:
            temp.write_text(updated,encoding='utf-8');os.replace(temp,config)
        finally:
            if temp.exists():temp.unlink()
        current=selected
    return {'language':current,'spanish_available':available}


def launch(data, language=None):
    """Keep new runtime settings/hdd separate; diagnostic environment never leaks in."""
    unlinked_path(data)
    if not ready(data):
        raise ValueError('Please import your supported Xbox ISO first.')
    info = runtime_files()
    ensure_spanish(data, info)
    data = data.resolve()
    runtime = data / 'xbe-runtime'
    if linked(runtime) or runtime.resolve().parent != data:
        raise ValueError('The runtime directory must stay inside the game data folder.')
    runtime.mkdir(parents=True, exist_ok=True)
    seed = info.get('shader_seed', {})
    files = {**info['files'], **info.get('artwork', {}),
             **{'shader-seed/' + name: digest for name, digest in seed.items()}}
    seed_directory = runtime / 'shader-seed'
    unlinked_path(seed_directory)
    obsolete_seed = [path for path in seed_directory.glob('v1-*.bin')
                     if shader_seed.filename(path.name) and path.name not in seed]
    for path in obsolete_seed:
        unlinked_path(path)
        if not path.is_file():
            raise ValueError('Invalid managed shader seed entry.')
    # Validate every destination before copying any part of an update.
    for name in files:
        dest = runtime / name
        if linked(dest) or linked(dest.parent) or not dest.resolve().is_relative_to(runtime):
            raise ValueError('Runtime update paths must stay inside the runtime folder.')
    for name, digest in files.items():
        dest = runtime / name
        if dest.is_file() and sha(dest) == digest:
            continue
        dest.parent.mkdir(parents=True, exist_ok=True)
        temp = dest.parent / ('.update-' + uuid.uuid4().hex)
        try:
            shutil.copy2(ROOT / 'runtime' / name, temp)
            if sha(temp) != digest:
                raise ValueError('Runtime copy verification failed')
            os.replace(temp, dest)
        finally:
            if temp.exists():
                temp.unlink()
    # Retire only the managed seed; never erase the player's learned shader cache.
    for path in obsolete_seed:
        path.unlink()
    if language is not None:language_settings(data,language)
    env = {k: v for k, v in os.environ.items()
           if not k.upper().startswith(('UCRR_', 'XBE_', 'XBOX_LOG_'))}
    env['UCRR_GAME_ASSETS'] = str(data / 'installed/game_files')
    # The release runtime is compiled quiet; only bounded errors survive a failed run.
    with open(os.devnull, 'wb') as null:
        proc = subprocess.Popen([str(runtime / RUNTIME_NAME)], cwd=runtime, env=env,
                                stdout=null, stderr=subprocess.PIPE,
                                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        tail = deque(maxlen=32)
        while True:
            chunk = proc.stderr.read(4096)
            if not chunk:
                break
            tail.append(chunk)
        code = proc.wait()
    if code:
        report = data / 'last-error.txt'
        header = ('Version: ' + info['version'] + '\nExit code: ' + hex(code & 0xffffffff) +
                  '\nRuntime SHA256: ' + info['files'][RUNTIME_NAME] + '\n')
        report.write_bytes(header.encode('utf-8') + b''.join(tail))
        raise RuntimeError('The game closed unexpectedly (code ' + hex(code & 0xffffffff) +
                           '). Error details were saved to ' + str(report))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=['import', 'status', 'launch', 'textures', 'language'])
    parser.add_argument('--data', type=Path, default=ROOT / 'GameData')
    parser.add_argument('--iso', type=Path)
    parser.add_argument('--pack', type=Path)
    parser.add_argument('--cancel', type=Path)
    parser.add_argument('--language', type=int, choices=range(5))
    args = parser.parse_args()
    try:
        if args.action == 'import':
            if args.iso is None or args.cancel is None:
                raise ValueError('Import requires an ISO and cancellation path.')
            import_iso(args.iso, args.data, args.cancel)
        elif args.action == 'textures':
            if args.pack is None or args.cancel is None:
                raise ValueError('Select the HD texture ZIP download.')
            from world_textures import install
            install(args.pack, args.data, args.cancel)
        elif args.action == 'status':
            return 0 if ready(args.data) else 2
        elif args.action == 'language':
            print(json.dumps(language_settings(args.data, args.language)))
        else:
            launch(args.data,args.language)
        return 0
    except Exception as error:
        progress(str(error), 0)
        return 1


if __name__ == '__main__':
    sys.exit(main())
