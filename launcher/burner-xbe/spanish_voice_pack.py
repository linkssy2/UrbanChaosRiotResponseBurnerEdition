"""Install a Spanish audio delta ZIP against verified local Xbox and text assets."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import stat
import uuid
import zipfile

from shader_seed import unlinked
from spanish_text import FILES

BLOCK = 1024 * 1024
MANIFEST_LIMIT = 32 * BLOCK
FILE_LIMIT = 2 * 1024**3
PAYLOAD_LIMIT = 3 * 1024**3
OUTPUT_LIMIT = 8 * 1024**3
SEGMENT_LIMIT = 500000
README = (
    'Urban Chaos: Riot Response - Spanish voice pack\n\n'
    'Import your supported Xbox game in Setup first. Then select this ZIP as the\n'
    'Spanish voice pack in Setup. Select Spanish and restart the game.\n'
    'Windows and Linux through Proton use this same pack. Linux is not native.\n'
    'Keep the ZIP intact; Setup verifies it and rebuilds the required files locally.\n'
    'Installing or updating the pack preserves saves and settings.\n\n'
    'Credits: Spanish dubbing from the original European PlayStation 2 edition.\n'
    'The separately bundled Spanish text translation is by ZARF.\n'
)
SAFS = frozenset(f'{number}.saf' for number in range(1, 7))
_STEMS = tuple('intro_' + str(number) for number in
               (1, 2, 3, 4, 5, 6, 8, 9, 11, 12, 13, 21, 22, 23, 24, 25, 26)) + (
                   'intro_12_edit', 'outro_13')
MOVIES = frozenset('movies/' + stem + '.xmv' for stem in _STEMS) | frozenset(
    'movies/pal_' + stem + '.xmv' for stem in _STEMS if stem not in
    ('intro_12_edit', 'intro_21', 'intro_22', 'intro_23', 'intro_24', 'intro_25', 'intro_26'))
OUTPUTS = frozenset(FILES) | SAFS | MOVIES


def _unique(pairs):
    result = {}
    for name, value in pairs:
        if name in result:
            raise ValueError('Duplicate Spanish pack metadata field.')
        result[name] = value
    return result


def _hash(value):
    return isinstance(value, str) and bool(re.fullmatch('[0-9a-f]{64}', value))


def _integer(value, maximum, minimum=0):
    return type(value) is int and minimum <= value <= maximum


def validate_manifest(document, game_files, text_files, text_sha):
    """Validate every recipe against the release's own source identities."""
    if (not isinstance(document, dict) or
            set(document) != {'format', 'version', 'text_sha256', 'files'} or
            document['format'] != 'uc-spanish-voice-pack' or
            type(document['version']) is not int or document['version'] != 1 or
            document['text_sha256'] != text_sha or not _hash(text_sha) or
            not isinstance(document['files'], list) or len(document['files']) != len(OUTPUTS)):
        raise ValueError('This Spanish voice pack does not match the included Spanish text.')
    names, payloads = set(), set()
    payload_total = output_total = segment_total = 0
    fields = {'name', 'origin', 'source', 'source_size', 'source_sha256', 'size',
              'sha256', 'payload', 'payload_size', 'payload_sha256', 'segments'}
    for index, row in enumerate(document['files']):
        if not isinstance(row, dict) or set(row) != fields:
            raise ValueError('Invalid Spanish voice file recipe.')
        name = row['name']
        if not isinstance(name, str) or name not in OUTPUTS or name in names:
            raise ValueError('Unexpected or duplicate Spanish voice asset.')
        names.add(name)
        origin = 'text' if name in FILES else 'game'
        source = ('Movies/' + name[7:]) if name in MOVIES else name
        if row['origin'] != origin or row['source'] != source:
            raise ValueError('Spanish pack refers to an unsupported source path.')
        expected = (text_files if origin == 'text' else game_files).get(source)
        if (not isinstance(expected, dict) or row['source_size'] != expected.get('size') or
                row['source_sha256'] != expected.get('sha256') or
                not _integer(row['source_size'], FILE_LIMIT, 1) or
                not _hash(row['source_sha256']) or not _hash(row['sha256']) or
                not _hash(row['payload_sha256']) or not _integer(row['size'], FILE_LIMIT, 1) or
                not _integer(row['payload_size'], FILE_LIMIT)):
            raise ValueError('Spanish pack does not match the supported original game files.')
        payload = f'payloads/{index:03}.bin'
        if row['payload'] != payload:
            raise ValueError('Invalid Spanish pack payload path.')
        payloads.add(payload)
        segments = row['segments']
        if not isinstance(segments, list) or not 0 < len(segments) <= SEGMENT_LIMIT:
            raise ValueError('Invalid Spanish pack segment list.')
        output_size = data_size = 0
        for segment in segments:
            if (not isinstance(segment, list) or len(segment) != 3 or
                    segment[0] not in ('copy', 'data') or
                    not _integer(segment[1], FILE_LIMIT) or
                    not _integer(segment[2], FILE_LIMIT, 1)):
                raise ValueError('Invalid Spanish pack segment.')
            kind, offset, size = segment
            limit = row['source_size'] if kind == 'copy' else row['payload_size']
            if offset + size > limit or (kind == 'data' and offset != data_size):
                raise ValueError('Spanish pack segment exceeds its verified source.')
            if kind == 'data':
                data_size += size
            output_size += size
        if output_size != row['size'] or data_size != row['payload_size']:
            raise ValueError('Spanish pack recipe sizes do not match its files.')
        segment_total += len(segments)
        payload_total += row['payload_size']
        output_total += row['size']
    if (names != OUTPUTS or payload_total > PAYLOAD_LIMIT or output_total > OUTPUT_LIMIT or
            segment_total > SEGMENT_LIMIT):
        raise ValueError('Spanish pack is incomplete or exceeds its installation limits.')
    return payloads, output_total


