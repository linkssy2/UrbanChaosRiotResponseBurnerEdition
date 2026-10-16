// Production route/lifetime/mixer bodies and original title/API caller pieces.
// No output device; this verifies the lost gain path, not flame audibility.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <deque>
#include <map>
#include <set>
#include <vector>
#include "xdk/hle.h"
#include "scripts/xbe_voice_mode.h"
#include "scripts/xbe_audio_cone.h"
namespace kernel { static std::array<uint8_t,0x500000> memory{}; uint8_t* g_base=memory.data(); }
namespace mem { static std::set<uint32_t> freed; void FreeSystem(uint32_t value){assert(freed.insert(value).second);} void FreeContiguous(uint32_t value){assert(freed.insert(value).second);} }
namespace audio { constexpr uint32_t kRate=48000; }
using namespace kernel;
// GENERATED_OWNER
static std::map<uint32_t,Voice> g_voices;
static std::recursive_mutex g_lock;
static UcAudioFactors g_listener_factors;
static float g_listener[3]{},g_front[3]{0,0,1},g_top[3]{0,1,0};
static float Db(int32_t v){return v<=-10000?0.f:std::pow(10.f,v/2000.f);}
// GENERATED_MATH
// GENERATED_LEVELS
// GENERATED_BINS
static uint32_t FramesOf(const Voice& v,uint32_t bytes){return bytes/v.align;}
static float Sample(Voice&,uint32_t,uint32_t,uint32_t){return .5f;}
static uint32_t completed=0;
static void Complete(Voice&,Packet&,uint32_t){++completed;}
static void Deactivate(Voice& v){v.sync=false;if(v.active)v.active=v.paused=v.ending=false;}
struct Done {uint32_t voice,callback,context,packet_context,status;};
static std::deque<Done> g_done;
static std::vector<Done> t_flushed;
constexpr uint32_t kFlushed=0x80004004;
// GENERATED_RELEASE
// GENERATED_RENDER

HLE_STDCALL(IDirectSoundBuffer_SetOutputBuffer,2){
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    auto it=g_voices.find(a[0]);return it==g_voices.end()?0x80004005u:SetAudioOutput(it->second,a[1]);
}
#define SYM_UrbanChaos_StreamOutput 00426156
HLE_STDCALL(UrbanChaos_StreamOutput,2){
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    auto it=g_voices.find(a[0]);return it==g_voices.end()||!it->second.stream?0x80070057u:SetAudioOutput(it->second,a[1]);
}
// GENERATED_ORIGINAL
static uint32_t parent_buffer;
static void FixtureQuery(X86Context& ctx,uint8_t*,uint32_t target){
    assert(target==0x12345678&&R32(ctx.esp+4)==0x4fee88&&R32(ctx.esp+8)==0);
    ctx.eax=parent_buffer;ctx.esp+=12;
}
#define x86_icall FixtureQuery
// GENERATED_TITLE
#undef x86_icall

