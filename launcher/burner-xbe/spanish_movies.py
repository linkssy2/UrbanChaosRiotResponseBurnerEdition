"""Replace only English audio spans in user-owned XMV copies with PS2 Spanish.

The caller verifies the two disc identities and atomically installs its staging
directory. No video, packet headers, timing, other tracks or originals change.
"""
from pathlib import Path
import audioop
import hashlib
import os
import struct
import subprocess
import tempfile
import time

import xbox_adpcm

_STEMS = tuple('intro_' + str(number) for number in
               (1, 2, 3, 4, 5, 6, 8, 9, 11, 12, 13, 21, 22, 23, 24, 25, 26)) + (
                   'intro_12_edit', 'outro_13')
MOVIES = {stem + '.xmv': 'pal_' + stem + '.bik' for stem in _STEMS}
MOVIES.update({'pal_' + stem + '.xmv': 'pal_' + stem + '.bik'
               for stem in ('intro_1', 'intro_2', 'intro_3', 'intro_4', 'intro_5',
                            'intro_6', 'intro_8', 'intro_9', 'intro_11', 'intro_12',
                            'intro_13', 'outro_13')})
LIMIT = 128 * 1024 * 1024
_FIT_ENDING = {'pal_intro_24.bik', 'pal_intro_26.bik'}
_PCM_LIMIT = 44100 * 601 * 2
_ERROR_LIMIT = 1024 * 1024
_DECODE_TIMEOUT = 120


