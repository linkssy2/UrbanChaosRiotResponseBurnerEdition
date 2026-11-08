"""Restore authored environmental and spatial DSP sends in the current mixer."""
import hashlib
from pathlib import Path
from prepare_xbe_audio_loop import once, between


def processors():
    vendor = Path(__file__).with_name('vendor') / 'xberecomp-dsp'
    expected = {'dsp_image.cpp': '34318036a0e8ebaec1678dac89124cb68bc186e317176214794cdaf99189774d',
                'dsp_image.h': '90f599985f0e32872c7ef7a54634c073343cdc5c217860b1df176c04b6fd5ecb'}
    for name, digest in expected.items():
        assert hashlib.sha256((vendor/name).read_bytes()).hexdigest() == digest, name
    source = (vendor/'dsp_image.cpp').read_text()
    header = (vendor/'dsp_image.h').read_text()
    signature = 'void Load(const std::vector<uint8_t>& img, size_t table, const std::vector<uint32_t>& states, int32_t reverb)'
    new_signature = 'bool Load(const std::vector<uint8_t>& img, size_t table, const std::vector<uint32_t>& states, const std::vector<uint32_t>& sizes, int32_t reverb, int32_t crosstalk)'
    source = once(source, signature, new_signature)
    header = once(header, signature, new_signature)
    header = once(header, '#include <cstdint>', '#include <cstdint>\n#include <cstddef>')
    header = once(header, 'bool Loaded();', 'bool Loaded();\nvoid Reset();')
    header = once(header, 'void Run(float* bins, uint32_t frames);',
                  '// Adapter-only frame slots, not guest mix-bin sends (voice capture is inactive).\n'
                  '// The host 7.1 side pair uses the retail crosstalk recurrence.\n'
                  'constexpr uint32_t kSideLeft = 30, kSideRight = 31;\nvoid Run(float* bins, uint32_t frames);')
    source = once(source, '#include "xdk/dsp_image.h"', '#include "xbe_dsp_image.h"')
    source = once(source, 'uint32_t state = 0;', 'uint32_t bytes = 0;\n    uint32_t state = 0;')
    source = once(source, 'uint32_t Word(const Effect& e, uint32_t i) { return R32(e.state + i * 4) & 0xFFFFFF; }',
                  'uint32_t Word(const Effect& e, uint32_t i) { return i < e.bytes / 4 ? R32(e.state + i * 4) & 0xFFFFFF : 0; }')
    source = once(source, '    return g_x[address];', '''    // Guest parameter writes cannot grow the DSP address map without bound.
    if (auto found = g_x.find(address); found != g_x.end()) return found->second;
    if (g_x.size() < 1024 && address < 0x10000) return g_x[address];
    static Buffer silence; silence.fill(0); return silence;''')
    source = once(source, new_signature + ' {\n    g_effects.clear();',
                  'void Reset() {\n    g_effects.clear();')
    source = once(source, '    g_reverb_index = reverb;\n    uint32_t count', '''    g_reverb_index = -1;
}

''' + new_signature + ''' {
    // Urban Chaos's sole image: two gains, its environmental reverb, crosstalk,
    // and five Xbox Live communicator capture modules (no host capture input).
    if (states.size() != 9 || sizes.size() != states.size() || reverb != 3 || crosstalk != 4) return false;
    std::vector<Effect> parsed;
    uint32_t count''')
    source = once(source, 'if (keys_at + count * kKeyBytes > img.size()) return;',
                  'if (table > img.size() || keys_at > img.size() || count * kKeyBytes > img.size() - keys_at) return false;')
    source = once(source, '        e.state = states[i];', '''        e.state = states[i]; e.bytes = sizes[i];
        if (!e.state || uint64_t(e.state) + e.bytes > 0x100000000ull || e.bytes < 92) return false;''')
    source = once(source, '            e.kind = KindOf(CodeHash(code));', '''            const uint64_t hash = CodeHash(code);
            constexpr uint64_t expected[] = {0xDDD7C789A714E88Bull,0x205DF40F9FE0159Bull,
                0xDDD7C789A714E88Bull,0x13CF05D0E014E22Eull,0xF1E0A321578CFED9ull,
                0x205DF40F9FE0159Bull,0x205DF40F9FE0159Bull,0x205DF40F9FE0159Bull,0x205DF40F9FE0159Bull};
            if (hash != expected[i]) return false;
            e.kind = KindOf(hash);''')
    source = once(source, '        }\n        kinds +=', '        } else { return false; }\n        kinds +=')
    source = once(source, '        g_effects.push_back(e);', '        parsed.push_back(e);')
    source = once(source, '    LOGR("dsound: effects image, reverb %d, effects%s", reverb, kinds.c_str());',
                  '    Reset();\n    g_effects=std::move(parsed);\n    g_reverb_index=reverb;\n'
                  '    LOGI("dsound: effects image, reverb %d, effects%s", reverb, kinds.c_str());\n    return true;')
    source = once(source, '    return "not reproduced";', '    return "inactive communicator capture";')
    source = once(source, 'std::vector<Effect> g_effects;', 'Crosstalk g_side_crosstalk;\nstd::vector<Effect> g_effects;')
    source = once(source, '    g_reverb = Reverb();', '    g_reverb = Reverb();\n    g_side_crosstalk = Crosstalk();')
    source = once(source, '        for (uint32_t b = 0; b < kSpeakers; b++) {', '''        // The Xbox image has front/rear pairs. Preserve the existing 7.1 side
        // placement using one extra host pair, without changing guest state.
        const Buffer* side_in[2] = {&At(kBinRead + kSideLeft * kBinStride), &At(kBinRead + kSideRight * kBinStride)};
        Buffer side[2]{}; Buffer* side_out[2] = {&side[0], &side[1]};
        g_side_crosstalk.Run(side_in, side_out, n, (Word(g_effects[4], kFlags) & kAddToOutput) != 0);
        for (uint32_t i = 0; i < n; ++i) {
            frame[i * kBins + kSideLeft] = side[0][i];
            frame[i * kBins + kSideRight] = side[1][i];
        }
        for (uint32_t b = 0; b < kSpeakers; b++) {''')
    source = once(source, 'void Run(float* bins, uint32_t frames) {',
                  'void Run(float* bins, uint32_t frames) {\n    if (!Loaded() || !bins) return;')
    return source, header


