"""Verified in-place package updates; installed game data is never an operation."""
import argparse
from contextlib import contextmanager
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
import signal

STAGE = '.launcher-update'
MANIFEST = 'package-manifest.json'
WRAPPERS = ('burner.sh', 'README-linux.txt')


def relative_path(name):
    if not isinstance(name, str) or not name or len(name) > 1024 or '\\' in name:
        raise ValueError('Invalid update path')
    for part in name.split('/'):
        if (not part or part in ('.', '..') or not part.isascii() or part.endswith(('.', ' ')) or
                re.search(r'[\x00-\x1f:<>"|?*]', part) or
                re.fullmatch(r'(?i)(con|prn|aux|nul|com[0-9]|lpt[0-9])(?:\..*)?', part) or
                part.lower() in ('gamedata', '.proton') or part.lower().startswith('.launcher-update')):
            raise ValueError('Invalid or user-data update path')
    return name


def safe(root, name):
    # The fixed transaction directory is allowed only for our own internal paths.
    relative_path(name.removeprefix(STAGE + '/') if name != STAGE else 'stage')
    path = Path(root)
    for part in (None, *name.split('/')):
        if part is not None:
            path /= part
        try:
            info = path.lstat()
        except FileNotFoundError:
            continue
        if path.is_symlink() or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Linked update paths are not supported')
        if not path.is_dir() and (not path.is_file() or info.st_nlink != 1):
            raise ValueError('Update files must be regular and not linked')
    return path


def owned(name, platform):
    return (name in (MANIFEST, 'Burner Edition Setup.exe', 'Play Burner Edition.exe',
                     'README.txt', 'cover.png', 'assets/README.txt') or
            name.startswith(('tools/', 'runtime/', 'licenses/')) or
            (platform == 'linux' and name in WRAPPERS))



def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            value.update(chunk)
    return value.hexdigest()


def read(path):
    if path.stat().st_size > 8 * 1024 * 1024:
        raise ValueError('Update metadata is too large')
    value = json.loads(path.read_text(encoding='utf-8-sig'))
    if not isinstance(value, dict):
        raise ValueError('Invalid update metadata')
    return value


