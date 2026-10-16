"""Preserve the retail absolute 48 kHz pitch contract in private audio hooks."""
from pathlib import Path
import hashlib
import struct


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(source, area):
    """Run after audio factors; change only the private source and fixture."""
    area = Path(area)
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table - base + i * 56) for i in range(count)]

    def image(va, size):
        section = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
        offset = section[3] + va - section[1]
        return xbe[offset:offset + size]

    pins = {
        (0x423bd7, 74): '1368b87eaab50f4ef532e3ce20a70aa2061bf6c9912c7f0da88d5d21489b1352',
        (0x424088, 25): '9339fa691f4f126a996c1741d3a12377f7886e4a1eb90162c26370fc8f4b002a',
        (0x42ac20, 130): '1c9679e502fc05a65af8e5b8153e2088f639928c86dffce78527eee2205a12d1',
        (0x42a018, 30): '06e8630df9abd22be0a0cf9161d826f46288a075fbba5b77b8a367ea5313faf1',
        (0x424f31, 28): 'e699f7d64ee65b2170e2a041723993e9bd18de165f7c5702bc98b1cdef767e09',
        (0x42503e, 5): '3928019e30f8842a07d14724d33e68bed20d0c1700ac6ed306dac870a83ced7a',
        (0x251a50, 41): 'd886f8978e9419bb6c609b03c09d7e8253e8eff421044b183571c8b9da4ccff6',
        (0x426374, 12): 'abe01ac467d30fb9072fe1722cb43efef578a655dc7c0163e46aab9581a0431a',
    }
    for (va, size), digest in pins.items():
        assert hashlib.sha256(image(va, size)).hexdigest() == digest, hex(va)

    source = once(source,
                  '    if (!v || pitch < -8192 || pitch > 8191) return 0x80070057u;\n'
                  '    v->freq = uint32_t(std::lround(v->rate * std::exp2(pitch / 4096.0)));',
                  '    if (!v) return 0x80070057u;\n'
                  '    // 424088 replaces the format pitch; zero is 48 kHz, not the asset rate.\n'
                  '    // 42AC20 clamps the resulting absolute pitch before publishing it.\n'
                  '    pitch = std::clamp(pitch, -32767, 8191);\n'
                  '    v->freq = uint32_t(std::lround(48000.0 * std::exp2(pitch / 4096.0)));')
    source = once(source, '    return step * std::exp2(pitch / 4096.0);',
                  '    // 42AC20 bounds base pitch plus Doppler, not each contribution alone.\n'
                  '    return std::clamp(step * std::exp2(pitch / 4096.0),\n'
                  '                      std::exp2(-32767 / 4096.0), std::exp2(8191 / 4096.0));')

    originals = ''
    for partition, addresses in [(124, ['0042A018', '0042AC20']), (123, ['00423BD7', '00424088'])]:
        generated = (area / f'generated/game/x86_recomp.{partition}.cpp').read_text()
        for address in addresses:
            body = between(generated, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}')
            if address == '0042A018':
                body = body.replace('__imp__sub_' + address, 'sub_' + address)
            originals += body + '\n'
    title = (area / 'generated/game/x86_recomp.61.cpp').read_text()
    title = between(title, 'X86_FUNC(__imp__sub_00251A50) {', '\nX86_WEAK_FUNC(sub_00251A50')
    title = title.replace('__imp__sub_00251A50', 'OriginalTitlePitch')
    constants = '\n'.join(f'W32({hex(va)}, {hex(struct.unpack("<I", image(va, 4))[0])});'
                          for va in [0x4c2d74, 0x44c510])
    fixture = Path(__file__).with_name('test_xbe_audio_pitch.cpp').read_text()
    for marker, body in [('// GENERATED_PITCH', between(source, '#define SYM_UrbanChaos_BufferPitch', '\nBUFFER(Release, 1)')),
                         ('// GENERATED_PLAYBACK', between(source, 'static double AudioPlaybackStep(', '\n// gain of voice channel')),
                         ('// GENERATED_ORIGINAL', originals), ('// GENERATED_TITLE', title),
                         ('// GENERATED_CONSTANTS', constants)]:
        fixture = once(fixture, marker, body)
    (area / 'src/compat/test_audio_pitch.cpp').write_text(fixture)
    return source
