"""Restore title-authored mono MIXIN routing, before the optional audio trace."""
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
    """After cone/factors/pitch/status, before prepare_xbe_audio_loop. No new TU."""
    area = Path(area)
    xbe = (area / 'assets/Default.xbe').read_bytes()
    u32 = lambda at: struct.unpack_from('<I', xbe, at)[0]
    sections = [struct.unpack_from('<6I', xbe, u32(0x120)-u32(0x104)+i*56)
                for i in range(u32(0x11c))]
    def image(va, size):
        s = next(s for s in sections if s[1] <= va and va+size <= s[1]+s[4])
        at = s[3]+va-s[1]
        return xbe[at:at+size]
    pins = {
        (0x251890,198): 'fa67437367e10d4e98920dca4888886b8ec149440e82c42c675429299f16c3f9',
        (0x252100,445): '959392a61817b1878719029ba2420fc8934c2eaf7f5c6b06b5365c504e94d75b',
        (0x252d20,234): 'eb632d6e0f776d1a5e752a3853895df72ac902e8feb393d3d6d5ba04479a08b2',
        (0x425183,213): 'c8142296a845a08df97dfb015a7d74651539b5d61895cb6b1b1d8ab2d51a104a',
        (0x4250b8,203): '69f8d65324d8f80ba891c0b2861161625d389920b5230d6260bf43f529fd541e',
        (0x425f23,28): '6e67b1d36b77fa07a0aa7b09b80d86a7a2fe88d4385bec4cbccbdc5c0d61be26',
        (0x426156,5): 'c0a8c739468146e237adedf833c27cc4110b30a7f241d2bd8698b0243aa05ef3',
        (0x42396b,73): '3a1a33b0b9cffb8f316d1479373be8eab13e0c58e38d12a7fe8178f9aa3b238b',
        (0x427735,9): 'ae4a39f1f86ea5a10dc0f01ad19c2ede2c9ddf1aed58b048c690bc9f8fdebf83',
    }
    for (va, size), digest in pins.items():
        assert hashlib.sha256(image(va,size)).hexdigest() == digest, hex(va)

    source = once(source, 'struct Voice {', 'struct Voice {\n'
                  '    uint32_t output = 0; // owned zero-byte MIXIN destination\n'
                  '    bool mixin = false, bins_empty = false;\n'
                  '    float route_gain = 1; // automatic mono-send normalization\n')
    # Extract the current shared spatial math verbatim: routed mono inputs have
    # the base attenuation, while the destination retains speaker panning.
    spatial = between(source, '    float att = 1;', '    float right[3] =')
    spatial_helper = ('static float AudioSpatialAttenuation(const Voice& v, const float d[3], float dist) {\n'
                      + spatial + '    return att;\n}\n\n')
    source = once(source, '// gain of voice channel', spatial_helper+'// gain of voice channel')
    # The extraction inserted a second occurrence; replace only within Levels.
    levels = between(source, 'void Levels(Voice& v, float g[2][2]) {', '// a stream goes quiet')
    levels = once(levels, spatial, '    float att = AudioSpatialAttenuation(v, d, dist);\n')
    levels = once(levels, 'void Levels(', 'void LevelsDirect(')
    old_levels = between(source, 'void Levels(Voice& v, float g[2][2]) {', '// a stream goes quiet')
    helpers = Path(__file__).with_name('xbe_audio_routing.inc').read_text()
    source = once(source, old_levels, levels+helpers+'\n')
    source = once(source, 'static double AudioPlaybackStep(const Voice& v) {',
                  'static double AudioPlaybackStep(const Voice& v, bool clamp_pitch=true) {')
    source = once(source, '    return std::clamp(step * std::exp2(pitch / 4096.0),',
                  '    if (!clamp_pitch) return step * std::exp2(pitch / 4096.0);\n'
                  '    return std::clamp(step * std::exp2(pitch / 4096.0),')
    source = once(source, '    v.nbins = 0;  // none listed: front left and right',
                  '    if (v.bins_empty && !mixbins) v.bin_mb[0] = v.bin_mb[1] = 0; // restored default sends\n'
                  '    v.bins_empty = false; // explicit reconfiguration after detachment\n'
                  '    v.nbins = 0;  // none listed: front left and right')
    source = once(source, 'uint32_t n = std::min<uint32_t>(R32(mixbins), 8), pairs = R32(mixbins + 4);',
                  'uint32_t n = std::min<uint32_t>(R32(mixbins), v.output ? 8-v.channels : 8), pairs = R32(mixbins + 4);')
    source = once(source, '    v.is3d = flags & 0x10;  // DSBCAPS_CTRL3D\n    v.headroom = DefaultHeadroom(flags);\n    if (bytes)',
                  '    v.is3d = flags & 0x10;  // DSBCAPS_CTRL3D\n    v.mixin = AudioIsMixin(flags, bytes);\n'
                  '    v.headroom = DefaultHeadroom(flags);\n    if (bytes)')
    source = once(source, '    for (auto& [obj, v] : g_voices) {\n        if (v.paused',
                  '    for (auto& [obj, v] : g_voices) {\n        if (v.mixin) continue; // gain bus, not a zero-length PCM voice\n        if (v.paused')
    source = once(source, 'double step = AudioPlaybackStep(v);', 'double step = RoutedPlaybackStep(v);')
    source = once(source, 'BUFFER(SetOutputBuffer, 2) { return 0; }  // the voice keeps its own mix bins',
                  'BUFFER(SetOutputBuffer, 2) { return SetAudioOutput(v, a[1]); }')
    source = once(source,
                  '    // shortcut: streams currently mix directly; implement host submix buses before accepting nonzero destinations.\n'
                  '    return a[1] ? 0x80004001u : 0;', '    return SetAudioOutput(*v, a[1]);')
    for marker in ['BUFFER(Release, 1) {', 'STREAM(Release, 1) {']:
        old = marker+'\n    if (--v.refs > 0) return uint32_t(v.refs);'
        source = once(source, old, old+'\n    ReleaseAudioOutput(v);')
    source = once(source, 'BUFFER(SetBufferData, 3) {',
                  'BUFFER(SetBufferData, 3) {\n    if (v.mixin && (a[1] || a[2])) return 0x80070057u;')
    source = once(source, 'BUFFER(SetFormat, 2) {',
                  'BUFFER(SetFormat, 2) {\n    if (v.mixin && a[1]) return 0x80070057u;')
    for marker in ['BUFFER(SetFormat, 2) {','STREAM(SetFormat, 2) {']:
        source = once(source, marker, marker+'\n    if (v.output && a[1] && R16(a[1]+2)>2) return 0x80070057u;')
    # Both buffer/stream SetFormat have this exact line; only those two sites.
    assert source.count('    ReadFormat(v, a[1]);') == 2
    source = source.replace('    ReadFormat(v, a[1]);',
                            '    ReadFormat(v, a[1]);\n    UpdateAudioRouteFormat(v);')

    fixture = Path(__file__).with_name('test_xbe_audio_routing.cpp').read_text()
    release_buffer = between(source, 'BUFFER(Release, 1) {', 'BUFFER(Unlock, 5)')
    release_buffer = once(release_buffer, 'BUFFER(Release, 1)', 'static uint32_t ReleaseBuffer(Voice& v)')
    release_stream = between(source, 'STREAM(Release, 1) {', 'STREAM(GetInfo, 2)')
    release_stream = once(release_stream, 'STREAM(Release, 1)', 'static uint32_t ReleaseStream(Voice& v)')
    generated123 = (area/'generated/game/x86_recomp.123.cpp').read_text()
    generated124 = (area/'generated/game/x86_recomp.124.cpp').read_text()
    original = between(generated124, 'X86_FUNC(__imp__sub_00427735) {', 'X86_WEAK_FUNC')
    original += 'X86_FUNC(sub_00427735) { __imp__sub_00427735(ctx,base); }\n'
    original += between(generated123, 'X86_FUNC(__imp__sub_0042396B) {', 'X86_WEAK_FUNC')
    title = (area/'generated/game/x86_recomp.61.cpp').read_text()
    # Actual caller suffix covers buffer and stream output wrappers and null
    # detach. Unrelated intrusive-list ownership is outside this audio fixture.
    caller = between(title, 'loc_002518F6:', 'X86_WEAK_FUNC(sub_00251890')
    caller = 'static void OriginalTitleOutput(X86Context& ctx,uint8_t* base) {\nX86_LOAD_FLAGS();\nuint32_t ca=0,cb=0;\n'+caller
    constants = '\n'.join(f'W32({va}, {u});' for va in [0x44bf94,0x44c174,0x44f854]
                          for u in [struct.unpack('<I',image(va,4))[0]])
    for marker, replacement in {
        '// GENERATED_OWNER': between(source, 'struct Packet {', 'struct Done {'),
        '// GENERATED_MATH': between(source, '// 427AAF/427765:', '// gain of voice channel'),
        '// GENERATED_LEVELS': levels+helpers,
        '// GENERATED_BINS': between(source, '// DSMIXBINS:', '// 600 mB unless'),
        '// GENERATED_RENDER': between(source, 'void Render(float* out, uint32_t frames) {', '\nint64_t NowMs()'),
        '// GENERATED_RELEASE': between(source, 'static void FlushStream(Voice& v) {', 'STREAM(AddRef, 1)')+release_buffer+release_stream,
        '// GENERATED_ORIGINAL': original,
        '// GENERATED_TITLE': caller,
        '// GENERATED_CONSTANTS': constants,
    }.items():
        fixture = once(fixture,marker,replacement)
    target = area/'src/compat';target.mkdir(parents=True,exist_ok=True)
    (target/'test_audio_routing.cpp').write_text(fixture)
    return source
