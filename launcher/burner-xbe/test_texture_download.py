"""Official HD download through the real verifier, installer and setup worker."""
from contextlib import contextmanager
import hashlib
import io
from pathlib import Path
import stat
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_game as worker
import release_download
import test_source_setup as source
from test_setup import file_snapshot
import world_textures as textures


def pack(fill=1, invalid=False):
    payload = b'UCWT0001' + struct.pack('<III', 4, 4, 3) + bytes([fill]) * 48
    digest = hashlib.sha256(payload).hexdigest()
    manifest = ('4 4 12 8 ' + 'a' * 64 + ' ' + digest + '\n').encode()
    output = io.BytesIO()
    with zipfile.ZipFile(output, 'w') as archive:
        archive.writestr('world-textures/manifest.txt', manifest)
        archive.writestr('world-textures/' + digest + '.uctex', payload)
        if invalid:
            archive.writestr('../outside', b'reject')
    return output.getvalue(), {'manifest.txt': manifest, digest + '.uctex': payload}


class Response(io.BytesIO):
    def __init__(self, body, url, *, headers=None, status=200, on_read=None):
        super().__init__(body)
        self.url, self.status, self.on_read = url, status, on_read
        self.headers = {'Content-Length': str(len(body))} if headers is None else headers

    def geturl(self):
        return self.url

    def read(self, size=-1):
        result = super().read(size)
        if self.on_read:
            self.on_read(result)
        return result


@contextmanager
def network(body, *, received=None, info_fields=None, response_fields=None, error=None):
    info = dict(textures.OFFICIAL_PACK, asset_size=len(body),
                asset_digest='sha256:' + hashlib.sha256(body).hexdigest())
    info.update(info_fields or {})
    response = Response(body if received is None else received, info['asset_url'],
                        **(response_fields or {}))
    opener = Mock()
    opener.open.return_value = response
    if error:
        opener.open.side_effect = error
    with patch.object(textures, 'OFFICIAL_PACK', info), \
            patch.object(release_download.urllib.request, 'build_opener', return_value=opener):
        try:
            yield opener
        finally:
            response.close()


class DownloadTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='texture-download-', dir=source.HERE.parents[1] / 'build')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.data = self.root / 'GameData'
        self.data.mkdir()
        self.cancel = self.root / 'cancel'
        self.target = self.data / 'xbe-runtime/world-textures'
        self.body, self.expected = pack()
        old, self.previous = pack(2)
        archive = self.root / 'old.zip'
        archive.write_bytes(old)
        with patch.object(textures, 'progress'):
            textures.install(archive, self.data, self.cancel)
        self.events = []
        progress = patch.object(textures, 'progress', lambda message, percent: self.events.append((message, percent)))
        progress.start()
        self.addCleanup(progress.stop)

    def clean(self):
        self.assertFalse(list(self.data.glob('.hd-download-*')))
        self.assertFalse(list((self.data / 'xbe-runtime').glob('.textures-*')))

    def unchanged(self):
        self.assertEqual(file_snapshot(self.target), self.previous)
        self.clean()

    def test_official_metadata_matches_verified_release(self):
        self.assertEqual(textures.OFFICIAL_PACK, {
            'asset_url': 'https://github.com/linkssy2/UrbanChaosRiotResponseBurnerEdition/releases/download/v0.09/UrbanChaos-BurnerEdition-0.09-HD-Game-Textures.zip',
            'asset_size': 207636056,
            'asset_digest': 'sha256:0fb97f18e36e547925a0569522f53419886bb13705e38c73f54a0a2af5650e09',
        })

    def test_success_verifies_then_replaces_and_cleans(self):
        with network(self.body) as opener:
            textures.download_install(self.data, self.cancel)
        self.assertEqual(file_snapshot(self.target), self.expected)
        self.assertEqual(opener.open.call_count, 1)
        request = opener.open.call_args.args[0]
        self.assertEqual(request.full_url, textures.OFFICIAL_PACK['asset_url'])
        self.assertEqual(request.get_header('Accept-encoding'), 'identity')
        self.assertTrue(any('Downloading' in message for message, _ in self.events))
        self.assertIn('installed', self.events[-1][0])
        self.assertEqual(self.events[-1][1], 100)
        self.clean()

    def test_response_failures_preserve_previous_pack_and_allow_retry(self):
        variants = [
            {'error': OSError('network unavailable')},
            {'info_fields': {'asset_digest': 'sha256:' + '0' * 64}},
            {'received': self.body[:-1], 'response_fields': {'headers': {'Content-Length': str(len(self.body))}}},
            {'received': self.body + b'x', 'response_fields': {'headers': {'Content-Length': str(len(self.body))}}},
            {'response_fields': {'status': 206}},
            {'response_fields': {'headers': {'Content-Length': '1'}}},
            {'response_fields': {'headers': {'Content-Length': 'invalid'}}},
        ]
        for variant in variants:
            with self.subTest(variant=variant), network(self.body, **variant):
                with self.assertRaises((ValueError, OSError)):
                    textures.download_install(self.data, self.cancel)
                self.unchanged()
        with network(self.body):
            textures.download_install(self.data, self.cancel)
        self.assertEqual(file_snapshot(self.target), self.expected)
        self.clean()

    def test_untrusted_response_and_read_failure_leave_no_partial(self):
        with network(self.body) as opener:
            opener.open.return_value.url = 'https://example.org/pack.zip'
            with self.assertRaisesRegex(ValueError, 'trusted HTTPS'):
                textures.download_install(self.data, self.cancel)
        self.unchanged()
        reads = 0
        def fail_read(block):
            nonlocal reads
            reads += 1
            if reads == 2:
                raise TimeoutError('read interrupted')
        with network(self.body, response_fields={'on_read': fail_read}), patch.object(release_download, 'CHUNK', 64):
            with self.assertRaises(TimeoutError):
                textures.download_install(self.data, self.cancel)
        self.assertGreater(reads, 1)
        self.unchanged()

    def test_cancel_before_download_and_during_partial_read(self):
        self.cancel.touch()
        with network(self.body) as opener, self.assertRaisesRegex(RuntimeError, 'cancelled'):
            textures.download_install(self.data, self.cancel)
        opener.open.assert_not_called()
        self.unchanged()
        self.cancel.unlink()
        with network(self.body, response_fields={'on_read': lambda block: self.cancel.touch()}), \
                patch.object(release_download, 'CHUNK', 64), self.assertRaisesRegex(RuntimeError, 'cancelled'):
            textures.download_install(self.data, self.cancel)
        self.unchanged()

    def test_cancel_during_install_keeps_existing_pack(self):
        def cancel_on_install(message, percent):
            if message == 'Installing HD game textures':
                self.cancel.touch()
        with network(self.body), patch.object(textures, 'progress', cancel_on_install), \
                self.assertRaisesRegex(RuntimeError, 'cancelled'):
            textures.download_install(self.data, self.cancel)
        self.unchanged()

    def test_valid_download_with_bad_archive_keeps_existing_pack(self):
        invalid, _ = pack(invalid=True)
        with network(invalid), self.assertRaises(ValueError):
            textures.download_install(self.data, self.cancel)
        self.assertFalse((self.root / 'outside').exists())
        self.unchanged()

    def test_publish_failure_restores_previous_pack(self):
        rename = Path.rename
        def fail_publish(path, destination):
            if path.name.startswith('.textures-') and not path.name.startswith('.textures-old-'):
                raise OSError('publication interrupted')
            return rename(path, destination)
        with network(self.body), patch.object(Path, 'rename', fail_publish), self.assertRaises(OSError):
            textures.download_install(self.data, self.cancel)
        self.unchanged()

    def test_disk_space_rejected_before_network_or_before_install(self):
        with network(self.body) as opener, patch.object(textures.shutil, 'disk_usage', return_value=SimpleNamespace(free=0)), \
                self.assertRaisesRegex(ValueError, 'free space'):
            textures.download_install(self.data, self.cancel)
        opener.open.assert_not_called()
        self.unchanged()
        with network(self.body), patch.object(textures.shutil, 'disk_usage',
                side_effect=[SimpleNamespace(free=2**30), SimpleNamespace(free=0)]), \
                self.assertRaisesRegex(ValueError, 'free space'):
            textures.download_install(self.data, self.cancel)
        self.unchanged()

    def test_linked_install_destinations_rejected_before_network(self):
        original = Path.lstat
        for linked in (self.data, self.data / 'xbe-runtime', self.target):
            def lstat(path, *args, **kwargs):
                if path == linked:
                    return SimpleNamespace(st_mode=stat.S_IFLNK)
                return original(path, *args, **kwargs)
            with self.subTest(path=linked), network(self.body) as opener, patch.object(Path, 'lstat', lstat), \
                    self.assertRaises(ValueError):
                textures.download_install(self.data, self.cancel)
            opener.open.assert_not_called()
            self.unchanged()


