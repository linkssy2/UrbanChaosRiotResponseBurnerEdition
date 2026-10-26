"""Production importer/launch-policy checks using tiny synthetic ISOs and fake processes.

No user install/save is modified and no gameplay is launched. Optional --package
also checks the compiled direct launcher against an isolated harmless worker.
"""
from pathlib import Path
import argparse
import copy
import hashlib
import io
import json
import os
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
from unittest.mock import patch
from types import SimpleNamespace
import zlib

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.dont_write_bytecode = True
sys.path[:0] = [str(HERE), str(REPO / 'third_party/xboxrecomp/tools/xiso')]
import import_game as worker
import package as packager


def digest(data):
    return hashlib.sha256(data).hexdigest()


def png_bytes(width=8, height=8):
    def chunk(name, data):
        return struct.pack('>I', len(data)) + name + data + struct.pack('>I', zlib.crc32(name + data))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress((b'\0' + b'\x13\x35\xc9\xff' * width) * height)) + chunk(b'IEND', b''))


def make_iso(path, files, base=0):
    """Real XDVDFS directory/partition encoding, not a mocked Disc implementation."""
    dirs = {'': []}
    for name, content in files.items():
        parts = name.split('/')
        for i in range(len(parts) - 1):
            dirs.setdefault('/'.join(parts[:i+1]), [])
    dir_sectors = {name: 40 + i for i, name in enumerate(dirs)}
    file_sectors = {}
    next_sector = 64
    for name, content in files.items():
        file_sectors[name] = next_sector
        next_sector += max(1, (len(content) + 2047) // 2048)
    for name in dirs:
        if name:
            parent, _, leaf = name.rpartition('/')
            dirs[parent].append((leaf, dir_sectors[name], 2048, 0x10))
    for name, content in files.items():
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


def file_snapshot(directory):
    """Small fixture bytes only; never follows a user installation."""
    return {path.relative_to(directory).as_posix(): path.read_bytes()
            for path in directory.rglob('*') if path.is_file()}


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
        # New art is copied beside the runtime, while packages without it still work.
        (package / 'runtime/hd-ui').mkdir()
        png = png_bytes()
        source_art = package / 'runtime/hd-ui/restored.png'
        source_art.write_bytes(png)
        art = {'hd-ui/restored.png': digest(png)}
        def write_runtime(artwork=art):
            (package / 'tools/runtime-manifest.json').write_text(json.dumps(
                {'version': packager.VERSION, 'files': runtime, 'artwork': artwork}))
        write_runtime()
        worker.launch(data)
        dest_art = state / 'hd-ui/restored.png'
        assert dest_art.read_bytes() == png
        worker.launch(data)
        assert dest_art.read_bytes() == png and not list(state.rglob('.update-*'))
        launched = len(seen)
        for invalid in (None, [], 'art', {'hd-ui/../bad.png': '0'*64}, {'hd-ui/sub/a.png': '0'*64},
                        {'hd-ui/a\\b.png': '0'*64}, {'hd-ui/x:ads.png': '0'*64}, {'hd-ui/CON.png': '0'*64},
                        {'runtime/a.png': '0'*64}, {'hd-ui/a.dll': '0'*64}, {'hd-ui/a.PNG': '0'*64},
                        {'hd-ui/' + 'a'*128 + '.png': '0'*64}, {'hd-ui/restored.png': None},
                        {'hd-ui/restored.png': 'A'*64}, {**art, 'hd-ui/RESTORED.png': digest(png)}):
            write_runtime(invalid)
            expect_error(lambda: worker.launch(data), ValueError)
            assert len(seen) == launched and dest_art.read_bytes() == png
        write_runtime()
        source_art.write_bytes(b'corrupt')
        expect_error(lambda: worker.launch(data), ValueError)
        source_art.unlink()
        expect_error(lambda: worker.launch(data), ValueError)
        source_art.write_bytes(png)
        # Size is checked before hashing or copying the package file.
        with source_art.open('wb') as file:
            file.seek(worker.ARTWORK_LIMIT)
            file.write(b'x')
        expect_error(lambda: worker.launch(data), ValueError)
        source_art.write_bytes(png)
        linked = worker.linked
        worker.linked = lambda path: path == state / 'hd-ui' or linked(path)
        try:
            expect_error(lambda: worker.launch(data), ValueError)
        finally:
            worker.linked = linked
        assert len(seen) == launched and dest_art.read_bytes() == png
        # Failure after a staged copy leaves the old approved image and saves intact.
        update = png_bytes(12, 12)
        source_art.write_bytes(update)
        write_runtime({'hd-ui/restored.png': digest(update)})
        copy_file = worker.shutil.copy2
        def corrupt_copy(source, target):
            if Path(source) == source_art:
                Path(target).write_bytes(b'incomplete')
                return target
            return copy_file(source, target)
        worker.shutil.copy2 = corrupt_copy
        try:
            expect_error(lambda: worker.launch(data), ValueError)
        finally:
            worker.shutil.copy2 = copy_file
        assert dest_art.read_bytes() == png and not list(state.rglob('.update-*'))
        replace = worker.os.replace
        def failed_art_replace(source, target):
            if Path(target) == dest_art:
                raise OSError('fixture: artwork replace failed')
            return replace(source, target)
        worker.os.replace = failed_art_replace
        try:
            expect_error(lambda: worker.launch(data), OSError)
        finally:
            worker.os.replace = replace
        assert dest_art.read_bytes() == png and not list(state.rglob('.update-*'))
        worker.launch(data)
        assert dest_art.read_bytes() == update
        assert (state / 'settings.toml').read_bytes() == b'user-settings'
        assert (state / 'xboxrecomp.ini').read_bytes() == b'user-controls'
        assert (state / 'hdd/profile').read_bytes() == b'user-save'
        expect_error(lambda: worker.launch(tmp / 'not-imported'), ValueError)
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
            'restored_artwork_verified_and_atomically_copied': True,
            'artwork_corruption_paths_and_copy_failures_rejected': True,
            'gameplay_launched': False, 'real_user_data_modified': False}


