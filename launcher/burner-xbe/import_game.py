"""Verified local ISO import and isolated D3D12 launch; release has no disc payloads."""
from pathlib import Path
from collections import deque
import argparse
import json
import os
import shutil
import subprocess
import sys
import uuid

from disc import Disc, XisoError, BLOCK, sha, supported_hash, progress, check_cancel, safe_name

ROOT = Path(__file__).resolve().parent.parent
RUNTIME_NAME = 'UrbanChaosXbeTrial.exe'
RUNTIME_FILES = (RUNTIME_NAME, 'zlib1.dll', 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')


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
    """Hash every disc file, then swap a complete stage without touching saves."""
    check_cancel(cancel)
    required = manifest()['files']
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
            'files': len(required), 'importer': 'burner-xbe-0.03'}))
        install = data / 'installed'
        backup = data / ('.previous-' + uuid.uuid4().hex)
        if install.exists():
            install.rename(backup)
        try:
            stage.rename(install)
        except BaseException:
            if backup.exists():
                backup.rename(install)
            raise
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


def runtime_files():
    info = json.loads((ROOT / 'tools/runtime-manifest.json').read_text())
    if set(info['files']) != set(RUNTIME_FILES):
        raise ValueError('Invalid runtime package file list')
    for name, digest in info['files'].items():
        if not (ROOT / 'runtime' / name).is_file() or sha(ROOT / 'runtime' / name) != digest:
            raise ValueError('The release runtime is incomplete or damaged. Extract the entire ZIP again.')
    return info


def launch(data):
    """Keep new runtime settings/hdd separate; diagnostic environment never leaks in."""
    if not ready(data):
        raise ValueError('Please import your supported Xbox ISO first.')
    info = runtime_files()
    data = data.resolve()
    runtime = data / 'xbe-runtime'
    runtime.mkdir(parents=True, exist_ok=True)
    for name, digest in info['files'].items():
        dest = runtime / name
        if dest.is_file() and sha(dest) == digest:
            continue
        temp = runtime / ('.update-' + uuid.uuid4().hex)
        try:
            shutil.copy2(ROOT / 'runtime' / name, temp)
            if sha(temp) != digest:
                raise ValueError('Runtime copy verification failed')
            os.replace(temp, dest)
        finally:
            if temp.exists():
                temp.unlink()
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
    parser.add_argument('action', choices=['import', 'status', 'launch'])
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--iso', type=Path)
    parser.add_argument('--cancel', type=Path)
    args = parser.parse_args()
    try:
        if args.action == 'import':
            if args.iso is None or args.cancel is None:
                raise ValueError('Import requires an ISO and cancellation path.')
            import_iso(args.iso, args.data, args.cancel)
        elif args.action == 'status':
            return 0 if ready(args.data) else 2
        else:
            launch(args.data)
        return 0
    except Exception as error:
        progress(str(error), 0)
        return 1


if __name__ == '__main__':
    sys.exit(main())
