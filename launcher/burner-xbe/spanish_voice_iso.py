"""Build optional Spanish audio locally from the supported, user-supplied PS2 ISO."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import time
import uuid

import spanish_movies
import spanish_voices
import spanish_voice_setup as setup
from disc import BLOCK, check_cancel, progress, sha
from ps2_audio import Ps2Disc, READ_LIMIT
from shader_seed import unlinked
from spanish_text import FILES

SAFS = frozenset(f'{number}.saf' for number in range(1, 7))
MOVIES = frozenset('movies/' + name for name in spanish_movies.MOVIES)
OUTPUTS = frozenset(FILES) | SAFS | MOVIES
OUTPUT_LIMIT = 8 * 1024**3
FILE_LIMIT = 2 * 1024**3
TOOL_FILES = frozenset('ffmpeg/' + name for name in (
    'ffmpeg.exe', 'avcodec-62.dll', 'avdevice-62.dll', 'avfilter-11.dll',
    'avformat-62.dll', 'avutil-60.dll', 'swresample-6.dll', 'swscale-9.dll',
    'LICENSE.txt', 'README.txt'))


def _unique(pairs):
    result = {}
    for name, value in pairs:
        if name in result:
            raise ValueError('Duplicate Spanish conversion metadata field.')
        result[name] = value
    return result


def _identity(path):
    info = path.stat()
    return info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns


def _verify(path, root, row, check, maximum=FILE_LIMIT):
    unlinked(path, root=root)
    if (not isinstance(row, dict) or type(row.get('size')) is not int or
            not 0 < row['size'] <= maximum or not isinstance(row.get('sha256'), str) or
            not re.fullmatch('[0-9a-f]{64}', row['sha256']) or
            not path.is_file() or path.stat().st_size != row['size']):
        raise ValueError('Missing or invalid Spanish conversion source: ' + path.name)
    before = _identity(path)
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        while block := stream.read(BLOCK):
            check()
            digest.update(block)
    if digest.hexdigest() != row['sha256'] or _identity(path) != before:
        raise ValueError('Damaged Spanish conversion source; re-import the game: ' + path.name)
    return before


def _tools(root, check):
    root = Path(root).absolute()
    folder, metadata = root / 'tools', root / 'tools/voice-tools.json'
    unlinked(metadata, root=root)
    if not metadata.is_file() or not 0 < metadata.stat().st_size <= 16384:
        raise ValueError('The bundled Spanish conversion tools are missing. Extract the complete build again.')
    info = json.loads(metadata.read_text(encoding='utf-8'), object_pairs_hook=_unique)
    if (not isinstance(info, dict) or set(info) != {'version', 'files'} or
            type(info['version']) is not int or info['version'] != 1 or
            not isinstance(info['files'], dict) or set(info['files']) != TOOL_FILES):
        raise ValueError('The bundled Spanish conversion tool list is invalid.')
    identities = {}
    for name, digest in info['files'].items():
        path = folder / name
        unlinked(path, root=root)
        row = {'size': path.stat().st_size if path.is_file() else 0, 'sha256': digest}
        identities[path] = _verify(path, root, row, check, 256 * BLOCK)
    return folder / 'ffmpeg/ffmpeg.exe', identities


def validate_tools(root, check=lambda: None):
    """Shared package/runtime validation of the complete offline tool bundle."""
    return _tools(root, check)[0]


def _budget(stage):
    total, names = 0, set()
    for path in stage.rglob('*'):
        unlinked(path, root=stage)
        if path.is_dir():
            if path != stage / 'movies':
                raise ValueError('Unexpected Spanish conversion directory.')
            continue
        name = path.relative_to(stage).as_posix()
        size = path.stat().st_size
        if name not in OUTPUTS or size > FILE_LIMIT:
            raise ValueError('Unexpected or oversized Spanish audio output: ' + name)
        total += size
        names.add(name)
    if total > OUTPUT_LIMIT:
        raise ValueError('Spanish audio conversion exceeds its installation limit.')
    return names


def _movie_source(disc, name, scratch, check):
    size = disc.size(name)
    if type(size) is not int or not 44 <= size <= spanish_movies.LIMIT:
        raise ValueError('Missing or oversized Spanish PS2 movie: ' + name)
    target = scratch / name
    with target.open('xb') as output:
        offset = 0
        while offset < size:
            check()
            count = min(READ_LIMIT, size - offset)
            block = disc.read(name, offset, count)
            if len(block) != count:
                raise ValueError('Truncated Spanish PS2 movie: ' + name)
            output.write(block)
            offset += count
    return target


def install(iso, data, cancel):
    import import_game as worker

    iso, data = Path(iso).absolute(), Path(data).absolute()
    unlinked(iso, root=iso.parent)
    unlinked(data, root=data)
    if not iso.is_file() or not 32 * 2048 <= iso.stat().st_size <= 9 * 1024**3:
        raise ValueError('Select your European PS2 Urban Chaos ISO (SLES-53991), not a ZIP or folder.')
    check = lambda: check_cancel(cancel)
    check()
    if not worker.ready(data):
        raise ValueError('Import your supported Xbox ISO before installing Spanish voices.')
    bundled = worker.spanish_bundle()
    if bundled is None:
        raise ValueError('The included Spanish text is missing. Extract the complete build again.')
    ffmpeg, identities = _tools(worker.ROOT, check)
    iso_identity = _identity(iso)
    # Reserve a complete replacement plus a single streamed movie and safety margin.
    # The previous audio install remains in place until atomic publication.
    if shutil.disk_usage(data).free < OUTPUT_LIMIT + spanish_movies.LIMIT + 256 * BLOCK:
        raise ValueError('At least 8.4 GiB of free space is needed to convert Spanish audio safely.')
    original = data / 'installed/game_files'
    parent = data / 'xbe-runtime/translations'
    translated, destination = parent / 'es-zarf', parent / 'es-voices'
    for path in (original, translated, destination):
        unlinked(path, root=data)
    game_files = worker.manifest()['files']
    text_files = {row['file']: row for row in bundled[1]['files']}
    if set(text_files) != set(FILES) or len(bundled[1]['files']) != len(FILES):
        raise ValueError('The included Spanish text is incomplete.')
    progress('Verifying original audio and Spanish text', 1)
    for name in sorted(OUTPUTS):
        source = 'Movies/' + name[7:] if name in MOVIES else name
        row = game_files.get(source)
        identities[original / source] = _verify(original / source, data, row, check)
        if name in FILES and (text_files[name].get('source_size') != row['size'] or
                              text_files[name].get('source_sha256') != row['sha256']):
            raise ValueError('Spanish text does not match the imported Xbox disc.')
    worker.ensure_spanish(data, cancel=cancel)
    for name, row in text_files.items():
        identities[translated / name] = _verify(translated / name, data, row, check)
    text_sha = sha(bundled[0])
    parent.mkdir(parents=True, exist_ok=True)
    temporary = parent / ('.spanish-voice-iso-' + uuid.uuid4().hex)
    stage, scratch = temporary / 'voices', temporary / 'movies'
    last_budget = 0.0

    def conversion_check():
        nonlocal last_budget
        check()
        now = time.monotonic()
        if now - last_budget >= 1:
            _budget(stage)
            if shutil.disk_usage(data).free < 128 * BLOCK:
                raise ValueError('Free space ran out during Spanish audio conversion.')
            last_budget = now

    try:
        with Ps2Disc(iso) as disc:
            # Fail before expensive conversion if any required movie is absent.
            for name in set(spanish_movies.MOVIES.values()):
                if not 44 <= disc.size(name) <= spanish_movies.LIMIT:
                    raise ValueError('Missing or oversized Spanish PS2 movie: ' + name)
            stage.mkdir(parents=True)
            scratch.mkdir()
            spanish_voices.build(original, translated, disc, stage, cancel=conversion_check,
                                 progress=lambda message, percent: progress(message, 5 + percent * 55 // 100))
            if _budget(stage) != frozenset(FILES) | SAFS:
                raise ValueError('Spanish gameplay audio conversion is incomplete.')
            (stage / 'movies').mkdir()
            for index, (name, ps2_name) in enumerate(sorted(spanish_movies.MOVIES.items())):
                conversion_check()
                for path in identities:
                    if path.is_relative_to(worker.ROOT / 'tools') and _identity(path) != identities[path]:
                        raise ValueError('The bundled conversion tools changed during installation.')
                movie = _movie_source(disc, ps2_name, scratch, check)
                try:
                    spanish_movies.convert(original / 'Movies' / name, movie, ffmpeg,
                                           stage / 'movies' / name, cancel=conversion_check)
                finally:
                    movie.unlink(missing_ok=True)
                _budget(stage)
                progress('Converting Spanish cutscenes', 60 + (index + 1) * 35 // len(MOVIES))
            if _budget(stage) != OUTPUTS:
                raise ValueError('Spanish audio conversion is incomplete; all dialogue and cutscenes are required.')
            unlinked(iso, root=iso.parent)
            if _identity(iso) != iso_identity:
                raise ValueError('The PS2 ISO changed during Spanish audio conversion.')
            for path, identity in identities.items():
                unlinked(path, root=worker.ROOT if path.is_relative_to(worker.ROOT) else data)
                if _identity(path) != identity:
                    raise ValueError('A conversion source changed. Close the game and try again: ' + path.name)
            check()
            files = setup.write_manifest(stage)
            if set(files) != OUTPUTS:
                raise ValueError('Spanish audio output verification is incomplete.')
            (stage / 'install.json').write_text(json.dumps({
                'version': 1, 'text_sha256': text_sha, 'files': files,
            }, indent=2) + '\n', encoding='utf-8')
            check()
            setup.publish(stage, destination, cancel)
        progress('Spanish voices installed. Select Spanish and restart the game.', 100)
    finally:
        if temporary.exists():
            unlinked(temporary, root=data)
            if temporary.resolve().parent != parent.resolve() or not temporary.name.startswith('.spanish-voice-iso-'):
                raise ValueError('Invalid Spanish audio staging directory.')
            shutil.rmtree(temporary)
