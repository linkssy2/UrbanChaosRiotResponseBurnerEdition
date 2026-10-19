"""Opt-in bounded audio evidence capture; changes no voice/codec/output policy."""
from pathlib import Path


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(source, output, area):
    """Run after audio routing preparation. Return private dsound, xaudio2."""
    area = Path(area)
    original = between(source, 'void Render(float* out, uint32_t frames) {', '\nint64_t NowMs()')
    traced = once(original, 'void Render(float* out, uint32_t frames) {',
                  'template<bool Track>\nvoid RenderTracked(float* out, uint32_t frames) {')
    traced = once(traced, '        if (v.mixin) continue; // gain bus, not a zero-length PCM voice',
                  '        if (v.mixin) {\n'
                  '            if constexpr(Track) { float bus[2][2]; Levels(v,bus); uc_audio_trace::VoiceSample(v,bus,0,0,0,g_listener); }\n'
                  '            continue; // gain bus, not a zero-length PCM voice\n        }')
    traced = once(traced, '        if (v.paused || v.sync || (!v.stream && !v.playing)) continue;',
                  '        if (v.paused || v.sync || (!v.stream && !v.playing)) {\n'
                  '            if constexpr(Track) { const float zero[2][2]{}; uc_audio_trace::VoiceSample(v,zero,0,0,0,g_listener); }\n'
                  '            continue;\n        }')
    traced = once(traced, '        double step = RoutedPlaybackStep(v);',
                  '        double step = RoutedPlaybackStep(v);\n'
                  '        // The mixer lock keeps buffer loop bounds fixed for this output block.\n'
                  '        const uint32_t start = !v.stream && v.looping ? FramesOf(v,v.loop_start) : 0;\n'
                  '        const uint32_t end = !v.stream ? FramesOf(v,v.looping ? v.loop_start+v.loop_len : v.play_len) : 0;\n'
                  '        double trace_source=0, trace_mixed=0; uint32_t trace_frames=0;')
    traced = once(traced, '                uint32_t start = v.looping ? FramesOf(v, v.loop_start) : 0;\n'
                         '                uint32_t end = FramesOf(v, v.looping ? v.loop_start + v.loop_len : v.play_len);\n', '')
    traced = once(traced, '            out[i * 2] += a * g[0][0] + b * g[1][0];',
                  '            if constexpr(Track) {\n'
                  '                trace_source += double(a)*a + double(b)*b;\n'
                  '                float l=a*g[0][0]+b*g[1][0], r=a*g[0][1]+b*g[1][1];\n'
                  '                trace_mixed += double(l)*l + double(r)*r; ++trace_frames;\n'
                  '            }\n'
                  '            out[i * 2] += a * g[0][0] + b * g[1][0];')
    traced = once(traced, '            v.pos += step;\n        }\n    }',
                  '            v.pos += step;\n        }\n'
                  '        if constexpr(Track) uc_audio_trace::VoiceSample(v,g,trace_source,trace_mixed,trace_frames,g_listener);\n'
                  '    }')
    source = once(source, original, traced + '\nvoid Render(float* out,uint32_t frames) {\n'
                  '    if(uc_audio_trace::SnapshotDue()) RenderTracked<true>(out,frames);\n'
                  '    else RenderTracked<false>(out,frames);\n}\n')
    source = once(source, '#include "audio/audio.h"',
                  '#include "audio/audio.h"\n#include "xbe_audio_loop.h"')
    for marker, event in [
        ('BUFFER(Play, 4) {', 'Play'), ('BUFFER(Stop, 1) {', 'Stop'),
        ('BUFFER(StopEx, 4) {', 'StopEx'), ('static void FlushStream(Voice& v) {', 'Flush'),
        ('STREAM(Pause, 2) {', 'Pause'),
    ]:
        argument = 'a[3]' if event in ('Play', 'StopEx') else 'a[1]' if event == 'Pause' else '0'
        source = once(source, marker, marker + f'\n    uc_audio_trace::Event(uc_audio_trace::{event},v,{argument});')
    for marker in ['BUFFER(Release, 1) {', 'STREAM(Release, 1) {']:
        old = marker + '\n    if (--v.refs > 0) return uint32_t(v.refs);'
        source = once(source, old, old + '\n    uc_audio_trace::Event(uc_audio_trace::Release,v);')
    source = once(source, '    v.packets.push_back(pk);',
                  '    v.packets.push_back(pk);\n    uc_audio_trace::Event(uc_audio_trace::Process,v,pk.size);')
    source = once(source, 'BUFFER(SetOutputBuffer, 2) { return SetAudioOutput(v, a[1]); }',
                  'BUFFER(SetOutputBuffer, 2) { uc_audio_trace::Event(uc_audio_trace::Route,v,a[1]); return SetAudioOutput(v, a[1]); }')
    source = once(source, '    return SetAudioOutput(*v, a[1]);',
                  '    uc_audio_trace::Event(uc_audio_trace::Route,*v,a[1]);\n    return SetAudioOutput(*v, a[1]);')

    output = once(output, '#include "audio/audio.h"',
                  '#include "audio/audio.h"\n#include "xbe_audio_loop.h"')
    output = once(output, '            render(b, kFrames);',
                  '            const uint64_t trace_start=uc_audio_trace::Active()?uc_audio_trace::ClockMicros():0;\n'
                  '            render(b, kFrames);\n'
                  '            const uint64_t trace_render=trace_start?uc_audio_trace::ClockMicros()-trace_start:0;')
    output = once(output, '            voice->SubmitSourceBuffer(&xb);',
                  '            const HRESULT submitted=voice->SubmitSourceBuffer(&xb);\n'
                  '            if(trace_start) uc_audio_trace::OutputState(st.BuffersQueued,trace_render,uint32_t(submitted));')

    target = area / 'src/compat'
    target.mkdir(parents=True, exist_ok=True)
    for name in ['xbe_audio_loop.h', 'xbe_audio_loop.cpp']:
        (target / name).write_text(Path(__file__).with_name(name).read_text())
    fixture = Path(__file__).with_name('test_xbe_audio_loop.cpp').read_text()
    fixture = once(fixture, '// GENERATED_ORIGINAL_RENDER', original.replace('void Render(', 'void RenderOriginal(', 1))
    fixture = once(fixture, '// GENERATED_TRACED_RENDER', traced)
    (target / 'test_audio_loop.cpp').write_text(fixture)
    return source, output
