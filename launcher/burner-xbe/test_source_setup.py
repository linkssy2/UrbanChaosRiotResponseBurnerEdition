"""Direct ISO and shared-install workflows against small real XDVDFS fixtures."""
from pathlib import Path
from unittest.mock import patch
import io
import json
import os
import stat
import sys
import tempfile
import unittest
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parent))
from test_setup import make_iso, digest, file_snapshot, HERE
import import_game as worker


class SourceSetupTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='source-setup-', dir=HERE.parents[1] / 'build')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.package = self.root / 'package'
        (self.package / 'tools').mkdir(parents=True)
        (self.package / 'runtime').mkdir()
        self.data = self.package / 'GameData'
        self.cancel = self.package / '.cancel-test'
        self.files = {'Default.xbe': b'fixture executable', '1-01.ent': b'fixture level',
                      '40-01.ent': b'fixture characters', 'Movies/intro.xmv': b'fixture movie',
                      '1.saf': b'fixture audio', 'extra.lst': b'disc only'}
        self.manifest = self.package / 'tools/disc-manifest.json'
        self.manifest.write_text(json.dumps({'version': 1, 'files': {
            name: {'size': len(value), 'sha256': digest(value)} for name, value in self.files.items()}}))
        runtime = {}
        for name in worker.RUNTIME_FILES:
            value = ('fake-' + name).encode()
            (self.package / 'runtime' / name).write_bytes(value)
            runtime[name] = digest(value)
        (self.package / 'tools/runtime-manifest.json').write_text(json.dumps({'version': 'test', 'files': runtime}))
        self.iso = self.root / 'Xbox disc.iso'
        make_iso(self.iso, self.files)
        for name, value in (('ROOT', self.package), ('progress', lambda *args: None)):
            mocked = patch.object(worker, name, value)
            mocked.start()
            self.addCleanup(mocked.stop)
        env = patch.dict(os.environ, {}, clear=True)
        env.start()
        self.addCleanup(env.stop)

    def setup(self, *, direct=True):
        worker.import_iso(self.iso, self.data, self.cancel, direct=direct)

    def command(self, *args):
        output = io.StringIO()
        with patch.object(sys, 'argv', ['import_game.py', *args]), patch('sys.stdout', output):
            code = worker.main()
        return code, [json.loads(line) for line in output.getvalue().splitlines()]

    def preserve_state(self):
        for name in ('xbe-runtime/settings.toml', 'xbe-runtime/hdd/save',
                     'xbe-runtime/world-textures/pack', 'xbe-runtime/translations/es-voices/voices'):
            path = self.data / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(('keep ' + name).encode())
        return file_snapshot(self.data / 'xbe-runtime')

    def test_direct_setup_keeps_host_inputs_and_iso_unchanged(self):
        before = self.iso.read_bytes()
        self.setup()
        files = file_snapshot(self.data / 'installed/game_files')
        self.assertEqual(files, {name: value for name, value in self.files.items() if name in worker.SOURCE_FILES})
        self.assertEqual(self.iso.read_bytes(), before)
        self.assertEqual(worker.installation_status(self.data)['source_mode'], 'iso')
        self.assertTrue(worker.ready(self.data))
        self.assertTrue(worker.verify_installation(self.data)['ready'])

    def test_omitted_file_hash_still_rejects_wrong_disc(self):
        self.setup()
        saved = self.preserve_state()
        before = file_snapshot(self.data / 'installed')
        wrong = dict(self.files, **{'Movies/intro.xmv': b'x' * len(self.files['Movies/intro.xmv'])})
        other = self.root / 'wrong.iso'
        make_iso(other, wrong)
        with self.assertRaisesRegex(ValueError, 'verification failed: Movies/intro.xmv'):
            worker.import_iso(other, self.data, self.cancel, direct=True)
        self.assertEqual(file_snapshot(self.data / 'installed'), before)
        self.assertEqual(file_snapshot(self.data / 'xbe-runtime'), saved)
        self.assertFalse(list(self.data.glob('.import-*')))

    def test_cancellation_and_prepare_failure_preserve_installation(self):
        self.setup()
        saved = self.preserve_state()
        before = file_snapshot(self.data / 'installed')
        def cancel_after_read(*unused):
            self.cancel.touch()
        with patch.object(worker, 'progress', cancel_after_read), self.assertRaisesRegex(RuntimeError, 'cancelled'):
            self.setup()
        self.cancel.unlink()
        with patch.object(worker, '_ensure_coop_assets', side_effect=ValueError('fixture derivation failed')):
            with self.assertRaisesRegex(ValueError, 'derivation failed'):
                self.setup()
        self.assertEqual(file_snapshot(self.data / 'installed'), before)
        self.assertEqual(file_snapshot(self.data / 'xbe-runtime'), saved)
        self.assertFalse(list(self.data.glob('.import-*')))

    def test_relocating_iso_requires_reselection_but_keeps_saves(self):
        self.setup()
        saved = self.preserve_state()
        moved = self.root / 'Relocated.xiso'
        self.iso.rename(moved)
        state = worker.installation_status(self.data)
        self.assertFalse(state['ready'])
        self.assertIn('new location', state['message'])
        worker.import_iso(moved, self.data, self.cancel, direct=True)
        self.assertTrue(worker.ready(self.data))
        self.assertEqual(file_snapshot(self.data / 'xbe-runtime'), saved)

    def test_changed_iso_is_reported_without_automatic_replacement(self):
        self.setup()
        with self.iso.open('ab') as stream:
            stream.write(b'x')
        state = worker.installation_status(self.data)
        self.assertFalse(state['ready'])
        self.assertIn('ISO changed', state['message'])
        assets = self.package / 'assets'
        assets.mkdir()
        make_iso(assets / 'valid.iso', self.files)
        self.assertEqual(worker.auto_setup(self.data, self.cancel), state)

    def test_source_changed_after_read_does_not_commit(self):
        self.setup()
        before = file_snapshot(self.data / 'installed')
        def changed(*args, **kwargs):
            with self.iso.open('ab') as stream:
                stream.write(b'changed during derivation')
        with patch.object(worker, '_ensure_coop_assets', changed), self.assertRaisesRegex(ValueError, 'source ISO changed'):
            self.setup()
        self.assertEqual(file_snapshot(self.data / 'installed'), before)

    def test_reuse_installation_verifies_and_selects_without_copying(self):
        self.setup(direct=False)
        saved = self.preserve_state()
        other = self.root / 'New release'
        (other / 'tools').mkdir(parents=True)
        (other / 'tools/disc-manifest.json').write_bytes(self.manifest.read_bytes())
        with patch.object(worker, 'ROOT', other):
            selected = worker.select_installation(self.package)
            self.assertEqual(selected, self.data)
            self.assertEqual(worker.selected_data(), self.data)
            self.assertFalse((other / 'GameData').exists())
            self.assertEqual(worker.selected_data(self.root / 'override'), self.root / 'override')
            self.assertEqual(json.loads((other / 'installation.json').read_text())['data'], str(self.data))
        self.assertEqual(file_snapshot(self.data / 'xbe-runtime'), saved)
        worker.select_installation(self.data)
        self.assertEqual(json.loads((self.package / 'installation.json').read_text())['data'], 'GameData')

    def test_reuse_rejects_bad_content_and_preserves_old_pointer(self):
        self.setup(direct=False)
        worker.select_installation(self.data)
        pointer = (self.package / 'installation.json').read_bytes()
        local = self.data / 'installed/game_files/Movies/intro.xmv'
        local.write_bytes(b'x' * local.stat().st_size)
        # Lightweight startup checks size; explicitly choosing a source checks all bytes.
        self.assertTrue(worker.ready(self.data))
        with self.assertRaisesRegex(ValueError, 'verification failed'):
            worker.select_installation(self.data)
        self.assertEqual((self.package / 'installation.json').read_bytes(), pointer)

    def test_selection_atomic_failure_and_cancel_keep_previous_pointer(self):
        self.setup()
        worker.select_installation(self.data)
        pointer = (self.package / 'installation.json').read_bytes()
        with patch.object(os, 'replace', side_effect=OSError('failed publication')):
            with self.assertRaises(OSError):
                worker.select_installation(self.data)
        self.cancel.touch()
        with self.assertRaisesRegex(RuntimeError, 'cancelled'):
            worker.select_installation(self.data, self.cancel)
        self.assertEqual((self.package / 'installation.json').read_bytes(), pointer)
        self.assertFalse(list(self.package.glob('.installation-*')))

    def test_auto_setup_only_uses_unambiguous_first_run_iso(self):
        self.assertFalse(worker.auto_setup(self.data, self.cancel)['ready'])
        assets = self.package / 'assets'
        assets.mkdir()
        make_iso(assets / 'one.ISO', self.files)
        make_iso(assets / 'two.xiso', self.files)
        state = worker.auto_setup(self.data, self.cancel)
        self.assertFalse(state['ready'])
        self.assertIn('More than one', state['message'])
        self.assertFalse(self.data.exists())
        (assets / 'two.xiso').unlink()
        state = worker.auto_setup(self.data, self.cancel)
        self.assertTrue(state['ready'])
        self.assertEqual(state['iso_path'], str(assets / 'one.ISO'))
        (assets / 'irrelevant.iso').write_bytes(b'not a game')
        self.assertEqual(worker.auto_setup(self.data, self.cancel), state)

    def test_selected_missing_install_is_not_overwritten_by_assets(self):
        assets = self.package / 'assets'
        assets.mkdir()
        make_iso(assets / 'disc.iso', self.files)
        missing = self.root / 'disconnected drive' / 'GameData'
        (self.package / 'installation.json').write_text(json.dumps({'version': 1, 'data': str(missing)}))
        self.assertFalse(worker.auto_setup(worker.selected_data(), self.cancel)['ready'])
        self.assertFalse(missing.exists())

    def test_borrowed_audio_sources_are_scoped_and_authenticated(self):
        self.setup()
        for failure in (False, True):
            with self.subTest(failure=failure):
                try:
                    with worker.original_sources(self.data, {'1.saf', 'Movies/intro.xmv'}, self.cancel) as original:
                        self.assertEqual(file_snapshot(original), {name: self.files[name] for name in ('1.saf', 'Movies/intro.xmv')})
                        self.assertFalse((self.data / 'installed/game_files/1.saf').exists())
                        if failure:
                            raise RuntimeError('conversion cancelled')
                except RuntimeError:
                    self.assertTrue(failure)
                self.assertFalse(original.exists())
        with self.assertRaisesRegex(ValueError, 'Unknown original'):
            with worker.original_sources(self.data, {'../escape'}, self.cancel):
                self.fail('unsafe source accepted')
        self.cancel.touch()
        with self.assertRaisesRegex(RuntimeError, 'cancelled'):
            with worker.original_sources(self.data, {'1.saf'}, self.cancel):
                self.fail('cancelled extraction accepted')
        self.assertFalse(list(self.data.glob('.source-*')))

    def test_launch_routes_iso_and_existing_install_without_leaking_diagnostics(self):
        self.setup()
        seen = []
        class Process:
            def __init__(self, args, **kwargs):
                seen.append(kwargs)
                self.stderr = io.BytesIO()
            def wait(self):
                return 0
        with patch.object(worker.subprocess, 'Popen', Process), patch.dict(os.environ, {'UCRR_GAME_ISO': 'wrong', 'XBE_TEST': '1'}):
            worker.launch(self.data)
        self.assertEqual(seen[-1]['env']['UCRR_GAME_ISO'], str(self.iso))
        self.assertEqual(seen[-1]['env']['UCRR_GAME_ASSETS'], str(self.data / 'installed/game_files'))
        self.assertNotIn('XBE_TEST', seen[-1]['env'])
        self.setup(direct=False)
        with patch.object(worker.subprocess, 'Popen', Process), patch.dict(os.environ, {'UCRR_GAME_ISO': 'wrong'}):
            worker.launch(self.data)
        self.assertNotIn('UCRR_GAME_ISO', seen[-1]['env'])

    def test_status_and_actions_share_selection(self):
        code, rows = self.command('status')
        self.assertEqual(code, 2)
        self.assertFalse(rows[-1]['ready'])
        code, _ = self.command('use-iso', '--iso', str(self.iso), '--cancel', str(self.cancel))
        self.assertEqual(code, 0)
        code, rows = self.command('status')
        self.assertEqual(code, 0)
        self.assertEqual(rows[-1]['iso_path'], str(self.iso))
        code, rows = self.command('verify')
        self.assertEqual(code, 0)
        self.assertTrue(rows[-1]['ready'])
        code, rows = self.command('use-installation', '--installation', str(self.package))
        self.assertEqual(code, 0)
        self.assertEqual(rows[-1]['data'], str(self.data))

    def test_shared_install_lock_rejects_second_worker_then_releases(self):
        self.data.mkdir()
        with worker.setup_lock(self.data):
            with self.assertRaisesRegex(ValueError, 'Close the game'):
                with worker.setup_lock(self.data):
                    self.fail('concurrent setup accepted')
            with worker.setup_lock(self.root / 'other data'):
                pass
        with worker.setup_lock(self.data):
            pass

    def test_pointer_traversal_and_linked_managed_content_are_rejected(self):
        pointer = self.package / 'installation.json'
        pointer.write_text(json.dumps({'version': 1, 'data': '../escape'}))
        with self.assertRaisesRegex(ValueError, 'Invalid selected'):
            worker.selected_data()
        pointer.unlink()
        self.setup()
        local = self.data / 'installed/game_files/1-01.ent'
        original_lstat = Path.lstat
        def linked_lstat(path, *args, **kwargs):
            if path == local:
                return SimpleNamespace(st_mode=stat.S_IFLNK, st_file_attributes=0)
            return original_lstat(path, *args, **kwargs)
        with patch.object(Path, 'lstat', linked_lstat):
            self.assertFalse(worker.ready(self.data))
            with self.assertRaisesRegex(ValueError, 'links or junctions'):
                worker.verify_installation(self.data)


if __name__ == '__main__':
    unittest.main()