def check_portable(tmp):
    """Run real import/copy/rollback paths against a private legacy fixture."""
    base = tmp / 'portable-check'
    template = base / 'template'
    (template / 'tools').mkdir(parents=True)
    (template / 'runtime').mkdir()
    assets = {'Default.xbe': b'portable fixture executable', 'scene.ent': b'portable fixture level'}
    (template / 'tools/disc-manifest.json').write_text(json.dumps({'version': 1, 'files': {
        name: {'size': len(value), 'sha256': digest(value)} for name, value in assets.items()}}))
    for name in worker.RUNTIME_FILES:
        (template / 'runtime' / name).write_bytes(('harmless-' + name).encode())
    (template / 'tools/runtime-manifest.json').write_text(json.dumps({'version': packager.VERSION,
        'files': {name: worker.sha(template / 'runtime' / name) for name in worker.RUNTIME_FILES}}))
    iso = base / 'original.iso'
    make_iso(iso, assets)
    cancel = base / 'cancel'
    legacy = Path(os.environ['LOCALAPPDATA']) / 'UrbanChaosBurnerEdition'
    old_root = worker.ROOT
    worker.ROOT = template
    try:
        worker.import_iso(iso, legacy, cancel)
        state = {
            'settings.toml': b'old graphics settings', 'xboxrecomp.ini': b'old controls',
            'hdd/E/save.dat': b'old progress', 'hdd2/E/player2.dat': b'old second player',
            'shader-cache/d3d12/cache.bin': b'old shader cache',
        }
        for name, content in {**state, 'UrbanChaosXbeTrial.exe': b'never migrate an executable',
                              'logs/old.log': b'never migrate diagnostics'}.items():
            path = legacy / 'xbe-runtime' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        legacy_before = file_snapshot(legacy)

        def new_package(name):
            package = base / name
            shutil.copytree(template, package)
            worker.ROOT = package
            return package, package / 'GameData'

        def assert_state(data, expected=state):
            for name, content in expected.items():
                assert (data / 'xbe-runtime' / name).read_bytes() == content, name

        package, data = new_package('first portable package')
        # Status and Play never quietly choose the legacy installation.
        with patch.object(worker.subprocess, 'Popen') as spawn:
            assert not worker.ready(data)
            expect_error(lambda: worker.launch(data), ValueError)
            spawn.assert_not_called()
        assert not data.exists() and file_snapshot(legacy) == legacy_before
        worker.import_iso(iso, data, cancel)
        assert worker.ready(data)
        assert_state(data)
        assert file_snapshot(data / 'xbe-runtime') == state
        assert file_snapshot(legacy) == legacy_before
        assert not list(data.glob('.import-*'))

        # Move everything, then resolve the runtime and disc relative to its new home.
        before_move = file_snapshot(package)
        moved = base / 'moved portable package'
        package.rename(moved)
        worker.ROOT = moved
        data = moved / 'GameData'
        assert file_snapshot(moved) == before_move and not package.exists()
        launches = []
        class Process:
            def __init__(self, args, **kwargs):
                launches.append((args, kwargs))
                self.stderr = io.BytesIO()
            def wait(self):
                return 0
        with patch.object(worker.subprocess, 'Popen', Process):
            assert worker.ready(data)
            worker.launch(data)
        argv, options = launches[-1]
        assert Path(argv[0]) == data / 'xbe-runtime' / worker.RUNTIME_NAME
        assert options['cwd'] == data / 'xbe-runtime'
        assert options['env']['UCRR_GAME_ASSETS'] == str(data / 'installed/game_files')
        assert_state(data)
        assert file_snapshot(legacy) == legacy_before

        # Repair uses current portable state even when old metadata is damaged.
        newest = {name: b'new portable ' + content for name, content in state.items()}
        for name, content in newest.items():
            (data / 'xbe-runtime' / name).write_bytes(content)
        marker = legacy / 'installed/installed.json'
        marker_before = marker.read_bytes()
        marker.write_text('{damaged old metadata')
        damaged_before = file_snapshot(legacy)
        worker.import_iso(iso, data, cancel)
        assert_state(data, newest)
        assert file_snapshot(legacy) == damaged_before
        marker.write_bytes(marker_before)

        # A failed migration never publishes partial state or changes the source.
        original_copy = shutil.copy2
        original_rename = Path.rename
        for failure in ('copy', 'cancel-copy', 'runtime-rename', 'install-rename',
                        'cancel-published', 'cancel-installed'):
            _, target = new_package('failure-' + failure)
            target.mkdir()
            # A prior verified import without an XBE runtime must also roll back.
            shutil.copytree(legacy / 'installed', target / 'installed')
            target_before = file_snapshot(target)
            touched = []
            def copy_state(source, destination, *args, **kwargs):
                if Path(source) == legacy / 'xbe-runtime/hdd/E/save.dat':
                    touched.append('copy')
                    if failure == 'copy':
                        raise OSError('fixture: state copy failed')
                    result = original_copy(source, destination, *args, **kwargs)
                    if failure == 'cancel-copy':
                        cancel.touch()
                    return result
                return original_copy(source, destination, *args, **kwargs)
            def rename_state(source, destination):
                destination = Path(destination)
                if destination == target / 'xbe-runtime':
                    touched.append('publish')
                    if failure == 'runtime-rename':
                        raise OSError('fixture: runtime publication failed')
                    result = original_rename(source, destination)
                    if failure == 'cancel-published':
                        cancel.touch()
                    return result
                if source.name.startswith('.import-') and destination == target / 'installed':
                    if failure == 'install-rename':
                        raise OSError('fixture: install publication failed')
                    result = original_rename(source, destination)
                    if failure == 'cancel-installed':
                        cancel.touch()
                    return result
                return original_rename(source, destination)
            try:
                with patch.object(shutil, 'copy2', copy_state), patch.object(Path, 'rename', rename_state):
                    expect_error(lambda: worker.import_iso(iso, target, cancel),
                                 RuntimeError if failure.startswith('cancel-') else OSError)
            finally:
                if cancel.exists():
                    cancel.unlink()
            assert touched and file_snapshot(target) == target_before, failure
            assert not (target / 'xbe-runtime').exists(), failure
            assert not list(target.glob('.import-*')), failure
            assert file_snapshot(legacy) == legacy_before, failure

        # Validate migration identity before copying any old settings or saves.
        for bad_marker in (b'{broken', json.dumps({'disc_manifest': '0' * 64}).encode()):
            _, target = new_package('bad-marker-' + digest(bad_marker)[:8])
            marker.write_bytes(bad_marker)
            before = file_snapshot(legacy)
            expect_error(lambda: worker.import_iso(iso, target, cancel), ValueError)
            assert not (target / 'installed').exists() and not (target / 'xbe-runtime').exists()
            assert file_snapshot(legacy) == before and not list(target.glob('.import-*'))
        marker.write_bytes(marker_before)

        # Simulate lstat's link/reparse evidence, preserving the real path validator.
        # This needs no Windows symlink privilege and touches no outside directory.
        lstat = Path.lstat
        def redirect_info(path, suspect, junction=False):
            if path == suspect:
                return SimpleNamespace(st_mode=stat.S_IFDIR if junction else stat.S_IFLNK,
                                       st_file_attributes=0x400 if junction else 0)
            return lstat(path)
        # Wine drive mappings/Steam library links are outside the managed roots.
        _, target = new_package('mapped-host-ancestors')
        with patch.object(Path, 'lstat', lambda path: redirect_info(path, base, True)), \
                patch.object(worker.subprocess, 'Popen', Process):
            worker.import_iso(iso, target, cancel)
            assert worker.ready(target)
            worker.launch(target)
            assert_state(target)
        _, target = new_package('mapped-legacy-ancestor')
        with patch.object(Path, 'lstat', lambda path: redirect_info(path, legacy.parent, True)):
            worker.import_iso(iso, target, cancel)
            assert_state(target)
        assert file_snapshot(legacy) == legacy_before
        for index, suspect in enumerate((legacy, legacy / 'xbe-runtime',
                                        legacy / 'xbe-runtime/hdd', legacy / 'xbe-runtime/hdd/E/save.dat')):
            _, target = new_package('linked-source-' + str(index))
            with patch.object(Path, 'lstat', lambda path, p=suspect, j=bool(index % 2):
                              redirect_info(path, p, j)):
                expect_error(lambda: worker.import_iso(iso, target, cancel), ValueError)
            assert not (target / 'installed').exists() and not (target / 'xbe-runtime').exists()
            assert file_snapshot(legacy) == legacy_before and not list(target.glob('.import-*'))
        for name in ('GameData', 'GameData/xbe-runtime'):
            package, target = new_package('linked-target-' + name.replace('/', '-'))
            suspect = package / name
            with patch.object(Path, 'lstat', lambda path: redirect_info(path, suspect, True)):
                expect_error(lambda: worker.import_iso(iso, target, cancel), ValueError)
            assert not (target / 'installed').exists() and not (target / 'xbe-runtime').exists()
            assert file_snapshot(legacy) == legacy_before and not list(target.glob('.import-*'))
    finally:
        worker.ROOT = old_root
    return {'portable_first_import_migration_and_source_preservation': True,
            'portable_status_and_play_never_fall_back_to_legacy': True,
            'portable_move_assets_saves_settings_cache_and_cwd': True,
            'portable_repair_prefers_current_state': True,
            'portable_migration_cancel_copy_commit_rollback': True,
            'mapped_host_and_legacy_ancestors_allowed': True,
            'portable_migration_identity_and_reparse_rejections': True}


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
    artwork = package / 'runtime/hd-ui/restored.png'
    artwork.parent.mkdir()
    png = png_bytes()
    artwork.write_bytes(png)
    allowed.add('runtime/hd-ui/restored.png')
    manifest_path = package / 'tools/runtime-manifest.json'
    info = json.loads(manifest_path.read_text())
    info['artwork'] = {'hd-ui/restored.png': digest(png)}
    manifest_path.write_text(json.dumps(info))
    assert packager.audit(package, allowed)['restored_artwork_files'] == 1
    artwork.write_bytes(b'bad-art')
    expect_error(lambda: packager.audit(package, allowed), ValueError)
    artwork.write_bytes(png)
    for invalid in (None, [], {}, {'hd-ui/../restored.png': digest(png)}, {'hd-ui/restored.png': '0'*64}):
        broken = dict(info, artwork=invalid)
        manifest_path.write_text(json.dumps(broken))
        expect_error(lambda: packager.audit(package, allowed), ValueError)
    manifest_path.write_text(json.dumps(info))
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


