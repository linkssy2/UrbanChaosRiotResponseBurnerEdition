"""Build Spanish level text from the user's ISO; the bundle contains no archives."""
from pathlib import Path
import hashlib
import json
import re
import struct
import zlib
from shader_seed import unlinked

FILES = tuple(sorted(f'{n}-01.ent' for n in
                     (1, 2, 3, 4, 5, 6, 8, 9, 11, 12, 13, 21, 22, 23, 24, 25,
                      26, 40, 41, 42, 43, 44, 45, 46, 47, 48, 99)))
ARCHIVE_LIMIT = 64 * 1024 * 1024
TEXT_LIMIT = 16 * 1024 * 1024


def _unique(items):
    result = {}
    for key, value in items:
        if key in result:
            raise ValueError('Duplicate Spanish bundle field.')
        result[key] = value
    return result


def load_bundle(path):
    path = Path(path).absolute()
    unlinked(path)
    if not path.is_file() or not 0 < path.stat().st_size <= 8 * 1024 * 1024:
        raise ValueError('The included Spanish text bundle is missing or damaged.')
    bundle = json.loads(path.read_text(encoding='utf-8'), object_pairs_hook=_unique)
    if (not isinstance(bundle, dict) or set(bundle) != {'format', 'files'} or
            type(bundle['format']) is not int or bundle['format'] != 1 or
            not isinstance(bundle['files'], list) or len(bundle['files']) != len(FILES)):
        raise ValueError('Unsupported Spanish text bundle.')
    names = []
    for item in bundle['files']:
        if not isinstance(item, dict) or set(item) != {
                'file', 'source_size', 'source_sha256', 'size', 'sha256', 'changes'}:
            raise ValueError('Invalid Spanish file record.')
        if item['file'] not in FILES:
            raise ValueError('Invalid Spanish archive name.')
        names.append(item['file'])
        for key in ('source_size', 'size'):
            if type(item[key]) is not int or not 0xC8 < item[key] <= ARCHIVE_LIMIT:
                raise ValueError('Invalid Spanish archive size.')
        for key in ('source_sha256', 'sha256'):
            if not isinstance(item[key], str) or not re.fullmatch('[0-9a-f]{64}', item[key]):
                raise ValueError('Invalid Spanish archive identity.')
        if not isinstance(item['changes'], list) or len(item['changes']) > 20000:
            raise ValueError('Invalid Spanish string list.')
        previous = -1
        for change in item['changes']:
            if (not isinstance(change, list) or len(change) != 2 or
                    type(change[0]) is not int or not previous < change[0] < TEXT_LIMIT or
                    not isinstance(change[1], str) or '\0' in change[1] or
                    len(change[1]) > 65535):
                raise ValueError('Invalid Spanish string replacement.')
            # Latin-1 preserves the game's bytecode values in the 0x80-0x9f range.
            change[1].encode('latin1')
            previous = change[0]
    if tuple(names) != FILES:
        raise ValueError('The Spanish bundle must contain each level exactly once.')
    return bundle


def _entry(data, name):
    if len(data) < 0xC8:
        raise ValueError('Invalid original level archive.')
    directory_size, count = struct.unpack_from('<II', data, 0xC0)
    end = 0xC4 + directory_size
    if not 0xC8 <= end <= len(data) or not 0 < count <= 100000:
        raise ValueError('Invalid level directory bounds.')
    cursor = 0xC8
    found = None
    for _ in range(count):
        if cursor + 2 > end:
            raise ValueError('Truncated level directory.')
        length = struct.unpack_from('<H', data, cursor)[0]
        cursor += 2
        if not length or cursor + length + 16 > end:
            raise ValueError('Invalid level entry bounds.')
        entry_name = data[cursor:cursor + length].rstrip(b'\0').decode('latin1')
        cursor += length
        offset, size, packed = struct.unpack_from('<III', data, cursor)
        if packed and (offset < end or offset + packed > len(data)):
            raise ValueError('Level entry extends outside archive.')
        if entry_name == name:
            if found is not None or not 0 < size <= TEXT_LIMIT or not packed:
                raise ValueError('Invalid original text entry.')
            decoder = zlib.decompressobj()
            text = decoder.decompress(data[offset:offset + packed], size + 1)
            if (len(text) != size or not decoder.eof or decoder.unused_data or
                    decoder.unconsumed_tail):
                raise ValueError('Invalid original text stream.')
            found = (cursor, text)
        cursor += 16
    if cursor != end or found is None:
        raise ValueError('Missing original level text.')
    return found


