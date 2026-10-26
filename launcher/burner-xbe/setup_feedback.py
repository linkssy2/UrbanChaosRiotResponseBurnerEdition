"""Opt-in setup dialogs and bounded failure details for direct Steam launches."""
from pathlib import Path
import os
import stat
import sys
import tempfile

ERROR_FILE = 'last-setup-error.txt'
MAX_ERROR_BYTES = 16384


def notify(message, error=False):
    """Show an acknowledged result without changing the worker's JSON output."""
    try:
        import ctypes
        box = ctypes.WinDLL('user32', use_last_error=True).MessageBoxW
        box.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint]
        box.restype = ctypes.c_int
        # Bring the acknowledgement forward when Steam launched us without a console.
        return bool(box(None, str(message), 'Urban Chaos - Burner Edition',
                        0x10000 | (0x10 if error else 0x40)))
    except Exception as failure:
        try:
            print(str(message) + '\nCould not show setup dialog: ' + str(failure), file=sys.stderr)
        except (OSError, ValueError):
            pass
        return False


def _check_destination(path):
    try:
        info = path.lstat()
    except FileNotFoundError:
        return
    if (not stat.S_ISREG(info.st_mode) or info.st_nlink != 1 or
            getattr(info, 'st_file_attributes', 0) & getattr(stat, 'FILE_ATTRIBUTE_REPARSE_POINT', 0x400)):
        raise ValueError('The setup error file must not be a link or special file.')


def report_error(root, action, error):
    """Keep the original error even if the package cannot receive a report.

    Resolve the caller's package root once, then atomically replace a checked
    regular leaf. Never create or write through a rejected GameData path.
    """
    message = str(action) + ' failed: ' + str(error)
    temporary = None
    try:
        root = Path(root).resolve(strict=True)
        destination = root / ERROR_FILE
        _check_destination(destination)
        payload = (message + '\n').encode('utf8', 'replace')[:MAX_ERROR_BYTES]
        payload = payload.decode('utf8', 'ignore').encode('utf8')
        descriptor, name = tempfile.mkstemp(prefix='.setup-error-', dir=root)
        temporary = Path(name)
        with os.fdopen(descriptor, 'wb') as output:
            output.write(payload)
        _check_destination(destination)
        os.replace(temporary, destination)
        return message + '\n\nDetails saved to: ' + str(destination)
    except Exception as failure:
        return message + '\n\nCould not save setup details: ' + str(failure)
    finally:
        if temporary is not None:
            try:
                temporary.unlink(missing_ok=True)
            except OSError:
                pass