def check_shader_seed(tmp):
    package = tmp / 'seed-package'
    seed_dir = package / 'runtime/shader-seed'
    seed_dir.mkdir(parents=True)
    (package / 'tools').mkdir()
    files = {'Default.xbe': b'seed fixture executable'}
    (package / 'tools/disc-manifest.json').write_text(json.dumps({'version': 1, 'files': {
        name: {'size': len(content), 'sha256': digest(content)} for name, content in files.items()}}))
    for name in worker.RUNTIME_FILES:
        (package / 'runtime' / name).write_bytes(('seed-fixture-' + name).encode())
    runtime = {name: worker.sha(package / 'runtime' / name) for name in worker.RUNTIME_FILES}
    name = 'v1-0123456789abcdef.bin'
    second = 'v1-1111111111111111.bin'
    source = b'UC-D3D12-DXBC-1:' + b'0' * 64 + b'4:main6:vs_5_04:test'
    def record(code=b'DXBCfixture-only', prefix=source):
        return struct.pack('<10IQ', 0x5343564e, 1, 0x55434431, 0, 0, 1, 2, 47,
                           len(prefix), len(code), 0) + prefix + code
    contents = record()
    (seed_dir / name).write_bytes(contents)
    seed = {name: digest(contents)}
    manifest_path = package / 'tools/runtime-manifest.json'
    def write_info(entries=seed):
        manifest_path.write_text(json.dumps({'version': packager.VERSION, 'files': runtime,
                                              'shader_seed': entries}))
    write_info()
    worker.ROOT = package
    iso = tmp / 'seed.iso'
    make_iso(iso, files)
    data = package / 'GameData'
    original_stat = Path.lstat
    def mapped_ancestor(path, *args, **kwargs):
        if path == tmp:
            return SimpleNamespace(st_mode=stat.S_IFDIR,
                                   st_file_attributes=stat.FILE_ATTRIBUTE_REPARSE_POINT)
        return original_stat(path, *args, **kwargs)
    with patch.dict(os.environ, {'LOCALAPPDATA': str(tmp / 'seed-isolated-appdata')}), \
            patch.object(Path, 'lstat', mapped_ancestor):
        worker.import_iso(iso, data, tmp / 'seed-cancel')
    seen = []
    class Process:
        def __init__(self, args, **kwargs):
            seen.append((args, kwargs))
            self.stderr = io.BytesIO()
        def wait(self):
            return 0
    with patch.object(worker.subprocess, 'Popen', Process):
        with patch.object(Path, 'lstat', mapped_ancestor):
            worker.launch(data)
        state = data / 'xbe-runtime'
        assert (state / 'shader-seed' / name).read_bytes() == contents
        protected = {'shader-cache/d3d12/v1-fixture.bin': b'user-learned shader',
                     'shader-cache/d3d12-pipelines/v1-fixture.bin': b'GPU-specific user cache',
                     'settings.toml': b'user settings', 'hdd/save.dat': b'user progress'}
        for relative, content in protected.items():
            path = state / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        def assert_protected():
            for relative, content in protected.items():
                assert (state / relative).read_bytes() == content
        # Updating a previous import installs new seeds and retires only managed seed files.
        (seed_dir / second).write_bytes(record(b'DXBCsecond'))
        write_info({second: digest((seed_dir / second).read_bytes())})
        (state / 'shader-seed/unrelated.txt').write_bytes(b'keep this file')
        worker.launch(data)
        assert not (state / 'shader-seed' / name).exists()
        assert (state / 'shader-seed' / second).read_bytes() == (seed_dir / second).read_bytes()
        assert (state / 'shader-seed/unrelated.txt').read_bytes() == b'keep this file'
        assert_protected()
        before = file_snapshot(state)
        launches = len(seen)
        for invalid in (None, [], 'seed', {name: '0' * 64}, {'../escape': '0' * 64},
                        {'shader-cache/d3d12/' + name: '0' * 64}, {'v1-0123456789ABCDEF.bin': '0' * 64},
                        {name: None}, {name: 'A' * 64}, {name + '.dll': '0' * 64},
                        {f'v1-{i:016x}.bin': '0' * 64 for i in range(2049)}):
            write_info(invalid)
            expect_error(lambda: worker.launch(data), ValueError)
            assert len(seen) == launches and file_snapshot(state) == before
        write_info()
        # Even a correctly hashed file must be DXBC, not a pipeline/driver blob.
        for bad in (record(b'GPU-specific-PSO'), record(prefix=b'wrong key prefix'), b'truncated',
                    contents + b'junk'):
            (seed_dir / name).write_bytes(bad)
            write_info({name: digest(bad)})
            expect_error(lambda: worker.launch(data), ValueError)
        (seed_dir / name).write_bytes(contents)
        write_info()
        original_stat = Path.lstat
        for redirected in (seed_dir, state / 'shader-seed'):
            def redirected_stat(path, *args, **kwargs):
                if path == redirected:
                    return SimpleNamespace(st_mode=stat.S_IFDIR,
                                           st_file_attributes=stat.FILE_ATTRIBUTE_REPARSE_POINT)
                return original_stat(path, *args, **kwargs)
            with patch.object(Path, 'lstat', redirected_stat):
                expect_error(lambda: worker.launch(data), ValueError)
        with patch.object(worker.shader_seed, 'BYTE_LIMIT', len(contents) - 1):
            expect_error(lambda: worker.launch(data), ValueError)
        with (seed_dir / name).open('wb') as file:
            file.seek(worker.shader_seed.FILE_LIMIT)
            file.write(b'x')
        expect_error(lambda: worker.launch(data), ValueError)
        (seed_dir / name).write_bytes(contents)
        # A failed staged copy does not publish a partial shader or erase the previous seed.
        copy_file = shutil.copy2
        def bad_copy(source_path, target):
            if Path(source_path) == seed_dir / name:
                Path(target).write_bytes(b'incomplete')
                return target
            return copy_file(source_path, target)
        with patch.object(worker.shutil, 'copy2', bad_copy):
            expect_error(lambda: worker.launch(data), ValueError)
        assert len(seen) == launches and file_snapshot(state) == before
        assert not list(state.rglob('.update-*'))
        replace = os.replace
        def failed_replace(source_path, target):
            if Path(target) == state / 'shader-seed' / name:
                raise OSError('fixture: shader seed publication failed')
            return replace(source_path, target)
        with patch.object(worker.os, 'replace', failed_replace):
            expect_error(lambda: worker.launch(data), OSError)
        assert len(seen) == launches and file_snapshot(state) == before
        worker.launch(data)
        assert_protected()
        # A portable folder move must not depend on the previous absolute seed location.
        moved = tmp / 'moved seed package'
        package.rename(moved)
        package = moved
        seed_dir = package / 'runtime/shader-seed'
        manifest_path = package / 'tools/runtime-manifest.json'
        data = package / 'GameData'
        state = data / 'xbe-runtime'
        worker.ROOT = package
        worker.launch(data)
        assert (state / 'shader-seed' / name).read_bytes() == contents
        assert_protected()

    # Source manifest and shipped package both require an exact seed allowlist.
    (seed_dir / second).unlink()
    (seed_dir / 'manifest.json').write_text(json.dumps({'version': 1, 'files': seed}))
    plan, identities = packager.shader_seed_sources(seed_dir)
    assert identities == seed and plan == {'runtime/shader-seed/' + name: seed_dir / name}
    (seed_dir / 'unlisted.bin').write_bytes(contents)
    expect_error(lambda: packager.shader_seed_sources(seed_dir), ValueError)
    (seed_dir / 'unlisted.bin').unlink()
    for invalid in ({'version': True, 'files': seed}, {'version': 2, 'files': seed},
                    {'version': 1, 'files': seed, 'extra': 1}):
        (seed_dir / 'manifest.json').write_text(json.dumps(invalid))
        expect_error(lambda: packager.shader_seed_sources(seed_dir), ValueError)
    (seed_dir / 'manifest.json').unlink()
    fixture_data = (package / 'GameData').resolve()
    assert fixture_data.is_relative_to(tmp.resolve()) and fixture_data.name == 'GameData'
    shutil.rmtree(fixture_data)  # This fixture's own imported data is not release payload.
    for launcher in packager.LAUNCHERS:
        (package / launcher).write_bytes(b'fixture launcher')
    allowed = {p.relative_to(package).as_posix() for p in package.rglob('*') if p.is_file()} - packager.LAUNCHERS
    assert packager.audit(package, allowed)['shader_seed_files'] == 1
    write_info({})
    expect_error(lambda: packager.audit(package, allowed), ValueError)
    write_info()
    return {'shader_seed_fresh_and_update_install_verified': True,
            'shader_seed_preserves_learned_cache_saves_and_settings': True,
            'shader_seed_portable_move_and_retired_files_verified': True,
            'shader_seed_schema_paths_hashes_bounds_dxbc_and_copy_failure_verified': True}


