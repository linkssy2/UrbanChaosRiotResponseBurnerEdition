"""Small feedback and packaging regressions; no dialogs or game are launched."""
from pathlib import Path
import ctypes
import io
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import types
import unittest
from unittest.mock import Mock, patch

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import setup_feedback as feedback
import package as packager


class FeedbackTests(unittest.TestCase):
    def test_native_dialog_flags_and_no_stdout(self):
        box = Mock(return_value=1)
        with patch.object(ctypes, 'WinDLL', return_value=types.SimpleNamespace(MessageBoxW=box), create=True), \
                patch('sys.stdout', new_callable=io.StringIO) as output:
            self.assertTrue(feedback.notify('Ready'))
            self.assertTrue(feedback.notify('Failed', error=True))
        self.assertEqual(output.getvalue(), '')
        self.assertEqual([x.args for x in box.call_args_list], [
            (None, 'Ready', 'Urban Chaos - Burner Edition', 0x10040),
            (None, 'Failed', 'Urban Chaos - Burner Edition', 0x10010)])

    def test_missing_ui_keeps_message_and_broken_pipe_is_safe(self):
        with patch.object(ctypes, 'WinDLL', side_effect=OSError('missing UI'), create=True), \
                patch('sys.stderr', new_callable=io.StringIO) as output:
            self.assertFalse(feedback.notify('Original error', error=True))
        self.assertIn('Original error', output.getvalue())
        self.assertIn('missing UI', output.getvalue())
        with patch.object(ctypes, 'WinDLL', side_effect=OSError('missing UI'), create=True), \
                patch('sys.stderr') as broken:
            broken.write.side_effect = BrokenPipeError()
            self.assertFalse(feedback.notify('Original error', error=True))

    def test_report_is_bounded_utf8_and_does_not_need_gamedata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            message = feedback.report_error(root, 'import', ValueError('bad disc'))
            self.assertIn('import failed: bad disc', message)
            self.assertEqual((root / feedback.ERROR_FILE).read_text(), 'import failed: bad disc\n')
            feedback.report_error(root, 'import', ValueError('\u00e9' * 20000))
            data = (root / feedback.ERROR_FILE).read_bytes()
            self.assertLessEqual(len(data), feedback.MAX_ERROR_BYTES)
            data.decode('utf8')
            self.assertEqual({p.name for p in root.iterdir()}, {feedback.ERROR_FILE})

    def test_write_failure_preserves_error_and_existing_report(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / feedback.ERROR_FILE
            target.write_text('existing')
            with patch.object(feedback.os, 'replace', side_effect=PermissionError('read only')):
                message = feedback.report_error(root, 'import', ValueError('unsupported ISO'))
            self.assertIn('unsupported ISO', message)
            self.assertIn('read only', message)
            self.assertEqual(target.read_text(), 'existing')
            self.assertEqual({p.name for p in root.iterdir()}, {feedback.ERROR_FILE})
            with patch.object(feedback.tempfile, 'mkstemp', side_effect=PermissionError('unwritable')):
                self.assertIn('original', feedback.report_error(root, 'import', ValueError('original')))

    def test_links_reparse_points_and_nonfiles_are_rejected(self):
        variants = [types.SimpleNamespace(st_mode=stat.S_IFLNK, st_nlink=1),
                    types.SimpleNamespace(st_mode=stat.S_IFREG, st_nlink=2),
                    types.SimpleNamespace(st_mode=stat.S_IFREG, st_nlink=1, st_file_attributes=0x400),
                    types.SimpleNamespace(st_mode=stat.S_IFDIR, st_nlink=1)]
        for info in variants:
            with self.subTest(info=info), patch.object(Path, 'lstat', return_value=info):
                with self.assertRaises(ValueError):
                    feedback._check_destination(Path('report'))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            original = root / 'keep.txt'
            original.write_text('untouched')
            os.link(original, root / feedback.ERROR_FILE)
            message = feedback.report_error(root, 'import', ValueError('original failure'))
            self.assertIn('original failure', message)
            self.assertIn('must not be a link', message)
            self.assertEqual(original.read_text(), 'untouched')

    def test_feedback_and_matching_ctypes_are_packaged(self):
        plan = packager.sources(HERE / 'fixture/UrbanChaosXbeTrial.exe')
        self.assertEqual(plan['tools/setup_feedback.py'], HERE / 'setup_feedback.py')
        for name in ('_ctypes.pyd', 'libffi-8.dll'):
            self.assertEqual(plan['tools/python/' + name], packager.REPO / '.tools/python' / name)
            self.assertTrue(plan['tools/python/' + name].is_file())

    @unittest.skipUnless(os.name == 'nt', 'The bundled interpreter is Windows x64')
    def test_isolated_packaged_python_loads_native_dialog_api(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in packager.PYTHON_FILES:
                shutil.copy2(packager.REPO / '.tools/python' / name, root / name)
            (root / 'python312._pth').write_text('python312.zip\n.\n..\n')
            result = subprocess.run([str(root / 'python.exe'), '-B', '-c',
                'import ctypes; assert ctypes.sizeof(ctypes.c_void_p)==8; '
                'assert ctypes.WinDLL("user32").MessageBoxW; print("ctypes ready")'],
                cwd=root, capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout.strip(), 'ctypes ready')


if __name__ == '__main__':
    unittest.main()
