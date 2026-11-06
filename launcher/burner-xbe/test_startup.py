"""Saved startup choice, safe local persistence and the real launch environment; no game."""
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
import io
import os
import stat
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_game as worker
import test_source_setup as source


class StartupTests(source.SourceSetupTests):
    def test_default_off_and_saved_toggle_without_import(self):
        code, rows = self.command('startup')
        self.assertEqual(code, 0)
        self.assertFalse(rows[-1]['skip_startup'])
        self.assertFalse(self.data.exists())
        for value in ('on', 'off', 'on'):
            code, rows = self.command('startup', '--skip-startup', value)
            self.assertEqual(code, 0)
            self.assertEqual(rows[-1]['skip_startup'], value == 'on')
            self.assertEqual(self.command('startup')[1][-1]['skip_startup'], value == 'on')
            self.assertEqual(self.command('status')[1][-1]['skip_startup'], value == 'on')
        self.assertEqual((self.data / '.skip-startup').read_bytes(), b'1\n')
        self.assertFalse((self.data / '.diagnostic-logging').exists())
        self.assertFalse((self.data / 'logs').exists())
        self.assertNotEqual(self.command('startup', '--skip-startup', 'maybe')[0], 0)
        self.assertTrue(worker.skip_startup_enabled())

    def test_invalid_linked_and_special_preferences_fail_closed(self):
        self.data.mkdir()
        marker = self.data / '.skip-startup'
        for value in (b'', b'1', b'true', b'1\nextra', b'1' * 4096):
            marker.write_bytes(value)
            self.assertFalse(worker.skip_startup_enabled())
        marker.unlink()
        marker.mkdir()
        self.assertFalse(worker.skip_startup_enabled())
        for value in ('on', 'off'):
            with self.assertRaises(ValueError):
                worker.startup_settings(value)
        marker.rmdir()
        external = self.root / 'private'
        external.write_bytes(b'1\n')
        os.link(external, marker)
        self.assertFalse(worker.skip_startup_enabled())
        for value in ('on', 'off'):
            with self.assertRaises(ValueError):
                worker.startup_settings(value)
        self.assertEqual(external.read_bytes(), b'1\n')
        marker.unlink()
        original = Path.lstat
        for redirected in (self.package, self.data, marker):
            for junction in (False, True):
                def linked(path, *args, **kwargs):
                    if path == redirected:
                        return SimpleNamespace(st_mode=stat.S_IFDIR if junction else stat.S_IFLNK,
                                               st_file_attributes=0x400 if junction else 0)
                    return original(path, *args, **kwargs)
                with patch.object(Path, 'lstat', linked):
                    self.assertFalse(worker.skip_startup_enabled())
                    for value in ('on', 'off'):
                        with self.assertRaises(ValueError):
                            worker.startup_settings(value)
        self.assertEqual(external.read_bytes(), b'1\n')

    def test_failed_save_preserves_choice_and_other_state(self):
        saved = self.preserve_state()
        worker.startup_settings('on')
        with patch.object(worker.os, 'replace', side_effect=OSError('disk error')):
            with self.assertRaises(OSError):
                worker.startup_settings('on')
        self.assertTrue(worker.skip_startup_enabled())
        self.assertEqual(list(self.data.glob('.skip-startup-*')), [])
        worker.startup_settings('off')
        self.assertEqual(source.file_snapshot(self.data / 'xbe-runtime'), saved)

    def test_selected_install_launch_uses_only_saved_choice(self):
        self.setup()
        self.preserve_state()
        (self.data / 'xbe-runtime/settings.toml').write_text('language = 0\n')
        saved = source.file_snapshot(self.data / 'xbe-runtime')
        data = self.root / 'existing'
        self.data.rename(data)
        inherited = {'UCRR_SKIP_STARTUP': '1', 'UCRR_SESSION_LOG': '1',
                     'UCRR_UNKNOWN': '1', 'XBE_CAPTURE_DIR': 'bad', 'BURNER_AIM_LOG': 'bad'}
        for value in ('off', 'on', 'off'):
            self.command('startup', '--skip-startup', value)
            with patch.dict(os.environ, inherited), patch.object(worker.subprocess, 'Popen',
                    return_value=SimpleNamespace(stderr=io.BytesIO(), wait=lambda: 0)) as popen:
                worker.launch(data)
            env = popen.call_args.kwargs['env']
            self.assertEqual(env.get('UCRR_SKIP_STARTUP'), '1' if value == 'on' else None)
            for key in inherited.keys() - {'UCRR_SKIP_STARTUP'}:
                self.assertNotIn(key, env)
            self.assertEqual(env['UCRR_GAME_ASSETS'], str(data / 'installed/game_files'))
            self.assertFalse((data / '.skip-startup').exists())
            self.assertTrue(all((data / 'xbe-runtime' / name).read_bytes() == value
                                for name, value in saved.items()))


if __name__ == '__main__':
    unittest.main()
