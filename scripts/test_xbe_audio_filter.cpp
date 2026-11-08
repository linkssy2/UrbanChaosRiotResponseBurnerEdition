// Actual title packet encoder, exported setters and production resampling/mixer.
// No device is opened; host response tests do not establish APU waveform parity.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <mutex>
#include <vector>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"
#include "xbe_voice_mode.h"
#include "xbe_audio_cone.h"
#include "xbe_audio_output.h"
#include "xbe_audio_filter.h"

namespace kernel {static std::array<uint8_t, 0x800000> memory{}; uint8_t* g_base=memory.data();}
namespace mem {
bool IsCommitted(uint32_t at){return at>=0x10000 && at<kernel::memory.size();}
void FreeSystem(uint32_t){assert(false);}
}
namespace audio {constexpr uint32_t kRate=48000;}
namespace uc_audio_trace {
inline bool SnapshotDue(){return false;}
template<class V> void VoiceSample(const V&,const float[2][2],double,double,uint32_t,const float*){}
}
using namespace kernel;
#include "scripts/xbe_input_memory.h"
// GENERATED_OWNER
struct Done{};
static std::vector<Done> t_flushed;
static void Run(X86Context&,const std::vector<Done>& pending){assert(pending.empty());}
static std::map<uint32_t,Voice> g_voices;
static std::recursive_mutex g_lock;
static UcAudioFactors g_listener_factors;
static float g_listener[3]{},g_front[3]{0,0,1},g_top[3]{0,1,0};
static Voice* Find(uint32_t object){auto it=g_voices.find(object);return it==g_voices.end()?nullptr:&it->second;}
static float Db(int32_t value){return value<=-10000?0.f:std::pow(10.f,value/2000.f);}
static uint32_t FramesOf(const Voice& v,uint32_t bytes){return v.tag==0x69?bytes/(36*v.channels)*64:bytes/v.align;}
// GENERATED_MATH
// GENERATED_LEVELS
// GENERATED_SAMPLE
static void Complete(Voice&,Packet&,uint32_t){}
static void Deactivate(Voice& v){v.sync=false;if(v.active)v.active=v.paused=v.ending=false;}
// GENERATED_ADAPTER
// GENERATED_BUFFER_MACRO
// GENERATED_STREAM_MACRO
// GENERATED_BUFFER
// GENERATED_STREAM
// GENERATED_RENDER_BEFORE
// GENERATED_RENDER_AFTER

[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_002440A0){
    const uint32_t destination=R32(ctx.esp+4);
    std::memset(HostPtr(destination),int(R32(ctx.esp+8)),R32(ctx.esp+12));
    ctx.eax=destination;ctx.esp+=4;
}
X86_FUNC(sub_002D6780){ctx.eax=uint32_t(int32_t(X87_ST(0)));x87_pop(ctx);ctx.esp+=4;}
X86_FUNC(sub_002D6EB0){const double power=X87_ST(0);x87_pop(ctx);X87_ST(0)=std::pow(X87_ST(0),power);ctx.esp+=4;}
// GENERATED_ORIGINAL

using Guest=void(*)(X86Context&,uint8_t*);
static uint32_t Invoke(Guest function,std::initializer_list<uint32_t> args,uint32_t object=0,bool cdecl=false){
    X86Context ctx{};ctx.esp=0x60000;ctx.fcw=0x37f;ctx.ecx=object;
    ctx.ebx=11;ctx.esi=12;ctx.edi=13;ctx.ebp=14;
    W32(ctx.esp,0xeeeeeeee);unsigned count=1;
    for(auto value:args)W32(ctx.esp+4*count++,value);
    W32(ctx.esp+4*count,0xabcdef01);
    function(ctx,g_base);
    assert(ctx.esp==0x60000+4*(cdecl?1:count));
    assert(ctx.ebx==11&&ctx.esi==12&&ctx.edi==13&&ctx.ebp==14);
    assert(R32(0x60000+4*count)==0xabcdef01);
    return ctx.eax;
}
static std::array<uint32_t,6> PacketFor(uint32_t code){return {1,0,code,32768,code,32768};}
static void PutPacket(const std::array<uint32_t,6>& p){for(unsigned i=0;i<6;++i)W32(0x11000+i*4,p[i]);}