def _digest(stream, size, check):
    digest = hashlib.sha256()
    while size:
        check()
        block = stream.read(min(BLOCK, size))
        if not block:
            raise ValueError('A Spanish voice pack file was truncated.')
        digest.update(block)
        size -= len(block)
    if stream.read(1):
        raise ValueError('A Spanish voice pack file exceeds its declared size.')
    return digest.hexdigest()


def inspect(archive, game_files, text_files, text_sha, check):
    """Admit ZIP members and hash all payloads without extracting any ZIP paths."""
    members = archive.infolist()
    if len(members) != len(OUTPUTS) + 2 or len({item.filename for item in members}) != len(members):
        raise ValueError('Unexpected or duplicate Spanish voice ZIP members.')
    by_name = {item.filename: item for item in members}
    metadata = by_name.get('manifest.json')
    if metadata is None or not 0 < metadata.file_size <= MANIFEST_LIMIT:
        raise ValueError('The Spanish voice ZIP has no valid manifest.')
    for item in members:
        mode = stat.S_IFMT(item.external_attr >> 16)
        if (item.is_dir() or item.flag_bits & 1 or mode not in (0, stat.S_IFREG) or
                item.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED) or
                not 0 <= item.file_size <= FILE_LIMIT):
            raise ValueError('Unsupported Spanish voice ZIP member.')
    document = json.loads(archive.read(metadata).decode('utf-8'), object_pairs_hook=_unique)
    payloads, output_total = validate_manifest(document, game_files, text_files, text_sha)
    if set(by_name) != payloads | {'manifest.json', 'README.txt'}:
        raise ValueError('The Spanish ZIP contains unexpected files.')
    if (by_name['README.txt'].file_size != len(README.encode('utf-8')) or
            archive.read('README.txt') != README.encode('utf-8')):
        raise ValueError('The Spanish voice ZIP instructions are missing or damaged.')
    for row in document['files']:
        item = by_name[row['payload']]
        if item.file_size != row['payload_size']:
            raise ValueError('The Spanish voice ZIP payload size is incorrect.')
        with archive.open(item) as stream:
            if _digest(stream, row['payload_size'], check) != row['payload_sha256']:
                raise ValueError('The Spanish voice ZIP payload is damaged.')
    return document, output_total


