"""Convert locally supplied PAL PS2 speech into Spanish-only Xbox assets.

The original sound programs, GUIDs, playback engine, subtitles and other audio
banks stay intact. Only clips matched by an identical named sound program and
reference slot are replaced. Unmatched clips retain the original recording.
"""
from pathlib import Path
import hashlib
import struct
import zlib

from ps2_audio import decode_adpcm, trim_adpcm
from shader_seed import unlinked
from spanish_text import ARCHIVE_LIMIT, FILES
from xbox_adpcm import encode

SPAN_LIMIT = 64 * 1024 * 1024
ZERO_GUID = bytes(16)


def _archive(data):
    if not 0xC8 <= len(data) <= ARCHIVE_LIMIT:
        raise ValueError('Invalid speech archive size.')
    directory, count = struct.unpack_from('<II', data, 0xC0)
    end, at, entries = 0xC4 + directory, 0xC8, []
    if not 0xC8 <= end <= len(data) or not 0 < count <= 100000:
        raise ValueError('Invalid speech archive directory.')
    for _ in range(count):
        if at + 2 > end:
            raise ValueError('Truncated speech archive directory.')
        length, = struct.unpack_from('<H', data, at)
        at += 2
        if not length or at + length + 16 > end:
            raise ValueError('Invalid speech archive entry.')
        name = data[at:at + length].rstrip(b'\0').decode('latin1').lower()
        at += length
        offset, size, packed, bank = struct.unpack_from('<IIII', data, at)
        if packed and (offset < end or offset + packed > len(data)):
            raise ValueError('Speech archive entry exceeds its file.')
        entries.append((name, bank, at, offset, size, packed))
        at += 16
    if at != end:
        raise ValueError('Unexpected speech archive directory tail.')
    return struct.unpack_from('<H', data, 0x16)[0], entries


def _unpack(data, entry):
    _, _, _, offset, size, packed = entry
    if not packed or not 0 < size <= SPAN_LIMIT:
        raise ValueError('Unsupported external speech archive entry.')
    decoder = zlib.decompressobj()
    value = decoder.decompress(data[offset:offset + packed], size + 1)
    if (len(value) != size or not decoder.eof or decoder.unused_data or
            decoder.unconsumed_tail):
        raise ValueError('Invalid compressed speech archive entry.')
    return value


def _riff(data):
    if len(data) < 12 or data[:4] != b'RIFF':
        raise ValueError('Missing speech RIFF header.')
    size = struct.unpack_from('<I', data, 4)[0] + 8
    if not 12 <= size <= len(data):
        raise ValueError('Invalid speech RIFF bounds.')
    at, chunks = 12, []
    while at < size:
        if at + 8 > size:
            raise ValueError('Truncated speech RIFF chunk.')
        tag, length = struct.unpack_from('<4sI', data, at)
        stop = at + 8 + length
        if stop + (length & 1) > size:
            raise ValueError('Speech RIFF chunk exceeds its resource.')
        chunks.append((tag, data[at + 8:stop]))
        at = stop + (length & 1)
    return data[8:12], chunks, size


def _one(chunks, tag):
    matches = [value for kind, value in chunks if kind == tag]
    if len(matches) != 1:
        raise ValueError('Missing or duplicate speech resource field.')
    return matches[0]


def _name(chunks):
    return _one(chunks, b'name').split(b'\0')[0].lower()


def _references(chunks):
    if not any(tag == b'amrf' for tag, _ in chunks):
        return []
    data = _one(chunks, b'amrf')
    if len(data) < 4:
        raise ValueError('Truncated sound program references.')
    count, = struct.unpack_from('<I', data)
    if len(data) != 4 + count * 20 or count > 20000:
        raise ValueError('Invalid sound program reference count.')
    return [(data[at:at + 16], data[at + 16:at + 20])
            for at in range(4, len(data), 20)]


def _program_shape(chunks):
    # Names contain irrelevant uninitialized padding after the terminating NUL.
    return [(tag, value) for tag, value in chunks if tag not in (b'name', b'amrf')]


def _ps2_bank(data):
    """Read the aligned RIFF prefix; the following APE relocation index is unused."""
    at, waves, programs = 0, {}, {}
    while data[at:at + 4] == b'RIFF':
        form, chunks, length = _riff(data[at:])
        if form == b'WAVS':
            guid = _one(chunks, b'guid')
            fmt, stream = _one(chunks, b'fmt '), _one(chunks, b'strm')
            if len(guid) != 16 or len(fmt) != 20 or len(stream) != 8 or guid in waves:
                raise ValueError('Invalid or duplicate PS2 speech descriptor.')
            tag, channels, rate, _, align, bits, extra, samples = struct.unpack('<HHIIHHHH', fmt)
            if (tag, channels, align, bits, extra, samples) != (0x600, 1, 16, 4, 2, 28):
                raise ValueError('Unsupported PS2 dialogue encoding.')
            if not 4000 <= rate <= 48000:
                raise ValueError('Invalid PS2 dialogue sample rate.')
            offset, size = struct.unpack('<II', stream)
            if not 0 < size <= SPAN_LIMIT or size % 16:
                raise ValueError('Invalid PS2 dialogue stream size.')
            waves[guid] = (offset, size, rate)
        elif form == b'AMPC':
            name = _name(chunks)
            if not name or name in programs:
                raise ValueError('Duplicate PS2 dialogue program.')
            _references(chunks)
            programs[name] = chunks
        else:
            raise ValueError('Unexpected PS2 dialogue resource.')
        stop = (at + length + 15) & ~15
        if any(data[at + length:stop]):
            raise ValueError('Invalid PS2 sound resource alignment.')
        at = stop
    if not waves or not programs or at >= len(data):
        raise ValueError('Missing PS2 dialogue bank or relocation index.')
    return waves, programs


