"""Local ISO installation transaction checks; no game or media conversion launched."""
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
import hashlib
import json
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE.parents[1] / 'third_party/xboxrecomp/tools/xiso')]
import import_game as worker
import spanish_voice_iso as installer
import spanish_voice_setup as setup


def digest(value):
    return hashlib.sha256(value).hexdigest()


class IsoInstallTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='spanish-iso-test-', dir=HERE.parents[1] / 'build')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.data = self.root / 'GameData'
        self.original = self.data / 'installed/game_files'
        self.parent = self.data / 'xbe-runtime/translations'
        self.translated = self.parent / 'es-zarf'
        self.destination = self.parent / 'es-voices'
        for path in (self.original, self.translated, self.destination, self.root / 'tools/ffmpeg'):
            path.mkdir(parents=True)
        self.cancel = self.root / 'cancel'
        self.iso = self.root / 'European PS2.iso'
        self.iso.write_bytes(bytes(65536))
        self.old = self.destination / 'previous.fixture'
        self.old.write_bytes(b'previous working audio')
        self.save = self.data / 'xbe-runtime/hdd/save'
        self.save.parent.mkdir()
        self.save.write_bytes(b'user progress')
        self.settings = self.data / 'xbe-runtime/settings.toml'
        self.settings.write_bytes(b'language=1\nfps_cap=120\n')
        self.game_files, rows = {}, []
        for name in sorted(installer.OUTPUTS):
            source = 'Movies/' + name[7:] if name in installer.MOVIES else name
            value = ('original ' + name).encode()
            path = self.original / source
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(value)
            row = {'size': len(value), 'sha256': digest(value)}
            self.game_files[source] = row
            if name in installer.FILES:
                translated = ('Spanish ' + name).encode()
                (self.translated / name).write_bytes(translated)
                rows.append({'file': name, 'size': len(translated), 'sha256': digest(translated),
                             'source_size': len(value), 'source_sha256': digest(value)})
        self.text = self.root / 'text.json'
        self.text.write_text('fixture text bundle')
        self.bundle = (self.text, {'files': rows})
        self.tools = {'version': 1, 'files': {}}
        for name in installer.TOOL_FILES:
            value = ('verified bundled tool ' + name).encode()
            (self.root / 'tools' / name).write_bytes(value)
            self.tools['files'][name] = digest(value)
        self.tool_manifest = self.root / 'tools/voice-tools.json'
        self.tool_manifest.write_text(json.dumps(self.tools))
        self.disc_closed = False
        self.reads, self.movies = [], []
        owner = self

        class Disc:
            def __init__(self, path):
                owner.assertEqual(path, owner.iso)

            def __enter__(self):
                return self

            def __exit__(self, *unused):
                owner.disc_closed = True

            def size(self, name):
                owner.assertIn(name, installer.spanish_movies.MOVIES.values())
                return 72

            def read(self, name, offset=0, size=None):
                owner.assertLessEqual(size, 64 * 1024 * 1024)
                owner.reads.append((name, offset, size))
                return b'B' * size

        def speech(original, translated, disc, output, cancel=None, progress=None):
            self.assertEqual((original, translated), (self.original, self.translated))
            self.assertEqual(list(output.iterdir()), [])
            for name in sorted(set(installer.FILES) | installer.SAFS):
                cancel()
                (output / name).write_bytes(('converted speech ' + name).encode())
            progress('speech fixture', 100)

        def movie(original, source, ffmpeg, output, cancel=None):
            self.assertEqual(ffmpeg, self.root / 'tools/ffmpeg/ffmpeg.exe')
            self.assertEqual(source.name, installer.spanish_movies.MOVIES[original.name])
            self.assertEqual(source.read_bytes(), b'B' * 72)
            self.movies.append(original.name)
            cancel()
            output.write_bytes(('converted movie ' + original.name).encode())

        self.speech, self.movie = speech, movie
        for target, name, value in (
                (worker, 'ROOT', self.root),
                (worker, 'ready', lambda data: data == self.data),
                (worker, 'spanish_bundle', lambda: self.bundle),
                (worker, 'ensure_spanish', lambda data, cancel=None: None),
                (worker, 'manifest', lambda: {'files': self.game_files}),
                (installer, 'Ps2Disc', Disc),
                (installer, 'progress', lambda *args: None),
                (installer.shutil, 'disk_usage', lambda path: SimpleNamespace(free=20 * 1024**3)),
                (installer.spanish_voices, 'build', speech),
                (installer.spanish_movies, 'convert', movie)):
            started = patch.object(target, name, value)
            started.start()
            self.addCleanup(started.stop)

    def install(self):
        installer.install(self.iso, self.data, self.cancel)

    def unchanged(self):
        self.assertEqual(self.old.read_bytes(), b'previous working audio')
        self.assertEqual(self.save.read_bytes(), b'user progress')
        self.assertEqual(self.settings.read_bytes(), b'language=1\nfps_cap=120\n')
        self.assertFalse(list(self.parent.glob('.spanish-voice-iso-*')))

    def test_complete_transaction_uses_every_movie_and_preserves_state(self):
        self.install()
        self.assertTrue(self.disc_closed)
        self.assertEqual(len(self.movies), 31)
        self.assertEqual(set(self.movies), set(installer.spanish_movies.MOVIES))
        info = json.loads((self.destination / 'install.json').read_text())
        self.assertEqual(info['version'], 1)
        self.assertEqual(info['text_sha256'], digest(self.text.read_bytes()))
        self.assertEqual(len(info['files']), 64)
        self.assertEqual(set(info['files']), installer.OUTPUTS)
        setup.validate(self.data, info['text_sha256'])
        for name, row in info['files'].items():
            value = (self.destination / name).read_bytes()
            self.assertEqual(row, {'size': len(value), 'sha256': digest(value)})
        self.assertEqual(len(list(self.parent.glob('.previous-es-voices-*'))), 1)
        self.assertEqual(self.save.read_bytes(), b'user progress')
        self.assertEqual(self.settings.read_bytes(), b'language=1\nfps_cap=120\n')
        self.assertFalse(list(self.parent.glob('.spanish-voice-iso-*')))

    def test_tool_hash_and_required_dependency_validation(self):
        self.assertEqual(installer.validate_tools(self.root), self.root / 'tools/ffmpeg/ffmpeg.exe')
        exe = self.root / 'tools/ffmpeg/ffmpeg.exe'
        previous = exe.read_bytes()
        exe.write_bytes(b'x' * len(previous))
        with self.assertRaises(ValueError):
            self.install()
        exe.write_bytes(previous)
        self.tools['files'].pop('ffmpeg/avcodec-62.dll')
        self.tool_manifest.write_text(json.dumps(self.tools))
        with self.assertRaises(ValueError):
            self.install()
        self.assertEqual(self.movies, [])
        self.unchanged()

    def test_every_source_is_hashed_before_conversion(self):
        for path in (self.original / '1.saf', self.original / 'Movies/intro_1.xmv',
                     self.original / '1-01.ent', self.translated / '1-01.ent'):
            previous = path.read_bytes()
            path.write_bytes(b'x' * len(previous))
            with self.subTest(path=path), patch.object(installer.spanish_voices, 'build') as build:
                with self.assertRaises(ValueError):
                    self.install()
                build.assert_not_called()
            path.write_bytes(previous)
        self.unchanged()

    def test_incomplete_gameplay_and_movie_outputs_never_publish(self):
        def missing_saf(*args, **kwargs):
            self.speech(*args, **kwargs)
            (args[3] / '6.saf').unlink()
        with patch.object(installer.spanish_voices, 'build', missing_saf), patch.object(setup, 'publish') as publish:
            with self.assertRaises(ValueError):
                self.install()
            publish.assert_not_called()
        # The old loose manifest writer accepts 33 gameplay files; ISO installation must require all64.
        with patch.object(installer.spanish_movies, 'convert', lambda *a, **k: None), patch.object(setup, 'publish') as publish:
            with self.assertRaises(ValueError):
                self.install()
            publish.assert_not_called()
        self.unchanged()

    def test_cancel_during_conversion_cleans_scratch_and_keeps_previous(self):
        def cancelled(*args, **kwargs):
            self.movie(*args, **kwargs)
            self.cancel.touch()
            kwargs['cancel']()
        with patch.object(installer.spanish_movies, 'convert', cancelled):
            with self.assertRaises(RuntimeError):
                self.install()
        self.assertTrue(self.disc_closed)
        self.unchanged()

    def test_conversion_error_cleans_scratch(self):
        with patch.object(installer.spanish_movies, 'convert', side_effect=ValueError('invalid Bink track')):
            with self.assertRaises(ValueError):
                self.install()
        self.unchanged()

    def test_failed_publish_and_late_cancel_roll_back(self):
        rename = Path.rename
        def fail_publish(path, target):
            if path.name == 'voices':
                raise OSError('publication fixture')
            return rename(path, target)
        with patch.object(Path, 'rename', fail_publish):
            with self.assertRaises(OSError):
                self.install()
        self.unchanged()
        calls = []
        def late_cancel(path):
            calls.append(path)
            if len(calls) == 2:
                raise RuntimeError('cancel after rename')
        with patch.object(setup, 'check_cancel', late_cancel):
            with self.assertRaises(RuntimeError):
                self.install()
        self.unchanged()

    def test_space_budget_and_invalid_disc(self):
        with patch.object(installer.shutil, 'disk_usage', return_value=SimpleNamespace(free=1024)):
            with self.assertRaises(ValueError):
                self.install()
        with patch.object(installer, 'Ps2Disc', side_effect=ValueError('unsupported SLES revision')):
            with self.assertRaises(ValueError):
                self.install()
        with patch.object(installer, 'OUTPUT_LIMIT', 1):
            with self.assertRaises(ValueError):
                self.install()
        self.unchanged()

    def test_source_mutation_and_unexpected_output_reject_publication(self):
        def mutate_source(*args, **kwargs):
            self.movie(*args, **kwargs)
            if len(self.movies) == 31:
                (self.original / '1.saf').write_bytes(b'changed while converting')
        with patch.object(installer.spanish_movies, 'convert', mutate_source):
            with self.assertRaises(ValueError):
                self.install()
        self.unchanged()

    def test_movie_source_reads_bounded_chunks_and_rejects_truncation(self):
        scratch = self.root / 'scratch'
        scratch.mkdir()
        reads = []
        class Disc:
            def size(self, name):
                return installer.READ_LIMIT + 13
            def read(self, name, offset, size):
                reads.append((offset, size))
                return b'B' * size
        movie = installer._movie_source(Disc(), 'pal_intro_1.bik', scratch, lambda: None)
        self.assertEqual(reads, [(0, 64 * 1024 * 1024), (64 * 1024 * 1024, 13)])
        self.assertEqual(movie.stat().st_size, 64 * 1024 * 1024 + 13)
        movie.unlink()
        with patch.object(Disc, 'read', return_value=b''):
            with self.assertRaises(ValueError):
                installer._movie_source(Disc(), movie.name, scratch, lambda: None)


if __name__ == '__main__':
    unittest.main()
