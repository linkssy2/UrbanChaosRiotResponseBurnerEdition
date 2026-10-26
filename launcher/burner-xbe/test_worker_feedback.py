"""Worker commands keep Steam errors visible and preserve the Windows JSON protocol."""
from pathlib import Path
import io
import json
import sys
import tempfile
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE.parents[1] / 'third_party/xboxrecomp/tools/xiso')]
import import_game as worker
import setup_feedback as feedback


class WorkerFeedbackTests(unittest.TestCase):
    def command(self, root, *args):
        output = io.StringIO()
        with patch.object(worker, 'ROOT', root), patch.object(sys, 'argv', ['import_game.py', *args]), \
                patch('sys.stdout', output), patch.object(feedback, 'notify', return_value=True) as notify:
            result = worker.main()
        return result, output.getvalue(), notify.call_args_list

    def test_failed_import_is_reported_without_creating_game_data(self):
        for interactive in (False, True):
            with self.subTest(interactive=interactive), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                args = ['import', '--iso', 'bad.iso', '--cancel', '.cancel']
                if interactive:
                    args.append('--interactive')
                with patch.object(worker, 'import_iso', side_effect=ValueError('unsupported ISO')):
                    code, output, dialogs = self.command(root, *args)
                self.assertEqual(code, 1)
                row = json.loads(output)
                self.assertEqual(row['percent'], 0)
                self.assertIn('unsupported ISO', row['message'])
                self.assertIn('unsupported ISO', (root / feedback.ERROR_FILE).read_text())
                self.assertEqual(len(dialogs), 2 if interactive else 0)
                if interactive:
                    self.assertEqual(dialogs[-1].kwargs, {'error': True})
                self.assertFalse((root / 'GameData').exists())

    def test_argument_errors_are_visible_and_saved(self):
        for args in ([], ['import', '--unknown'], ['language', '--language', 'nonsense']):
            with self.subTest(args=args), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                code, output, dialogs = self.command(root, *args, '--interactive')
                self.assertEqual(code, 1)
                self.assertTrue((root / feedback.ERROR_FILE).exists())
                self.assertIn('failed:', json.loads(output)['message'])
                self.assertEqual(len(dialogs), 1)
                self.assertEqual(dialogs[0].kwargs, {'error': True})

    def test_import_success_is_acknowledged_and_play_does_not_add_dialogs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with patch.object(worker, 'import_iso') as imported:
                code, _, dialogs = self.command(root, 'import', '--iso', 'good.iso', '--cancel', '.cancel', '--interactive')
            self.assertEqual(code, 0)
            imported.assert_called_once()
            self.assertEqual(len(dialogs), 2)
            self.assertIn('successfully', dialogs[-1].args[0])
            with patch.object(worker, 'launch'):
                code, _, dialogs = self.command(root, 'launch', '--interactive')
            self.assertEqual((code, dialogs), (0, []))
            self.assertFalse((root / feedback.ERROR_FILE).exists())

    def test_language_query_keeps_exact_json_shape(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(worker, 'language_settings',
                return_value={'language': 4, 'spanish_available': True}):
            code, output, dialogs = self.command(Path(directory), 'language')
        self.assertEqual(code, 0)
        self.assertEqual(json.loads(output), {'language': 4, 'spanish_available': True})
        self.assertEqual(dialogs, [])

    def test_voice_iso_dispatch_and_visible_failure(self):
        import spanish_voice_iso
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with patch.object(spanish_voice_iso, 'install') as install:
                code, _, dialogs = self.command(root, 'voices', '--iso', 'European PS2.iso',
                    '--cancel', '.cancel', '--interactive')
            self.assertEqual(code, 0)
            self.assertEqual(install.call_args.args[0], Path('European PS2.iso'))
            self.assertEqual(len(dialogs), 2)
            self.assertIn('Spanish voices installed', dialogs[-1].args[0])
            with patch.object(spanish_voice_iso, 'install', side_effect=ValueError('Unsupported PS2 disc')):
                code, output, dialogs = self.command(root, 'voices', '--iso', 'European PS2.iso',
                    '--cancel', '.cancel', '--interactive')
            self.assertEqual(code, 1)
            self.assertIn('Unsupported PS2 disc', json.loads(output)['message'])
            self.assertTrue(dialogs[-1].kwargs['error'])
            with patch.object(spanish_voice_iso, 'install') as install:
                code, output, _ = self.command(root, 'voices', '--pack', 'Spanish.zip', '--cancel', '.cancel')
            self.assertEqual(code, 1)
            install.assert_not_called()
            self.assertIn('PS2 ISO', json.loads(output)['message'])


if __name__ == '__main__':
    unittest.main()