def write(path, value):
    descriptor, name = tempfile.mkstemp(prefix='.burner-update-', dir=path.parent)
    temporary = Path(name)
    try:
        with os.fdopen(descriptor, 'w', encoding='utf-8') as target:
            json.dump(value, target)
            target.flush()
            os.fsync(target.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def copy_atomic(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    descriptor, name = tempfile.mkstemp(prefix='.burner-update-', dir=destination.parent)
    temporary = Path(name)
    try:
        with os.fdopen(descriptor, 'wb') as target, source.open('rb') as original:
            shutil.copyfileobj(original, target, 1024 * 1024)
            target.flush()
            os.fsync(target.fileno())
        shutil.copystat(source, temporary)
        os.replace(temporary, destination)
    finally:
        temporary.unlink(missing_ok=True)


@contextmanager
def exclusive(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    path = safe(path.parent, path.name)
    with path.open('a+b') as lock:
        if os.name == 'nt':
            import msvcrt
            if not path.stat().st_size:
                lock.write(b'\0')
                lock.flush()
            lock.seek(0)
            try:
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
            except OSError as error:
                raise ValueError('Another launcher or update is using this installation. Close it first.') from error
        else:
            import fcntl
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except OSError as error:
                raise ValueError('Another launcher or update is using this installation. Close it first.') from error
        yield


def transaction(root):
    file = safe(root, STAGE + '/transaction.json')
    if not file.exists():
        return None
    value = read(file)
    if (value.get('phase') not in ('prepared', 'applying', 'committed', 'rolled_back') or
            value.get('platform') not in ('windows', 'linux') or not isinstance(value.get('files'), dict)):
        raise ValueError('Invalid pending update; keep the update backup and repair the launcher package')
    for name, row in value['files'].items():
        safe(root, name)
        if not owned(name, value['platform']) or not isinstance(row, dict) or not {'old', 'new'} <= row.keys():
            raise ValueError('Update metadata includes an unmanaged path')
        for key in ('old', 'new'):
            if row.get(key) is not None and not re.fullmatch('[0-9a-f]{64}', str(row[key])):
                raise ValueError('Invalid update checksum')
        if row.get('old') is None and row.get('new') is None:
            raise ValueError('Empty update operation')
    return value


def pending(root):
    value = transaction(root)
    return value is not None and value['phase'] in ('prepared', 'applying')


def cleanup(root):
    """Discard a finished transaction after the replacement launcher works."""
    root = Path(root).absolute()
    scratch = safe(root, STAGE)
    if not scratch.exists():
        return
    try:
        with exclusive(safe(root, '.release-update.lock')):
            value = transaction(root)
            if value and value['phase'] in ('committed', 'rolled_back'):
                shutil.rmtree(scratch)
    except (OSError, ValueError):
        # Windows may still be releasing the helper DLLs; a later launch retries.
        pass


def package_files(root, platform):
    path = safe(root, MANIFEST)
    manifest = read(path)
    files = manifest.get('files')
    if not isinstance(files, dict) or not files or manifest.get('complete') is not True:
        raise ValueError('Incomplete launcher package')
    result = dict(files)
    result[MANIFEST] = digest(path)
    if platform == 'linux':
        for name in WRAPPERS:
            result[name] = digest(safe(root, name))
    for name, checksum in result.items():
        safe(root, name)
        if not owned(name, platform) or not re.fullmatch('[0-9a-f]{64}', str(checksum)):
            raise ValueError('Invalid managed package file')
    return result


def prepare(root, current_version, platform, notify=lambda *a, **k: None, check_cancel=lambda: None):
    import release_download
    root = Path(root).absolute()
    scratch = safe(root, STAGE)
    if scratch.exists():
        previous = transaction(root)
        if previous and previous['phase'] in ('prepared', 'applying'):
            raise ValueError('A previous update needs recovery. Restart the launcher first.')
        # Only our fixed scratch directory is removed, never installation data.
        shutil.rmtree(scratch)
    scratch.mkdir()
    try:
        release = release_download.prepare(current_version, platform, scratch, notify, check_cancel)
        for name in ('download.zip', 'download.tar.gz'):
            safe(scratch, name).unlink(missing_ok=True)
        incoming = safe(scratch, 'new')
        old = package_files(root, platform)
        new = package_files(incoming, platform)
        if any('tools/' + name not in new for name in
               ('release_update.py', 'release_download.py', 'release_check.py')):
            raise ValueError('This release lacks safe automatic-update support. Download its complete package manually.')
        runner = safe(scratch, 'runner')
        runner.mkdir()
        payload = root / 'tools'
        for name in ('release_update.py', 'release_download.py', 'release_check.py'):
            shutil.copy2(safe(payload, name), runner / name)
        if os.name == 'nt':
            for name in old:
                if name.startswith('tools/python/'):
                    path = safe(root, name)
                    if digest(path) != old[name]:
                        raise ValueError('The updater Python runtime is damaged. Extract a complete launcher package.')
                    shutil.copy2(path, runner / path.name)
            (runner / 'python312._pth').write_text('python312.zip\n.\n', encoding='ascii')
        operations = {}
        for name in sorted(set(old) | set(new)):
            check_cancel()
            target = safe(root, name)
            if name in new and digest(safe(incoming, name)) != new[name]:
                raise ValueError('Staged update changed after verification')
            if target.exists() and not target.is_file():
                raise ValueError('An update destination is not a regular file')
            actual = digest(target) if target.exists() else None
            if actual == new.get(name):
                continue
            if name not in old and target.exists():
                raise ValueError('The update would replace an unrecognized user file: ' + name)
            # shortcut: Steam starts embedded Python directly; use a fresh package for
            # interpreter upgrades until Steam has an independent recovery bootstrap.
            if platform == 'linux' and name.startswith('tools/python/') and name in old:
                raise ValueError('This release changes the Steam Python runtime. Download and extract its complete Linux package to update safely.')
            if actual is not None:
                backup = safe(scratch, 'backup/' + name)
                copy_atomic(target, backup)
                if digest(backup) != actual:
                    raise ValueError('Update backup failed verification')
            operations[name] = dict(old=actual, new=new.get(name))
        check_cancel()
        value = dict(phase='prepared', platform=platform, version=release['version'], files=operations)
        write(scratch / 'transaction.json', value)
        notify('progress', 'Download verified. Restarting the launcher to apply the update.', percent=100)
        return value
    except BaseException:
        if not (scratch / 'transaction.json').exists():
            shutil.rmtree(scratch)
        raise


def recover(root):
    root = Path(root).absolute()
    value = transaction(root)
    if not value or value['phase'] not in ('prepared', 'applying'):
        return False
    scratch = safe(root, STAGE)
    # Validate every backup before restoring any destination. Keep copies for retries.
    for name, row in value['files'].items():
        if row['old'] is not None and digest(safe(scratch, 'backup/' + name)) != row['old']:
            raise ValueError('Update backup is damaged. Keep it and repair the launcher package.')
    if value['phase'] == 'applying':
        for name, row in value['files'].items():
            target = safe(root, name)
            if row['old'] is not None:
                if not target.is_file() or digest(target) != row['old']:
                    copy_atomic(safe(scratch, 'backup/' + name), target)
            elif target.exists():
                if not target.is_file() or digest(target) != row['new']:
                    raise ValueError('A newly installed file changed; refusing to delete it during recovery')
                target.unlink()
    value['phase'] = 'rolled_back'
    write(scratch / 'transaction.json', value)
    return True


def apply(root):
    root = Path(root).absolute()
    value = transaction(root)
    if not value or value['phase'] != 'prepared':
        raise ValueError('No verified update is ready to apply')
    scratch = safe(root, STAGE)
    growth, largest = 0, 0
    for name, row in value['files'].items():
        target = safe(root, name)
        if (digest(target) if target.exists() else None) != row['old']:
            raise ValueError('Launcher files changed while the update was being prepared')
        if row['new'] is not None and digest(safe(scratch, 'new/' + name)) != row['new']:
            raise ValueError('Staged update failed verification')
        if row['old'] is not None and digest(safe(scratch, 'backup/' + name)) != row['old']:
            raise ValueError('Update backup failed verification')
        old_size = safe(scratch, 'backup/' + name).stat().st_size if row['old'] is not None else 0
        new_size = safe(scratch, 'new/' + name).stat().st_size if row['new'] is not None else 0
        growth += max(0, new_size - old_size)
        largest = max(largest, old_size, new_size)
    if shutil.disk_usage(root).free < growth + largest + 16 * 1024 * 1024:
        raise ValueError('Not enough free disk space to apply and safely recover this update. Free space and retry; existing files were not changed.')
    value['phase'] = 'applying'
    write(scratch / 'transaction.json', value)
    try:
        for name, row in sorted(value['files'].items(), key=lambda item: (item[0].endswith(MANIFEST), item[0])):
            target = safe(root, name)
            if row['new'] is None:
                target.unlink()
            else:
                copy_atomic(safe(scratch, 'new/' + name), target)
        for name, row in value['files'].items():
            target = safe(root, name)
            if (digest(target) if target.exists() else None) != row['new']:
                raise ValueError('Installed update failed verification')
        value['phase'] = 'committed'
        write(scratch / 'transaction.json', value)
    except Exception:
        recover(root)
        raise
    return value['version']


def game_running():
    if os.name != 'nt':
        for process in Path('/proc').glob('[0-9]*/cmdline'):
            try:
                if any(part.replace(b'\\', b'/').rsplit(b'/', 1)[-1].lower() in
                       (b'urbanchaosxbetrial.exe', b'import_game.py', b'burner edition setup.exe', b'play burner edition.exe')
                       for part in process.read_bytes().split(b'\0')):
                    return True
            except (OSError, PermissionError):
                continue
        return False
    # The same process-name contract as setup_backend, without importing replaced files.
    from ctypes import wintypes
    class Entry(ctypes.Structure):
        _fields_ = [('size', wintypes.DWORD), ('usage', wintypes.DWORD), ('pid', wintypes.DWORD),
                    ('heap', ctypes.c_size_t), ('module', wintypes.DWORD), ('threads', wintypes.DWORD),
                    ('parent', wintypes.DWORD), ('priority', wintypes.LONG), ('flags', wintypes.DWORD),
                    ('name', wintypes.WCHAR * 260)]
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    kernel.Process32FirstW.argtypes = kernel.Process32NextW.argtypes = (wintypes.HANDLE, ctypes.POINTER(Entry))
    kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
    handle = kernel.CreateToolhelp32Snapshot(2, 0)
    if handle == ctypes.c_void_p(-1).value:
        raise OSError('Could not check whether Burner Edition is running')
    try:
        entry = Entry()
        entry.size = ctypes.sizeof(entry)
        more = kernel.Process32FirstW(handle, ctypes.byref(entry))
        while more:
            if entry.name.casefold() == 'urbanchaosxbetrial.exe':
                return True
            more = kernel.Process32NextW(handle, ctypes.byref(entry))
        if ctypes.get_last_error() not in (0, 18):
            raise OSError('Could not finish checking running applications')
        return False
    finally:
        kernel.CloseHandle(handle)


def launch_worker(root, install_root, launcher_pid=None, *, restart=True, interactive=False):
    root = Path(root).absolute()
    runner = safe(root, STAGE + '/runner')
    ready = safe(root, STAGE + '/worker-ready.json')
    ready.unlink(missing_ok=True)
    command = [str(runner / 'python.exe'), '-B', str(runner / 'release_update.py'), '--apply', str(root),
               '--install-root', str(install_root), '--wait-pid', str(os.getpid()),
               ]
    if launcher_pid is not None:
        command += ['--wait-pid', str(launcher_pid)]
    if restart:
        command.append('--restart')
    if interactive:
        command.append('--interactive')
    process = subprocess.Popen(command, cwd=runner, stdin=subprocess.DEVNULL,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    deadline = time.monotonic() + 10
    while not ready.exists():
        if process.poll() is not None or time.monotonic() >= deadline:
            raise ValueError('The update helper could not start. Restart the launcher to recover safely.')
        time.sleep(0.05)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--apply', type=Path)
    action.add_argument('--recover', type=Path)
    action.add_argument('--download', type=Path)
    action.add_argument('--check', type=Path)
    parser.add_argument('--wait-pid', type=int, action='append', default=[])
    parser.add_argument('--install-root', type=Path)
    parser.add_argument('--restart', action='store_true')
    parser.add_argument('--interactive', action='store_true')
    args = parser.parse_args(argv)
    if args.download or args.check:
        return native_update(args.download or args.check, download=args.download is not None)
    root = (args.apply or args.recover).absolute()
    kernel = handles = None
    try:
        if args.wait_pid:
            from ctypes import wintypes
            kernel = ctypes.WinDLL('kernel32', use_last_error=True)
            kernel.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
            kernel.OpenProcess.restype = wintypes.HANDLE
            kernel.WaitForSingleObject.argtypes = (wintypes.HANDLE, wintypes.DWORD)
            kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
            handles = []
            for pid in args.wait_pid:
                if pid <= 0:
                    raise ValueError('Invalid launcher process')
                handle = kernel.OpenProcess(0x100000, False, pid)
                if not handle:
                    if ctypes.get_last_error() == 87:
                        continue
                    raise OSError('Could not wait for the launcher to close')
                handles.append(handle)
            write(safe(root, STAGE + '/worker-ready.json'), dict(pid=os.getpid()))
            for handle in handles:
                if kernel.WaitForSingleObject(handle, 60000) != 0:
                    raise ValueError('The launcher did not close; the update was not applied')
        with exclusive(safe(root, STAGE + '/worker.lock')):
            # Use the same byte-range locks as the setup/play worker.
            with exclusive(safe(root, '.burner.lock')):
                # Native Linux activation only replaces package files; Windows paths
                # in installation.json belong to the Proton setup worker.
                install_root = args.install_root if os.name == 'nt' else root
                if install_root is None:
                    location = safe(root, 'installation.json')
                    selected = read(location).get('data') if location.exists() else 'GameData'
                    if selected == 'GameData':
                        install_root = root / 'GameData'
                    elif isinstance(selected, str) and Path(selected).is_absolute() and '..' not in Path(selected).parts:
                        install_root = Path(selected)
                    else:
                        raise ValueError('Invalid saved installation location')
                from contextlib import ExitStack
                with ExitStack() as locks:
                    if install_root.exists() and install_root.absolute() != root:
                        locks.enter_context(exclusive(safe(install_root, '.burner.lock')))
                    if game_running():
                        raise ValueError('Close the game before applying an update')
                    if args.recover:
                        recover(root)
                    else:
                        apply(root)
        if args.restart:
            command = [str(root / 'Burner Edition Setup.exe')]
            if args.apply:
                command.append('--complete-update')
            subprocess.Popen(command, cwd=root, close_fds=True)
        if args.interactive:
            ctypes.windll.user32.MessageBoxW(None, 'Update finished. Restore the normal launch command in Steam and press Play. Your game data and saves were kept.', 'Burner Edition update', 0x10040)
        return 0
    except Exception as error:
        # The backup survives both ordinary errors and interruption during rollback.
        write(safe(root, STAGE + '/last-error.json'), dict(error=str(error)))
        if os.name == 'nt':
            ctypes.windll.user32.MessageBoxW(None, str(error) + '\n\nRestart the launcher to recover. Your saves are kept.',
                                            'Burner Edition update', 0x10)
        else:
            print(str(error), file=sys.stderr)
        return 1
    finally:
        for handle in handles or []:
            kernel.CloseHandle(handle)


def platform(root):
    return 'linux' if safe(root, 'burner.sh').is_file() else 'windows'


def check(root):
    import release_check
    return release_check.check(read(safe(root, MANIFEST))['version'], platform(root))


def bootstrap(root):
    """Do not import possibly mixed worker modules after an interrupted update."""
    if not pending(root):
        if safe(root, STAGE).exists():
            try:
                with exclusive(safe(root, '.burner.lock')):
                    cleanup(root)
            except (OSError, ValueError):
                pass  # An active game/helper owns the files; retry cleanup next launch.
        return False
    if platform(root) == 'linux':
        message = 'An update needs recovery. Close the Steam shortcut, then run ./burner.sh recover-update from the package folder. Your saves are kept.'
        print(json.dumps({'message': message}), flush=True)
        if '--interactive' in sys.argv:
            ctypes.windll.user32.MessageBoxW(None, message, 'Burner Edition update', 0x10010)
        return True
    runner = safe(root, STAGE + '/runner')
    command = [str(safe(runner, 'python.exe')), '-B', str(safe(runner, 'release_update.py')),
               '--recover', str(root), '--wait-pid', str(os.getpid())]
    if '--interactive' in sys.argv:
        command.append('--interactive')
    subprocess.Popen(command, cwd=runner, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                     stderr=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW)
    print(json.dumps({'message': 'An interrupted update is being recovered. Wait for setup to close, then reopen it. Your saves are kept.'}), flush=True)
    return True


def native_update(root, *, download):
    """Linux uses native file operations so shell permissions survive replacement."""
    root = Path(root).absolute()
    cancelled = False
    def cancel(signum, frame):
        nonlocal cancelled
        cancelled = True
    def check_cancel():
        if cancelled:
            raise ValueError('Update cancelled. Your existing files were kept.')
    def notify(event, message, **fields):
        print(message, flush=True)
    try:
        if os.name == 'nt' or platform(root) != 'linux':
            raise ValueError('Use Manage > Check for updates in the Windows launcher.')
        if not download:
            print(check(root)['message'])
            return 0
        signal.signal(signal.SIGINT, cancel)
        signal.signal(signal.SIGTERM, cancel)
        with exclusive(safe(root, '.burner.lock')), exclusive(safe(root, '.release-update.lock')):
            if game_running():
                raise ValueError('Close the game and Steam setup/play shortcut before updating.')
            current = read(safe(root, MANIFEST))['version']
            prepare(root, current, 'linux', notify, check_cancel)
            try:
                check_cancel()
                if game_running():
                    raise ValueError('Close the game and launcher before updating.')
                installed = apply(root)
            except Exception:
                recover(root)
                raise
        print('Updated to ' + installed + '. Your game data, saves and Steam shortcut were kept. Press Play in Steam.', flush=True)
        return 0
    except Exception as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
