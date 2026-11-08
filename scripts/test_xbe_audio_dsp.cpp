// Generated production mixer and image processor; no audio device or game launch.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <vector>
#include "xdk/hle.h"
#include "xdk/layout.h"
#include "xbe_voice_mode.h"
#include "xbe_audio_cone.h"
#include "xbe_audio_output.h"
#include "xbe_dsp_image.h"
#include "xbe_audio_filter.h"
namespace uc_audio_trace {
inline bool SnapshotDue(){return false;}
template<class V> void VoiceSample(const V&,const float[2][2],double,double,uint32_t,const float*){}
}
namespace logx {void Write(Level,const char*,...) {}}
namespace kernel {static std::array<uint8_t,16*1024*1024> memory{};uint8_t* g_base=memory.data();}
namespace mem {
static std::map<uint32_t,uint32_t> allocations;
bool IsCommitted(uint32_t address){return address>=0x10000&&address<kernel::memory.size();}
uint32_t AllocSystem(uint32_t bytes,uint32_t){
    bytes=(bytes+15)&~15u;uint32_t address=0x100000;
    for(const auto& [at,size]:allocations){if(address+bytes<=at)break;address=std::max(address,at+size);}
    assert(address+bytes<kernel::memory.size());allocations[address]=bytes;return address;
}
void FreeSystem(uint32_t address){assert(allocations.erase(address)==1);}
}
namespace audio {constexpr uint32_t kRate=48000;}
using namespace kernel;
#include "scripts/xbe_input_memory.h"
// GENERATED_OWNER
static std::map<uint32_t,Voice> g_voices;
static std::recursive_mutex g_lock;
static UcAudioFactors g_listener_factors;
static float g_listener[3]{},g_front[3]{0,0,1},g_top[3]{0,1,0};
static float Db(int32_t value){return value<=-10000?0.f:std::pow(10.f,value/2000.f);}
static uint32_t FramesOf(const Voice& v,uint32_t bytes){return v.tag==0x69?bytes/(36*v.channels)*64:bytes/v.align;}
// GENERATED_MATH
// GENERATED_BEFORE_LEVELS
// GENERATED_LEVELS
// GENERATED_SAMPLE
static void Complete(Voice&,Packet&,uint32_t){}
static void Deactivate(Voice& v){v.sync=false;if(v.active)v.active=v.paused=v.ending=false;}
// GENERATED_RENDER_BEFORE
// GENERATED_RENDER_AFTER
// GENERATED_HEADROOM

