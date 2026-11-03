"""Local, opt-in, bounded support logs shared by Windows and Proton setup."""
from pathlib import Path
import datetime
import os
import stat
import sys
import uuid

import shader_seed

ACTION_LIMIT = 8 * 1024 * 1024
GAME_LIMIT = 64 * 1024 * 1024
KEEP = 30
_action = None


def checked(root, relative):
    path = Path(root) / 'GameData' / relative
    shader_seed.unlinked(path, root=root)
    if path.exists():
        info = path.stat()
        if not stat.S_ISDIR(info.st_mode) and (not stat.S_ISREG(info.st_mode) or info.st_nlink != 1):
            raise ValueError('Diagnostic paths must not be linked or special files.')
    return path


def enabled(root):
    try:
        path = checked(root, '.diagnostic-logging')
        return path.is_file() and path.stat().st_size == 2 and path.read_bytes() == b'1\n'
    except (OSError, ValueError):
        return False


def status(root):
    return {'diagnostic_logging': enabled(root), 'diagnostics_dir': str(Path(root) / 'GameData/logs')}


def configure(root, value):
    if value not in ('on', 'off'):
        raise ValueError('Choose diagnostic logging on or off.')
    path = checked(root, '.diagnostic-logging')
    if value == 'off':
        path.unlink(missing_ok=True)
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = checked(root, '.diagnostic-' + uuid.uuid4().hex)
        try:
            with temporary.open('xb') as stream:
                stream.write(b'1\n')
            os.replace(temporary, path)
        finally:
            temporary.unlink(missing_ok=True)
    return dict(status(root), message='Diagnostic logging ' + value + '. Applies to the next game session; logs stay local.')


class Session:
    def __init__(self, root, kind, limit):
        self.stream = None
        self.remaining = limit
        self.path = None
        if not enabled(root):
            return
        try:
            folder = checked(root, 'logs')
            folder.mkdir(parents=True, exist_ok=True)
            # Only retire our own regular session files, never arbitrary player files.
            prior = sorted(folder.glob(kind + '-*.log'))
            for path in prior[:max(0, len(prior) - KEEP + 1)]:
                checked(root, 'logs/' + path.name).unlink()
            stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
            self.path = checked(root, 'logs/' + kind + '-' + stamp + '-' + uuid.uuid4().hex[:8] + '.log')
            self.stream = self.path.open('xb', buffering=0)
        except (OSError, ValueError):
            self.stream = None

    def append(self, payload):
        if self.stream is None or self.remaining <= 0:
            return
        if isinstance(payload, str):
            payload = payload.encode('utf-8', 'replace')
        marker = b'\n[Diagnostic log size limit reached]\n'
        if len(payload) > self.remaining:
            payload = payload[:max(0, self.remaining - len(marker))] + marker[:self.remaining]
        try:
            written = self.stream.write(payload)
            self.remaining -= written
        except OSError:
            self.close()  # A full/read-only disk must not prevent playing or setup.

    def close(self):
        if self.stream is not None:
            try:
                self.stream.close()
            except OSError:
                pass
            self.stream = None


def start(root, action):
    global _action
    finish()
    _action = Session(root, 'launcher', ACTION_LIMIT)
    record('Action=' + action + ' Python=' + sys.version + ' platform=' + sys.platform)


def record(message):
    if _action is not None:
        _action.append(datetime.datetime.now(datetime.timezone.utc).isoformat() + ' ' + str(message) + '\n')


def finish():
    global _action
    if _action is not None:
        _action.close()
        _action = None