def _mapping(xbox, entries, ps2_programs, ps2_waves):
    result, conflicts, unmatched = {}, set(), []
    for entry in entries:
        if entry[1] != 1 or not entry[0].endswith('.amp'):
            continue
        form, chunks, _ = _riff(_unpack(xbox, entry))
        if form != b'AMPC':
            raise ValueError('Unsupported Xbox dialogue program.')
        name = _name(chunks)
        other = ps2_programs.get(name)
        old = _references(chunks)
        if (other is None or _program_shape(chunks) != _program_shape(other) or
                len(old) != len(_references(other))):
            unmatched.append(name.decode('latin1'))
            continue
        for (guid, flags), (ps2_guid, ps2_flags) in zip(old, _references(other)):
            if guid == ZERO_GUID:
                continue
            if flags != ps2_flags or ps2_guid not in ps2_waves:
                conflicts.add(guid)
                continue
            value = ps2_waves[ps2_guid]
            if guid in result and result[guid] != value:
                conflicts.add(guid)
            result[guid] = value
    for guid in conflicts:
        result.pop(guid, None)
    return result, unmatched, len(conflicts)


def _serialize(form, chunks):
    body = bytearray(form)
    for tag, value in chunks:
        body.extend(tag + struct.pack('<I', len(value)) + value + bytes(len(value) & 1))
    return b'RIFF' + struct.pack('<I', len(body)) + body


