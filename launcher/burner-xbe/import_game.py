"""Verified local ISO import and isolated D3D12 launch; release has no disc payloads."""
from pathlib import Path
from collections import deque
import argparse
import importlib.util
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


def unlinked_path(path, *, root=None):
    """Check managed descendants without rejecting the host's mapped ancestors."""
    shader_seed.unlinked(path, root=root)


def copy_state(source, dest, cancel, *, source_root=None, dest_root=None):
    """Copy the allowlisted save/config tree without following filesystem links."""
    check_cancel(cancel)
    unlinked_path(source, root=source_root)
    unlinked_path(dest, root=dest_root)
    info = source.stat(follow_symlinks=False)
    if stat.S_ISDIR(info.st_mode):
        dest.mkdir()
        for entry in source.iterdir():
            safe_name(entry.name)
            copy_state(entry, dest / entry.name, cancel, source_root=source_root, dest_root=dest_root)
    elif stat.S_ISREG(info.st_mode) and info.st_nlink == 1:
        shutil.copy2(source, dest, follow_symlinks=False)
        unlinked_path(source, root=source_root)
        unlinked_path(dest, root=dest_root)
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
    unlinked_path(runtime, root=data)
    if runtime.exists():
        return None
    local = os.environ.get('LOCALAPPDATA')
    if not local or not Path(local).is_absolute():
        return None
    legacy = Path(local) / 'UrbanChaosBurnerEdition'
    source = legacy / 'xbe-runtime'
    unlinked_path(source, root=legacy)
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
        unlinked_path(entry, root=legacy)
        if entry.is_dir() != bool(is_directory):
            raise ValueError('Invalid previous game state: ' + entry.name)
        copy_state(entry, staged / entry.name, cancel, source_root=legacy, dest_root=data)
    return staged


def publish_import(data, stage, staged_runtime, cancel):
    """Commit assets and optional first-import state together, restoring on failure."""
    install = data / 'installed'
    runtime = data / 'xbe-runtime'
    backup = data / ('.previous-' + uuid.uuid4().hex)
    unlinked_path(install, root=data)
    unlinked_path(runtime, root=data)
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
    unlinked_path(ROOT / 'tools/disc-manifest.json', root=ROOT)
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
    unlinked_path(data, root=data)
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
        _ensure_coop_assets(game, data, cancel=cancel)
        (stage / 'installed.json').write_text(json.dumps({
            'version': 1, 'disc_manifest': sha(ROOT / 'tools/disc-manifest.json'),
            'files': len(required), 'importer': 'burner-xbe-0.08'}))
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
        unlinked_path(install / 'installed.json', root=data)
        marker = json.loads((install / 'installed.json').read_text())
        required = manifest()['files']
        if marker['disc_manifest'] != sha(ROOT / 'tools/disc-manifest.json'):
            return False
        for name, row in required.items():
            path = install / 'game_files' / name
            unlinked_path(path, root=data)
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
        unlinked_path(path, root=ROOT)
        if (linked(path.parent) or linked(path) or not path.is_file() or
                not 0 < path.stat().st_size <= ARTWORK_LIMIT or sha(path) != digest):
            raise ValueError('The release artwork is incomplete or damaged. Extract the entire ZIP again.')
        seen.add(name.lower())
    return artwork


def runtime_files():
    unlinked_path(ROOT / 'tools/runtime-manifest.json', root=ROOT)
    info = json.loads((ROOT / 'tools/runtime-manifest.json').read_text())
    if not isinstance(info, dict) or not isinstance(info.get('files'), dict) or set(info['files']) != set(RUNTIME_FILES):
        raise ValueError('Invalid runtime package file list')
    for name, digest in info['files'].items():
        unlinked_path(ROOT / 'runtime' / name, root=ROOT)
        if (not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest) or
                not (ROOT / 'runtime' / name).is_file() or sha(ROOT / 'runtime' / name) != digest):
            raise ValueError('The release runtime is incomplete or damaged. Extract the entire ZIP again.')
    artwork_files(info)
    shader_seed.validate(ROOT / 'runtime/shader-seed', info.get('shader_seed', {}), root=ROOT)
    spanish_bundle(info)
    return info


def spanish_bundle(info=None):
    if info is None:
        unlinked_path(ROOT / 'tools/runtime-manifest.json', root=ROOT)
        info = json.loads((ROOT / 'tools/runtime-manifest.json').read_text())
    files = info.get('spanish', {})
    if not files:
        return None
    if not isinstance(files, dict) or set(files) != {'spanish/text.json', 'spanish/credits.txt'}:
        raise ValueError('Invalid bundled Spanish file list.')
    for name, digest in files.items():
        path = ROOT / 'runtime' / name
        unlinked_path(path, root=ROOT)
        if (not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest) or
                not path.is_file() or not 0 < path.stat().st_size <= 8 * BLOCK or sha(path) != digest):
            raise ValueError('The bundled Spanish translation is damaged. Extract the entire release again.')
    path = ROOT / 'runtime/spanish/text.json'
    return path, spanish_text.load_bundle(path, root=ROOT)


