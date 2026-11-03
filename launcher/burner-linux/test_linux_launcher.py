"""Exercise the real Bash entry point with a recording Proton substitute; no game launch."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
BASH = os.environ.get('BURNER_TEST_BASH') or (
    'C:/Program Files/Git/bin/bash.exe' if os.name == 'nt' else shutil.which('bash'))


class LinuxLauncherTests(unittest.TestCase):
    def setUp(self):
        base = REPO / 'build/linux-launcher-fixtures'
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix='case-', dir=base)
        self.folder = Path(self.temp.name).resolve()
        self.assertTrue(self.folder.is_relative_to(base.resolve()))
        self.root = self.folder / "Burner Edition ' folder"
        self.root.mkdir()
        self.script = self.root / 'burner.sh'
        shutil.copyfile(HERE / 'burner.sh', self.script)
        for name in ('tools/python/python.exe', 'tools/import_game.py',
                     'tools/runtime-manifest.json', 'runtime/UrbanChaosXbeTrial.exe'):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        self.steam = self.folder / 'Steam client'
        (self.steam / 'steamapps').mkdir(parents=True)
        self.proton = self.folder / 'Proton choice' / 'proton'
        self.proton.parent.mkdir()
        self.proton.write_text('''#!/usr/bin/env bash
set -eu
printf 'CALL\\0%s\\0%s\\0%s\\0' "$PWD" "$STEAM_COMPAT_DATA_PATH" "$STEAM_COMPAT_CLIENT_INSTALL_PATH" >> "$BURNER_CALL_LOG"
printf '%s\\0' "$@" >> "$BURNER_CALL_LOG"
printf 'END\\0' >> "$BURNER_CALL_LOG"
if [[ -n ${BURNER_ENV_LOG:-} ]]; then
  printf '%s\\0%s\\0%s\\0' "${PROTON_LOG:-}" "${PROTON_LOG_DIR:-}" "${VKD3D_DEBUG:-}" > "$BURNER_ENV_LOG"
fi
case "$1" in
  getcompatpath)
    if [[ ${BURNER_FAIL_CONVERT:-0} != 0 ]]; then exit "$BURNER_FAIL_CONVERT"; fi
    if [[ ${BURNER_EMPTY_CONVERT:-0} == 1 ]]; then exit 0; fi
    printf 'Z:%s\\r\\n' "$2" ;;
  waitforexitandrun) exit "${BURNER_RUN_EXIT:-0}" ;;
  *) exit 88 ;;
esac
''', encoding='utf-8', newline='\n')
        self.proton.chmod(0o755)
        self.log = self.folder / 'calls.bin'
        self.env = dict(os.environ, BURNER_STEAM=self.steam.as_posix(),
                        BURNER_PROTON=self.proton.as_posix(), BURNER_CALL_LOG=self.log.as_posix())
        for key in ('BURNER_FAIL_CONVERT', 'BURNER_EMPTY_CONVERT', 'BURNER_RUN_EXIT', 'BURNER_ENV_LOG'):
            self.env.pop(key, None)

    def tearDown(self):
        self.assertTrue(self.folder.is_relative_to((REPO / 'build/linux-launcher-fixtures').resolve()))
        self.temp.cleanup()

    def run_script(self, *args, env=None):
        return subprocess.run([BASH, self.script.as_posix(), *args], cwd=self.folder,
                              env=env or self.env, capture_output=True, text=True)

    def calls(self):
        records = []
        if not self.log.exists():
            return records
        parts = self.log.read_bytes().decode().split('\0')
        while parts and parts[0]:
            self.assertEqual(parts.pop(0), 'CALL')
            end = parts.index('END')
            records.append(parts[:end])
            del parts[:end + 1]
        return records

    def test_syntax_and_help_without_runtime(self):
        check = subprocess.run([BASH, '-n', self.script.as_posix()], capture_output=True)
        self.assertEqual(check.returncode, 0, check.stderr)
        (self.root / 'tools/python/python.exe').unlink()
        result = self.run_script('--help')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('not a native Linux port', result.stdout)
        self.assertIn('voices "/path/to/Urban Chaos PS2 Europe.iso"', result.stdout)
        self.assertIn('SLES-53991', result.stdout)
        self.assertIn('use-iso', result.stdout)
        self.assertIn('use-installation', result.stdout)
        self.assertIn('is not required', result.stdout)
        self.assertFalse((self.root / '.proton').exists())

    def test_play_preserves_prefix_and_game_state(self):
        (self.root / '.proton').mkdir()
        marker = self.root / '.proton/existing-prefix'
        marker.write_bytes(b'preserve prefix')
        saved = self.root / 'GameData/xbe-runtime/hdd/save'
        saved.parent.mkdir(parents=True)
        saved.write_bytes(b'preserve save')
        result = self.run_script('play')
        self.assertEqual(result.returncode, 0, result.stderr)
        converted, launch = self.calls()
        cwd, prefix, steam = launch[:3]
        self.assertEqual(prefix, cwd + '/.proton')
        self.assertTrue(steam.endswith('/Steam client'))
        self.assertEqual(converted[3:], ['getcompatpath', cwd])
        win = 'Z:' + cwd
        self.assertEqual(launch[3:], ['waitforexitandrun', win + '\\tools\\python\\python.exe',
                         '-B', win + '\\tools\\import_game.py', 'launch'])
        self.assertEqual(marker.read_bytes(), b'preserve prefix')
        self.assertEqual(saved.read_bytes(), b'preserve save')

    @unittest.skipIf(os.name == 'nt', 'Native Linux update entry point')
    def test_update_uses_native_python_without_proton_or_steam(self):
        updater = self.root / 'tools/release_update.py'
        updater.write_text('import sys\nprint(repr(sys.argv[1:]))\n')
        env = dict(self.env, BURNER_PROTON='/does/not/exist', BURNER_STEAM='/missing')
        for action, option in (('update', '--download'), ('check-updates', '--check')):
            result = self.run_script(action, env=env)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn(option, result.stdout)
            self.assertFalse(self.calls())

    @unittest.skipIf(os.name == 'nt', 'Native Linux update recovery')
    def test_update_recovery_uses_independent_saved_helper(self):
        recovery = self.root / '.launcher-update/runner'
        recovery.mkdir(parents=True)
        (recovery.parent / 'transaction.json').write_text('{"phase":"applying"}')
        (recovery / 'release_update.py').write_text('import sys\nprint("RECOVERY", repr(sys.argv[1:]))\n')
        result = self.run_script('recover-update')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('RECOVERY', result.stdout)
        self.assertIn('--recover', result.stdout)
        self.assertFalse(self.calls())

    def test_import_textures_and_voices_quote_relative_input(self):
        name = "source pack ' $not_a_command ;.iso"
        (self.folder / name).touch()
        for action, option in [('import', '--iso'), ('use-iso', '--iso'),
                               ('textures', '--pack'), ('voices', '--iso')]:
            with self.subTest(action=action):
                if self.log.exists():
                    self.log.unlink()
                result = self.run_script(action, name)
                self.assertEqual(result.returncode, 0, result.stderr)
                calls = self.calls()
                self.assertEqual(len(calls), 3)
                source = calls[1][4]
                self.assertTrue(source.endswith('/' + name))
                argv = calls[2][3:]
                self.assertEqual(argv[4], action)
                self.assertEqual(argv[5:8], [option, 'Z:' + source, '--cancel'])
                self.assertIn('\\.proton\\.cancel-', argv[8])
                self.assertNotIn('\r', '\n'.join(argv))

    def test_language_dispatch(self):
        for value, expected in [(None, []), ('english', ['--language', '0']),
                                ('SPANISH', ['--language', '4']), ('2', ['--language', '2'])]:
            with self.subTest(value=value):
                if self.log.exists():
                    self.log.unlink()
                result = self.run_script('language', *([] if value is None else [value]))
                self.assertEqual(result.returncode, 0, result.stderr)
                argv = self.calls()[-1][3:]
                self.assertEqual(argv[4], 'language')
                self.assertEqual(argv[5:], expected)

    def test_reuse_existing_install_quotes_folder_without_copy(self):
        previous = self.folder / "existing game ' $no ; folder"
        previous.mkdir()
        marker = previous / 'keep-save'
        marker.write_bytes(b'original state')
        result = self.run_script('use-installation', previous.as_posix())
        self.assertEqual(result.returncode, 0, result.stderr)
        converted, source, launch = self.calls()
        self.assertEqual(launch[7:], ['use-installation', '--installation', 'Z:' + source[4]])
        self.assertEqual(marker.read_bytes(), b'original state')
        self.assertFalse((self.root / 'GameData').exists())

    def test_verify_and_automatic_setup_route_to_shared_worker(self):
        for action in ('verify', 'auto-setup'):
            with self.subTest(action=action):
                if self.log.exists():
                    self.log.unlink()
                result = self.run_script(action)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(self.calls()[-1][7:], [action])

    def test_validation_does_not_initialize_prefix(self):
        for args in [('unknown',), ('import',), ('import', 'missing.iso'),
                     ('use-iso',), ('use-iso', 'missing.iso'), ('use-installation',),
                     ('use-installation', 'missing'), ('verify', 'extra'), ('auto-setup', 'extra'),
                     ('textures', 'one', 'two'), ('voices',), ('voices', 'missing.iso'),
                     ('voices', 'one', 'two'), ('language', 'unknown'), ('play', 'extra')]:
            with self.subTest(args=args):
                result = self.run_script(*args)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((self.root / '.proton').exists())
                self.assertEqual(self.calls(), [])

    def test_missing_package_proton_and_steam(self):
        for variable in ('BURNER_STEAM', 'BURNER_PROTON'):
            with self.subTest(variable=variable):
                result = self.run_script('play', env=dict(self.env, **{variable: 'missing'}))
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((self.root / '.proton').exists())
        (self.root / 'runtime/UrbanChaosXbeTrial.exe').unlink()
        self.assertNotEqual(self.run_script('play').returncode, 0)
        self.assertFalse((self.root / '.proton').exists())

    def test_proton_failures_propagate_without_launch(self):
        result = self.run_script('play', env=dict(self.env, BURNER_FAIL_CONVERT='17'))
        self.assertEqual(result.returncode, 17)
        self.assertEqual(len(self.calls()), 1)
        self.log.unlink()
        result = self.run_script('play', env=dict(self.env, BURNER_EMPTY_CONVERT='1'))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(len(self.calls()), 1)

    def test_status_and_game_exit_codes_propagate(self):
        for action, code in [('status', 2), ('play', 7)]:
            with self.subTest(action=action):
                result = self.run_script(action, env=dict(self.env, BURNER_RUN_EXIT=str(code)))
                self.assertEqual(result.returncode, code, result.stderr)
                if action == 'play':
                    self.assertIn('exit 7', result.stderr)
                    self.assertIn('recommended Steam shortcut', result.stderr)
                else:
                    self.assertEqual(result.stderr, '')

    @unittest.skipIf(os.name == 'nt', 'Native Linux symlink behavior is checked separately in WSL')
    def test_symlink_entry_and_parent_keep_actual_package_and_prefix(self):
        shortcut = self.folder / 'desktop shortcut.sh'
        shortcut.symlink_to(self.script.relative_to(self.folder))
        parent = self.folder / 'library link'
        parent.symlink_to(self.root, target_is_directory=True)
        for entry in (shortcut, parent / 'burner.sh'):
            with self.subTest(entry=entry):
                if self.log.exists():
                    self.log.unlink()
                result = subprocess.run([BASH, str(entry), 'status'], cwd=self.folder,
                                        env=self.env, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                launch = self.calls()[-1]
                self.assertEqual(launch[0], str(self.root))
                self.assertEqual(launch[1], str(self.root / '.proton'))
                self.assertFalse((self.folder / '.proton').exists())

    def test_default_installed_proton_discovery(self):
        expected = self.steam / 'steamapps/common/Proton - Experimental/proton'
        expected.parent.mkdir(parents=True)
        shutil.copyfile(self.proton, expected)
        expected.chmod(0o755)
        environment = dict(self.env)
        environment.pop('BURNER_PROTON')
        result = self.run_script('status', env=environment)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.calls()[-1][7], 'status')

    def test_only_installed_proton_does_not_require_experimental(self):
        expected = self.steam / 'steamapps/common/Proton 10.0/proton'
        expected.parent.mkdir(parents=True)
        shutil.copyfile(self.proton, expected)
        expected.chmod(0o755)
        environment = dict(self.env)
        environment.pop('BURNER_PROTON')
        result = self.run_script('status', env=environment)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.calls()[-1][7], 'status')

    def test_ambiguous_proton_requires_a_choice_before_prefix_creation(self):
        for name in ('Proton 9.0', 'Proton 10.0'):
            path = self.steam / 'steamapps/common' / name / 'proton'
            path.parent.mkdir(parents=True)
            shutil.copyfile(self.proton, path)
            path.chmod(0o755)
        environment = dict(self.env)
        environment.pop('BURNER_PROTON')
        result = self.run_script('play', env=environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('BURNER_PROTON', result.stderr)
        self.assertFalse((self.root / '.proton').exists())
        self.assertEqual(self.calls(), [])

    def test_preserves_explicit_proton_and_driver_environment(self):
        record = self.folder / 'environment.bin'
        result = self.run_script('play', env=dict(self.env, BURNER_ENV_LOG=record.as_posix(),
            PROTON_LOG='1', PROTON_LOG_DIR='/chosen log folder', VKD3D_DEBUG='warn'))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(record.read_bytes().decode().split('\0'), ['1', '/chosen log folder', 'warn', ''])

    def test_diagnostic_command_and_native_output(self):
        result = self.run_script('diagnostics', 'on')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('--logging', self.calls()[-1])
        self.assertIn('on', self.calls()[-1])
        self.assertFalse((self.root / 'GameData/logs').exists())
        self.assertNotEqual(self.run_script('diagnostics','bad').returncode, 0)
        data=self.root/'GameData';data.mkdir()
        (data/'.diagnostic-logging').write_bytes(b'1\n')
        self.proton.write_text(self.proton.read_text().replace('waitforexitandrun) exit', 'waitforexitandrun) echo "Proton startup error" >&2; exit'),newline='\n')
        result=self.run_script('play',env=dict(self.env,BURNER_RUN_EXIT='7'))
        self.assertEqual(result.returncode,7,result.stderr)
        logs=list((data/'logs').glob('proton-*.log'))
        self.assertEqual(len(logs),1)
        self.assertIn('Proton startup error',logs[0].read_text())
        self.assertIn('Proton startup error',result.stderr)
        self.proton.write_text(self.proton.read_text().replace('echo "Proton startup error" >&2;', 'head -c 9437184 /dev/zero; echo "after-cap" >&2;'),newline='\n')
        result=self.run_script('play',env=dict(self.env,BURNER_RUN_EXIT='7'))
        self.assertEqual(result.returncode,7)
        self.assertIn('after-cap',result.stderr)
        self.assertEqual(max(p.stat().st_size for p in (data/'logs').glob('proton-*.log')),8388608)

    def test_existing_prefix_file_is_never_replaced(self):
        prefix = self.root / '.proton'
        prefix.write_bytes(b'keep this file')
        result = self.run_script('play')
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(prefix.read_bytes(), b'keep this file')
        self.assertEqual(self.calls(), [])


if __name__ == '__main__':
    unittest.main()
