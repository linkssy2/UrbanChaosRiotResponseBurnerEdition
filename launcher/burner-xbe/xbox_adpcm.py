"""Xbox IMA blocks: 36 bytes/channel, 64 playable frames, little-endian PCM.

Uses audioop from the launcher's pinned Python 3.12 runtime.
"""
import audioop
import struct

_SWAP = bytes((value >> 4) | ((value & 15) << 4) for value in range(256))


def encode(pcm, channels=1, cancel=None):
    if channels not in (1, 2) or len(pcm) % (2 * channels):
        raise ValueError('Xbox ADPCM requires complete mono or stereo PCM16 frames.')
    if not pcm:
        return b''
    frames = len(pcm) // (2 * channels)
    pcm += bytes((-frames % 64) * 2 * channels)
    planes = ([pcm] if channels == 1 else
              [audioop.tomono(pcm, 2, 1, 0), audioop.tomono(pcm, 2, 0, 1)])
    indices = [0] * channels
    result = bytearray()
    for offset in range(0, len(planes[0]), 128):
        if cancel and offset % (128 * 1024) == 0:
            cancel()
        headers, payloads = [], []
        for channel, plane in enumerate(planes):
            block = plane[offset:offset + 128]
            predictor = struct.unpack_from('<h', block)[0]
            headers.append(struct.pack('<hBB', predictor, indices[channel], 0))
            # Xbox ignores frame 65; repeat frame 64 to fill the final nibble.
            packed, state = audioop.lin2adpcm(block[2:] + block[-2:], 2,
                                            (predictor, indices[channel]))
            indices[channel] = state[1]
            payloads.append(packed.translate(_SWAP))
        result.extend(b''.join(headers))
        for group in range(0, 32, 4):
            for payload in payloads:
                result.extend(payload[group:group + 4])
    if cancel:
        cancel()
    return bytes(result)


def decode(encoded, channels=1):
    """Decode the playable 64 frames, for conversion checks and test fixtures."""
    if channels not in (1, 2) or len(encoded) % (36 * channels):
        raise ValueError('Invalid Xbox ADPCM block extent.')
    result = bytearray()
    for offset in range(0, len(encoded), 36 * channels):
        block = encoded[offset:offset + 36 * channels]
        planes = []
        for channel in range(channels):
            predictor, index, reserved = struct.unpack_from('<hBB', block, channel * 4)
            if index > 88 or reserved:
                raise ValueError('Invalid Xbox ADPCM predictor header.')
            packed = b''.join(block[channels * 4 + (group * channels + channel) * 4:
                                    channels * 4 + (group * channels + channel + 1) * 4]
                              for group in range(8))
            pcm, _ = audioop.adpcm2lin(packed.translate(_SWAP), 2, (predictor, index))
            planes.append(struct.pack('<h', predictor) + pcm[:126])
        result.extend(planes[0] if channels == 1 else audioop.add(
            audioop.tostereo(planes[0], 2, 1, 0),
            audioop.tostereo(planes[1], 2, 0, 1), 2))
    return bytes(result)
