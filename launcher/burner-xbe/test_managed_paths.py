"""Managed roots allow host links while rejecting redirected managed paths."""
from pathlib import Path
import stat
import sys
import tempfile
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shader_seed import unlinked


def rejects(operation):
    try:
        operation()
    except ValueError:
        return
    raise AssertionError('Unsafe managed path accepted')


with tempfile.TemporaryDirectory(prefix='managed-path-test-') as directory:
    host = Path(directory)
    root = host / 'package'
    leaf = root / 'GameData/xbe-runtime/file.bin'
    leaf.parent.mkdir(parents=True)
    leaf.write_bytes(b'unchanged')
    original_stat = Path.lstat
    for junction in (False, True):
        def redirected_stat(path, *args, **kwargs):
            if path == redirected:
                return SimpleNamespace(st_mode=stat.S_IFDIR if junction else stat.S_IFLNK,
                                       st_file_attributes=0x400 if junction else 0)
            return original_stat(path, *args, **kwargs)
        redirected = host
        with patch.object(Path, 'lstat', redirected_stat):
            unlinked(leaf, root=root)
            rejects(lambda: unlinked(leaf))
        for redirected in (root, root/'GameData', leaf.parent, leaf, root/'missing-link'):
            checked = redirected if redirected.name == 'missing-link' else leaf
            with patch.object(Path, 'lstat', redirected_stat):
                rejects(lambda: unlinked(checked, root=root))
    unlinked(root/'new/missing/file', root=root)
    rejects(lambda: unlinked(host/'package-sibling/file', root=root))
    rejects(lambda: unlinked(root/'GameData/../escape', root=root))
    rejects(lambda: unlinked(root, root=root/'GameData'))
    # A path on a different Windows drive never belongs to the trust root.
    if root.drive:
        other_drive = 'Z:' if root.drive.upper() != 'Z:' else 'Y:'
        rejects(lambda: unlinked(Path(other_drive + '/file'), root=root))
    assert leaf.read_bytes() == b'unchanged'

print('PASS: host mappings allowed, strict developer defaults, managed roots/ancestors/leaves '
      'and dangling links rejected, traversal/siblings/drives rejected')