class WorkerTests(unittest.TestCase):
    # Reuse the real small disc fixture without inheriting its unrelated tests.
    setUp = source.SourceSetupTests.setUp
    setup = source.SourceSetupTests.setup
    command = source.SourceSetupTests.command

    def external(self):
        self.setup(direct=False)
        external = self.root / 'External GameData'
        self.data.rename(external)
        self.data = external
        worker.select_installation(external)

    def test_selected_external_install_receives_download_without_duplicate_data(self):
        self.external()
        body, expected = pack()
        before = file_snapshot(self.data / 'installed')
        with network(body):
            code, _ = self.command('download-textures', '--cancel', str(self.cancel))
        self.assertEqual(code, 0)
        self.assertEqual(file_snapshot(self.data / 'xbe-runtime/world-textures'), expected)
        self.assertEqual(file_snapshot(self.data / 'installed'), before)
        self.assertFalse((self.package / 'GameData').exists())
        self.assertFalse(list(self.data.glob('.hd-download-*')))

    def test_package_and_selected_install_locks_block_before_network(self):
        self.external()
        body, _ = pack()
        for directory in (self.package, self.data):
            with self.subTest(lock=directory), worker.setup_lock(directory), network(body) as opener:
                code, _ = self.command('download-textures', '--cancel', str(self.cancel))
                self.assertEqual(code, 1)
                opener.open.assert_not_called()
        self.assertFalse((self.data / 'xbe-runtime/world-textures').exists())

    def test_missing_setup_rejected_before_network(self):
        body, _ = pack()
        with network(body) as opener:
            code, _ = self.command('download-textures', '--cancel', str(self.cancel))
        self.assertEqual(code, 1)
        opener.open.assert_not_called()
        self.assertFalse(list(self.data.glob('.hd-download-*')))

    def test_arguments_require_cancel_and_forbid_local_pack(self):
        self.setup()
        body, _ = pack()
        for arguments in ((), ('--pack', 'pack.zip'), ('--cancel', str(self.cancel), '--pack', 'pack.zip')):
            with self.subTest(arguments=arguments), network(body) as opener:
                code, _ = self.command('download-textures', *arguments)
                self.assertEqual(code, 1)
                opener.open.assert_not_called()
        self.assertFalse(list(self.data.glob('.hd-download-*')))


if __name__ == '__main__':
    unittest.main()