def reconstruct(archive, document, original, translated, stage, check, progress=None):
    """Build a fresh stage, retaining source handles for each checked output."""
    for index, row in enumerate(document['files']):
        check()
        source_root = translated if row['origin'] == 'text' else original
        source = source_root / row['source']
        unlinked(source, root=source_root)
        target = stage / row['name']
        unlinked(target, root=stage)
        target.parent.mkdir(parents=True, exist_ok=True)
        with source.open('rb') as local, archive.open(row['payload']) as payload:
            if source.stat().st_size != row['source_size'] or _digest(local, row['source_size'], check) != row['source_sha256']:
                raise ValueError('Original game files changed. Re-import your supported Xbox ISO: ' + row['source'])
            digest = hashlib.sha256()
            with target.open('xb') as output:
                for kind, offset, size in row['segments']:
                    stream = local if kind == 'copy' else payload
                    if kind == 'copy':
                        stream.seek(offset)
                    while size:
                        check()
                        block = stream.read(min(size, BLOCK))
                        if not block:
                            raise ValueError('A Spanish voice source changed during installation.')
                        output.write(block)
                        digest.update(block)
                        size -= len(block)
            if target.stat().st_size != row['size'] or digest.hexdigest() != row['sha256']:
                raise ValueError('A reconstructed Spanish voice file failed verification: ' + row['name'])
        if progress:
            progress('Installing Spanish voices', 10 + int((index + 1) * 88 / len(document['files'])))


def install(pack, data, cancel):
    import import_game as worker
    import spanish_voice_setup as setup
    from disc import check_cancel, progress, sha

    pack, data = Path(pack).absolute(), Path(data).absolute()
    unlinked(pack, root=pack.parent)
    unlinked(data, root=data)
    if not pack.is_file() or not 0 < pack.stat().st_size <= PAYLOAD_LIMIT + MANIFEST_LIMIT + BLOCK:
        raise ValueError('Select the complete Spanish voice pack ZIP.')
    if not worker.ready(data):
        raise ValueError('Import your supported Xbox ISO before installing Spanish voices.')
    bundled = worker.spanish_bundle()
    if bundled is None:
        raise ValueError('The included Spanish text is missing. Extract the complete build again.')
    check = lambda: check_cancel(cancel)
    check()
    worker.ensure_spanish(data, cancel=cancel)
    original = data / 'installed/game_files'
    parent = data / 'xbe-runtime/translations'
    translated, destination = parent / 'es-zarf', parent / 'es-voices'
    for path in (original, translated, destination):
        unlinked(path, root=data)
    text_sha = sha(bundled[0])
    text_files = {row['file']: row for row in bundled[1]['files']}
    game_files = worker.manifest()['files']
    stage = parent / ('.spanish-voice-pack-' + uuid.uuid4().hex)
    try:
        progress('Checking Spanish voice pack', 1)
        with zipfile.ZipFile(pack) as archive:
            document, output_total = inspect(archive, game_files, text_files, text_sha, check)
            if shutil.disk_usage(data).free < output_total + 256 * BLOCK:
                raise ValueError('Not enough free space to install Spanish voices while keeping the current pack.')
            check()
            stage.mkdir()
            reconstruct(archive, document, original, translated, stage, check, progress)
            files = setup.write_manifest(stage)
            (stage / 'install.json').write_text(json.dumps({
                'version': 1, 'text_sha256': text_sha, 'files': files,
            }, indent=2) + '\n', encoding='utf-8')
            check()
            setup.publish(stage, destination, cancel)
        progress('Spanish voices installed. Select Spanish and restart the game.', 100)
    finally:
        if stage.exists():
            if stage.resolve().parent != parent.resolve() or not stage.name.startswith('.spanish-voice-pack-'):
                raise ValueError('Invalid Spanish voice staging directory.')
            shutil.rmtree(stage)
