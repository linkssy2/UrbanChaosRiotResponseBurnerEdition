"""Public opt-in path, retention and actual worker launch policy; no game launch."""
from pathlib import Path
from unittest.mock import patch
from types import SimpleNamespace
import io
import os
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import diagnostics
import import_game as worker
import test_source_setup as source


class DiagnosticsTests(source.SourceSetupTests):
    def tearDown(self):
        diagnostics.finish()

    def test_default_off_and_toggle_without_import(self):
        diagnostics.start(self.package, 'status')
        diagnostics.record('not retained')
        self.assertFalse((self.package / 'GameData').exists())
        code, rows = self.command('diagnostics', '--logging', 'on')
        self.assertEqual(code, 0)
        self.assertTrue(rows[-1]['diagnostic_logging'])
        logs = self.package / 'GameData/logs'
        self.assertTrue(list(logs.glob('launcher-*.log')))
        before = set(logs.iterdir())
        self.assertEqual(self.command('diagnostics', '--logging', 'off')[0], 0)
        self.assertFalse(diagnostics.enabled(self.package))
        self.assertTrue(before <= set(logs.iterdir()))
        snapshot = set(logs.iterdir())
        self.command('status')
        self.assertEqual(set(logs.iterdir()), snapshot)

    def test_failure_logging_and_invalid_choice(self):
        self.command('diagnostics', '--logging', 'on')
        self.assertEqual(self.command('textures')[0], 1)
        text = ''.join(p.read_text() for p in (self.data / 'logs').glob('launcher-*.log'))
        self.assertIn('Select the HD texture ZIP', text)
        self.assertEqual(self.command('diagnostics', '--logging', 'maybe')[0], 1)
        self.assertTrue(diagnostics.enabled(self.package))

    def test_retention_cap_and_preserved_other_files(self):
        diagnostics.configure(self.package, 'on')
        for i in range(35):
            session = diagnostics.Session(self.package, 'game', 128)
            session.append(b'x' * 256)
            session.append('ignored')
            session.close()
        logs = self.data / 'logs'
        files = list(logs.glob('game-*.log'))
        self.assertEqual(len(files), 30)
        self.assertTrue(all(p.stat().st_size == 128 for p in files))
        self.assertTrue(all(b'limit reached' in p.read_bytes() for p in files))
        other = logs / 'my-notes.txt'; other.write_text('keep')
        diagnostics.Session(self.package, 'game', 128).close()
        self.assertEqual(other.read_text(), 'keep')

    def test_linked_marker_and_log_path_fail_closed(self):
        self.data.mkdir()
        external = self.root / 'private'; external.write_bytes(b'1\n')
        marker = self.data / '.diagnostic-logging'
        os.link(external, marker)
        self.assertFalse(diagnostics.enabled(self.package))
        with self.assertRaises(ValueError):diagnostics.configure(self.package, 'on')
        marker.unlink()
        diagnostics.configure(self.package, 'on')
        (self.data / 'logs').write_bytes(b'not a directory')
        session = diagnostics.Session(self.package, 'game', 128)
        session.append('does not break setup');session.close()
        self.assertEqual(external.read_bytes(), b'1\n')

    def test_runtime_policy_on_off_selected_install_and_failure(self):
        self.setup()
        saved = self.preserve_state()
        (self.data / 'xbe-runtime/settings.toml').write_text('language = 0\n')
        saved = source.file_snapshot(self.data / 'xbe-runtime')
        # A linked external installation still logs in the current launcher's folder.
        data = self.root / 'existing'; self.data.rename(data)
        for active in (False, True, False):
            diagnostics.configure(self.package, 'on' if active else 'off')
            calls = []
            def popen(command, **kwargs):
                calls.append(kwargs)
                return SimpleNamespace(stderr=io.BytesIO(b'runtime detail\n'), wait=lambda: 0)
            with patch.dict(os.environ, {'UCRR_SESSION_LOG':'1','XBE_CAPTURE_DIR':'bad','BURNER_AIM_LOG':'bad'}), patch.object(worker.subprocess, 'Popen', popen):
                worker.launch(data)
            env = calls[-1]['env']
            self.assertEqual(env.get('UCRR_SESSION_LOG'), '1' if active else None)
            self.assertNotIn('XBE_CAPTURE_DIR',env)
            self.assertNotIn('BURNER_AIM_LOG',env)
            self.assertTrue(all((data/'xbe-runtime'/n).read_bytes()==v for n,v in saved.items()))
        files = list((self.data/'logs').glob('game-*.log'))
        self.assertEqual(len(files),1)
        self.assertIn('runtime detail',files[0].read_text())
        self.assertIn('Runtime SHA256',files[0].read_text())
        with patch.object(worker.subprocess,'Popen',return_value=SimpleNamespace(stderr=io.BytesIO(b'failure'),wait=lambda:2)):
            with self.assertRaises(RuntimeError):worker.launch(data)
        self.assertIn(b'failure',(data/'last-error.txt').read_bytes())
        self.assertEqual(len(list((self.data/'logs').glob('game-*.log'))),1)


if __name__ == '__main__':unittest.main()