def check_artwork_manifest(tmp):
    directory = tmp / 'approved-artwork'
    directory.mkdir()
    png = png_bytes()
    path = directory / 'restored.png'
    path.write_bytes(png)
    row = {'name': 'hud\\fixture.tga', 'width': 2, 'height': 2, 'format': 15,
           'source_bytes': 16, 'source_sha256': digest(b'original-mip-data'),
           'png': path.name, 'png_sha256': digest(png), 'crop': [0, 0, 0, 0], 'alpha': 'source'}
    manifest = directory / 'manifest.json'
    def write(rows=None, version=1):
        manifest.write_text(json.dumps({'version': version, 'assets': [row] if rows is None else rows}))
    write()
    plan, artwork = packager.artwork_sources(directory)
    assert plan == {'runtime/hd-ui/restored.png': path}
    assert artwork == {'hd-ui/restored.png': digest(png)}
    alias = dict(row, name='frontend\\same-texture.tga')
    write([row, alias])
    assert packager.artwork_sources(directory) == (plan, artwork)
    write([dict(row, placement='canvas')])
    assert packager.artwork_sources(directory) == (plan, artwork)
    write([dict(row, alpha='png', placement='source_bounds')])
    assert packager.artwork_sources(directory) == (plan, artwork)
    write([dict(row, alpha='png', placement='source_bounds', crop=[0, 0, 4, 5])])
    assert packager.artwork_sources(directory) == (plan, artwork)
    for leaf in ('lc_medal01.tga', 'lc_medal02.tga', 'lc_medal03.tga', 'lc_medal04.tga',
                 'lc_medal01smg.tga', 'reward_star.tga'):
        write([dict(row, name='frontend\\frt_content\\'+leaf, alpha='png', placement='source_bounds')])
        assert packager.artwork_sources(directory) == (plan, artwork)
    for bad in (dict(row, placement='source_bounds'),
                dict(alias, alpha='png', placement='source_bounds'),
                dict(row, placement=None), dict(row, placement='unknown')):
        write([bad])
        expect_error(lambda: packager.artwork_sources(directory), ValueError)
    write([dict(row, alpha='png'), dict(row, alpha='png', placement='source_bounds')])
    expect_error(lambda: packager.artwork_sources(directory), ValueError)
    changes = {'name': ('../escape', 'hud\\..\\escape.tga', 'HUD\\fixture.tga'),
               'width': (0, -1, True, 1025), 'height': (0, 1025), 'format': (0, 14, True),
               'source_bytes': (0, 2*1024*1024+1), 'source_sha256': ('0', 'A'*64, None),
               'png_sha256': ('0'*64, None), 'png': ('../restored.png', 'x.dll', 'x.PNG', 'NUL.png', 'x/y.png'),
               'alpha': ('unknown', None), 'color': ('unknown', None, 'source_chroma'), 'crop': ([0, 0, 0, 1], [1, 0, 8, 8], [0, 0, 4, 5],
                                                 [0, 0, 3, 3], [True, 0, 8, 8], [0, 0, 8], None)}
    for field, values in changes.items():
        for value in values:
            bad = copy.deepcopy(row)
            bad[field] = value
            write([bad])
            expect_error(lambda: packager.artwork_sources(directory), ValueError)
    write([row], True)
    expect_error(lambda: packager.artwork_sources(directory), ValueError)
    write([dict(row, extra='unapproved')])
    expect_error(lambda: packager.artwork_sources(directory), ValueError)
    write([row, dict(alias, crop=[0, 0, 4, 4])])
    expect_error(lambda: packager.artwork_sources(directory), ValueError)
    write([row, dict(alias, png='RESTORED.png')])
    expect_error(lambda: packager.artwork_sources(directory), ValueError)
    write()
    path.write_bytes(b'corrupt')
    expect_error(lambda: packager.artwork_sources(directory), ValueError)
    path.unlink()
    expect_error(lambda: packager.artwork_sources(directory), ValueError)
    path.write_bytes(png)
    assert packager.artwork_sources(directory) == (plan, artwork)
    # Explicit blue-thumbnail treatment is opt-in; legacy/default PNG colors
    # and shared master images remain valid. All identities are still pinned.
    blue_png = png_bytes(128, 128)
    path.write_bytes(blue_png)
    blue = dict(row, name='frontend\\frt_content\\level_image_01smg.tga', width=32,
                height=32, png_sha256=digest(blue_png), color='source_chroma')
    write([blue])
    assert packager.artwork_sources(directory)[1] == {'hd-ui/restored.png': digest(blue_png)}
    reward = dict(blue, name='frontend\\frt_content\\reward_image_assault_fadesmg.tga')
    write([reward])
    assert packager.artwork_sources(directory)[1] == {'hd-ui/restored.png': digest(blue_png)}
    for bad in (dict(blue, width=64), dict(blue, alpha='png'), dict(blue, name='hud\\smg.tga'),
                dict(blue, name='frontend\\frt_content\\level_image_01sm.tga'),
                dict(blue, name='frontend\\frt_content\\icons\\smg.tga')):
        write([bad])
        expect_error(lambda: packager.artwork_sources(directory), ValueError)
    write([blue, dict(blue, color='png')])
    expect_error(lambda: packager.artwork_sources(directory), ValueError)