def _decode_audio(command, cancel):
    # Disk-backed output avoids pipe deadlocks and unbounded captured stderr.
    with tempfile.TemporaryFile() as output, tempfile.TemporaryFile() as errors:
        process = subprocess.Popen(command, stdin=subprocess.DEVNULL,
            stdout=output, stderr=errors,
            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        started = time.monotonic()
        try:
            while True:
                if cancel:
                    cancel()
                status = process.poll()
                if (os.fstat(output.fileno()).st_size > _PCM_LIMIT or
                        os.fstat(errors.fileno()).st_size > _ERROR_LIMIT):
                    raise ValueError('PS2 movie decoder output exceeds its size limit.')
                if status is not None:
                    if status:
                        raise subprocess.CalledProcessError(status, command)
                    break
                if time.monotonic() - started >= _DECODE_TIMEOUT:
                    raise subprocess.TimeoutExpired(command, _DECODE_TIMEOUT)
                time.sleep(.05)
            output.seek(0)
            return output.read(_PCM_LIMIT + 1)
        finally:
            if process.poll() is None:
                process.kill()
            process.wait()


def _tempo(name, seconds, frames):
    if name.lower() not in _FIT_ENDING:
        return 1.0
    # These PS2 Spanish lines continue into a tail absent from both Xbox movies.
    factor = seconds / (frames / 44100)
    if seconds != 29.84 or not 1.02 < factor < 1.04:
        raise ValueError('Unsupported Spanish bonus-movie ending timeline.')
    return factor


def xmv_audio(data):
    """Return track 1 format and exact block-aligned spans in the original file."""
    if len(data) < 36 or len(data) > LIMIT or data[12:16] != b'xobX':
        raise ValueError('Invalid Xbox movie header.')
    version, width, height, duration, count, _ = struct.unpack_from('<IIIIHH', data, 16)
    if (version != 4 or width != 640 or height != 480 or
            not 0 < duration <= 600000 or count not in (2, 5)):
        raise ValueError('Unsupported Xbox dialogue movie format.')
    header = 36 + 12 * count
    if len(data) < header:
        raise ValueError('Truncated Xbox movie tracks.')
    codec, channels, rate, bits, flags = struct.unpack_from('<HHIHH', data, 48)
    if codec != 0x69 or channels not in (1, 2) or rate != 44100 or bits != 4 or flags:
        raise ValueError('Unsupported Xbox movie voice format.')
    offset = header
    size = struct.unpack_from('<I', data, 4)[0] - header
    spans = []
    while size:
        if size < 12 + 4 * count or offset + size > len(data):
            raise ValueError('Invalid Xbox movie packet extent.')
        next_size, video, _ = struct.unpack_from('<III', data, offset)
        video = (video & 0x7fffff) - 4 * count
        if video < 0:
            raise ValueError('Invalid Xbox movie video span.')
        cursor = offset + 12 + 4 * count + video
        previous = 0
        for index in range(count):
            amount = struct.unpack_from('<I', data, offset + 12 + index * 4)[0] & 0x7fffff
            if not amount and index:
                amount = previous
            if cursor + amount > offset + size:
                raise ValueError('Xbox movie audio extends outside its packet.')
            if index == 1:
                if amount % (36 * channels):
                    raise ValueError('Xbox movie voice packet splits an ADPCM block.')
                spans.append((cursor, amount))
            cursor += amount
            previous = amount
        offset += size
        size = next_size
    if not spans or not sum(size for _, size in spans):
        raise ValueError('Xbox movie has no voice audio.')
    return channels, rate, spans


def replace_audio(original, encoded):
    channels, _, spans = xmv_audio(original)
    if len(encoded) != sum(size for _, size in spans) or len(encoded) % (36 * channels):
        raise ValueError('Spanish voice length differs from original movie capacity.')
    result = bytearray(original)
    cursor = 0
    for offset, size in spans:
        result[offset:offset + size] = encoded[cursor:cursor + size]
        cursor += size
    return bytes(result)


def decode_spanish(source, ffmpeg, frames, channels, cancel=None):
    """Decode Bink track ID 5, retaining its time origin and checking cropped tails."""
    source = Path(source)
    if not source.is_file() or not 44 <= source.stat().st_size <= LIMIT:
        raise ValueError('Missing or oversized PS2 movie.')
    with source.open('rb') as stream:
        header = stream.read(116)
    if header[:4] != b'BIKi' or struct.unpack_from('<I', header, 4)[0] + 8 != source.stat().st_size:
        raise ValueError('Unsupported PS2 Bink movie.')
    video_frames = struct.unpack_from('<I', header, 8)[0]
    width, height, numerator, denominator, _, tracks = struct.unpack_from('<6I', header, 20)
    if (width != 640 or height != 448 or not numerator or not denominator or
            not 0 < video_frames * denominator / numerator <= 600 or tracks != 6 or
            len(header) < 116 or set(struct.unpack_from('<6I', header, 92)) != set(range(6)) or
            channels not in (1, 2) or not 0 < frames <= 44100 * 600):
        raise ValueError('Invalid PS2 dialogue movie streams.')
    ids = struct.unpack_from('<6I', header, 92)
    rate, flags = struct.unpack_from('<HH', header, 68 + ids.index(5) * 4)
    if rate != 48000 or flags != 0x5000:
        raise ValueError('Unsupported PS2 Spanish movie voice format.')
    tempo = _tempo(source.name, video_frames * denominator / numerator, frames)
    filters = ['-af', 'atempo=' + repr(tempo)] if tempo != 1 else []
    if cancel:
        cancel()
    try:
        decoded = _decode_audio([str(ffmpeg), '-nostdin', '-v', 'error', '-i', str(source),
                                 '-map', '0:i:5', '-vn', '-ac', '1', '-ar', '44100', *filters,
                                 '-t', '601', '-fs', str(_PCM_LIMIT + 2),
                                 '-f', 's16le', 'pipe:1'], cancel)
    except (OSError, subprocess.SubprocessError) as error:
        raise ValueError('Could not decode the PS2 Spanish movie with FFmpeg.') from error
    if cancel:
        cancel()
    if (not decoded or len(decoded) % 2 or len(decoded) > _PCM_LIMIT or
            len(decoded) < (frames - 44100) * 2):
        raise ValueError('Unexpected PS2 Spanish movie audio duration.')
    tail = decoded[frames * 2:]
    if tail and audioop.max(tail, 2) > 16:
        raise ValueError('The PS2 Spanish movie has audible dialogue beyond the Xbox ending.')
    decoded = decoded[:frames * 2].ljust(frames * 2, b'\0')
    return decoded if channels == 1 else audioop.tostereo(decoded, 2, 1, 1)


def convert(original_path, ps2_path, ffmpeg, output_path, cancel=None):
    original_path, ps2_path, output_path = map(Path, (original_path, ps2_path, output_path))
    if MOVIES.get(original_path.name.lower()) != ps2_path.name.lower():
        raise ValueError('The PS2 movie does not match the original Xbox movie.')
    if (output_path.resolve() in (original_path.resolve(), ps2_path.resolve()) or
            output_path.exists() or not original_path.is_file() or original_path.stat().st_size > LIMIT):
        raise ValueError('Movie conversion needs a new output and intact source files.')
    if cancel:
        cancel()
    original = original_path.read_bytes()
    channels, rate, spans = xmv_audio(original)
    frames = sum(size for _, size in spans) // (36 * channels) * 64
    pcm = decode_spanish(ps2_path, ffmpeg, frames, channels, cancel)
    encoded = xbox_adpcm.encode(pcm, channels, cancel)
    translated = replace_audio(original, encoded)
    if cancel:
        cancel()
    with output_path.open('xb') as stream:
        stream.write(translated)
    tempo = _tempo(ps2_path.name, 29.84, frames)
    return {'file': original_path.name.lower(), 'size': len(translated),
            'sha256': hashlib.sha256(translated).hexdigest(), 'sample_frames': frames,
            'sample_rate': rate, 'channels': channels, 'bink_track_id': 5,
            'audio_bytes': len(encoded), 'source_sha256': hashlib.sha256(original).hexdigest(),
            'voice_tempo_factor': tempo, 'pitch_preserved': True}