static void TitleAndExports(){
    // GENERATED_CONSTANTS
    auto& buffer=g_voices[1];auto& stream=g_voices[2];stream.stream=true;
    W32(0x10020,1);W32(0x10024,2);
    for(uint32_t cutoff=0;cutoff<65536;++cutoff){
        const uint32_t code=Invoke(sub_00244140,{std::bit_cast<uint32_t>(float(cutoff))},0,true);
        assert(code==0||code>=32768);
        Invoke(sub_00251D80,{cutoff,0},0x10000);
        const int signed_code=code<32768?int(code):int(code)-65536;
        const float expected=float(std::exp2(signed_code/4096.0));
        assert(buffer.filter.frequency==expected&&stream.filter.frequency==expected);
    }
    const Guest hooks[]={sub_00424F4D,sub_00424D10,sub_00425043};
    for(unsigned h=0;h<3;++h){
        const uint32_t owner=h?2:1;
        auto valid=PacketFor(0xd000);PutPacket(valid);
        assert(Invoke(hooks[h],{owner,0x11000})==0);
        auto& state=g_voices[owner].filter;
        float a=.5f,b=-.25f;AudioFilterSample(state,a,b);
        const auto before=state;
        for(unsigned word=0;word<6;++word){
            auto invalid=valid;invalid[word]^=1;PutPacket(invalid);
            assert(Invoke(hooks[h],{owner,0x11000})==0x80070057u);
            assert(std::memcmp(&before,&state,sizeof(state))==0);
        }
        assert(Invoke(hooks[h],{owner,0xfffffff8})==0x80070057u);
        assert(std::memcmp(&before,&state,sizeof(state))==0);
        assert(Invoke(hooks[h],{999,0x11000})!=0);
        PutPacket(valid);assert(Invoke(hooks[h],{owner,0x11000})==0);
        assert(std::memcmp(&before,&state,sizeof(state))==0); // repeated writes never reset
        auto bypass=PacketFor(0);PutPacket(bypass);
        assert(Invoke(hooks[h],{owner,0x11000})==0&&state.frequency==1);
        assert(state.low[0]==0&&state.band[0]==0);
    }
    g_voices[3].mixin=true;PutPacket(PacketFor(0xd000));
    assert(Invoke(sub_00424F4D,{3,0x11000})==0x80004001u);
    assert(Invoke(sub_00425043,{1,0x11000})==0x80070057u);
    for(uint32_t code=0;code<65536;++code){
        UcAudioFilter f;const auto p=PacketFor(code);
        assert(AudioFilterSet(f,p.data())==(code==0||code>=32768));
        assert(f.frequency>=1.f/256&&f.frequency<=1);
    }
    UcAudioFilter f;assert(!AudioFilterSet(f,nullptr));
    auto wide=PacketFor(65536);assert(!AudioFilterSet(f,wide.data()));
    std::puts("filter: 65536 original title packets, buffer/stream aliases and rejected writes passed");
}

static double Tone(uint32_t code,double hz){
    UcAudioFilter filter;auto p=PacketFor(code);assert(AudioFilterSet(filter,p.data()));
    double output=0,input=0;
    for(unsigned i=0;i<48000;++i){
        const float x=float(.25*std::sin(6.283185307179586*hz*i/48000));
        float a=x,b=0;AudioFilterSample(filter,a,b);
        assert(b==0&&std::isfinite(a));
        if(i>=24000){output+=double(a)*a;input+=double(x)*x;}
    }
    return std::sqrt(output/input);
}
static void Response(){
    const uint32_t code=Invoke(sub_00244140,{std::bit_cast<uint32_t>(1000.f)},0,true);
    assert(Tone(code,100)>.98&&Tone(code,100)<1.03);
    assert(Tone(code,10000)<.03&&Tone(code,4000)<.1);
    for(uint32_t c:{0x8000u,0xa000u,code,0xf000u,0xffffu}){
        UcAudioFilter f;auto p=PacketFor(c);assert(AudioFilterSet(f,p.data()));
        double low=0,band=0,sum=0;
        for(unsigned i=0;i<32768;++i){
            const float x=i?0.f:.5f;
            low+=double(f.frequency)*band;
            band+=double(f.frequency)*(x-low-band);
            float a=x,b=0;AudioFilterSample(f,a,b);
            assert(std::abs(a-low)<2e-6&&b==0&&std::isfinite(a));sum+=a;
        }
        assert(std::abs(sum-.5)<.002&&std::abs(low)<1e-8&&std::abs(band)<1e-8);
    }
    UcAudioFilter bypass;
    for(float value:{-.5f,-0.f,0.f,.25f,1.f}){
        float a=value,b=-value;const float old_a=a,old_b=b;
        AudioFilterSample(bypass,a,b);
        assert(std::bit_cast<uint32_t>(a)==std::bit_cast<uint32_t>(old_a));
        assert(std::bit_cast<uint32_t>(b)==std::bit_cast<uint32_t>(old_b));
    }
    std::puts("filter: impulse/DC response, high-frequency attenuation, channel isolation and dry identity passed");
}

