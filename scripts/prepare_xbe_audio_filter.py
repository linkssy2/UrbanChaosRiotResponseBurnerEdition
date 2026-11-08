"""Restore the title's per-voice lowpass on the shared 48 kHz software mixer.

Run after surround and before environmental DSP. Original packets are retained;
the waveform is a host state-variable adaptation, not Xbox hardware emulation.
"""
import hashlib
import struct
from pathlib import Path
from prepare_xbe_audio_loop import once, between


ADAPTER = r'''
static_assert(audio::kRate == 48000, "Retail filter coefficients require the 48 kHz mixer");
static uint32_t SetVoiceFilter(Voice& voice, uint32_t pointer) {
    if (!TrialGuestReadable(pointer, 24)) return 0x80070057u;
    // MIXIN buses are gain/routing owners in this mixer, not PCM processors.
    if (voice.mixin) return 0x80004001u;
    uint32_t packet[6];
    for (unsigned i = 0; i < 6; ++i) packet[i] = R32(pointer + i * 4);
    return AudioFilterSet(voice.filter, packet) ? 0 : 0x80070057u;
}
'''

STREAM_ALIAS = r'''// Retail 425043 jumps to 424D10 SetFilter; the symbol database calls it SetEG.
HLE_STDCALL(IDirectSoundStream_SetEG, 2) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    Voice* voice = Find(a[0]);
    return voice && voice->stream ? SetVoiceFilter(*voice, a[1]) : 0x80070057u;
}'''


def prepare(source, area):
    area = Path(area)
    xbe = (area/'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table-base+i*56) for i in range(count)]

    def image(address, size):
        section = next(s for s in sections if s[1] <= address and address+size <= s[1]+s[4])
        offset = section[3]+address-section[1]
        return xbe[offset:offset+size]

    pins = {
        (0x251d80, 135): '6eabac144e25b8132e3cb57b063939c23ebc87397ca45947e51e2f7c3bbf9e2d',
        (0x244140, 99): '9c643c74483fbdfb2099b67b5b7d3cecb2a9f3b8eca0c0b78259b876b4ae33b6',
        (0x2441b0, 76): '48a4575ff5ef31f336da159522edfcaea48ea891e88d4a97050b21646010197f',
        (0x424f4d, 28): 'f978d10ff1b9e2d68d719e5f3d21d042f467e49609d14d1e79ba5376d2c36a3c',
    }
    for (address, size), digest in pins.items():
        assert hashlib.sha256(image(address, size)).hexdigest() == digest, hex(address)
    assert image(0x425043, 5) == bytes.fromhex('e9c8fcffff')
    assert image(0x44c174, 4) == bytes(4)
    assert struct.unpack('<I', image(0x4839a4, 4))[0] == 48000
    symbols = (area/'generated/game/image_symbols.h').read_text()
    for name, address in [('IDirectSoundBuffer_SetFilter', '00424F4D'),
                          ('CDirectSoundStream_SetFilter', '00424D10'),
                          ('IDirectSoundStream_SetEG', '00425043')]:
        assert f'#define SYM_{name} {address}' in symbols

    render_before = between(source, 'template<bool Track>\nvoid RenderTracked(', '\nint64_t NowMs()')
    source = once(source, '#include "xbe_audio_output.h"',
                  '#include "xbe_audio_output.h"\n#include "xbe_audio_filter.h"')
    source = once(source, 'struct Voice {', 'struct Voice {\n    UcAudioFilter filter;')
    source = once(source, '// buffers\n', ADAPTER+'\n// buffers\n')
    source = once(source, 'BUFFER(SetFilter, 2) { return 0; }',
                  'BUFFER(SetFilter, 2) { return SetVoiceFilter(v, a[1]); }')
    source = once(source, 'STREAM(SetFilter, 2) { return 0; }',
                  'STREAM(SetFilter, 2) { return SetVoiceFilter(v, a[1]); }')
    source = once(source, '// the library\'s own writes voice registers a host stream does not have; envelopes are not played\n'
                  'HLE_STDCALL(IDirectSoundStream_SetEG, 2) { return 0; }', STREAM_ALIAS)
    source = once(source, '            b += (b1 - b) * t;',
                  '            b += (b1 - b) * t;\n            AudioFilterSample(v.filter, a, b);')

    original = ''
    for partition, addresses in [(60, ['00244140', '002441B0']), (61, ['00251D80'])]:
        unit = (area/f'generated/game/x86_recomp.{partition}.cpp').read_text()
        for address in addresses:
            body = between(unit, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}')
            original += body.replace('__imp__sub_'+address, 'sub_'+address)+'\n'
    constants = '\n'.join(f'W32({hex(at)}, {hex(struct.unpack("<I", image(at, 4))[0])});'
                          for at in [0x44c13c, 0x44c174, 0x44c510, 0x45029c, 0x4839a4,
                                     *range(0x483a00, 0x483a24, 4)])
    local = Path(__file__).parent
    fixture = (local/'test_xbe_audio_filter.cpp').read_text()
    substitutions = {
        '// GENERATED_OWNER': between(source, 'struct Packet {', 'struct Done {'),
        '// GENERATED_MATH': between(source, '// 427AAF/427765:', '// gain of voice channel'),
        '// GENERATED_LEVELS': between(source, '// gain of voice channel', '// a stream goes quiet:'),
        '// GENERATED_SAMPLE': between(source, 'const int kIndex[16]', 'void Complete('),
        '// GENERATED_ADAPTER': ADAPTER,
        '// GENERATED_BUFFER_MACRO': between(source, '#define BUFFER(name, nargs)', '#define SYM_UrbanChaos_BufferDistanceFactor'),
        '// GENERATED_STREAM_MACRO': between(source, '#define STREAM(name, nargs)', '// the stream\'s callbacks'),
        '// GENERATED_BUFFER': between(source, 'BUFFER(SetFilter, 2)', '\nBUFFER(SetHeadroom'),
        '// GENERATED_STREAM': between(source, 'STREAM(SetFilter, 2)', '\nSTREAM(SetHeadroom'),
        '// GENERATED_RENDER_BEFORE': render_before.replace('RenderTracked', 'RenderBeforeTracked').replace('void Render(', 'void RenderBefore('),
        '// GENERATED_RENDER_AFTER': between(source, 'template<bool Track>\nvoid RenderTracked(', '\nint64_t NowMs()'),
        '// GENERATED_ORIGINAL': original,
        '// GENERATED_CONSTANTS': constants,
    }
    for marker, replacement in substitutions.items():
        fixture = once(fixture, marker, replacement)
    target = area/'src/compat'
    (target/'xbe_audio_filter.h').write_text((local/'xbe_audio_filter.h').read_text())
    (target/'test_audio_filter.cpp').write_text(fixture)
    return source
