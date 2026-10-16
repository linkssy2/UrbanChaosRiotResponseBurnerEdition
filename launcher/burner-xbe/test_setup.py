"""Production importer/launch-policy checks using tiny synthetic ISOs and fake processes.

No user install/save is modified and no gameplay is launched. Optional --package
also checks the compiled direct launcher against an isolated harmless worker.
"""
from pathlib import Path
import argparse
import hashlib
import io
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.dont_write_bytecode = True
sys.path[:0] = [str(HERE), str(REPO / 'third_party/xboxrecomp/tools/xiso')]
import import_game as worker
import package as packager


def digest(data):
    return hashlib.sha256(data).hexdigest()


def make_iso(path, files, base=0):
    """Real XDVDFS directory/partition encoding, not a mocked Disc implementation."""
    dirs = {'': []}
    for name, content in files.items():
        parts = name.split('/')
        for i in range(len(parts) - 1):
            dirs.setdefault('/'.join(parts[:i+1]), [])
    dir_sectors = {name: 40 + i for i, name in enumerate(dirs)}
    file_sectors = {name: 64 + i for i, name in enumerate(files)}
    for name in dirs:
        if name:
            parent, _, leaf = name.rpartition('/')
            dirs[parent].append((leaf, dir_sectors[name], 2048, 0x10))
    for name, content in files.items():
        assert len(content) <= 2048
        parent, _, leaf = name.rpartition('/')
        dirs[parent].append((leaf, file_sectors[name], len(content), 0))
    with path.open('wb') as out:
        out.seek(base + 32 * 2048)
        header = bytearray(2048)
        header[:18] = b'MICROSOFT*XBOX*MEDIA'
        struct.pack_into('<II', header, 0x14, dir_sectors[''], 2048)
        out.write(header)
        for name, records in dirs.items():
            block = bytearray(2048)
            at = 0
            for leaf, sector, size, attr in records:
                encoded = leaf.encode('ascii')
                struct.pack_into('<HHIIBB', block, at, 0, 0, sector, size, attr, len(encoded))
                block[at+14:at+14+len(encoded)] = encoded
                at += (14 + len(encoded) + 3) & ~3
            out.seek(base + dir_sectors[name] * 2048)
            out.write(block)
        for name, content in files.items():
            out.seek(base + file_sectors[name] * 2048)
            out.write(content)


def expect_error(fn, exception=Exception):
    try:
        fn()
    except exception:
        return
    raise AssertionError('Expected rejection')