static Voice Source(uint32_t rate,unsigned channels,bool stream){
    Voice v;v.obj=1;v.rate=rate;v.channels=uint16_t(channels);v.align=uint16_t(channels*2);
    v.data=0x20000;v.bytes=v.play_len=v.loop_len=768*v.align;v.playing=true;v.stream=stream;
    for(unsigned i=0;i<768;++i)for(unsigned c=0;c<channels;++c)
        W16(v.data+(i*channels+c)*2,uint16_t(int16_t((i*7919+c*1231)%14000-7000)));
    if(stream)for(unsigned i=0;i<3;++i)v.packets.push_back({v.data+i*256*v.align,256u*v.align,0,0,0});
    return v;
}
static std::vector<float> Mix(const Voice& voice,bool before,bool split){
    g_voices.clear();g_voices[1]=voice;
    std::vector<float> out(300*trial_audio::output_channels.load());
    if(split){
        unsigned at=0;
        for(unsigned count:{1u,0u,63u,65u,7u,164u}){
            Render(out.data()+at*trial_audio::output_channels.load(),count);at+=count;
        }
        assert(at==300);
    }else if(before)RenderBefore(out.data(),300);else Render(out.data(),300);
    return out;
}
static void Mixer(){
    for(uint32_t mode:{trial_audio::Stereo,trial_audio::Surround51,trial_audio::Surround71}){
        trial_audio::requested=mode;trial_audio::output_channels=mode==0?2:mode==1?6:8;
        const unsigned output_channels=trial_audio::output_channels.load();
        for(uint32_t rate:{16000u,39750u,48000u,96000u})for(unsigned channels:{1u,2u})for(bool stream:{false,true}){
            auto v=Source(rate,channels,stream);
            const auto dry=Mix(v,true,false);assert(dry==Mix(v,false,false));
            const auto p=PacketFor(0xd000);assert(AudioFilterSet(v.filter,p.data()));
            const auto filtered=Mix(v,false,false);assert(filtered==Mix(v,false,true));
            std::array<UcAudioFilter,4> expected{};
            for(auto& f:expected)assert(AudioFilterSet(f,p.data()));
            bool changed=false;
            for(unsigned i=0;i<300;++i)for(unsigned pair=0;pair<output_channels/2;++pair){
                const auto at=i*output_channels+pair*2;
                float a=dry[at],b=dry[at+1];AudioFilterSample(expected[pair],a,b);
                assert(std::abs(filtered[at]-a)<1e-6&&std::abs(filtered[at+1]-b)<1e-6);
                changed|=filtered[at]!=dry[at];
            }
            assert(changed);
        }
    }
    // Two voices with cancelling PCM must not cancel before per-voice filtering.
    trial_audio::requested=trial_audio::Stereo;trial_audio::output_channels=2;
    Voice a=Source(48000,1,false),b=a;b.obj=2;b.data=0x30000;
    for(unsigned i=0;i<768;++i)W16(b.data+i*2,uint16_t(-int16_t(R16(a.data+i*2))));
    const auto p=PacketFor(0xd000);assert(AudioFilterSet(a.filter,p.data()));
    auto expected=Mix(a,false,false);const auto other=Mix(b,false,false);
    g_voices.clear();g_voices[1]=a;g_voices[2]=b;std::vector<float> combined(600);Render(combined.data(),300);
    bool nonzero=false;for(unsigned i=0;i<600;++i){assert(combined[i]==expected[i]+other[i]);nonzero|=combined[i]!=0;}
    assert(nonzero&&g_voices[2].filter.frequency==1);
    std::puts("filter: production PCM interpolation, 16/39.75/48/96k sources, packets, blocks and 2/6/8-channel output passed");
}
int main(){TitleAndExports();Response();Mixer();}
