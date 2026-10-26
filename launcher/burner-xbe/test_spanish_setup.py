"""Bundled language installation, upgrade rollback, and portable save preservation."""
from pathlib import Path
import hashlib
import json
import sys
import tempfile
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE.parents[1] / 'third_party/xboxrecomp/tools/xiso')]
import import_game as worker


def digest(data):
    return hashlib.sha256(data).hexdigest()


def snapshot(path):
    return {p.relative_to(path).as_posix(): p.read_bytes() for p in path.rglob('*') if p.is_file()}


with tempfile.TemporaryDirectory(prefix='spanish-setup-', dir=HERE.parents[1] / 'build') as folder:
    base = Path(folder)
    data = base / 'GameData'
    original = data / 'installed/game_files'
    original.mkdir(parents=True)
    (original / 'source.fixture').write_bytes(b'unchanged imported data')
    runtime = data / 'xbe-runtime'
    (runtime / 'hdd').mkdir(parents=True)
    (runtime / 'hdd/save').write_bytes(b'saved progress')
    (runtime / 'settings.toml').write_text('language = 1\nfps_cap = 120\n')
    (runtime / 'shader-cache').mkdir()
    (runtime / 'shader-cache/learned').write_bytes(b'learned cache')
    keep = snapshot(data)
    payload = b'translated fixture' * 20
    rows = [{'file': f'{i}-01.ent', 'size': len(payload), 'sha256': digest(payload)} for i in range(27)]
    bundle = (base / 'text.json', {'files': rows})
    manifest = ''.join(f"{r['file']} {r['size']}\n" for r in rows)
    calls = []

    def build(source, path, output, cancel=None, *, source_root=None,
              bundle_root=None, output_root=None):
        assert source == original and path == bundle[0]
        assert source_root == output_root == data and bundle_root == worker.ROOT
        calls.append(output)
        output.mkdir()
        for row in rows:
            if cancel:
                cancel()
            (output / row['file']).write_bytes(payload)
        (output / 'manifest.txt').write_text(manifest)

    def unchanged():
        now = snapshot(data)
        assert all(now[k] == v for k, v in keep.items())

    with patch.object(worker, 'spanish_bundle', return_value=bundle), \
            patch.object(worker.spanish_text, 'build', side_effect=build), \
            patch.object(worker, 'ready', return_value=True):
        # Querying Setup does not alter files, but included Spanish is selectable.
        assert worker.language_settings(data) == {'language': 1, 'spanish_available': True}
        assert snapshot(data) == keep
        worker.ensure_spanish(data)
        dest = runtime / 'translations/es-zarf'
        assert len(calls) == 1 and (dest / 'manifest.txt').read_text() == manifest
        unchanged()
        before = snapshot(data)
        worker.ensure_spanish(data)
        assert len(calls) == 1 and snapshot(data) == before
        # Same-sized damage is repaired, and the prior pack is preserved.
        (dest / '0-01.ent').write_bytes(b'x' * len(payload))
        worker.ensure_spanish(data)
        assert len(calls) == 2 and (dest / '0-01.ent').read_bytes() == payload
        backups = list(dest.parent.glob('.previous-es-zarf-*'))
        assert len(backups) == 1 and (backups[0] / '0-01.ent').read_bytes() == b'x' * len(payload)
        unchanged()
        # Failure while publishing restores the currently installed pack.
        (dest / '0-01.ent').write_bytes(b'y' * len(payload))
        before = snapshot(data)
        rename = Path.rename

        def fail_publish(path, target):
            if path.name.startswith('.spanish-'):
                raise OSError('fixture rename failure')
            return rename(path, target)

        with patch.object(Path, 'rename', fail_publish):
            try:
                worker.ensure_spanish(data)
            except OSError:
                pass
            else:
                raise AssertionError('Expected publish failure')
        assert snapshot(data) == before and not list(dest.parent.glob('.spanish-*'))
        cancel = base / 'cancel'
        cancel.touch()
        try:
            worker.ensure_spanish(data, cancel=cancel)
        except RuntimeError:
            pass
        else:
            raise AssertionError('Expected cancellation')
        assert snapshot(data) == before
        # Choosing Spanish prepares files before changing only the language key.
        assert worker.language_settings(data, 4) == {'language': 4, 'spanish_available': True}
        assert (runtime / 'settings.toml').read_text() == 'language = 4\nfps_cap = 120\n'
        assert (runtime / 'hdd/save').read_bytes() == b'saved progress'
        for language in range(5):
            assert worker.language_settings(data, language)['language'] == language

print('PASS: bundled availability, first install, warm reuse, damaged upgrade repair, rollback, cancellation, all languages and saves')