def check_worker(tmp):
    package = tmp / 'package'
    (package / 'tools').mkdir(parents=True)
    (package / 'runtime').mkdir()
    files = {'Default.xbe': b'fixture-xbe-code', 'scene.ent': b'fixture-assets',
             'Movies/intro.xmv': b'fixture-original-movie'}
    required = {name: {'size': len(content), 'sha256': digest(content)} for name, content in files.items()}
    (package / 'tools/disc-manifest.json').write_text(json.dumps({'version': 1, 'files': required}))
    assert worker.RUNTIME_FILES == packager.RUNTIME_FILES
    for name in worker.RUNTIME_FILES:
        (package / 'runtime' / name).write_bytes(('fixture-only-' + name).encode('ascii'))
    runtime = {name: worker.sha(package / 'runtime' / name) for name in worker.RUNTIME_FILES}
    (package / 'tools/runtime-manifest.json').write_text(json.dumps({'version': packager.VERSION, 'files': runtime}))
    worker.ROOT = package
    messages = []
    worker.progress = lambda text, percent=0: messages.append((text, percent))
    data = tmp / 'userdata'
    (data / 'saves').mkdir(parents=True)
    (data / 'saves/legacy-save').write_bytes(b'never-touch')
    (data / 'runtime').mkdir()
    (data / 'runtime/xboxrecomp.ini').write_bytes(b'legacy-settings')
    cancel = tmp / 'cancel'
    iso = tmp / 'good.xiso.iso'
    make_iso(iso, files)
    assert not worker.ready(data)
    worker.import_iso(iso, data, cancel)
    assert worker.ready(data) and messages[-1] == ('Ready to play', 100)
    install = data / 'installed'
    assert {p.name for p in install.iterdir()} == {'game_files', 'installed.json'}
    assert not (install / 'hybrid-assets').exists() and not (install / 'native-movies').exists()
    for name, content in files.items():
        assert (install / 'game_files' / name).read_bytes() == content
    before = worker.sha(install / 'installed.json')
    # The original0.02 marker remains accepted without its obsolete converted outputs.
    (install / 'installed.json').write_text(json.dumps({'version': 1,
        'disc_manifest': worker.sha(package / 'tools/disc-manifest.json'), 'files': len(required)}))
    assert worker.ready(data)
    before = worker.sha(install / 'installed.json')
    bad = tmp / 'bad.iso'
    bad.write_bytes(b'not-a-disc')
    expect_error(lambda: worker.import_iso(bad, data, cancel), worker.XisoError)
    cancelled = tmp / 'cancelled'
    cancelled.touch()
    expect_error(lambda: worker.import_iso(iso, data, cancelled), RuntimeError)
    corrupt = dict(files)
    corrupt['scene.ent'] = b'x' * len(files['scene.ent'])
    assert len(corrupt['scene.ent']) == len(files['scene.ent'])
    make_iso(bad, corrupt)
    expect_error(lambda: worker.import_iso(bad, data, cancel), ValueError)
    corrupt = dict(files)
    corrupt['Default.xbe'] = b'unknown-xbe-code'
    assert len(corrupt['Default.xbe']) == len(files['Default.xbe'])
    make_iso(bad, corrupt)
    expect_error(lambda: worker.import_iso(bad, data, cancel), ValueError)
    assert worker.sha(install / 'installed.json') == before and not list(data.glob('.import-*'))
    saved_progress = worker.progress
    def cancel_after_first_file(text, percent=0):
        saved_progress(text, percent)
        if text == 'Importing game files':
            cancel.touch()
    worker.progress = cancel_after_first_file
    try:
        expect_error(lambda: worker.import_iso(iso, data, cancel), RuntimeError)
        assert worker.sha(install / 'installed.json') == before and not list(data.glob('.import-*'))
    finally:
        worker.progress = saved_progress
        cancel.unlink()
    rename = Path.rename
    def failed_swap(path, target):
        if path.name.startswith('.import-') and Path(target).name == 'installed':
            raise OSError('fixture: atomic install swap failed')
        return rename(path, target)
    Path.rename = failed_swap
    try:
        expect_error(lambda: worker.import_iso(iso, data, cancel), OSError)
        assert worker.sha(install / 'installed.json') == before and not list(data.glob('.import-*'))
    finally:
        Path.rename = rename
    # Same import paths on a full-disc partition offset, preserving the prior complete import.
    full_iso = tmp / 'full.iso'
    make_iso(full_iso, files, 0x30600)
    worker.import_iso(full_iso, data, cancel)
    assert worker.ready(data) and len(list(data.glob('.previous-*'))) == 1
    for invalid in ('../escape', 'a/b', 'a\\b', 'CON', 'x:ads', 'a.', 'NUL.txt', ''):
        expect_error(lambda n=invalid: worker.safe_name(n), ValueError)
    # Reject malformed ISO filenames through the real directory walk.
    make_iso(bad, {'../escape': b'x'})
    expect_error(lambda: worker.import_iso(bad, data, cancel), ValueError)
    (install / 'game_files/scene.ent').write_bytes(b'x')
    assert not worker.ready(data)
    (install / 'game_files/scene.ent').write_bytes(files['scene.ent'])
    assert worker.ready(data)
    # Exercise the real copy/setup/error loop while replacing only process creation.
    seen = []
    class Process:
        def __init__(self, args, **kwargs):
            seen.append((args, kwargs))
            self.stderr = io.BytesIO()
        def wait(self):
            return 0
    old_process = worker.subprocess.Popen
    old_env = dict(os.environ)
    worker.subprocess.Popen = Process
    try:
        os.environ.update(XBE_PERF='1', UCRR_GAME_ASSETS='wrong', UCRR_NATIVE_DIAGNOSTICS='1', XBOX_LOG_GPU='1')
        worker.launch(data)
        state = data / 'xbe-runtime'
        (state / 'settings.toml').write_bytes(b'user-settings')
        (state / 'xboxrecomp.ini').write_bytes(b'user-controls')
        (state / 'hdd').mkdir()
        (state / 'hdd/profile').write_bytes(b'user-save')
        worker.launch(data)
        assert (state / 'settings.toml').read_bytes() == b'user-settings'
        assert (state / 'xboxrecomp.ini').read_bytes() == b'user-controls'
        assert (state / 'hdd/profile').read_bytes() == b'user-save'
        for name in worker.RUNTIME_FILES:
            assert worker.sha(state / name) == runtime[name]
        assert (data / 'saves/legacy-save').read_bytes() == b'never-touch'
        assert (data / 'runtime/xboxrecomp.ini').read_bytes() == b'legacy-settings'
        args, kwargs = seen[-1]
        assert Path(args[0]) == state.resolve() / worker.RUNTIME_NAME
        assert kwargs['cwd'] == state.resolve()
        assert kwargs['env']['UCRR_GAME_ASSETS'] == str(data.resolve() / 'installed/game_files')
        assert not any(k.startswith(('XBE_', 'XBOX_LOG_')) or
                       (k.startswith('UCRR_') and k != 'UCRR_GAME_ASSETS') for k in kwargs['env'])
        assert not (data / 'last-error.txt').exists()
        class Failed(Process):
            def __init__(self, args, **kwargs):
                super().__init__(args, **kwargs)
                self.stderr = io.BytesIO(b'error\n' * 50000)
            def wait(self):
                return 0xc0000005
        worker.subprocess.Popen = Failed
        expect_error(lambda: worker.launch(data), RuntimeError)
        report = (data / 'last-error.txt').read_bytes()
        assert len(report) < 133000 and b'0xc0000005' in report and b'Runtime SHA256:' in report
        (package / 'runtime/zlib1.dll').write_bytes(b'tampered')
        expect_error(lambda: worker.launch(data), ValueError)
        (package / 'runtime/zlib1.dll').write_bytes(b'fixture-only-zlib1.dll')
        for missing in ('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll'):
            broken = dict(runtime)
            del broken[missing]
            (package / 'tools/runtime-manifest.json').write_text(json.dumps({'version': packager.VERSION, 'files': broken}))
            expect_error(lambda: worker.runtime_files(), ValueError)
        broken = dict(runtime, **{'unapproved.dll': '0'*64})
        (package / 'tools/runtime-manifest.json').write_text(json.dumps({'version': packager.VERSION, 'files': broken}))
        expect_error(lambda: worker.runtime_files(), ValueError)
        (package / 'tools/runtime-manifest.json').write_text(json.dumps({'version': packager.VERSION, 'files': runtime}))
        (package / 'runtime/msvcp140.dll').unlink()
        expect_error(lambda: worker.runtime_files(), ValueError)
    finally:
        worker.subprocess.Popen = old_process
        os.environ.clear()
        os.environ.update(old_env)
    # Retail hash exception applies only to Default.xbe, never arbitrary game files.
    original = json.loads((HERE.parent / 'burner/disc-manifest.json').read_text())['files']['Default.xbe']['sha256']
    retail = 'c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1'
    assert worker.supported_hash('Default.xbe', original, original)
    assert worker.supported_hash('Default.xbe', retail, original)
    assert not worker.supported_hash('Default.xbe', '0'*64, original)
    assert not worker.supported_hash('scene.ent', retail, original)
    return {'real_xdvdfs_import': True, 'base0_and_video_partition': True,
            'existing_002_import_reused': True, 'all_payload_hashes_checked': True,
            'malformed_revision_cancel_and_corruption_rejected': True,
            'prior_install_and_both_save_trees_preserved': True,
            'runtime_copy_and_environment_verified': True, 'bounded_failure_report': True,
            'gameplay_launched': False, 'real_user_data_modified': False}