def ensure_spanish(data, info=None, cancel=None):
    """Generate only the managed translation; keep imported originals and saves untouched."""
    bundled = spanish_bundle(info)
    if bundled is None:
        return
    path, bundle = bundled
    original = data / 'installed/game_files'
    parent = data / 'xbe-runtime/translations'
    dest = parent / 'es-zarf'
    unlinked_path(original, root=data)
    unlinked_path(dest, root=data)
    check = lambda: check_cancel(cancel) if cancel is not None else None
    check()
    expected = ''.join(f"{row['file']} {row['size']}\n" for row in bundle['files'])
    paths = [dest / row['file'] for row in bundle['files']]
    for target in [dest / 'manifest.txt', *paths]:
        unlinked_path(target, root=data)
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
        spanish_text.build(original, path, stage, cancel=check,
                           source_root=data, bundle_root=ROOT, output_root=data)
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


def coop_recipe(info=None, key='coop_assets'):
    """An optional local-derivation recipe, never bundled original resources."""
    if info is None:
        unlinked_path(ROOT / 'tools/runtime-manifest.json', root=ROOT)
        info = json.loads((ROOT / 'tools/runtime-manifest.json').read_text())
    if not isinstance(info, dict):
        raise ValueError('Invalid runtime package metadata.')
    if key not in info:
        return None  # Older packages do not derive these resources.
    recipe = info[key]
    if (not isinstance(recipe, dict) or set(recipe) != {'version', 'size', 'sha256'} or
            type(recipe['version']) is not int or recipe['version'] != 1 or
            type(recipe['size']) is not int or not 0xC8 < recipe['size'] <= 16 * BLOCK or
            not isinstance(recipe['sha256'], str) or
            not re.fullmatch('[0-9a-f]{64}', recipe['sha256'])):
        raise ValueError('Invalid co-op resource recipe.')
    return recipe


def _ensure_coop_archive(game, data, key, info=None, cancel=None):
    """Derive a managed LST; original disc files and player state stay untouched."""
    recipe = coop_recipe(info, key)
    if recipe is None:
        return
    check = lambda: check_cancel(cancel) if cancel is not None else None
    check()
    level = {'coop_turrets03': '3-01', 'coop_turrets08': '8-01'}.get(key)
    turret = level is not None
    injection = key == 'coop_injection'
    names = (level + '.ent',) if turret else (('1-01.ent',) if injection else ('40-01.ent',))
    sources = [game / name for name in names]
    dest = game / (key.replace('coop_', 'coop') + '.lst' if turret or injection else 'coopcharacters.lst')
    for path in (*sources, dest):
        unlinked_path(path, root=data)
    rows = manifest()['files']
    for source in sources:
        row = rows.get(source.name)
        if (row is None or not source.is_file() or source.stat().st_size != row['size'] or
                sha(source) != row['sha256']):
            raise ValueError('An original co-op resource archive is missing or changed. Import your supported Xbox ISO again.')
    if dest.exists() and not dest.is_file():
        raise ValueError('Invalid managed co-op resource path.')
    if dest.is_file() and dest.stat().st_size == recipe['size'] and sha(dest) == recipe['sha256']:
        return
    helper_name = ('coop_turret_assets.py' if turret else
                   'coop_injection_assets.py' if injection else 'coop_assets.py')
    for name in (helper_name, 'analyze_frontend.py'):
        helper = ROOT / 'tools' / name
        unlinked_path(helper, root=ROOT)
        if not helper.is_file():
            raise ValueError('The co-op resource setup is incomplete. Extract the entire build again.')
    temporary = game / ('.coop-characters-' + uuid.uuid4().hex)
    try:
        progress('Preparing co-op turret resources' if turret else
                 'Preparing partner healing animation' if injection else 'Preparing co-op character resources', 99)
        spec = importlib.util.spec_from_file_location('burner_' + key, ROOT / 'tools' / helper_name)
        builder = importlib.util.module_from_spec(spec)
        old_path = sys.path[:]
        try:
            sys.path.insert(0, str(ROOT / 'tools'))
            spec.loader.exec_module(builder)
        finally:
            sys.path[:] = old_path
        report = (builder.build_archive(game, temporary, level=level, cancel=check) if turret else
                  builder.build_archive(game, temporary, cancel=check) if injection else
                  builder.build(sources[0], temporary, cancel=check))
        check()
        if (report['sha256'] != recipe['sha256'] or report['size'] != recipe['size'] or
                temporary.stat().st_size != recipe['size'] or sha(temporary) != recipe['sha256'] or
                any(sha(source) != rows[source.name]['sha256'] for source in sources)):
            raise ValueError('Co-op resource verification failed. Your existing data was kept.')
        unlinked_path(dest, root=data)
        os.replace(temporary, dest)
    finally:
        if temporary.exists():
            temporary.unlink()


def _ensure_coop_assets(game, data, info=None, cancel=None):
    for key in ('coop_assets', 'coop_turrets03', 'coop_turrets08', 'coop_injection'):
        _ensure_coop_archive(game, data, key, info, cancel)


