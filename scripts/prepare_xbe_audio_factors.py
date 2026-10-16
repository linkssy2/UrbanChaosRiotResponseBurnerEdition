"""Connect retail voice factors to the host attenuation and playback rate."""
from pathlib import Path
import hashlib
import struct


STATE = r'''
// Slots: metres per distance unit, Doppler, rolloff, then velocity XYZ. The retail
// setters queue fields and an immediate setter commits the whole voice state.
struct UcAudioFactors {
    float value[6] = {1, 1, 1, 0, 0, 0};
    float pending[6] = {1, 1, 1, 0, 0, 0};
    uint8_t dirty = 0;
};
static void FactorCommit(UcAudioFactors& state) {
    for (unsigned i = 0; i < 6; ++i)
        if (state.dirty & (1u << i)) state.value[i] = state.pending[i];
    state.dirty = 0;
}
static uint32_t FactorWrite(UcAudioFactors& state, unsigned slot, float value) {
    if (!std::isfinite(value) || value < 0 || (!slot && value == 0)) return 0x80070057u;
    state.pending[slot] = value;
    state.dirty |= uint8_t(1u << slot);
    return 0;
}
static uint32_t VelocityWrite(UcAudioFactors& state, const Args& a) {
    float values[3];
    for (unsigned i = 0; i < 3; ++i) {
        values[i] = F32(a[1 + i]);
        if (!std::isfinite(values[i])) return 0x80070057u;
    }
    for (unsigned i = 0; i < 3; ++i) state.pending[3 + i] = values[i];
    state.dirty |= 0x38;
    return 0;
}
'''

MATH = r'''
// 427AAF/427765: rolloff changes the logarithmic distance curve, not the
// geometric direction or distance unit conversion. Custom curves stay authored.
static float FactorAttenuation(const Voice& v, float distance) {
    double rolloff = double(v.factors.value[2]) * g_listener_factors.value[2];
    double scaled = (double(std::min(distance, v.max_dist)) / v.min_dist - 1) * rolloff;
    return float(1 / (1 + std::max(0.0, scaled)));
}
// 427A64/427DC1: radial relative velocity, then the title's 342 m/s model
// and 4096-unit octave pitch. This preserves the SDK's endpoint clamps.
static double AudioPlaybackStep(const Voice& v) {
    double step = double(v.freq ? v.freq : v.rate) / audio::kRate;
    if (!v.is3d || v.mode == 2) return step;
    double d[3], length2 = 0, dot = 0;
    for (unsigned i = 0; i < 3; ++i) {
        d[i] = double(v.p[i]) - (v.mode == 1 ? 0 : double(g_listener[i]));
        length2 += d[i] * d[i];
        dot += d[i] * (double(v.factors.value[i + 3]) - (v.mode == 1 ? 0 : double(g_listener_factors.value[i + 3])));
    }
    if (!std::isfinite(length2) || !std::isfinite(dot) || length2 <= 1e-12) return step;
    float radial = float(dot / std::sqrt(length2));
    if (radial == 0 || v.factors.value[1] == 0 || g_listener_factors.value[1] == 0) return step;
    float distance = v.factors.value[0] * g_listener_factors.value[0];
    float doppler = v.factors.value[1] * g_listener_factors.value[1];
    if (distance == 0 || doppler == 0) return step;
    double speed = double(distance) * doppler * radial;
    if (std::isnan(speed) || speed == 0) return step;
    int pitch;
    if (speed >= 342) pitch = -32767;
    else if (speed < -342) pitch = 4096;
    else {
        float ratio = float(1.0 - speed * double(0.002923976629972458f));
        pitch = ratio > 0 ? int(std::nearbyint(4096 * std::log2(double(ratio)))) : -32767;
    }
    return step * std::exp2(pitch / 4096.0);
}
'''

LISTENER = r'''
static uint32_t ListenerFactor(const Args& a, unsigned slot) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    uint32_t result = FactorWrite(g_listener_factors, slot, F32(a[1]));
    if (!result && !(a[2] & 1)) FactorCommit(g_listener_factors);
    return result;
}
HLE_STDCALL(IDirectSound_SetDopplerFactor, 3) { return ListenerFactor(a, 1); }
HLE_STDCALL(IDirectSound_SetRolloffFactor, 3) { return ListenerFactor(a, 2); }
HLE_STDCALL(IDirectSound_SetDistanceFactor, 3) { return ListenerFactor(a, 0); }
HLE_STDCALL(IDirectSound_SetVelocity, 5) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    uint32_t result = VelocityWrite(g_listener_factors, a);
    if (!result && !(a[4] & 1)) FactorCommit(g_listener_factors);
    return result;
}
'''

