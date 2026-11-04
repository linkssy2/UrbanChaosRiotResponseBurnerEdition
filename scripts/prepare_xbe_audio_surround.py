"""Darkwatch-style output capacity and routing, adapted to Urban Chaos MIXINs.

Run after loop tracing and decode caching. The SDK checkout is not modified.
"""
from pathlib import Path
from prepare_xbe_audio_loop import once, between


OUTPUT_SETUP = r'''        const HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
        IXAudio2* xa=nullptr;
        IXAudio2MasteringVoice* master=nullptr;
        IXAudio2SourceVoice* voice=nullptr;
        auto stop=[&]{
            trial_audio::output_channels=2;
            if(voice)voice->DestroyVoice();
            if(master)master->DestroyVoice();
            if(xa)xa->Release();
            if(SUCCEEDED(com))CoUninitialize();
        };
        if(FAILED(XAudio2Create(&xa,0,XAUDIO2_DEFAULT_PROCESSOR))||FAILED(xa->CreateMasteringVoice(&master))){
            LOGE("audio: XAudio2 unavailable, no sound");stop();return;
        }
        DWORD mask=0;
        uint32_t channels=SUCCEEDED(master->GetChannelMask(&mask))?trial_audio::Capacity(mask):2;
        auto create=[&]{
            WAVEFORMATEXTENSIBLE wf{};
            wf.Format.wFormatTag=channels>2?WAVE_FORMAT_EXTENSIBLE:WAVE_FORMAT_IEEE_FLOAT;
            wf.Format.nChannels=WORD(channels);wf.Format.nSamplesPerSec=kRate;
            wf.Format.wBitsPerSample=32;wf.Format.nBlockAlign=WORD(channels*sizeof(float));
            wf.Format.nAvgBytesPerSec=kRate*wf.Format.nBlockAlign;
            if(channels>2){
                wf.Format.cbSize=22;wf.Samples.wValidBitsPerSample=32;
                wf.dwChannelMask=channels==8?0x63F:0x3F;
                wf.SubFormat={WAVE_FORMAT_IEEE_FLOAT,0,0x0010,{0x80,0,0,0xAA,0,0x38,0x9B,0x71}};
            }
            return xa->CreateSourceVoice(&voice,&wf.Format);
        };
        HRESULT hr=create();
        while(FAILED(hr)&&channels>2){channels=channels==8?6:2;hr=create();}
        if(FAILED(hr)){LOGE("audio: source voice unavailable, no sound");stop();return;}
        trial_audio::output_channels=channels;
        if(FAILED(voice->Start())){LOGE("audio: source voice start failed");stop();return;}
        LOGI("audio: output capacity=%u channels mask=%08X requested=%u active=%u",
            channels,unsigned(mask),trial_audio::requested.load(),trial_audio::Active());
'''


def prepare(source, output, area):
    area=Path(area);local=Path(__file__).parent;target=area/'src/compat'
    original_render=between(source,'template<bool Track>\nvoid RenderTracked(', '\nint64_t NowMs()')
    source=once(source,'#include "xbe_audio_loop.h"','#include "xbe_audio_loop.h"\n#include "xbe_audio_output.h"')
    helpers=(local/'xbe_audio_surround.inc').read_text()
    source=once(source,'// a stream goes quiet:',helpers+'\n// a stream goes quiet:')
    render=original_render
    render=once(render,'    std::memset(out, 0, frames * 2 * sizeof(float));',
        '    const uint32_t channels=trial_audio::output_channels.load();\n'
        '    const uint32_t mode=trial_audio::Active(trial_audio::requested.load(),channels);\n'
        '    std::memset(out,0,frames*channels*sizeof(float));')
    render=once(render,'        Levels(v, g);','''        float surround[2][8]{};
        if(mode==trial_audio::Stereo)Levels(v,g);
        else {
            SurroundLevels(v,mode==trial_audio::Surround71,surround);
            for(unsigned c=0;c<2;++c)for(unsigned s=0;s<2;++s)g[c][s]=surround[c][s];
        }''')
    render=once(render,'            out[i * 2] += a * g[0][0] + b * g[1][0];\n'
                       '            out[i * 2 + 1] += a * g[0][1] + b * g[1][1];',
        '''            if(mode==trial_audio::Stereo){
                out[i*channels]+=a*g[0][0]+b*g[1][0];
                out[i*channels+1]+=a*g[0][1]+b*g[1][1];
            }else for(unsigned s=0;s<channels;++s)
                out[i*channels+s]+=a*surround[0][s]+b*surround[1][s];''')
    render=once(render,'i < frames * 2','i < frames * channels')
    # Existing trace's gain/RMS columns remain explicitly front L/R; its packet
    # and clock observations still describe the same once-per-frame mix.
    source=once(source,original_render,render)
    output=once(output,'#include <xaudio2.h>','#include <xaudio2.h>\n#include <mmreg.h>\n#include "xbe_audio_output.h"')
    old=between(output,'        CoInitializeEx(', '        constexpr uint32_t kFrames')
    output=once(output,old,OUTPUT_SETUP)
    output=once(output,'bufs(kSlots * kFrames * 2)','bufs(kSlots * kFrames * channels)')
    output=once(output,'(n % kSlots) * kFrames * 2','(n % kSlots) * kFrames * channels')
    output=once(output,'xb.AudioBytes = kFrames * 8;','xb.AudioBytes = kFrames * channels * sizeof(float);')
    output=once(output,'            n++;','            if(FAILED(submitted)){LOGE("audio: submit failed %08X",unsigned(submitted));stop();return;}\n            n++;')
    for name in ('xbe_audio_output.h','xbe_audio_settings.cpp'):
        (target/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_audio_surround.cpp').read_text()
    substitutions={
        '// GENERATED_OWNER':between(source,'struct Packet {','struct Done {'),
        '// GENERATED_MATH':between(source,'// 427AAF/427765:', '// gain of voice channel'),
        '// GENERATED_LEVELS':between(source,'// gain of voice channel','// a stream goes quiet:'),
        '// GENERATED_SAMPLE':between(source,'const int kIndex[16]','void Complete('),
        '// GENERATED_RENDER_BEFORE':original_render.replace('RenderTracked','RenderBeforeTracked').replace('void Render(','void RenderBefore('),
        '// GENERATED_RENDER_AFTER':render,
    }
    for marker,body in substitutions.items():fixture=once(fixture,marker,body)
    (target/'test_audio_surround.cpp').write_text(fixture)
    fixture=(local/'test_xbe_audio_output.cpp').read_text()
    fixture=once(fixture,'// GENERATED_OUTPUT',between(output,'void StartOutput(', '\n}  // namespace audio'))
    (target/'test_audio_output.cpp').write_text(fixture)
    return source,output