static std::vector<uint8_t> Image(const char* path){
    std::ifstream file(path,std::ios::binary);assert(file);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});
}
static void Load(const std::vector<uint8_t>& image){
    W32(0x10000,3);W32(0x10004,4);
    assert(EffectsDesc(image,0x10000)!=0&&dsp::Loaded());
}
static void Near(float a,float b){assert(std::abs(a-b)<2e-6f);}
static void Headroom(unsigned value){
    for(unsigned bin=0;bin<dsp::kBins;++bin){const uint32_t args[]={0,bin,value};assert(SetHeadroom(args)==0);}
}
static Voice Source(unsigned channels=1){
    Voice v;v.obj=1;v.channels=uint16_t(channels);v.align=uint16_t(channels*2);
    v.data=0x20000;v.bytes=v.play_len=v.loop_len=480*channels*2;v.playing=v.looping=true;
    for(unsigned i=0;i<480*channels;++i)W16(v.data+i*2,uint16_t(i%3?600:-300));
    return v;
}
static void Listener(bool deferred){
    const float p[]={-1000,-500,0,1.49f,.83f,-2602,.007f,200,.011f,100,100,5000};
    for(unsigned i=0;i<12;++i)W32(0x11000+i*4,i<2||i==5||i==7?uint32_t(int32_t(p[i])):std::bit_cast<uint32_t>(p[i]));
    assert(SetAudioListenerEffect(0x11000,deferred?1:0)==0);
}
static void DryParity(const std::vector<uint8_t>& image){
    for(unsigned channels:{2u,6u,8u})for(unsigned mode:{0u,1u,2u})for(bool active:{false,true}){
        ResetAudioEffects();if(active)Load(image);
        trial_audio::output_channels=channels;trial_audio::requested=mode;
        auto source=Source(2);source.is3d=true;source.p[0]=4;source.p[2]=-3;source.min_dist=2;
        Voice bus;bus.obj=2;bus.mixin=true;bus.is3d=true;bus.p[2]=-5;bus.min_dist=2;bus.volume=.3f;
        g_voices={{1,source},{2,bus}};assert(SetAudioOutput(g_voices.at(1),2)==0);
        const auto initial=g_voices;
        std::vector<float> before(480*channels),after(480*channels+2,123.f);
        RenderBefore(before.data(),480);const double position=g_voices.at(1).pos;
        g_voices=initial;Render(after.data()+1,480);
        assert(g_voices.at(1).pos==position&&after.front()==123&&after.back()==123);
        assert(!std::memcmp(before.data(),after.data()+1,before.size()*sizeof(float)));
    }
}
static void Gains(const std::vector<uint8_t>& image){
    Load(image);Headroom(1);float wet[2],dry[2][2];auto v=Source();v.nbins=1;v.bin[0]=10;
    AudioEffectLevels(v,wet);Near(wet[0],.5f);Near(wet[1],0);
    Levels(v,dry);for(auto& row:dry)for(float value:row)Near(value,0);
    float surround[2][8];SurroundLevels(v,true,surround);for(auto& row:surround)for(float value:row)Near(value,0);
    v.channels=2;v.bin[1]=10;v.nbins=2;v.volume=.4f;v.bin_mb[10]=-600;
    AudioEffectLevels(v,wet);Near(wet[0],.2f*Db(-600));Near(wet[1],wet[0]);
    v.bin_mb[10]=-10000;AudioEffectLevels(v,wet);Near(wet[0],0);
    auto source=Source(2);Voice bus;bus.obj=2;bus.mixin=true;bus.nbins=1;bus.bin[0]=10;
    bus.volume=.4f;bus.is3d=true;bus.p[2]=4;bus.min_dist=1;
    g_voices={{1,source},{2,bus}};assert(SetAudioOutput(g_voices.at(1),2)==0);
    AudioEffectLevels(g_voices.at(1),wet);
    Near(wet[0],g_voices.at(1).route_gain*.5f*.4f*.25f);Near(wet[1],wet[0]);
    g_voices.at(1).output=999;AudioEffectLevels(g_voices.at(1),wet);Near(wet[0],0);
}
static void SpatialBins(Voice& voice,bool direct,bool wet){
    voice.nbins=0;
    for(uint8_t bin:{6,8,7,9})voice.bin[voice.nbins++]=bin;
    if(direct){voice.bin[voice.nbins++]=2;voice.bin[voice.nbins++]=3;voice.bin_mb[3]=-1200;}
    if(wet){voice.bin[voice.nbins++]=10;voice.bin_mb[10]=-600;}
}
static void CrossReference(std::vector<float>& samples,unsigned channels){
    // Independent scalar recurrence from the original effect's delay/state.
    for(unsigned left:{0u,4u,6u}){
        if(left>=channels)continue;
        float memory[2][11]{},low[2]{};unsigned at=0;
        for(unsigned f=0;f<samples.size()/channels;++f){
            low[0]=.924f*low[0]-.0585f*memory[0][at];
            low[1]=.924f*low[1]-.0585f*memory[1][at];
            const float a=std::clamp(samples[f*channels+left]+low[1],-1.f,1.f);
            const float b=std::clamp(samples[f*channels+left+1]+low[0],-1.f,1.f);
            samples[f*channels+left]=memory[0][at]=a;
            samples[f*channels+left+1]=memory[1][at]=b;at=(at+1)%11;
        }
    }
}
static void DryCrosstalk(const std::vector<uint8_t>& image){
    Headroom(0);
    for(unsigned mode:{0u,1u,2u})for(bool routed:{false,true})for(bool direct:{false,true})for(bool spatial:{false,true}){
        const unsigned channels=mode==0?2:mode==1?6:8;
        trial_audio::output_channels=channels;trial_audio::requested=mode;
        auto source=Source();source.is3d=spatial;source.p[0]=3;source.p[2]=-2;source.volume=.4f;
        SpatialBins(source,direct,false);
        Voice bus;bus.obj=2;bus.mixin=true;bus.is3d=spatial;bus.p[0]=-2;bus.p[2]=-3;bus.volume=.7f;
        SpatialBins(bus,direct,false);
        g_voices={{1,source},{2,bus}};
        if(routed){
            assert(SetAudioOutput(g_voices.at(1),2)==0);
            if(direct){auto& v=g_voices.at(1);v.nbins=2;v.bin[0]=0;v.bin[1]=1;}
        }
        const auto initial=g_voices;
        std::vector<float> before(480*channels),after(480*channels);
        RenderBefore(before.data(),480);
        Load(image);W32(g_audio_effects[4].address+16,0);g_voices=initial;
        Render(after.data(),480);
        for(unsigned i=0;i<after.size();++i)Near(before[i],after[i]);
        auto speaker_only=initial;
        for(auto& [id,voice]:speaker_only){
            unsigned kept=0;const unsigned original=voice.nbins;
            for(unsigned slot=0;slot<original;++slot){
                const auto bin=voice.bin[slot];if(bin<6||bin>9)voice.bin[kept++]=bin;
            }
            if(original&&kept==0)voice.bins_empty=true;
            voice.nbins=uint8_t(kept);
        }
        std::vector<float> dry(480*channels);g_voices=speaker_only;RenderBefore(dry.data(),480);
        auto expected=before;for(unsigned i=0;i<expected.size();++i)expected[i]-=dry[i];
        CrossReference(expected,channels);for(unsigned i=0;i<expected.size();++i)expected[i]+=dry[i];
        Load(image);g_voices=initial;Render(after.data(),480);
        bool changed=false;
        for(unsigned i=0;i<after.size();++i){Near(expected[i],after[i]);changed|=std::abs(before[i]-after[i])>1e-7;}
        assert(changed);
        if(mode==2&&spatial){
            double side=0;for(unsigned f=0;f<480;++f)side+=std::abs(after[f*channels+6])+std::abs(after[f*channels+7]);
            assert(side>0);
        }
    }
    const uint32_t invalid_bin[]={0,32,1},invalid_headroom[]={0,6,8};
    assert(SetHeadroom(invalid_bin)!=0&&SetHeadroom(invalid_headroom)!=0);
    for(unsigned mode:{0u,1u,2u}){
        const unsigned channels=mode==0?2:mode==1?6:8;
        trial_audio::output_channels=channels;trial_audio::requested=mode;
        auto source=Source();source.is3d=true;source.p[0]=3;source.p[2]=-2;
        SpatialBins(source,false,false);const std::map<uint32_t,Voice> initial{{1,source}};
        std::vector<float> reference(480*channels),attenuated(480*channels);
        Load(image);Headroom(0);g_voices=initial;Render(reference.data(),480);
        for(unsigned steps:{1u,3u,7u}){
            Load(image);Headroom(steps);g_voices=initial;Render(attenuated.data(),480);
            for(unsigned i=0;i<reference.size();++i)Near(attenuated[i],std::ldexp(reference[i],-int(steps)));
        }
    }
    Headroom(1);
}
static std::vector<float> MixedSends(const std::vector<uint8_t>& image,unsigned mode,unsigned sends){
    Load(image);Listener(false);
    const unsigned channels=mode==0?2:mode==1?6:8;
    trial_audio::requested=mode;trial_audio::output_channels=channels;
    auto voice=Source();voice.is3d=true;voice.p[0]=1;voice.p[2]=-2;voice.volume=.25f;
    if(sends==2){voice.nbins=1;voice.bin[0]=10;voice.bin_mb[10]=-600;}
    else SpatialBins(voice,false,sends==3);
    g_voices={{1,voice}};std::vector<float> out(4800*channels);
    for(unsigned frame=0;frame<4800;frame+=480)Render(out.data()+frame*channels,480);
    return out;
}
static void CombinedSends(const std::vector<uint8_t>& image){
    for(unsigned mode:{0u,1u,2u}){
        const auto dry=MixedSends(image,mode,1),wet=MixedSends(image,mode,2),both=MixedSends(image,mode,3);
        for(unsigned i=0;i<both.size();++i)Near(both[i],dry[i]+wet[i]);
    }
}
static std::vector<float> Impulse(const std::vector<uint8_t>& image,unsigned chunk,bool commit){
    Load(image);Listener(true);
    if(commit)CommitAudioListenerEffect();
    constexpr unsigned frames=48000;
    std::vector<float> out(frames*dsp::kBins);out[10]=.25f;
    for(unsigned frame=0;frame<frames;frame+=chunk)dsp::Run(out.data()+frame*dsp::kBins,std::min(chunk,frames-frame));
    for(float value:out)assert(std::isfinite(value)&&std::abs(value)<=1);
    return out;
}
static void Effects(const std::vector<uint8_t>& image){
    const auto deferred=Impulse(image,480,false);
    for(unsigned f=0;f<48000;++f)for(unsigned s=0;s<6;++s)assert(deferred[f*dsp::kBins+s]==0);
    const auto small=Impulse(image,32,true),large=Impulse(image,480,true);
    assert(small==large);
    double early=0,late=0;unsigned first=48000;
    for(unsigned f=0;f<48000;++f)for(unsigned s:{0u,1u,4u,5u}){
        const double v=small[f*dsp::kBins+s];
        if(std::abs(v)>1e-8)first=std::min(first,f);
        (f<12000?early:late)+=v*v;
    }
    assert(first>0&&first<5000&&early>1e-6&&late>1e-9&&late<early);
    Load(image);Listener(false);const auto address=g_audio_effects[0].address;
    const uint32_t prior=R32(address+32);W32(0x12000,1234);
    assert(SetAudioEffectData(0,32,0x12000,4,1)==0&&R32(address+32)==prior);
    CommitAudioEffectData();assert(R32(address+32)==1234);
    assert(GetAudioEffectData(0,32,0x12010,4)==0&&R32(0x12010)==1234);
    W32(0x12000,4321);assert(SetAudioEffectData(0,32,0x12000,4,0)==0&&R32(address+32)==4321);
    assert(SetAudioEffectData(99,0,0x12000,4,0)!=0);
    assert(SetAudioEffectData(0,UINT32_MAX,0x12000,4,0)!=0);
    assert(SetAudioEffectData(0,0,UINT32_MAX-1,4,0)!=0);
    assert(SetAudioListenerEffect(0xfffffff0,0)!=0);
    W32(0x11000+12,std::bit_cast<uint32_t>(NAN));assert(SetAudioListenerEffect(0x11000,0)!=0);
    auto bad=image;bad[2072]^=1;const auto old=g_audio_effects[0].address;
    assert(!EffectsDesc(bad,0x10000)&&dsp::Loaded()&&g_audio_effects[0].address==old);
    ResetAudioEffects();assert(!dsp::Loaded()&&mem::allocations.empty());
    for(unsigned length:{0u,100u,2048u}){
        auto bad=image;bad.resize(length);assert(!EffectsDesc(bad,0x10000));
    }
    assert(!EffectsDesc(bad,0x10000)&&!dsp::Loaded());
    assert(mem::allocations.empty());
    std::printf("DSP impulse: first audible frame=%u early energy=%.9g late energy=%.9g\n",first,early,late);
}
static void MixedTail(const std::vector<uint8_t>& image){
    for(unsigned mode:{0u,1u,2u}){
        Load(image);Listener(false);
        trial_audio::requested=mode;trial_audio::output_channels=mode==0?2:mode==1?6:8;
        auto v=Source();v.nbins=1;v.bin[0]=10;v.looping=false;v.bytes=v.play_len=2;W16(v.data,8192);
        g_voices={{1,v}};const unsigned channels=trial_audio::output_channels.load();
        std::vector<float> out(480*channels+2,123.f);double energy[8]{};
        for(unsigned block=0;block<12;++block){
            Render(out.data()+1,480);assert(out.front()==123&&out.back()==123);
            assert(!g_voices.at(1).playing);
            for(unsigned f=0;f<480;++f)for(unsigned c=0;c<channels;++c){
                const float value=out[1+f*channels+c];assert(std::isfinite(value));
                if(block)energy[c]+=double(value)*value;
            }
        }
        assert(energy[0]>0&&energy[1]>0);
        if(mode)assert(energy[4]>0&&energy[5]>0&&energy[2]==0&&energy[3]==0);
        if(mode==2)assert(energy[6]==0&&energy[7]==0);
    }
    ResetAudioEffects();
}
int main(int argc,char** argv){
    assert(argc==2);const auto image=Image(argv[1]);
    DryParity(image);Gains(image);DryCrosstalk(image);CombinedSends(image);Effects(image);MixedTail(image);
    std::puts("PASS: exact unaffected direct stereo/5.1/7.1, authored spatial crosstalk/7.1 placement, routed MIXIN samples/cursors, nonduplicated dry+wet sends, deferred listener/effect data, deterministic impulse tail, reload cleanup and malformed input bounds");
}
