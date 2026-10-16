"""Connect the retail cone APIs to the shared host voice and mixer owner.

Run after the VoiceMode preparation, before prepare_xbe_audio_factors. This
stages only private artifacts; no upstream/generated input is changed.
"""
from pathlib import Path
import hashlib
import struct


COMMIT = r'''
static void CommitVoice3D(Voice& v) {
    uc_voice_mode_commit(v);
    ConeCommit(v);
    // CommitVoice3D extensions
}

'''

ADAPTER = r'''
// Qualified public buffer interfaces. Both host voice kinds share cone state;
// no stream export is invented where the linked title has no such entry point.
#define SYM_UrbanChaos_BufferConeAngles 00425F5B
#define SYM_UrbanChaos_BufferConeOrientation 00425F7F
#define SYM_UrbanChaos_BufferConeOutsideVolume 00425FB4
static uint32_t VoiceCone(const Args& a, unsigned field) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    Voice* v = Find(a[0]);
    if (!v) return 0x80070057u;
    uint32_t apply;
    if (field == 4) {
        v->pending_cone.inside = a[1]; v->pending_cone.outside = a[2];
        apply = a[3];
    } else if (field == 8) {
        for (unsigned i = 0; i < 3; ++i) v->pending_cone.direction[i] = F32(a[i + 1]);
        apply = a[4];
    } else {
        v->pending_cone.volume = int32_t(a[1]);
        apply = a[2];
    }
    v->cone_dirty |= field;
    // Retail tests bit zero and commits the entire pending voice on immediate.
    if (!(apply & 1)) CommitVoice3D(*v);
    return 0;
}
HLE_STDCALL(UrbanChaos_BufferConeAngles, 4) { return VoiceCone(a, 4); }
HLE_STDCALL(UrbanChaos_BufferConeOrientation, 5) { return VoiceCone(a, 8); }
HLE_STDCALL(UrbanChaos_BufferConeOutsideVolume, 3) { return VoiceCone(a, 16); }

'''


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(source, area):
    area = Path(area)
    xbe = (area / 'assets/Default.xbe').read_bytes()
    u32 = lambda at: struct.unpack_from('<I', xbe, at)[0]
    table = u32(0x120) - u32(0x104)
    sections = [struct.unpack_from('<6I', xbe, table + i * 56) for i in range(u32(0x11c))]

    def image(va, size):
        section = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
        start = section[3] + va - section[1]
        return xbe[start:start + size]

    pins = {
        (0x425f5b, 36): 'c303aef6f4a50c5a1f49fb67ca807fe471df63191f8db815d3d3e75819a28a20',
        (0x425f7f, 53): 'c05fa7a32e84278366c2c50a71a60a8c1c357b69536f306dc8e24be4649a39c3',
        (0x425fb4, 32): '9ad5039859ca7ee3dfdd94a7ea6561a32f41345b447f41e03d68988bd347762f',
        (0x4253a3, 67): 'b7fd63e41c239e9557f60eb6fe10f16f078b610386d3d031f8e5812846bee5bc',
        (0x4253e6, 82): '8e39728a1e7823c630761648246fb3849bcf00ab48e79104374c2dcc693776dd',
        (0x425438, 51): 'f9088b06de17e8bacad3274dd4ddfb7b6995761e531cca1b2eeacfbf35a393cd',
        (0x4279ce, 150): 'cc34c5840206d9a5a9092e7bc544bb198d915acd97208a1a13f4df1706108f19',
        (0x427bc0, 144): '492ac4ffc2c73a92eaed05daf949e37428d0678c8520681925dc4856806fa836',
        (0x427735, 9): 'ae4a39f1f86ea5a10dc0f01ad19c2ede2c9ddf1aed58b048c690bc9f8fdebf83',
        (0x42773e, 39): '5f4549c0bec7a3dc6cc1e1bcd1796f1271d397160f7620a1898d617aa5b1eb2b',
        (0x251a80, 92): '57282331eb7236984837d2d271759ac10d2380af5b7833ad8c064d253d459887',
    }
    for (va, size), digest in pins.items():
        assert hashlib.sha256(image(va, size)).hexdigest() == digest, hex(va)

    source = once(source, '#include "xbe_voice_mode.h"',
                  '#include "xbe_voice_mode.h"\n#include "xbe_audio_cone.h"')
    source = once(source, 'struct Voice {', 'struct Voice {\n    UcCone cone, pending_cone;\n    uint32_t cone_dirty = 0;')
    source = once(source, 'struct Done {', COMMIT + 'struct Done {')
    source = once(source, 'for(auto& [id,v]:g_voices)uc_voice_mode_commit(v);',
                  'for(auto& [id,v]:g_voices)CommitVoice3D(v);')
    source = once(source, '    return uc_voice_mode(Find(a[0]),a[1],a[2]);',
                  '    Voice* v = Find(a[0]);\n    uint32_t result = uc_voice_mode(v,a[1],a[2]&1);\n'
                  '    if (!result && !(a[2]&1)) CommitVoice3D(*v);\n    return result;')
    source = once(source, '#define SYM_UrbanChaos_BufferVoiceProperties',
                  ADAPTER + '#define SYM_UrbanChaos_BufferVoiceProperties')
    source = once(source, '    float right[3] =', '    att *= ConeGain(v.cone, d);\n    float right[3] =')

    generated = (area / 'generated/game/x86_recomp.124.cpp').read_text()
    original = ''
    for address in ('00427735', '0042773E', '004279CE', '00427BC0'):
        original += between(generated, 'X86_FUNC(__imp__sub_' + address + ') {', 'X86_WEAK_FUNC')
        if address in ('00427735', '0042773E'):
            original += 'X86_FUNC(sub_' + address + ') { __imp__sub_' + address + '(ctx, base); }\n'
    title = (area / 'generated/game/x86_recomp.61.cpp').read_text()
    prefix = between(title, 'X86_FUNC(__imp__sub_00251A80) {', '\t// 00251ADC mov')
    prefix = prefix.replace('__imp__sub_00251A80', 'OriginalTitleConePrefix')
    prefix += '    title_cone_prefix_reached = true;\nloc_00251B74:\nloc_00251B75:\n    return;\n}\n'

    fixture = Path(__file__).with_name('test_xbe_audio_cone.cpp').read_text()
    substitutions = {
        '// GENERATED_OWNER': between(source, 'struct Packet {', 'struct Done {'),
        '// GENERATED_ADAPTER': ADAPTER,
        '// GENERATED_LEVELS': between(source, 'void Levels(Voice& v, float g[2][2]) {', '// a stream goes quiet'),
        '// GENERATED_MODE': between(source, '#define SYM_UrbanChaos_BufferMode', ADAPTER),
        '// GENERATED_DEFERRED': between(source, 'HLE_STDCALL(IDirectSound_CommitDeferredSettings, 1)', 'HLE_STDCALL(IDirectSound_SetDopplerFactor'),
        '// GENERATED_ORIGINAL': original,
        '// GENERATED_TITLE_PREFIX': prefix,
    }
    for marker, replacement in substitutions.items():
        fixture = once(fixture, marker, replacement)
    (area / 'src/compat/xbe_audio_cone.h').write_text(Path(__file__).with_name('xbe_audio_cone.h').read_text())
    (area / 'src/compat/test_audio_cone.cpp').write_text(fixture)
    return source