def _converted(chunks, pcm, rate, offset, cancel=None):
    encoded = encode(pcm, 1, cancel=cancel)
    if not encoded:
        raise ValueError('Empty Spanish speech clip.')
    fmt = struct.pack('<HHIIHHHH', 0x69, 1, rate, rate * 36 // 64, 36, 4, 2, 64)
    fact = struct.pack('<I', len(pcm) // 2)
    base = [(tag, fmt if tag == b'fmt ' else fact if tag == b'fact' else
             encoded[:min(len(value), len(encoded))] if tag == b'pdat' else value)
            for tag, value in chunks if tag not in (b'strm', b'rsaf')]
    header_bytes = len(_serialize(b'WAVS', base))
    data_offset = (header_bytes + 16 + 2047) & ~2047
    padding = data_offset - header_bytes - 16
    span = _serialize(b'WAVS', base + [(b'pad ', bytes(padding)), (b'data', encoded)])
    span += bytes(-len(span) % 2048)
    descriptor = list(base)
    descriptor.extend([(b'strm', struct.pack('<II', data_offset, len(encoded))),
                       (b'rsaf', struct.pack('<II', offset, len(span)))])
    return _serialize(b'WAVS', descriptor), span


def _sha(path, cancel=None):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        while part := stream.read(1024 * 1024):
            if cancel:
                cancel()
            digest.update(part)
    return digest.hexdigest()


def build(xbox_dir, translated_dir, disc, output_dir, cancel=None, progress=None):
    """Write local ENT/SAF files into an existing empty staging directory.

    ``disc`` is an open ps2_audio.Ps2Disc. The caller validates supported Xbox
    source identities and atomically installs this directory with its manifest.
    Memory is bounded by one level archive and one sound clip, never a whole SAF.
    """
    xbox_dir, translated_dir, output_dir = (
        Path(path).absolute() for path in (xbox_dir, translated_dir, output_dir))
    for path in (xbox_dir, translated_dir, output_dir):
        unlinked(path, root=path)
    if not output_dir.is_dir() or any(output_dir.iterdir()):
        raise ValueError('Spanish voice staging folder must be empty.')
    report = {'format': 1, 'levels': [], 'files': [], 'saf_recipes': {}, 'converted_unique_clips': 0,
              'copied_stream_bytes': 0, 'converted_stream_bytes': 0}
    backing, outputs, copied, converted = {}, {}, {}, {}
    try:
        for level_index, name in enumerate(FILES):
            if cancel:
                cancel()
            if progress:
                progress('Converting Spanish dialogue: ' + name,
                         level_index * 100 // len(FILES))
            source, translated = xbox_dir / name, translated_dir / name
            unlinked(source, root=xbox_dir)
            unlinked(translated, root=translated_dir)
            if max(source.stat().st_size, translated.stat().st_size) > ARCHIVE_LIMIT:
                raise ValueError('Speech level archive exceeds the size limit.')
            original = source.read_bytes()
            stream_id, entries = _archive(original)
            target = bytearray(translated.read_bytes())
            target_id, target_entries = _archive(target)
            if target_id != stream_id or [e[:3] for e in entries] != [e[:3] for e in target_entries]:
                raise ValueError('Spanish text archive does not match the Xbox level.')
            ps2 = disc.read(name)
            ps2_id, ps2_entries = _archive(ps2)
            banks = [e for e in ps2_entries if e[0] == 'data5.ape']
            if len(banks) != 1:
                raise ValueError('Missing Spanish PS2 dialogue bank: ' + name)
            waves, programs = _ps2_bank(_unpack(ps2, banks[0]))
            mapping, unmatched, conflicts = _mapping(original, entries, programs, waves)
            level = {'file': name, 'matched_clips': 0, 'fallback_clips': 0,
                     'unmatched_programs': unmatched, 'unresolved_program_references': conflicts,
                     'fallback_names': []}
            if stream_id not in outputs:
                saf = xbox_dir / f'{stream_id}.saf'
                unlinked(saf, root=xbox_dir)
                backing[stream_id] = saf.open('rb')
                outputs[stream_id] = (output_dir / f'{stream_id}.saf').open('xb')
                report['saf_recipes'][f'{stream_id}.saf'] = []
            src, dst = backing[stream_id], outputs[stream_id]
            ps2_saf = f'{ps2_id}.saf'
            for entry in entries:
                if not entry[0].endswith('.wav'):
                    continue
                form, chunks, _ = _riff(_unpack(original, entry))
                if form != b'WAVS':
                    continue
                guid = _one(chunks, b'guid')
                old_offset, old_size = struct.unpack('<II', _one(chunks, b'rsaf'))
                if not 0 < old_size <= SPAN_LIMIT:
                    raise ValueError('Invalid original audio stream length.')
                replacement = mapping.get(guid) if entry[1] == 1 else None
                key = (stream_id, old_offset, old_size)
                if replacement is not None:
                    ps2_offset, ps2_size, rate = replacement
                    conversion_key = key + (ps2_saf, *replacement)
                    if conversion_key not in converted:
                        if cancel:
                            cancel()
                        encoded = trim_adpcm(disc.read(ps2_saf, ps2_offset, ps2_size))
                        pcm = decode_adpcm(encoded, cancel=cancel)
                        descriptor, span = _converted(chunks, pcm, rate, dst.tell(), cancel)
                        report['saf_recipes'][f'{stream_id}.saf'].append(
                            {'data': [dst.tell(), len(span)]})
                        dst.write(span)
                        converted[conversion_key] = descriptor
                        report['converted_unique_clips'] += 1
                        report['converted_stream_bytes'] += len(span)
                    descriptor = converted[conversion_key]
                    level['matched_clips'] += 1
                else:
                    if entry[1] == 1:
                        level['fallback_clips'] += 1
                        level['fallback_names'].append(entry[0])
                    if key not in copied:
                        if cancel:
                            cancel()
                        src.seek(0, 2)
                        if old_offset + old_size > src.tell():
                            raise ValueError('Original audio stream exceeds its SAF.')
                        src.seek(old_offset)
                        header = src.read(min(old_size, 2048))
                        if header[:4] != b'RIFF' or header[8:12] != b'WAVS' or header[12:20] != b'guid\x10\0\0\0' or header[20:36] != guid:
                            raise ValueError('Original audio descriptor/stream identity mismatch.')
                        src.seek(old_offset)
                        copied[key] = dst.tell()
                        report['saf_recipes'][f'{stream_id}.saf'].append(
                            {'copy': [f'{stream_id}.saf', old_offset, old_size]})
                        remaining = old_size
                        while remaining:
                            if cancel:
                                cancel()
                            data = src.read(min(remaining, 1024 * 1024))
                            if not data:
                                raise ValueError('Truncated original audio stream.')
                            dst.write(data)
                            remaining -= len(data)
                        report['copied_stream_bytes'] += old_size
                    descriptor = _serialize(form, [(tag, struct.pack('<II', copied[key], old_size)
                                                   if tag == b'rsaf' else value)
                                                  for tag, value in chunks])
                packed = zlib.compress(descriptor, 9)
                struct.pack_into('<III', target, entry[2], len(target), len(descriptor), len(packed))
                target.extend(packed)
            (output_dir / name).write_bytes(target)
            report['levels'].append(level)
        if not report['converted_unique_clips']:
            raise ValueError('The supplied disc contains no matching Spanish dialogue.')
    finally:
        for handle in (*backing.values(), *outputs.values()):
            handle.close()
    for path in sorted(output_dir.iterdir()):
        if cancel:
            cancel()
        report['files'].append({'file': path.name, 'size': path.stat().st_size,
                                'sha256': _sha(path, cancel)})
    if progress:
        progress('Spanish dialogue conversion complete.', 100)
    return report
