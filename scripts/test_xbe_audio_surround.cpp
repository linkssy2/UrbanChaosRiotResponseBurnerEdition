// Actual production gain, decoder and render bodies. No sound device opened.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <deque>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>
#include "xdk/hle.h"
#include "xbe_voice_mode.h"
#include "xbe_audio_cone.h"
#include "xbe_audio_output.h"
#include "app/app.h"
namespace uc_audio_trace {
inline bool SnapshotDue(){return false;}
template<class V> void VoiceSample(const V&,const float[2][2],double,double,uint32_t,const float*){}
}
namespace kernel {static std::array<uint8_t,65536> memory{};uint8_t* g_base=memory.data();}
namespace mem {void FreeSystem(uint32_t){assert(false);}}
namespace audio {constexpr uint32_t kRate=48000;}
using namespace kernel;
// GENERATED_OWNER
static std::map<uint32_t,Voice> g_voices;
static std::recursive_mutex g_lock;
static UcAudioFactors g_listener_factors;
static float g_listener[3]{},g_front[3]{0,0,1},g_top[3]{0,1,0};
static float Db(int32_t v){return v<=-10000?0.f:std::pow(10.f,v/2000.f);}
static uint32_t FramesOf(const Voice& v,uint32_t bytes){return v.tag==0x69?bytes/(36*v.channels)*64:bytes/v.align;}
// GENERATED_MATH
// GENERATED_LEVELS
// GENERATED_SAMPLE
static unsigned completed;
static std::vector<uint32_t> completed_packets;
static void Complete(Voice&,Packet& packet,uint32_t){++completed;completed_packets.push_back(packet.context);}
static void Deactivate(Voice& v){v.sync=false;if(v.active)v.active=v.paused=v.ending=false;}
// GENERATED_RENDER_BEFORE
// GENERATED_RENDER_AFTER
namespace app {
static bool fail_save;static unsigned saves,saved;
std::recursive_mutex& UiLock(){static std::recursive_mutex lock;return lock;}
void Save(){if(fail_save)throw std::runtime_error("disk full");++saves;saved=TrialSoundOutput;}
}
static void Near(float a,float b){assert(std::abs(a-b)<2e-6f);}
static Voice Make(unsigned channels=1){
    Voice v;v.obj=1;v.channels=uint16_t(channels);v.align=uint16_t(2*channels);
    v.data=0x1000;v.bytes=v.play_len=v.loop_len=512*channels;v.playing=v.looping=true;
    for(unsigned i=0;i<256*channels;++i)W16(v.data+2*i,uint16_t(i%3?8192:-4096));
    return v;
}
static void StateEqual(const std::map<uint32_t,Voice>& expected){
    assert(g_voices.size()==expected.size());
    for(auto& [id,v]:g_voices){const auto& e=expected.at(id);
        assert(v.pos==e.pos&&v.playing==e.playing&&v.active==e.active&&v.paused==e.paused&&v.sync==e.sync&&v.ending==e.ending);
        assert(v.refs==e.refs&&v.output==e.output&&v.data==e.data&&v.owned==e.owned&&v.callback==e.callback&&v.context==e.context);
        assert(v.packets.size()==e.packets.size());
        for(size_t i=0;i<v.packets.size();++i){const auto& a=v.packets[i];const auto& b=e.packets[i];
            assert(a.data==b.data&&a.size==b.size&&a.used==b.used&&a.completed_ptr==b.completed_ptr&&a.status_ptr==b.status_ptr&&a.context==b.context);
        }
    }
}
static void LiveSwitches(){
    using namespace trial_audio;
    constexpr unsigned modes[]={Stereo,Surround51,Surround71,Stereo,Surround71,Surround51,Stereo,Surround71};
    auto loop=Make();loop.is3d=true;loop.p[0]=10;loop.p[2]=-4;loop.min_dist=100;loop.pos=19.25;loop.freq=44100;
    auto stream=loop;stream.obj=2;stream.stream=stream.active=stream.ending=true;stream.pos=0;stream.freq=0;
    stream.callback=0x1234;stream.context=0x5678;
    stream.packets={{0x3000,1800,0x8000,0x8010,11},{0x3708,1800,0x8004,0x8014,22},{0x3e10,1800,0x8008,0x8018,33}};
    for(unsigned i=0;i<2700;++i)W16(0x3000+2*i,4096);
    const std::map<uint32_t,Voice> initial={{1,loop},{2,stream}};
    std::vector<std::map<uint32_t,Voice>> states;
    std::vector<std::vector<uint32_t>> completions;
    std::vector<float> before(480*2);
    g_voices=initial;completed=0;completed_packets.clear();
    for(unsigned mode:modes){
        (void)mode;RenderBefore(before.data(),480);states.push_back(g_voices);completions.push_back(completed_packets);
    }
    assert((completed_packets==std::vector<uint32_t>{11,22,33}));
    for(unsigned channels:{2u,6u,8u}){
        output_channels=channels;g_voices=initial;completed=0;completed_packets.clear();
        std::vector<float> out(480*channels+2,123.f);std::string message;unsigned block=0;
        for(unsigned mode:modes){
            assert(Apply(mode,message)&&requested==mode);
            Render(out.data()+1,480);StateEqual(states[block]);
            assert(completed_packets==completions[block++]&&completed==completed_packets.size());
            assert(out.front()==123&&out.back()==123);
            float energy[8]{};
            for(unsigned frame=0;frame<480;++frame)for(unsigned channel=0;channel<channels;++channel){
                const float sample=out[1+frame*channels+channel];assert(std::isfinite(sample)&&std::abs(sample)<=1);
                energy[channel]+=sample*sample;
            }
            assert(energy[0]+energy[1]>0&&energy[2]==0&&energy[3]==0);
            const unsigned active=Active(mode,channels);
            assert((energy[4]+energy[5]>0)==(active!=Stereo));
            assert((energy[6]+energy[7]>0)==(active==Surround71));
        }
    }
}
static void Parity(std::map<uint32_t,Voice> initial){
    std::vector<float> before(600*2);
    g_voices=initial;completed=0;RenderBefore(before.data(),600);
    const auto expected=g_voices;const auto done=completed;
    for(unsigned channels:{2u,6u,8u}){
        trial_audio::output_channels=channels;trial_audio::requested=trial_audio::Stereo;
        std::vector<float> out(600*channels+2,123.f);
        g_voices=initial;completed=0;Render(out.data()+1,600);StateEqual(expected);assert(done==completed);
        assert(out.front()==123&&out.back()==123);
        for(unsigned f=0;f<600;++f)for(unsigned s=0;s<channels;++s)
            assert(out[1+f*channels+s]==(s<2?before[f*2+s]:0.f));
        for(unsigned mode:{trial_audio::Surround51,trial_audio::Surround71}){
            trial_audio::requested=mode;g_voices=initial;completed=0;Render(out.data()+1,600);
            StateEqual(expected);assert(done==completed&&out.front()==123&&out.back()==123);
            for(unsigned i=1;i<out.size()-1;++i)assert(std::isfinite(out[i])&&std::abs(out[i])<=1);
        }
    }
}
int main(){
    using namespace trial_audio;
    for(unsigned mask:{0u,3u,0x33u,0x600u})assert(Capacity(mask)==2);
    for(unsigned mask:{0x3fu,0x60fu,0xffu})assert(Capacity(mask)==6);
    assert(Capacity(0x63f)==8&&ValidMode(99)==Stereo);
    for(unsigned cap:{2u,6u,8u})for(unsigned m=0;m<3;++m){
        assert(Active(m,cap)==(cap==2?0:m==2&&cap==6?1:m));
    }
    std::string message;
    TrialSoundOutput=999;Initialize();assert(TrialSoundOutput==0&&requested==0);
    for(unsigned m=0;m<3;++m){assert(Apply(m,message)&&app::saved==m&&requested==m);Initialize();assert(requested==m);}
    app::fail_save=true;assert(!Apply(0,message)&&TrialSoundOutput==2&&requested==2);app::fail_save=false;
    assert(!Apply(999,message)&&TrialSoundOutput==2&&app::saves==3);
    LiveSwitches();
    for(unsigned channels:{1u,2u}){
        auto v=Make(channels);Parity({{1,v}});
        v.is3d=true;v.p[0]=4;v.p[2]=-7;Parity({{1,v}});
        v.stream=true;v.active=v.ending=true;v.packets={{v.data,32,0,0,0},{v.data+32,48,0,0,0}};Parity({{1,v}});
        v=Make(channels);v.tag=0x69;v.bytes=v.play_len=v.loop_len=72*channels;
        std::memset(HostPtr(v.data),0,v.bytes);Parity({{1,v}});
    }
    float a[2][8],b[2][8],stereo[2][2];auto v=Make();v.is3d=true;v.min_dist=100;
    // Full turns retain the original distance/cone/left-right energy. 7.1
    // redistributes only rear energy; no synthesized centre, LFE or stereo rear.
    for(int deg=-180;deg<=180;++deg){
        const float r=deg*.017453292519943295f;v.p[0]=std::sin(r)*10;v.p[2]=std::cos(r)*10;
        LevelsDirect(v,stereo);SurroundLevelsDirect(v,false,a);SurroundLevelsDirect(v,true,b);
        for(unsigned c=0;c<2;++c){
            Near(a[c][2],0);Near(a[c][3],0);Near(b[c][2],0);Near(b[c][3],0);
            for(unsigned side=0;side<2;++side){
                Near(a[c][side],b[c][side]);
                Near(a[c][side]*a[c][side]+a[c][4+side]*a[c][4+side],stereo[c][side]*stereo[c][side]);
                Near(b[c][4+side]*b[c][4+side]+b[c][6+side]*b[c][6+side],a[c][4+side]*a[c][4+side]);
            }
        }
    }
    v.p[0]=10;v.p[2]=0;SurroundLevelsDirect(v,true,b);assert(b[0][7]>.7f&&b[0][5]<1e-6&&b[0][6]==0);
    v.p[0]=0;v.p[2]=-10;SurroundLevelsDirect(v,true,b);assert(b[0][4]>.99f&&b[0][5]>.99f&&b[0][6]<1e-6);
    v.mode=1;SurroundLevelsDirect(v,true,a);g_listener[0]=200;SurroundLevelsDirect(v,true,b);assert(!std::memcmp(a,b,sizeof(a)));
    g_listener[0]=0;v.mode=0;v.p[0]=10;v.p[2]=0;g_front[0]=1;g_front[2]=0;SurroundLevelsDirect(v,true,b);
    assert(b[0][0]>.99f&&b[0][4]==0&&b[0][6]==0);g_front[0]=0;g_front[2]=1;
    for(unsigned bin=0;bin<10;++bin){
        v=Make();v.nbins=1;v.bin[0]=uint8_t(bin);SurroundLevelsDirect(v,true,b);
        const unsigned dest=bin<6?bin:bin<8?bin-6:bin-4;
        for(unsigned s=0;s<8;++s)Near(b[0][s],s==dest?1.f:0.f);
    }
    auto source=Make(2);source.obj=1;
    Voice bus;bus.obj=2;bus.mixin=true;bus.is3d=true;bus.p[2]=-5;bus.min_dist=1;
    Voice parent=bus;parent.obj=3;parent.volume=.4f;
    g_voices={{1,source},{2,bus},{3,parent}};
    assert(SetAudioOutput(g_voices.at(1),2)==0&&SetAudioOutput(g_voices.at(2),3)==0);
    assert(SetAudioOutput(g_voices.at(3),1)!=0);
    SurroundLevels(g_voices.at(1),true,b);Near(b[0][4],.5f*.2f*.4f*.2f);Near(b[1][4],b[0][4]);
    Parity(g_voices);
    g_voices.at(1).output=999;SurroundLevels(g_voices.at(1),true,b);
    for(auto& row:b)for(float gain:row)assert(gain==0);
    auto spread=RearSpread(NAN,0);assert(spread[0]==1&&spread[1]==0);
    std::puts("PASS: persisted modes/failure rollback; consecutive live switches preserve voices, cursors and packet completion order; exact stereo PCM/ADPCM; 2/6/8 frame bounds, clocks and streams; 361 directions; 7.1 energy; direct centre/LFE; listener and nested MIXIN routing");
}