def ensure_coop_assets(data, info=None, cancel=None):
    _ensure_coop_assets(data / 'installed/game_files', data, info, cancel)


def language_settings(data, selected=None):
    """Read/write only the portable language key; preserve every other setting."""
    runtime=Path(data)/'xbe-runtime'
    config=runtime/'settings.toml'
    unlinked_path(config, root=data)
    text=config.read_text(encoding='utf-8') if config.exists() else ''
    settings=tomllib.loads(text)
    current=settings.get('language',0)
    if type(current) is not int or current not in range(5): current=0
    bundled = spanish_bundle()
    pack=runtime/'translations/es-zarf'
    available=False
    if bundled is None and (pack/'manifest.txt').is_file():
        unlinked_path(pack/'manifest.txt', root=data)
        rows=(pack/'manifest.txt').read_text().splitlines()
        names=set()
        for row in rows:
            fields=row.split()
            if len(fields)!=2 or not re.fullmatch(r'[0-9]{1,2}-01\.ent',fields[0]) or not fields[1].isdigit():break
            name,size=fields[0],int(fields[1]);path=pack/name
            unlinked_path(path, root=data)
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
    unlinked_path(data, root=data)
    if not ready(data):
        raise ValueError('Please import your supported Xbox ISO first.')
    info = runtime_files()
    ensure_coop_assets(data, info)
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
    unlinked_path(seed_directory, root=data)
    obsolete_seed = [path for path in seed_directory.glob('v1-*.bin')
                     if shader_seed.filename(path.name) and path.name not in seed]
    for path in obsolete_seed:
        unlinked_path(path, root=data)
        if not path.is_file():
            raise ValueError('Invalid managed shader seed entry.')
    # Validate every destination before copying any part of an update.
    for name in files:
        dest = runtime / name
        unlinked_path(dest, root=data)
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
    if (runtime / 'translations/es-voices').exists() and language_settings(data)['language'] == 4:
        from spanish_voice_setup import validate
        bundle = spanish_bundle(info)
        if bundle is not None:
            validate(data, sha(bundle[0]))
    env = {k: v for k, v in os.environ.items()
           if not k.upper().startswith(('UCRR_', 'XBE_', 'XBOX_LOG_')) and k.upper() != 'BURNER_AIM_LOG'}
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
    import setup_feedback

    class SetupParser(argparse.ArgumentParser):
        def error(self, message):
            raise ValueError(message)

    parser = SetupParser()
    parser.add_argument('action', choices=['import', 'status', 'launch', 'textures', 'voices', 'language'])
    parser.add_argument('--data', type=Path, default=ROOT / 'GameData')
    parser.add_argument('--iso', type=Path)
    parser.add_argument('--pack', type=Path)
    parser.add_argument('--cancel', type=Path)
    parser.add_argument('--language', type=int, choices=range(5))
    parser.add_argument('--interactive', action='store_true',
                        help='Show setup results and errors when launched through Steam')
    interactive = '--interactive' in sys.argv[1:]
    action = 'Setup'
    try:
        args = parser.parse_args()
        action = args.action
        if args.interactive and action in ('import', 'textures', 'voices'):
            setup_feedback.notify('Press OK to start ' + {'import': 'ISO import.', 'textures': 'HD texture installation.', 'voices': 'Spanish voice installation.'}[action] +
                '\n\nThis can take several minutes. Leave Steam running; another message will confirm success or explain any error.')
        if args.action == 'import':
            if args.iso is None or args.cancel is None:
                raise ValueError('Import requires an ISO and cancellation path.')
            import_iso(args.iso, args.data, args.cancel)
        elif args.action == 'textures':
            if args.pack is None or args.cancel is None:
                raise ValueError('Select the HD texture ZIP download.')
            from world_textures import install
            install(args.pack, args.data, args.cancel)
        elif args.action == 'voices':
            if args.iso is None or args.cancel is None or args.pack is not None:
                raise ValueError('Select your European PS2 ISO (SLES-53991) to import Spanish voices.')
            from spanish_voice_iso import install
            install(args.iso, args.data, args.cancel)
        elif args.action == 'status':
            imported = ready(args.data)
            if args.interactive:
                setup_feedback.notify('Game data is ready.' if imported else 'Import your supported Xbox ISO first.')
            return 0 if imported else 2
        elif args.action == 'language':
            print(json.dumps(language_settings(args.data, args.language)))
        else:
            launch(args.data,args.language)
        if args.interactive and action != 'launch':
            setup_feedback.notify({'import': 'ISO import completed successfully. Restore the normal play command in Steam.',
                'textures': 'HD textures installed successfully. Restore the normal play command in Steam.',
                'voices': 'Spanish voices installed. Select Spanish, then restore the normal play command in Steam.',
                'language': 'Language settings are ready. Restore the normal play command in Steam.'}[action])
        return 0
    except Exception as error:
        message = setup_feedback.report_error(ROOT, action, error)
        try:
            progress(message, 0)
        except (OSError, ValueError):
            pass
        if interactive:
            setup_feedback.notify(message, error=True)
        return 1


if __name__ == '__main__':
    sys.exit(main())
