"""Shared setup derivation, upgrades and rollback; no game process is launched."""
from pathlib import Path
import hashlib
import io
import json
import os
import shutil
import stat
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.dont_write_bytecode = True
sys.path[:0] = [str(HERE), str(REPO / 'scripts'),
               str(REPO / 'third_party/xboxrecomp/tools/xiso')]
import import_game as worker
import package as packager
import coop_assets
import coop_turret_assets
import coop_injection_assets
from test_coop_assets import archive
from analyze_frontend import parse_archive, unpack_entry
from test_setup import make_iso


def digest(content):
    return hashlib.sha256(content).hexdigest()


def snapshot(path):
    return {p.relative_to(path).as_posix(): p.read_bytes() for p in path.rglob('*') if p.is_file()}


class CoopSetup(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='coop-setup-', dir=REPO / 'build')
        self.base = Path(self.temp.name)
        self.package = self.base / 'package'
        self.tools = self.package / 'tools'
        self.tools.mkdir(parents=True)
        (self.package / 'runtime').mkdir()
        for name in ('coop_assets.py', 'coop_turret_assets.py', 'coop_injection_assets.py', 'analyze_frontend.py'):
            shutil.copy2(REPO / 'scripts' / name, self.tools / name)
        required = list(coop_assets.ROOTS) + list(coop_assets.ANIMATIONS) + sorted(coop_assets.TEMPLATES)
        required += list(coop_assets.LIFECYCLE_RESOURCES)
        required += [root[:-4] + ext for root in coop_assets.ROOTS for ext in ('.mdlhk', '.entsupp')]
        source = self.base / 'original.ent'
        archive(source, [(name, b'original fixture', 0) for name in required] +
                [('40-01.ent', b'world excluded', 0), ('hud\\menu.tga', b'UI excluded', 0)])
        output = self.base / 'expected.lst'
        report = coop_assets.build(source, output)
        self.expected = output.read_bytes()
        self.recipe = {key: report[key] for key in ('size', 'sha256')}
        self.recipe['version'] = 1
        self.files = {'Default.xbe': b'fixture code', '40-01.ent': source.read_bytes()}
        self.manifest = {'files': {name: {'size': len(content), 'sha256': digest(content)}
                                  for name, content in self.files.items()}}
        (self.tools / 'disc-manifest.json').write_text(json.dumps(self.manifest))
        runtime = {}
        for name in worker.RUNTIME_FILES:
            content = ('fixture ' + name).encode()
            (self.package / 'runtime' / name).write_bytes(content)
            runtime[name] = digest(content)
        self.info = {'version': 'test', 'files': runtime, 'coop_assets': self.recipe}
        (self.tools / 'runtime-manifest.json').write_text(json.dumps(self.info))
        self.data = self.base / 'GameData'
        self.game = self.data / 'installed/game_files'
        self.game.mkdir(parents=True)
        for name, content in self.files.items():
            (self.game / name).write_bytes(content)
        (self.game.parent / 'installed.json').write_text(json.dumps({
            'disc_manifest': worker.sha(self.tools / 'disc-manifest.json')}))
        self.dest = self.game / 'coopcharacters.lst'
        for name, content in {'settings.toml': b'language = 0\nfps_cap = 120\n',
                              'xboxrecomp.ini': b'custom bindings', 'hdd/profile': b'saved progress',
                              'shader-cache/learned': b'user shader cache'}.items():
            path = self.data / 'xbe-runtime' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        self.keep = snapshot(self.data)
        self.patches = [patch.object(worker, 'ROOT', self.package),
                        patch.object(worker, 'progress', lambda *args: None),
                        patch.dict(os.environ, {'LOCALAPPDATA': str(self.base / 'unused-appdata')})]
        for p in self.patches:
            p.start()
        self.cancel = self.base / 'cancel'

    def tearDown(self):
        for p in reversed(self.patches):
            p.stop()
        self.temp.cleanup()

    def assert_preserved(self):
        now = snapshot(self.data)
        for key, content in self.keep.items():
            self.assertEqual(now[key], content, key)
        self.assertFalse(list(self.game.glob('.coop-characters-*')))

    def test_existing_install_repair_and_atomic_failure(self):
        worker.ensure_coop_assets(self.data)
        self.assertEqual(self.dest.read_bytes(), self.expected)
        before = snapshot(self.data)
        with patch.object(worker.importlib.util, 'spec_from_file_location', side_effect=AssertionError('warm rebuild')):
            worker.ensure_coop_assets(self.data)
        self.assertEqual(snapshot(self.data), before)
        # Same-size damage is repaired, while the source and every user file stay unchanged.
        self.dest.write_bytes(b'x' * len(self.expected))
        worker.ensure_coop_assets(self.data)
        self.assertEqual(self.dest.read_bytes(), self.expected)
        self.dest.write_bytes(b'old local derivative')
        before = snapshot(self.data)
        with patch.object(worker.os, 'replace', side_effect=OSError('fixture publish failure')):
            with self.assertRaises(OSError):
                worker.ensure_coop_assets(self.data)
        self.assertEqual(snapshot(self.data), before)
        # Candidate identity failure and cancellation preserve the previous derivative.
        wrong = dict(self.info, coop_assets=dict(self.recipe, sha256='0' * 64))
        with self.assertRaises(ValueError):
            worker.ensure_coop_assets(self.data, wrong)
        self.assertEqual(snapshot(self.data), before)
        calls = 0
        def cancel_during_build(path):
            nonlocal calls
            calls += 1
            if calls > 4:
                raise RuntimeError('fixture cancellation')
        with patch.object(worker, 'check_cancel', cancel_during_build):
            with self.assertRaises(RuntimeError):
                worker.ensure_coop_assets(self.data, cancel=self.cancel)
        self.assertEqual(snapshot(self.data), before)
        self.assert_preserved()

    def test_incomplete_archive_upgrade(self):
        self.dest.write_bytes(self.expected)
        raw, entries = parse_archive(self.dest)
        for omitted in (coop_assets.LIFECYCLE_RESOURCES, coop_assets.WEAPON_ROOTS):
            archive(self.dest, [(e['name'], unpack_entry(raw, e), e['bank']) for e in entries
                                if e['name'].lower() not in omitted])
            self.assertNotEqual(self.dest.read_bytes(), self.expected)
            worker.ensure_coop_assets(self.data)
            self.assertEqual(self.dest.read_bytes(), self.expected)
            self.assert_preserved()

    def test_turret_archive_install_repair_and_isolation(self):
        owned = REPO / 'game_files'
        if not all((owned / (level + '.ent')).is_file() for level in coop_turret_assets.SPECS):
            self.skipTest('Owned original helicopter archives required')
        shutil.copy2(REPO / 'scripts/coop_turret_assets.py', self.tools / 'coop_turret_assets.py')
        for level in coop_turret_assets.SPECS:
            name = level + '.ent'
            shutil.copy2(owned / name, self.game / name)
            self.manifest['files'][name] = {'size': (self.game / name).stat().st_size,
                                          'sha256': worker.sha(self.game / name)}
        (self.tools / 'disc-manifest.json').write_text(json.dumps(self.manifest))
        expected = self.base / 'turrets.lst'
        report = coop_turret_assets.build_archive(self.game, expected, level='3-01')
        recipe = dict(version=1, size=report['size'], sha256=report['sha256'])
        info = dict(self.info, coop_turrets03=recipe)
        dest = self.game / 'coopturrets03.lst'
        worker.ensure_coop_assets(self.data, info)
        self.assertEqual(dest.read_bytes(), expected.read_bytes())
        self.assertEqual(self.dest.read_bytes(), self.expected)
        before = snapshot(self.data)
        with patch.object(worker.importlib.util, 'spec_from_file_location', side_effect=AssertionError('warm rebuild')):
            worker.ensure_coop_assets(self.data, info)
        self.assertEqual(snapshot(self.data), before)
        dest.write_bytes(b'previous turret derivative')
        before = snapshot(self.data)
        with patch.object(worker.os, 'replace', side_effect=OSError('fixture publish failure')):
            with self.assertRaises(OSError):
                worker.ensure_coop_assets(self.data, info)
        self.assertEqual(snapshot(self.data), before)
        for invalid in (None, {}, dict(recipe, version=True), dict(recipe, sha256='0' * 64)):
            with self.assertRaises(ValueError):
                worker.ensure_coop_assets(self.data, dict(info, coop_turrets03=invalid))
            self.assertEqual(snapshot(self.data), before)
        with patch.object(worker, 'check_cancel', side_effect=RuntimeError('cancelled')):
            with self.assertRaises(RuntimeError):
                worker.ensure_coop_assets(self.data, info, self.cancel)
        self.assertEqual(snapshot(self.data), before)
        worker.ensure_coop_assets(self.data, info)
        self.assertEqual(dest.read_bytes(), expected.read_bytes())
        second = self.base / 'turrets08.lst'
        report = coop_turret_assets.build_archive(self.game, second, level='8-01')
        info['coop_turrets08'] = dict(version=1, size=report['size'], sha256=report['sha256'])
        worker.ensure_coop_assets(self.data, info)
        self.assertEqual((self.game / 'coopturrets08.lst').read_bytes(), second.read_bytes())
        self.assertEqual(dest.read_bytes(), expected.read_bytes())
        for level in coop_turret_assets.SPECS:
            source = self.game / (level + '.ent')
            self.assertEqual(worker.sha(source), worker.sha(owned / source.name))
            original = source.read_bytes()
            source.write_bytes(bytes([original[0] ^ 1]) + original[1:])
            before = snapshot(self.data)
            with self.assertRaises(ValueError):
                worker.ensure_coop_assets(self.data, info)
            self.assertEqual(snapshot(self.data), before)
            source.write_bytes(original)
        self.assert_preserved()

    def test_original_identity_and_recipe_validation(self):
        for invalid in (None, {}, [], dict(self.recipe, version=True),
                        dict(self.recipe, size=0), dict(self.recipe, sha256='bad'),
                        dict(self.recipe, source='../escape')):
            with self.assertRaises(ValueError):
                worker.ensure_coop_assets(self.data, dict(self.info, coop_assets=invalid))
        worker.ensure_coop_assets(self.data, {'files': self.info['files']})
        self.assertFalse(self.dest.exists())  # Old packages retain their existing behavior.
        source = self.game / '40-01.ent'
        source.write_bytes(b'x' * len(self.files['40-01.ent']))
        before = snapshot(self.data)
        with self.assertRaises(ValueError):
            worker.ensure_coop_assets(self.data)
        self.assertEqual(snapshot(self.data), before)
        self.assertFalse(self.dest.exists())

    def injection_fixture(self):
        source = REPO / 'game_files/1-01.ent'
        if not source.is_file():
            self.skipTest('Owned original first-mission archive required')
        self.files[source.name] = source.read_bytes()
        (self.game / source.name).write_bytes(self.files[source.name])
        self.manifest['files'][source.name] = {
            'size': len(self.files[source.name]), 'sha256': digest(self.files[source.name])}
        (self.tools / 'disc-manifest.json').write_text(json.dumps(self.manifest))
        (self.game.parent / 'installed.json').write_text(json.dumps({
            'disc_manifest': worker.sha(self.tools / 'disc-manifest.json')}))
        expected = self.base / 'injection.lst'
        report = coop_injection_assets.build_archive(self.game, expected)
        recipe = dict(version=1, size=report['size'], sha256=report['sha256'])
        self.assertEqual(recipe, packager.COOP_INJECTION)
        self.info['coop_injection'] = recipe
        (self.tools / 'runtime-manifest.json').write_text(json.dumps(self.info))
        self.keep = snapshot(self.data)
        return self.game / 'coopinjection.lst', expected.read_bytes()

    def test_injection_install_repair_atomic_failure_and_cancel(self):
        dest, expected = self.injection_fixture()
        worker.ensure_coop_assets(self.data)
        self.assertEqual(dest.read_bytes(), expected)
        self.assertEqual(self.dest.read_bytes(), self.expected)
        _, records = parse_archive(dest)
        self.assertEqual([entry['name'] for entry in records], [coop_injection_assets.ALIAS])
        before = snapshot(self.data)
        with patch.object(worker.importlib.util, 'spec_from_file_location', side_effect=AssertionError('warm rebuild')):
            worker.ensure_coop_assets(self.data)
        self.assertEqual(snapshot(self.data), before)
        dest.write_bytes(b'x' * len(expected))
        worker.ensure_coop_assets(self.data)
        self.assertEqual(dest.read_bytes(), expected)
        dest.write_bytes(b'previous partner animation')
        before = snapshot(self.data)
        with patch.object(worker.os, 'replace', side_effect=OSError('fixture publish failure')):
            with self.assertRaises(OSError):
                worker.ensure_coop_assets(self.data)
        self.assertEqual(snapshot(self.data), before)
        for invalid in (None, {}, dict(self.info['coop_injection'], version=True),
                        dict(self.info['coop_injection'], sha256='0' * 64)):
            with self.assertRaises(ValueError):
                worker.ensure_coop_assets(self.data, dict(self.info, coop_injection=invalid))
            self.assertEqual(snapshot(self.data), before)
        calls = 0
        def cancel_after_archive_write(path):
            nonlocal calls
            calls += 1
            if calls == 5:
                self.assertTrue(list(self.game.glob('.coop-characters-*')))
                raise RuntimeError('fixture cancellation after archive write')
        with patch.object(worker, 'check_cancel', cancel_after_archive_write):
            with self.assertRaises(RuntimeError):
                worker.ensure_coop_assets(self.data, cancel=self.cancel)
        self.assertEqual(calls, 5)
        self.assertEqual(snapshot(self.data), before)
        helper = self.tools / 'coop_injection_assets.py'
        helper.unlink()
        with self.assertRaises(ValueError):
            worker.ensure_coop_assets(self.data)
        self.assertEqual(snapshot(self.data), before)
        shutil.copy2(REPO / 'scripts/coop_injection_assets.py', helper)
        worker.ensure_coop_assets(self.data)
        self.assertEqual(dest.read_bytes(), expected)
        self.assert_preserved()
        source = self.game / '1-01.ent'
        original = source.read_bytes()
        source.write_bytes(bytes([original[0] ^ 1]) + original[1:])
        before = snapshot(self.data)
        with self.assertRaises(ValueError):
            worker.ensure_coop_assets(self.data)
        self.assertEqual(snapshot(self.data), before)

    def test_injection_iso_import_launch_and_managed_paths(self):
        dest, expected = self.injection_fixture()
        self.keep.pop('installed/installed.json')  # A successful import writes its new verification marker.
        iso = self.base / 'injection.iso'
        make_iso(iso, self.files)
        worker.import_iso(iso, self.data, self.cancel)
        self.assertTrue(worker.ready(self.data))
        self.assertEqual(dest.read_bytes(), expected)
        self.assert_preserved()
        # Launch repairs the derived animation before starting the runtime.
        dest.unlink()
        calls = []
        class Process:
            def __init__(process, args, **kwargs):
                self.assertEqual(dest.read_bytes(), expected)
                calls.append(args)
                process.stderr = io.BytesIO()
            def wait(process):
                return 0
        with patch.object(worker.subprocess, 'Popen', Process):
            worker.launch(self.data)
        self.assertEqual(len(calls), 1)
        before = snapshot(self.data)
        original_stat = Path.lstat
        redirected = self.base
        def linked_stat(path, *args, **kwargs):
            if path == redirected:
                return SimpleNamespace(st_mode=stat.S_IFLNK, st_file_attributes=0)
            return original_stat(path, *args, **kwargs)
        with patch.object(Path, 'lstat', linked_stat):
            worker.ensure_coop_assets(self.data)
        for redirected in (self.game / '1-01.ent', dest):
            with patch.object(Path, 'lstat', linked_stat):
                with self.assertRaises(ValueError):
                    worker.ensure_coop_assets(self.data)
            self.assertEqual(snapshot(self.data), before)
        bad = dict(self.info, coop_injection=dict(self.info['coop_injection'], sha256='0' * 64))
        (self.tools / 'runtime-manifest.json').write_text(json.dumps(bad))
        with self.assertRaises(ValueError):
            worker.import_iso(iso, self.data, self.cancel)
        self.assertEqual(snapshot(self.data), before)
        self.assertFalse(list(self.data.glob('.import-*')))
        self.assert_preserved()

    def test_fresh_iso_import_and_failure_rollback(self):
        iso = self.base / 'fixture.iso'
        make_iso(iso, self.files)
        worker.import_iso(iso, self.data, self.cancel)
        self.assertTrue(worker.ready(self.data))
        self.assertEqual(self.dest.read_bytes(), self.expected)
        for name, content in self.files.items():
            self.assertEqual((self.game / name).read_bytes(), content)
        before = snapshot(self.data)
        info = dict(self.info, coop_assets=dict(self.recipe, sha256='0' * 64))
        (self.tools / 'runtime-manifest.json').write_text(json.dumps(info))
        with self.assertRaises(ValueError):
            worker.import_iso(iso, self.data, self.cancel)
        self.assertEqual(snapshot(self.data), before)
        self.assertFalse(list(self.data.glob('.import-*')))

    def test_upgrade_runs_before_process_and_packaging_has_only_tools(self):
        calls = []
        class Process:
            def __init__(process, args, **kwargs):
                self.assertEqual(self.dest.read_bytes(), self.expected)
                self.assertNotIn('UCRR_COOP', kwargs['env'])
                calls.append(args)
                process.stderr = io.BytesIO()
            def wait(process):
                return 0
        with patch.object(worker.subprocess, 'Popen', Process):
            worker.launch(self.data)
        self.assertEqual(len(calls), 1)
        self.assert_preserved()
        plan = packager.sources(self.package / 'runtime' / worker.RUNTIME_NAME)
        self.assertEqual(plan['tools/coop_assets.py'], REPO / 'scripts/coop_assets.py')
        self.assertEqual(plan['tools/coop_turret_assets.py'], REPO / 'scripts/coop_turret_assets.py')
        self.assertEqual(plan['tools/coop_injection_assets.py'], REPO / 'scripts/coop_injection_assets.py')
        self.assertEqual(plan['tools/analyze_frontend.py'], REPO / 'scripts/analyze_frontend.py')
        self.assertFalse(any(Path(name).suffix == '.lst' for name in plan))

    def test_normal_launch_sanitizes_environment_and_keeps_validation(self):
        import setup_feedback
        inherited = {'UCRR_COOP_TEST': '1', 'UCRR_GAME_ASSETS': 'wrong',
                     'UCRR_NATIVE_DIAGNOSTICS': '1', 'XBE_PERF': '1',
                     'XBOX_LOG_GPU': '1', 'BURNER_AIM_LOG': 'unrequested.log',
                     'SteamAppId': 'fixture-steam-id'}
        calls = []
        class Process:
            def __init__(process, args, **kwargs):
                self.assertEqual(self.dest.read_bytes(), self.expected)
                self.assertEqual(args, [str(self.data / 'xbe-runtime' / worker.RUNTIME_NAME)])
                self.assertEqual(kwargs['cwd'], self.data / 'xbe-runtime')
                calls.append(kwargs['env'])
                process.stderr = io.BytesIO()
            def wait(process):
                return 0
        for inherited_coop in ('0', '1'):
            with self.subTest(inherited_coop=inherited_coop):
                inherited['UCRR_COOP_TEST'] = inherited_coop
                args = ['import_game.py', 'launch', '--data', str(self.data), '--interactive']
                with patch.object(sys, 'argv', args), patch.dict(os.environ, inherited), \
                        patch.object(worker.subprocess, 'Popen', Process), \
                        patch.object(setup_feedback, 'notify') as notify:
                    self.assertEqual(worker.main(), 0)
                    self.assertEqual(os.environ['UCRR_GAME_ASSETS'], 'wrong')
                    notify.assert_not_called()  # A successful launch does not open setup dialogs.
                env = calls[-1]
                expected = {'UCRR_GAME_ASSETS'}
                self.assertEqual({name for name in env if name.upper().startswith(('UCRR_', 'XBE_', 'XBOX_LOG_'))}, expected)
                self.assertEqual(env['UCRR_GAME_ASSETS'], str(self.game))
                self.assertNotIn('UCRR_COOP_TEST', env)
                self.assertNotIn('BURNER_AIM_LOG', env)
                self.assertEqual({key.upper(): value for key, value in env.items()}['STEAMAPPID'], inherited['SteamAppId'])
                self.assert_preserved()
        self.assertEqual(len(calls), 2)
        # The unified launcher retains the original installed-disc gate.
        (self.game / 'Default.xbe').write_bytes(b'invalid executable')
        with patch.object(worker.subprocess, 'Popen') as process:
            with self.assertRaises(ValueError):
                worker.launch(self.data)
            process.assert_not_called()

    def test_removed_private_option_is_not_a_second_launch_path(self):
        import setup_feedback
        for action in ('launch', 'import', 'status', 'textures', 'voices', 'language'):
            with self.subTest(action=action):
                before = snapshot(self.data)
                args = ['import_game.py', action, '--coop-test', '--data', str(self.data), '--interactive']
                with patch.object(sys, 'argv', args), \
                        patch.object(worker, 'ready', side_effect=AssertionError('unexpected dispatch')), \
                        patch.object(worker, 'import_iso', side_effect=AssertionError('unexpected import')), \
                        patch.object(worker, 'language_settings', side_effect=AssertionError('unexpected language write')), \
                        patch.object(worker.subprocess, 'Popen') as process, \
                        patch.object(setup_feedback, 'report_error', return_value='rejected') as report, \
                        patch.object(setup_feedback, 'notify') as notify:
                    self.assertEqual(worker.main(), 1)
                    self.assertIsInstance(report.call_args.args[2], ValueError)
                    self.assertEqual(str(report.call_args.args[2]), 'unrecognized arguments: --coop-test')
                    notify.assert_called_once_with('rejected', error=True)
                    process.assert_not_called()
                self.assertEqual(snapshot(self.data), before)

    def test_managed_links_rejected_host_mappings_allowed(self):
        original_stat = Path.lstat
        redirected = self.base
        def linked_stat(path, *args, **kwargs):
            if path == redirected:
                return SimpleNamespace(st_mode=stat.S_IFLNK, st_file_attributes=0)
            return original_stat(path, *args, **kwargs)
        # Proton's mapped host ancestors are permitted; managed redirects are not.
        with patch.object(Path, 'lstat', linked_stat):
            worker.ensure_coop_assets(self.data)
        before = snapshot(self.data)
        for redirected in (self.data, self.game, self.game / '40-01.ent', self.dest):
            with patch.object(Path, 'lstat', linked_stat):
                with self.assertRaises(ValueError):
                    worker.ensure_coop_assets(self.data)
            self.assertEqual(snapshot(self.data), before)

    def test_package_recipe_and_no_original_archive_payload(self):
        metadata = self.tools / 'runtime-manifest.json'
        good = dict(self.info, coop_assets=packager.COOP_ASSETS, **packager.COOP_TURRETS,
                    coop_injection=packager.COOP_INJECTION)
        metadata.write_text(json.dumps(good))
        allowed = set(snapshot(self.package))
        def audit():
            return packager.audit(self.package, allowed, require_launchers=False)
        self.assertEqual(audit()['original_payload_files_in_package'], 0)
        for invalid in (None, [], {}, dict(packager.COOP_ASSETS, version=True),
                        dict(packager.COOP_ASSETS, sha256='0' * 64)):
            metadata.write_text(json.dumps(dict(good, coop_assets=invalid)))
            with self.assertRaises(ValueError):
                audit()
        for key, recipe in dict(packager.COOP_TURRETS, coop_injection=packager.COOP_INJECTION).items():
            for invalid in (None, [], {}, dict(recipe, version=True), dict(recipe, sha256='0' * 64)):
                metadata.write_text(json.dumps(dict(good, **{key: invalid})))
                with self.assertRaises(ValueError):
                    audit()
        metadata.write_text(json.dumps(good))
        derivative = self.package / 'runtime/coopcharacters.lst'
        derivative.write_bytes(self.expected)
        with self.assertRaises(ValueError):
            packager.audit(self.package, allowed | {'runtime/coopcharacters.lst'}, require_launchers=False)
        derivative.unlink()
        helper = self.tools / 'coop_injection_assets.py'
        helper.unlink()
        with self.assertRaises(ValueError):
            packager.audit(self.package, allowed - {'tools/coop_injection_assets.py'}, require_launchers=False)
        shutil.copy2(REPO / 'scripts/coop_injection_assets.py', helper)
        (self.tools / 'analyze_frontend.py').unlink()
        with self.assertRaises(ValueError):
            packager.audit(self.package, allowed - {'tools/analyze_frontend.py'}, require_launchers=False)


if __name__ == '__main__':
    unittest.main()
