"""Reuse adjacent ADPCM blocks without changing the title's existing audio mix.

Selective port from XboxXBERecomp 1bb872b181c40de83ca2f69d9cd45b442a362d1d:
DecodedBlock/ForgetBlocks and its non-current-slot invalidation, with the local
carried-block eviction kept equivalent to the former one-slot cache. The upstream
DSP, routing, HRTF, interpolation and output-queue changes are not imported.
"""
from pathlib import Path

from prepare_xbe_perf import function, once


OLD_CACHE = '''    // ADPCM block cache
    int64_t block = -1;
    int16_t decoded[65 * 8];'''
NEW_CACHE = '''    // Adjacent interpolation samples can straddle two ADPCM blocks.
    int64_t block[2] = {-1, -1};
    uint8_t last = 0;
    bool carried_block = false;
    int16_t decoded[2][65 * 8];'''
HELPERS = '''// XboxXBERecomp 1bb872b: retain both sides of a sample interpolation boundary.
void ForgetBlocks(Voice& v) {
    v.block[0] = v.block[1] = -1;
    v.carried_block = false;
}
const int16_t* DecodedBlock(Voice& v, int64_t at) {
    // Guest memory can change between mixes; preserve the old one-slot eviction.
    if (v.carried_block && v.block[v.last] != at) {
        v.block[v.last] = -1;
        v.carried_block = false;
    }
    for (uint8_t k = 0; k < 2; ++k) {
        if (v.block[k] != at) continue;
        v.last = k;
        return v.decoded[k];
    }
    const uint8_t k = v.last ^ 1;
    DecodeBlock(static_cast<const uint8_t*>(HostPtr(uint32_t(at))), std::min<int>(v.channels, 8), v.decoded[k]);
    v.block[k] = at;
    v.last = k;
    return v.decoded[k];
}

'''
BEGIN_MIX = '        v.block[v.last ^ 1] = -1;\n        v.carried_block = true;'


def transform(source):
    source = once(source, OLD_CACHE, NEW_CACHE)
    source = once(source, '// sample of channel c at frame f of memory starting at data',
                  HELPERS + '// sample of channel c at frame f of memory starting at data')
    source = once(source, '''        if (at != v.block) {
            DecodeBlock(static_cast<const uint8_t*>(HostPtr(uint32_t(at))), std::min<int>(v.channels, 8), v.decoded);
            v.block = at;
        }
        return v.decoded[(f % 64) * v.channels + c] / 32768.0f;''',
                  '        return DecodedBlock(v, at)[(f % 64) * v.channels + c] / 32768.0f;')
    # The added slot must not retain older guest data across output blocks.
    source = once(source, '        double step = RoutedPlaybackStep(v);',
                  '        double step = RoutedPlaybackStep(v);\n'
                  '        // Preserve the original current block; forget the extra cached block between mixes.\n' + BEGIN_MIX)
    resets = ('BUFFER(SetBufferData, 3) {', 'BUFFER(SetPlayRegion, 3) {',
              'BUFFER(SetFormat, 2) {', 'STREAM(SetFormat, 2) {')
    for signature in resets:
        old = function(source, signature)
        source = once(source, old, once(old, '    v.block = -1;', '    ForgetBlocks(v);'))
    assert 'v.block = -1;' not in source
    return source


def prepare(source, area):
    result = transform(source)
    fixture = Path(__file__).with_name('test_xbe_audio_decode_cache.cpp').read_text()
    decoder = source[source.index('const int kIndex[16]'):source.index('// sample of channel c')]
    for label, body, fields in [('BEFORE', source, OLD_CACHE), ('AFTER', result, NEW_CACHE)]:
        code = decoder + function(body, 'float Sample(')
        if label == 'AFTER':
            code = decoder + HELPERS + function(body, 'float Sample(')
        code = once(code, 'void DecodeBlock(const uint8_t* b, int ch, int16_t* out) {',
                    'void DecodeBlock(const uint8_t* b, int ch, int16_t* out) {\n    ++decodes;')
        fixture = once(fixture, f'/* {label}_FIELDS */', fields)
        fixture = once(fixture, f'/* {label}_CODE */', code)
        setters = function(body, 'void ReadFormat(')
        for signature, name, size in [('BUFFER(SetBufferData, 3)', 'BufferData', 3),
                                      ('BUFFER(SetPlayRegion, 3)', 'PlayRegion', 3),
                                      ('BUFFER(SetFormat, 2)', 'BufferFormat', 2),
                                      ('STREAM(SetFormat, 2)', 'StreamFormat', 2)]:
            setters += '\n' + function(body, signature + ' {').replace(
                signature, f'uint32_t {name}(Voice& v, const std::array<uint32_t,{size}>& a)', 1)
        fixture = once(fixture, f'/* {label}_SETTERS */', setters)
    fixture = once(fixture, '/* BEGIN_MIX */', BEGIN_MIX.strip())
    target = Path(area) / 'src/compat/test_audio_decode_cache.cpp'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(fixture)
    return result
