"""Text-only Spanish reconstruction, corruption rejection, and source preservation."""
from pathlib import Path
import copy
import hashlib
import json
import struct
import sys
import tempfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
import spanish_text as spanish


def digest(data):
    return hashlib.sha256(data).hexdigest()


def text_bank():
    key = bytearray(56)
    key[:5] = b'hello'
    key[49] = 2
    lookup = struct.pack('<6I', 0, 17, 23, 6, 29, 31)
    text = b'Hello\0Keep\0'
    audio = b'VOICE'
    bank = bytes(key) + lookup + text + audio
    return struct.pack('<HH5I', 4, 32, 56, 24, len(text), len(audio), 1) + bank * 4


def expected_bank():
    original = text_bank()
    translated = b'Hola, se\xf1or\x81\0Keep\0'
    width = len(translated)
    old_bank = original[24:]
    table = bytearray(old_bank[:80])
    struct.pack_into('<I', table, 68, translated.index(b'Keep'))
    english = original[24:24 + 56 + 24 + 11 + 5]
    return (struct.pack('<HH5I', 4, 32, 56, 24, width, 5, 1) +
            table + translated + b'VOICE' +
            (english[:-5] + bytes(width - 11) + b'VOICE') * 3)


def archive(name):
    text = zlib.compress(text_bank())
    untouched = zlib.compress(b'unrelated texture bytes')
    names = [name.replace('.ent', '.exttxt').encode('ascii') + b'\0', b'unchanged.dds\0']
    end = 0xC8 + sum(2 + len(n) + 16 for n in names)
    result = bytearray(0xC8)
    struct.pack_into('<II', result, 0xC0, end - 0xC4, 2)
    cursor = None
    for n, offset, size, packed in (
            (names[0], end, len(text_bank()), len(text)),
            (names[1], end + len(text), 23, len(untouched))):
        result.extend(struct.pack('<H', len(n)) + n)
        if cursor is None:
            cursor = len(result)
        result.extend(struct.pack('<4I', offset, size, packed, 0))
    result.extend(text + untouched)
    expected = bytearray(result)
    packed = zlib.compress(expected_bank(), 9)
    struct.pack_into('<III', expected, cursor, len(result), len(expected_bank()), len(packed))
    expected.extend(packed)
    return bytes(result), bytes(expected)


def fails(operation):
    try:
        operation()
    except (ValueError, OSError, UnicodeError):
        return
    raise AssertionError('Damaged input was accepted')


with tempfile.TemporaryDirectory(prefix='spanish-text-test-') as directory:
    root = Path(directory)
    source = root / 'original'
    source.mkdir()
    records, targets, originals = [], {}, {}
    for name in spanish.FILES:
        data, target = archive(name)
        (source / name).write_bytes(data)
        originals[name] = data
        targets[name] = target
        records.append({'file': name, 'source_size': len(data), 'source_sha256': digest(data),
                        'size': len(target), 'sha256': digest(target),
                        'changes': [[56, 'Hola, se\u00f1or\u0081']]})
    bundle = {'format': 1, 'files': records}
    path = root / 'text.json'
    def write(value):
        path.write_text(json.dumps(value), encoding='utf-8')
    write(bundle)
    output = root / 'translated'
    generated = spanish.build(source, path, output)
    assert len(generated) == 27
    for item in generated:
        assert (output / item['file']).read_bytes() == targets[item['file']]
        assert (source / item['file']).read_bytes() == originals[item['file']]
    assert (output / 'manifest.txt').read_text() == ''.join(
        f"{r['file']} {r['size']}\n" for r in records)
    fails(lambda: spanish.build(source, path, output))
    assert (output / records[0]['file']).read_bytes() == targets[records[0]['file']]

    mutations = [
        lambda b: b.update(format=True),
        lambda b: b['files'].pop(),
        lambda b: b['files'][0].update(file='../outside.ent'),
        lambda b: b['files'][0].update(file=b['files'][1]['file']),
        lambda b: b['files'][0].update(source_size=spanish.ARCHIVE_LIMIT + 1),
        lambda b: b['files'][0].update(sha256='invalid'),
        lambda b: b['files'][0].update(changes=[[56, 'broken\0text']]),
        lambda b: b['files'][0].update(changes=[[56, '\u1000']]),
        lambda b: b['files'][0].update(changes=[[True, 'invalid']]),
        lambda b: b['files'][0].update(changes=[[56, 'a'], [56, 'b']]),
        lambda b: b['files'][0].update(changes=[[spanish.TEXT_LIMIT, 'a']]),
    ]
    for mutate in mutations:
        value = copy.deepcopy(bundle)
        mutate(value)
        write(value)
        fails(lambda: spanish.load_bundle(path))
    path.write_text('{"format":1,"format":1,"files":[]}', encoding='utf-8')
    fails(lambda: spanish.load_bundle(path))

    for index, field in enumerate(('source_sha256', 'sha256')):
        value = copy.deepcopy(bundle)
        value['files'][0][field] = '0' * 64
        write(value)
        fails(lambda: spanish.build(source, path, root / f'bad-hash-{index}'))
    value = copy.deepcopy(bundle)
    value['files'][0]['changes'] = [[57, 'wrong lookup']]
    write(value)
    fails(lambda: spanish.build(source, path, root / 'bad-slot'))
    write(bundle)
    def cancel():
        raise ValueError('Cancelled')
    fails(lambda: spanish.build(source, path, root / 'cancelled', cancel))
    assert not list((root / 'cancelled').iterdir())
    for name, data in originals.items():
        assert (source / name).read_bytes() == data
    fails(lambda: spanish._entry(b'bad', 'x.exttxt'))
    damaged = bytearray(text_bank())
    struct.pack_into('<I', damaged, 24 + 52, 0xFFFFFFFC)
    fails(lambda: spanish._translate(bytes(damaged), []))

print('Spanish bundle: 27 reconstructed archives, untouched voices/languages/resources, '
      'source preservation, existing output protection, cancellation and corruption checks passed')