VOICE = r'''
#define SYM_UrbanChaos_BufferDistanceFactor 004260A6
#define SYM_UrbanChaos_BufferDopplerFactor 004260CA
#define SYM_UrbanChaos_BufferRolloffFactor 004260EE
static uint32_t VoiceFactor(const Args& a, unsigned slot) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    Voice* v = Find(a[0]);
    if (!v) return 0x80070057u;
    uint32_t result = FactorWrite(v->factors, slot, F32(a[1]));
    if (!result && !(a[2] & 1)) CommitVoice3D(*v);
    return result;
}
HLE_STDCALL(UrbanChaos_BufferDistanceFactor, 3) { return VoiceFactor(a, 0); }
HLE_STDCALL(UrbanChaos_BufferDopplerFactor, 3) { return VoiceFactor(a, 1); }
HLE_STDCALL(UrbanChaos_BufferRolloffFactor, 3) { return VoiceFactor(a, 2); }
static uint32_t VoiceVelocity(Voice& v, const Args& a) {
    uint32_t result = VelocityWrite(v.factors, a);
    if (!result && !(a[4] & 1)) CommitVoice3D(v);
    return result;
}
'''


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(source, area):
    """Run after prepare_xbe_audio_cone; no shared runtime inputs are edited."""
    area = Path(area)
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table - base + i * 56) for i in range(count)]

    def image(va, size):
        s = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
        at = s[3] + va - s[1]
        return xbe[at:at + size]

    hashes = {0x4260a6: '5b60c05be1f9e2ec77f1bb3e1ce3800d70cf040250075a9310fedd96dc63c83f',
              0x4260ca: '462649c3edd98aab8d666ddac37ab0ee8fe2c6eeea6536c8e613742082837c04',
              0x4260ee: '88854b3be146cf57a751e14c006887272e8b72b97d0e60ee1702957bb0830b9a'}
    for va, digest in hashes.items():
        assert hashlib.sha256(image(va, 36)).hexdigest() == digest
    # F32 is normally below Voice. The helper above needs only bit conversion.
    state = STATE.replace('F32(a[1 + i])', 'std::bit_cast<float>(a[1 + i])')
    source = once(source, '#include <algorithm>', '#include <algorithm>\n#include <bit>')
    source = once(source, 'struct Voice {', state + '\nstruct Voice {\n    UcAudioFactors factors;')
    source = once(source, 'float g_listener[3]', 'UcAudioFactors g_listener_factors;\nfloat g_listener[3]')
    source = once(source, '// CommitVoice3D extensions', 'FactorCommit(v.factors);\n    // CommitVoice3D extensions')
    source = once(source, 'for(auto& [id,v]:g_voices)CommitVoice3D(v);',
                  'for(auto& [id,v]:g_voices)CommitVoice3D(v);\n    FactorCommit(g_listener_factors);')
    old = '\n'.join('HLE_STDCALL(IDirectSound_Set' + name + ', ' + str(args) + ') { return 0; }'
                    for name, args in [('DopplerFactor', 3), ('RolloffFactor', 3), ('DistanceFactor', 3), ('Velocity', 5)])
    source = once(source, old, LISTENER)
    source = once(source, '// gain of voice channel', MATH + '\n// gain of voice channel')
    source = once(source, 'att = v.min_dist / std::min(dist, v.max_dist);', 'att = FactorAttenuation(v, dist);')
    # The same expression appears in AudioPlaybackStep itself; replace Render only.
    render = between(source, 'void Render(', '\nint64_t NowMs(')
    source = once(source, render, once(render, 'double step = double(v.freq ? v.freq : v.rate) / audio::kRate;',
                                     'double step = AudioPlaybackStep(v);'))
    source = once(source, '// Public SetMode interfaces:', VOICE + '\n// Public SetMode interfaces:')
    source = once(source, 'BUFFER(SetVelocity, 5) { return 0; }', 'BUFFER(SetVelocity, 5) { return VoiceVelocity(v, a); }')
    source = once(source, 'STREAM(SetVelocity, 5) { return 0; }', 'STREAM(SetVelocity, 5) { return VoiceVelocity(v, a); }')

    original = (area / 'generated/game/x86_recomp.124.cpp').read_text()
    oracle = ''
    for address in ['0042A018', '00427DC1', '004260A6', '004260CA', '004260EE']:
        body = between(original, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}')
        if address == '0042A018': body = body.replace('__imp__sub_' + address, 'sub_' + address)
        oracle += body + '\n'
    title = (area / 'generated/game/x86_recomp.61.cpp').read_text()
    producer = between(title, '\t// 00251B46 mov ecx, [edi+0x18]', '\t// 00251B73 pop ebx')
    producer = 'static void OriginalTitleFactors(X86Context& ctx, uint8_t* base) {\n' + producer + '}\n'
    fixture = Path(__file__).with_name('test_xbe_audio_factors.cpp').read_text()
    constants = '\n'.join(f'W32({hex(va)}, {hex(struct.unpack("<I", image(va, 4))[0])});'
                          for va in [0x42f4b4, 0x4c2e18, 0x4c2e14, 0x44bf94, 0x44c174])
    for marker, body in [('// GENERATED_STATE', state), ('// GENERATED_MATH', MATH),
                         ('// GENERATED_LISTENER', LISTENER), ('// GENERATED_VOICE', VOICE),
                         ('// GENERATED_ORIGINAL', oracle), ('// GENERATED_CONSTANTS', constants),
                         ('// GENERATED_TITLE', producer),
                         ('// GENERATED_COMMIT', between(source, 'static void CommitVoice3D(Voice& v) {', '\nstruct Done {')),
                         ('// GENERATED_DEFERRED', between(source, 'HLE_STDCALL(IDirectSound_CommitDeferredSettings, 1)', '\nstatic uint32_t ListenerFactor('))]:
        fixture = once(fixture, marker, body)
    (area / 'src/compat/test_audio_factors.cpp').write_text(fixture)
    return source