def check_asset_audit(tmp):
    package = tmp / 'audit-package'
    (package / 'tools').mkdir(parents=True)
    (package / 'runtime').mkdir()
    disc_payload = b'fixture-original-game-payload'
    (package / 'tools/disc-manifest.json').write_text(json.dumps({'files': {'scene.ent': {
        'size': len(disc_payload), 'sha256': digest(disc_payload)}}}))
    for name in worker.RUNTIME_FILES:
        (package / 'runtime' / name).write_bytes(('fixture-only-' + name).encode('ascii'))
    (package / 'tools/runtime-manifest.json').write_text(json.dumps({'version': packager.VERSION,
        'files': {name: packager.sha(package / 'runtime' / name)
                  for name in worker.RUNTIME_FILES}}))
    for name in packager.LAUNCHERS:
        (package / name).write_bytes(b'fixture-launcher-code')
    allowed = {p.relative_to(package).as_posix() for p in package.rglob('*') if p.is_file()} - packager.LAUNCHERS
    assert packager.audit(package, allowed)['original_payload_files_in_package'] == 0
    leaked = package / 'forgotten.bin'
    leaked.write_bytes(disc_payload)
    expect_error(lambda: packager.audit(package, allowed), ValueError)
    expect_error(lambda: packager.audit(package, allowed | {'forgotten.bin'}), ValueError)
    leaked.unlink()
    (package / 'forgotten.ent').write_bytes(b'not-even-known-hash')
    expect_error(lambda: packager.audit(package, allowed | {'forgotten.ent'}), ValueError)
    (package / 'forgotten.ent').unlink()
    (package / 'runtime/zlib1.dll').write_bytes(b'modified-library')
    expect_error(lambda: packager.audit(package, allowed), ValueError)


def check_compiled_direct(package, tmp):
    harness = tmp / 'direct-harness'
    (harness / 'tools').mkdir(parents=True)
    shutil.copy2(package / 'Play Burner Edition.exe', harness / 'Play Burner Edition.exe')
    shutil.copytree(package / 'tools/python', harness / 'tools/python')
    (harness / 'tools/import_game.py').write_text(
        "from pathlib import Path\nimport sys\n"
        "if sys.argv[1]=='launch':Path(__file__).with_name('launched.txt').write_text('ok')\n")
    result = subprocess.run([str(harness / 'Play Burner Edition.exe')], timeout=20,
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    assert result.returncode == 0 and (harness / 'tools/launched.txt').read_text() == 'ok'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--package', type=Path)
    parser.add_argument('--evidence', type=Path, default=REPO / 'build/burner-xbe-launcher-checks.json')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='burner-xbe-test-', dir=REPO / 'build') as directory:
        tmp = Path(directory)
        result = check_worker(tmp)
        check_asset_audit(tmp)
        result['strict_asset_allowlist_negative_cases'] = True
        if args.package:
            check_compiled_direct(args.package.resolve(), tmp)
            result['compiled_direct_exe_with_stub_worker'] = True
    args.evidence.parent.mkdir(parents=True, exist_ok=True)
    args.evidence.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: verified ISO import, rollback, rejection, isolated runtime launch and bounded error handling')


if __name__ == '__main__':
    main()
