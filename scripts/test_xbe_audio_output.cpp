// Execute the production worker with fake devices: layouts, fallback, queue
// ownership and failure cleanup. This does not establish physical audibility.
#undef NDEBUG
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include "xbe_audio_output.h"
using HRESULT=int32_t;using DWORD=uint32_t;using WORD=uint16_t;using BYTE=uint8_t;
constexpr HRESULT failed=int32_t(0x80004005);
#define FAILED(v) ((v)<0)
#define SUCCEEDED(v) ((v)>=0)
constexpr int COINIT_MULTITHREADED=0,XAUDIO2_DEFAULT_PROCESSOR=0,WAVE_FORMAT_IEEE_FLOAT=3,WAVE_FORMAT_EXTENSIBLE=0xfffe;
constexpr unsigned XAUDIO2_VOICE_NOSAMPLESPLAYED=1;
struct WAVEFORMATEX {uint16_t wFormatTag,nChannels;uint32_t nSamplesPerSec,nAvgBytesPerSec;uint16_t nBlockAlign,wBitsPerSample,cbSize;};
struct Guid {uint32_t a;uint16_t b,c;uint8_t d[8];};
struct WAVEFORMATEXTENSIBLE {WAVEFORMATEX Format;union {WORD wValidBitsPerSample;} Samples;DWORD dwChannelMask;Guid SubFormat;};
struct XAUDIO2_VOICE_STATE {uint32_t BuffersQueued=0;};
struct XAUDIO2_BUFFER {uint32_t AudioBytes=0;const BYTE* pAudioData=nullptr;};
static std::mutex lock;static std::condition_variable done_cv;static bool done;
static unsigned mask,reject_above,fail_stage,width,submitted,mixed,sleeps,errors;
struct Queued {const float* data;float value;};
static std::vector<unsigned> attempts,modes;static std::deque<Queued> queued;static std::string order;
#define LOGI(...) ((void)0)
#define LOGE(...) (++errors)
namespace uc_audio_trace {
bool Active(){return false;}uint64_t ClockMicros(){return 0;}void OutputState(uint32_t,uint64_t,uint32_t){}
}
static HRESULT CoInitializeEx(void*,int){order+='I';return 0;}
static void CoUninitialize(){order+='U';{std::lock_guard guard(lock);done=true;}done_cv.notify_one();}
static void Sleep(unsigned ms){assert(ms==2);++sleeps;}
struct IXAudio2SourceVoice {
    HRESULT Start(){assert(trial_audio::output_channels==width);return fail_stage==4?failed:0;}
    void GetState(XAUDIO2_VOICE_STATE* st,unsigned){
        if(queued.size()>=3&&sleeps){
            const auto& q=queued.front();for(unsigned i=0;i<480*width;++i)assert(q.data[i]==q.value);
            queued.pop_front();
        }
        st->BuffersQueued=unsigned(queued.size());
    }
    HRESULT SubmitSourceBuffer(const XAUDIO2_BUFFER* b){
        assert(b->AudioBytes==480*width*4);const float* p=reinterpret_cast<const float*>(b->pAudioData);
        for(const auto& q:queued)assert(q.data!=p);
        const float value=float(mixed*4+trial_audio::Active());
        for(unsigned i=0;i<480*width;++i)assert(p[i]==value);
        trial_audio::requested=(submitted%3);assert(trial_audio::output_channels==width);
        if(submitted++==6)return failed;
        queued.push_back({p,value});return 0;
    }
    void DestroyVoice(){
        for(const auto& q:queued)for(unsigned i=0;i<480*width;++i)assert(q.data[i]==q.value);
        queued.clear();order+='S';
    }
};
struct IXAudio2MasteringVoice {
    HRESULT GetChannelMask(DWORD* p){*p=mask;return fail_stage==5?failed:0;}
    void DestroyVoice(){order+='M';}
};
static IXAudio2SourceVoice source;static IXAudio2MasteringVoice master;
struct IXAudio2 {
    HRESULT CreateMasteringVoice(IXAudio2MasteringVoice** out){if(fail_stage==2)return failed;*out=&master;return 0;}
    HRESULT CreateSourceVoice(IXAudio2SourceVoice** out,const WAVEFORMATEX* wf){
        attempts.push_back(wf->nChannels);assert(wf->nSamplesPerSec==48000&&wf->wBitsPerSample==32);
        assert(wf->nBlockAlign==wf->nChannels*4&&wf->nAvgBytesPerSec==48000*wf->nBlockAlign);
        if(wf->nChannels>2){
            auto& ex=*reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wf);
            assert(wf->wFormatTag==0xfffe&&wf->cbSize==22&&ex.Samples.wValidBitsPerSample==32);
            assert(ex.dwChannelMask==(wf->nChannels==8?0x63fu:0x3fu));
            assert(ex.SubFormat.a==3&&ex.SubFormat.c==0x10&&ex.SubFormat.d[7]==0x71);
        }else assert(wf->nChannels==2&&wf->wFormatTag==3&&wf->cbSize==0);
        if(fail_stage==3||wf->nChannels>reject_above)return failed;
        width=wf->nChannels;*out=&source;return 0;
    }
    void Release(){order+='A';}
};
static IXAudio2 engine;
static HRESULT XAudio2Create(IXAudio2** out,unsigned,int){if(fail_stage==1)return failed;*out=&engine;return 0;}
namespace audio {
constexpr unsigned kRate=48000;
// GENERATED_OUTPUT
}
static void Run(unsigned m,unsigned cap,unsigned fail){
    mask=m;reject_above=cap;fail_stage=fail;done=false;width=submitted=mixed=sleeps=errors=0;attempts.clear();modes.clear();order.clear();
    trial_audio::requested=2;
    audio::StartOutput([](float* out,unsigned frames){
        assert(frames==480&&trial_audio::output_channels==width);++mixed;
        modes.push_back(trial_audio::Active());
        std::fill_n(out,frames*width,float(mixed*4+modes.back()));
    });
    {std::unique_lock guard(lock);assert(done_cv.wait_for(guard,std::chrono::seconds(3),[]{return done;}));}
    assert(errors==1&&queued.empty()&&trial_audio::output_channels==2);
    if(fail==1)assert(order=="IU"&&attempts.empty());
    else if(fail==2)assert(order=="IAU"&&attempts.empty());
    else if(fail==3)assert(order=="IMAU"&&mixed==0);
    else {
        assert(order=="ISMAU");
        if(fail==4)assert(!mixed);
        else {
            assert(mixed==7&&submitted==7&&sleeps>0);
            for(unsigned i=0;i<modes.size();++i)assert(modes[i]==trial_audio::Active(i?(i-1)%3:2,width));
        }
    }
    if(fail!=1&&fail!=2){
        unsigned wanted=fail==5?2:trial_audio::Capacity(mask);size_t n=0;
        while(true){assert(n<attempts.size()&&attempts[n++]==wanted);if((fail!=3&&wanted<=cap)||wanted==2)break;wanted=wanted==8?6:2;}
        assert(n==attempts.size());
    }
}
int main(){
    for(unsigned mask:{3u,0x33u,0x3fu,0x60fu,0xffu,0x63fu})Run(mask,8,0);
    Run(0x63f,6,0);Run(0x63f,2,0);Run(0x3f,2,0);
    for(unsigned failure=1;failure<=5;++failure)Run(0x63f,8,failure);
    std::puts("PASS: production XAudio2 worker, 14 endpoint/failure cases, masks/float formats, 8-to-6-to-2 fallback, live mode switches without voice recreation, immutable queued audio and cleanup");
}