def prepare(source, area):
    target = Path(area)/'src/compat'
    local = Path(__file__).parent
    cpp, header = processors()
    (target/'xbe_dsp_image.cpp').write_text(cpp)
    (target/'xbe_dsp_image.h').write_text(header)
    source = once(source, '#include "xbe_audio_output.h"', '#include "xbe_audio_output.h"\n#include "xbe_dsp_image.h"')
    source = once(source, '// gain of voice channel',
                  '// Only changed under g_lock while deriving the authored spatial send.\n'
                  'static bool g_audio_split_crosstalk = false;\n\n// gain of voice channel')
    source = once(source, '// a stream goes quiet:', (local/'xbe_audio_dsp.inc').read_text()+'\n// a stream goes quiet:')
    source = once(source, '            if (speakers && (b == 3 || b > 9)) continue;',
                  '            if (dsp::Loaded() && b == 10) continue;\n'
                  '            if (g_audio_split_crosstalk && b >= 6 && b <= 9) continue;\n'
                  '            if (speakers && (b == 3 || b > 9)) continue;')
    source = once(source, '            const unsigned bin=bins[s];const float gain=',
                  '            const unsigned bin=bins[s];\n'
                  '            if(g_audio_split_crosstalk&&bin>=6&&bin<=9)continue;\n'
                  '            const float gain=')
    source = once(source, 'else if(!speakers&&!v.output)g[c][0]',
                  'else if(!speakers&&!v.output&&!(dsp::Loaded()&&bin==10))g[c][0]')
    render = between(source, 'template<bool Track>\nvoid RenderTracked(', '\nint64_t NowMs()')
    updated = once(render, '    for (auto& [obj, v] : g_voices) {',
                   '    const bool effects = dsp::Loaded();\n    if(effects)g_audio_effect_mix.assign(size_t(frames)*dsp::kBins,0.f);\n    for (auto& [obj, v] : g_voices) {')
    updated = once(updated, '        uint32_t base = v.data + v.play_start;',
                   '        float wet[2]{}, cross[2][8]{};\n'
                   '        if(effects){AudioEffectLevels(v,wet);AudioSplitCrosstalk(v,mode,g,surround,cross);}\n'
                   '        uint32_t base = v.data + v.play_start;')
    updated = once(updated, '            v.pos += step;',
                   '            if(effects)AudioMixEffectSends(i,a,b,wet,cross);\n            v.pos += step;')
    updated = once(updated, '    for (uint32_t i = 0; i < frames * channels;',
                   '    if(effects)MixAudioEffects(out,frames,channels,mode);\n    for (uint32_t i = 0; i < frames * channels;')
    source = once(source, render, updated)
    source = once(source, 'HLE_STDCALL(IDirectSound_SetEffectData, 6) { return 0; }', '''HLE_STDCALL(IDirectSound_SetEffectData, 6) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    return SetAudioEffectData(a[1],a[2],a[3],a[4],a[5]);
}''')
    source = once(source, 'HLE_STDCALL(IDirectSound_SetMixBinHeadroom, 3) { return 0; }', '''HLE_STDCALL(IDirectSound_SetMixBinHeadroom, 3) {
    if(a[1]>=dsp::kBins||a[2]>7)return 0x80070057u;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    g_audio_effect_headroom[a[1]]=uint8_t(a[2]);
    return 0;
}''')
    source = once(source, '    FactorCommit(g_listener_factors);',
                  '    FactorCommit(g_listener_factors);\n    CommitAudioListenerEffect();')
    source = once(source, 'HLE_STDCALL(IDirectSound_SetI3DL2Listener, 3) { return 0; }', '''HLE_STDCALL(IDirectSound_SetI3DL2Listener, 3) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    return SetAudioListenerEffect(a[1],a[2]);
}''')
    old = between(source, '// Effects run on the console', '// (this, DSOUTPUTLEVELS out')
    new = '''HLE_STDCALL(XAudioDownloadEffectsImage, 4) {
    if(!TrialGuestReadable(a[0],1)||!TrialGuestReadable(a[1],8)||!TrialGuestReadable(a[3],4))return 0x80070057u;
    std::string name;
    for(unsigned i=0;i<260;++i){
        if(uint64_t(a[0])+i>UINT32_MAX||!TrialGuestReadable(a[0]+i,1))return 0x80070057u;
        const char c=*static_cast<const char*>(HostPtr(a[0]+i));if(!c)break;name+=c;
    }
    if(name.size()==260)return 0x80070057u;
    std::vector<uint8_t> image;
    if(a[2]&1){
        for(const auto& section:g_image->sections)if(section.name==name&&section.vsize<=8u*1024*1024&&TrialGuestReadable(section.va,section.vsize)){
            const auto* data=static_cast<const uint8_t*>(HostPtr(section.va));image.assign(data,data+section.vsize);break;
        }
    }else if(!ReadGameFile(name,image))return 0x80070057u;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    const uint32_t descriptor=EffectsDesc(image,a[1]);W32(a[3],descriptor);
    return descriptor?0:0x80070057u;
}
HLE_STDCALL(IDirectSound_DownloadEffectsImage, 5) {
    if(a[2]>8u*1024*1024||!TrialGuestReadable(a[1],a[2])||!TrialGuestReadable(a[3],8)||!TrialGuestReadable(a[4],4))return 0x80070057u;
    const auto* data=static_cast<const uint8_t*>(HostPtr(a[1]));
    std::vector<uint8_t> image(data,data+a[2]);
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    const uint32_t descriptor=EffectsDesc(image,a[3]);W32(a[4],descriptor);
    return descriptor?0:0x80070057u;
}
HLE_STDCALL(IDirectSound_GetEffectData, 5) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    return GetAudioEffectData(a[1],a[2],a[3],a[4]);
}
HLE_STDCALL(IDirectSound_CommitEffectData, 1) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);CommitAudioEffectData();return 0;
}
'''
    source = once(source, old, new)
    # A standalone fixture exercises the actual patched mixer with no host device.
    fixture = (local/'test_xbe_audio_dsp.cpp').read_text()
    replacements = {
        '// GENERATED_OWNER': between(source,'struct Packet {','struct Done {'),
        '// GENERATED_MATH': between(source,'// 427AAF/427765:', '// gain of voice channel'),
        '// GENERATED_LEVELS': between(source,'// gain of voice channel','// a stream goes quiet:'),
        '// GENERATED_SAMPLE': between(source,'const int kIndex[16]','void Complete('),
        '// GENERATED_RENDER_BEFORE': render.replace('RenderTracked','RenderBeforeTracked').replace('void Render(','void RenderBefore('),
        '// GENERATED_RENDER_AFTER': updated,
        '// GENERATED_HEADROOM': between(source,'HLE_STDCALL(IDirectSound_SetMixBinHeadroom, 3)',
                                         'HLE_STDCALL(IDirectSound_CommitDeferredSettings, 1)')
            .replace('HLE_STDCALL(IDirectSound_SetMixBinHeadroom, 3)',
                     'static uint32_t SetHeadroom(const uint32_t* a)'),
    }
    # The before renderer needs pre-DSP levels to prove unchanged dry samples.
    fixture = once(fixture, '// GENERATED_BEFORE_LEVELS',
        replacements['// GENERATED_LEVELS'].split('// Environmental sends')[0]
        .replace('            if (dsp::Loaded() && b == 10) continue;\n','')
        .replace('            if (g_audio_split_crosstalk && b >= 6 && b <= 9) continue;\n','')
        .replace('            if(g_audio_split_crosstalk&&bin>=6&&bin<=9)continue;\n','')
        .replace('&&!(dsp::Loaded()&&bin==10)','')
        .replace('Levels(', 'BeforeLevels(').replace('LevelsDirect(', 'BeforeLevelsDirect(')
        .replace('SurroundLevels(', 'BeforeSurroundLevels(').replace('SurroundLevelsDirect(', 'BeforeSurroundLevelsDirect(')
        .replace('ReleaseAudioOutput(', 'BeforeReleaseAudioOutput(').replace('UpdateAudioRouteFormat(', 'BeforeUpdateAudioRouteFormat(')
        .replace('SetAudioOutput(', 'BeforeSetAudioOutput(').replace('AudioIsMixin(', 'BeforeAudioIsMixin(')
        .replace('AudioMonoSendGain(', 'BeforeAudioMonoSendGain(').replace('RoutedPlaybackStep(', 'BeforeRoutedPlaybackStep('))
    replacements['// GENERATED_RENDER_BEFORE'] = replacements['// GENERATED_RENDER_BEFORE'].replace('Levels(', 'BeforeLevels(').replace('SurroundLevels(', 'BeforeSurroundLevels(')
    for marker, body in replacements.items(): fixture = once(fixture, marker, body)
    (target/'test_audio_dsp.cpp').write_text(fixture)
    return source