def check_compiled_direct(package, tmp):
    harness = tmp / 'direct-harness'
    (harness / 'tools').mkdir(parents=True)
    shutil.copy2(package / 'Play Burner Edition.exe', harness / 'Play Burner Edition.exe')
    shutil.copytree(package / 'tools/python', harness / 'tools/python')
    (harness / 'tools/import_game.py').write_text(
        "from pathlib import Path\nimport json,sys\n"
        "root=Path(__file__).resolve().parent.parent\n"
        "data=Path(sys.argv[sys.argv.index('--data')+1]).resolve()\n"
        "assert data==root/'GameData'\nassert Path.cwd()==root\n"
        "with (root/'tools/worker-events.jsonl').open('a',encoding='utf-8') as f:\n"
        " f.write(json.dumps({'action':sys.argv[1],'data':str(data),'cwd':str(Path.cwd())})+'\\n')\n"
        "if sys.argv[1]=='launch':(data/'launched.txt').write_text('harmless worker')\n")
    (harness / 'GameData').mkdir()
    (harness / 'GameData/save.fixture').write_bytes(b'portable saved progress')
    unrelated = tmp / 'unrelated-launch-directory'
    unrelated.mkdir()
    for current in (harness, tmp / 'moved direct harness'):
        if current != harness:
            harness.rename(current)
        result = subprocess.run([str(current / 'Play Burner Edition.exe')], cwd=unrelated,
                                timeout=20, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        assert result.returncode == 0
        assert (current / 'GameData/launched.txt').read_text() == 'harmless worker'
        assert (current / 'GameData/save.fixture').read_bytes() == b'portable saved progress'
        events = [json.loads(line) for line in (current / 'tools/worker-events.jsonl').read_text().splitlines()]
        assert [event['action'] for event in events[-2:]] == ['status', 'launch']
        assert all(Path(event['data']) == current / 'GameData' and Path(event['cwd']) == current
                   for event in events[-2:])
    assert not harness.exists() and not list(unrelated.iterdir())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--package', type=Path)
    parser.add_argument('--evidence', type=Path, default=REPO / 'build/burner-xbe-launcher-checks.json')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='burner-xbe-test-', dir=REPO / 'build') as directory:
        tmp = Path(directory)
        # A first-import migration must never inspect the real user's AppData.
        with patch.dict(os.environ, {'LOCALAPPDATA': str(tmp / 'isolated-appdata')}):
            result = check_worker(tmp)
            result.update(check_portable(tmp))
            check_asset_audit(tmp)
            check_artwork_manifest(tmp)
            result.update(check_shader_seed(tmp))
            result['strict_asset_allowlist_negative_cases'] = True
            result['approved_artwork_schema_hashes_crop_and_aliases_verified'] = True
            if args.package:
                check_compiled_direct(args.package.resolve(), tmp)
                result['compiled_direct_exe_with_stub_worker'] = True
                result['compiled_direct_portable_data_cwd_and_move_verified'] = True
    args.evidence.parent.mkdir(parents=True, exist_ok=True)
    args.evidence.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: verified ISO import, rollback, rejection, isolated runtime launch and bounded error handling')


if __name__ == '__main__':
    main()