static Voice& Make(uint32_t id,bool bus=false){
    auto& v=g_voices[id];v.obj=id;v.mixin=bus;v.headroom=1;
    if(!bus){v.data=0x14000;v.bytes=v.play_len=v.loop_len=16;v.playing=v.looping=true;}
    return v;
}
static void Release(uint32_t id){
    auto& v=g_voices.at(id);if(v.stream)ReleaseStream(v);else ReleaseBuffer(v);
}
static void Near(float a,float b){assert(std::abs(a-b)<1e-6f);}
static void Caller(uint32_t source,uint32_t stream,uint32_t bus){
    X86Context c{};c.fcw=0x37f;c.edi=0x11000;c.esi=bus?0x12000:0;c.esp=0x18000;
    W32(c.edi+0x20,source);W32(c.edi+0x24,stream);W32(0x12000,0x12100);W32(0x12100,0x12345678);
    W32(c.esp,13);W32(c.esp+4,12);W32(c.esp+8,11);W32(c.esp+12,0xfedcba98);W32(c.esp+16,bus);
    parent_buffer=bus;OriginalTitleOutput(c,g_base);
    assert(c.esp==0x18000+20&&c.edi==13&&c.esi==12&&c.ebx==11);
}
int main(){
    // GENERATED_CONSTANTS
    assert(AudioIsMixin(0x2000,0)&&AudioIsMixin(0x2010,0));
    assert(!AudioIsMixin(0x2000,16)&&!AudioIsMixin(0x80000,0)&&!AudioIsMixin(0,0));
    auto& bus=Make(1,true);auto& child=Make(2);auto& stream=Make(3);stream.stream=true;
    stream.active=true;stream.packets.push_back({0x14000,64,0,0,0});
    child.bin_mb[0]=child.bin_mb[1]=-6000;child.nbins=2;child.bin[0]=0;child.bin[1]=1;
    Caller(2,3,1);assert(child.output==1&&stream.output==1&&bus.refs==3&&child.nbins==0);
    float g[2][2];Levels(child,g);Near(g[0][0],1);Near(g[0][1],1);Near(g[1][0],0);
    float previous=2;
    for(int mb:{0,-602,-1200,-6000,-10000}){
        bus.bin_mb[0]=bus.bin_mb[1]=mb;Levels(child,g);
        Near(g[0][0],Db(mb));assert(g[0][0]<=previous);previous=g[0][0];
    }
    bus.bin_mb[0]=bus.bin_mb[1]=0;Levels(child,g);Near(g[0][0],1);
    // Parent ownership release cannot cut off a still-routed child or stream.
    Release(1);assert(g_voices.at(1).refs==2&&!mem::freed.contains(1));
    assert(!SetAudioOutput(child,1)&&g_voices.at(1).refs==2);
    bus.playing=true;float samples[8];Render(samples,4);
    assert(bus.playing&&bus.pos==0&&child.playing&&completed==0);Near(samples[0],1);
    // Stereo normalization matches the original SDK's float->millibel helper.
    child.channels=2;assert(!SetAudioOutput(child,0));assert(child.bins_empty);
    Levels(child,g);Near(g[0][0],0);assert(!SetAudioOutput(child,1));
    X86Context c{};c.fcw=0x37f;c.esp=0x19000;W32(c.esp+4,std::bit_cast<uint32_t>(.5f));
    __imp__sub_0042396B(c,g_base);Near(child.route_gain,Db(int32_t(c.eax)));
    Levels(child,g);Near(g[0][0],child.route_gain);Near(g[1][0],child.route_gain);
    // The original SetMixBins can add speaker sends while the mono bus remains.
    W32(0x13000,2);W32(0x13004,0x13010);W32(0x13010,0);W32(0x13014,0);W32(0x13018,1);W32(0x1301c,0);
    SetBins(child,0x13000);Levels(child,g);Near(g[0][0],1+child.route_gain);
    Caller(2,0,0);assert(!child.bins_empty);Levels(child,g);Near(g[0][0],1);
    Caller(0,3,0);assert(stream.bins_empty&&!g_voices.contains(1)&&mem::freed.contains(1));
    // Detached routing cannot accidentally attach to a later object at same ID.
    auto& replacement=Make(1,true);mem::freed.erase(1);replacement.volume=.1f;Levels(stream,g);Near(g[0][0],0);
    stream.bin_mb[0]=stream.bin_mb[1]=-10000;
    SetBins(stream,0);Levels(stream,g);Near(g[0][0],1);
    auto& b2=Make(4,true);auto& b3=Make(5,true);
    assert(!SetAudioOutput(b2,5));assert(SetAudioOutput(b3,4)==0x80070057u&&b3.output==0);
    assert(SetAudioOutput(b2,4)==0x80070057u);assert(SetAudioOutput(child,0xdead)==0x80070057u);
    assert(SetAudioOutput(child,3)==0x80070057u);assert(SetAudioOutput(child,2)==0x80004001u);
    assert(!SetAudioOutput(child,4));b2.volume=.5f;b3.volume=.25f;Levels(child,g);Near(g[0][0],child.route_gain*.125f);
    W32(0x13000,8);for(unsigned i=0;i<8;++i){W32(0x13010+i*8,i);W32(0x13014+i*8,0);}
    SetBins(child,0x13000);assert(child.nbins==6); // room for stereo mono-bus send
    child.nbins=0;
    // A target's spatial fade and immediate-parent pitch/Doppler reach sources.
    b3.is3d=true;b3.min_dist=1;b3.max_dist=20;b3.p[2]=10;Levels(child,g);Near(g[0][0],child.route_gain*.0125f);
    b2.freq=24000;Near(float(RoutedPlaybackStep(child)),.5f);
    // Parent and child pitch/Doppler contributions clamp only after summing.
    child.freq=96000;b2.is3d=true;b2.p[2]=10;b2.factors.value[5]=400;
    assert(std::abs(RoutedPlaybackStep(child)-std::exp2(-32767/4096.0))<1e-12);
    Release(4);Release(5);Release(2);assert(!g_voices.contains(4)&&!g_voices.contains(5));
    assert(g_voices.contains(1)&&g_voices.contains(3));Release(1);Release(3);assert(g_voices.empty());
    std::puts("audio routing: original buffer/stream caller, SDK normalization, authored parent fade, gain chains, detach, cycles, refs/reuse, direct sends, pitch, and production mixer passed");
}
