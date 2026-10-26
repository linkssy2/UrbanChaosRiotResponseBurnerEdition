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
        for key in ('BURNER_FAIL_CONVERT', 'BURNER_EMPTY_CONVERT', 'BURNER_RUN_EXIT'):
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
                         '-B', win + '\\tools\\import_game.py', 'launch', '--data', win + '\\GameData'])
        self.assertEqual(marker.read_bytes(), b'preserve prefix')
        self.assertEqual(saved.read_bytes(), b'preserve save')

    def test_import_textures_and_voices_quote_relative_input(self):
        name = "source pack ' $not_a_command ;.iso"
        (self.folder / name).touch()
        for action, option in [('import', '--iso'), ('textures', '--pack'), ('voices', '--iso')]:
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
                self.assertEqual(argv[7:10], [option, 'Z:' + source, '--cancel'])
                self.assertIn('\\.proton\\.cancel-', argv[10])
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
                self.assertEqual(argv[7:], expected)

    def test_validation_does_not_initialize_prefix(self):
        for args in [('unknown',), ('import',), ('import', 'missing.iso'),
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

    def test_existing_prefix_file_is_never_replaced(self):
        prefix = self.root / '.proton'
        prefix.write_bytes(b'keep this file')
        result = self.run_script('play')
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(prefix.read_bytes(), b'keep this file')
        self.assertEqual(self.calls(), [])


if __name__ == '__main__':
    unittest.main()