def _translate(original, changes):
    if len(original) < 24:
        raise ValueError('Truncated original text bank.')
    header = list(struct.unpack_from('<HH5I', original))
    languages, version, keys, lookups, texts, audio, count = header
    width = keys + lookups + texts + audio
    if (not 0 < languages <= 16 or version != 32 or not count or
            keys != count * 56 or lookups % 12 or not width or
            len(original) != 24 + languages * width):
        raise ValueError('Unsupported original text bank.')
    bank = original[24:24 + width]
    table = bytearray(bank[:keys + lookups])
    replacements = {at: value.encode('latin1') for at, value in changes}
    used, names = set(), set()
    text = bytearray()
    for index in range(count):
        record = bank[index * 56:(index + 1) * 56]
        key = record[:40].split(b'\0')[0]
        variants = record[49]
        start = struct.unpack_from('<I', record, 52)[0]
        if not key or key in names or start % 12 or start + variants * 12 > lookups:
            raise ValueError('Invalid original text key or variant range.')
        names.add(key)
        for variant in range(variants):
            at = keys + start + variant * 12
            offset = struct.unpack_from('<I', bank, at)[0]
            if offset >= texts:
                raise ValueError('Original text offset outside bank.')
            begin = keys + lookups + offset
            end = bank.find(b'\0', begin, keys + lookups + texts)
            if end < 0:
                raise ValueError('Unterminated original text.')
            value = replacements.get(at, bank[begin:end])
            if at in replacements:
                used.add(at)
            struct.pack_into('<I', table, at, len(text))
            text.extend(value + b'\0')
            if len(text) > TEXT_LIMIT:
                raise ValueError('Spanish text exceeds the bank limit.')
    if used != replacements.keys():
        raise ValueError('Spanish replacement does not match an original text slot.')
    new_width = max(texts, len(text))
    header[4] = new_width
    result = bytearray(struct.pack('<HH5I', *header))
    result.extend(table + text + bytes(new_width - len(text)) + bank[keys + lookups + texts:])
    # Every audio block and non-English language is retained from the user's ISO.
    for index in range(1, languages):
        bank = original[24 + index * width:24 + (index + 1) * width]
        result.extend(bank[:keys + lookups + texts] + bytes(new_width - texts) +
                      bank[keys + lookups + texts:])
    return bytes(result)


def build(original_game_directory, bundle_path, output_directory, cancel=None):
    """Create a fresh staging folder; the caller owns atomic installation/rollback."""
    bundle = load_bundle(bundle_path)
    original_game_directory = Path(original_game_directory).absolute()
    output_directory = Path(output_directory).absolute()
    unlinked(original_game_directory)
    unlinked(output_directory)
    output_directory.mkdir(parents=True, exist_ok=False)
    result = []
    for item in bundle['files']:
        if cancel:
            cancel()
        path = original_game_directory / item['file']
        unlinked(path)
        if not path.is_file() or path.stat().st_size != item['source_size']:
            raise ValueError('Spanish requires the supported, unmodified Xbox ISO files.')
        original = path.read_bytes()
        if hashlib.sha256(original).hexdigest() != item['source_sha256']:
            raise ValueError('Spanish requires the supported, unmodified Xbox ISO files.')
        cursor, bank = _entry(original, Path(item['file']).stem + '.exttxt')
        translated = _translate(bank, item['changes'])
        packed = zlib.compress(translated, 9)
        archive = bytearray(original)
        struct.pack_into('<III', archive, cursor, len(original), len(translated), len(packed))
        archive.extend(packed)
        if len(archive) != item['size'] or hashlib.sha256(archive).hexdigest() != item['sha256']:
            raise ValueError('Spanish text verification failed; no installed files were replaced.')
        if cancel:
            cancel()
        (output_directory / item['file']).write_bytes(archive)
        result.append({key: item[key] for key in ('file', 'size', 'sha256')})
    (output_directory / 'manifest.txt').write_text(
        ''.join(f"{item['file']} {item['size']}\n" for item in result), encoding='ascii')
    return result
