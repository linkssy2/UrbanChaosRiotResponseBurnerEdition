// Retail status/gate and actual host queue/mixer lifecycle. Samples are a
// deterministic constant; this verifies status and packet timing, not audio fidelity.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <vector>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"
#include "xbe_stream_status.h"
namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;
struct Packet { uint32_t data, size, completed_ptr, status_ptr, context, used = 0; };
struct Voice {
    uint32_t obj=1, data=0, play_start=0, loop_start=0, loop_len=0, play_len=0;
    uint32_t max_packets=2;
    bool active=false, ending=false, paused=false, sync=false;
    bool stream=true, playing=false, looping=false;
    double pos=0;
    std::deque<Packet> packets;
};
struct Done { uint32_t voice, status; };
static std::vector<Done> g_done, t_flushed;
static std::map<uint32_t,Voice> g_voices;
static std::recursive_mutex g_lock;
constexpr uint32_t kPacketPending=0x8000000a, kFlushed=0x80004004;
static void Complete(Voice& v, Packet& p, uint32_t status) {
    if(p.completed_ptr) W32(p.completed_ptr,p.used);
    if(p.status_ptr) W32(p.status_ptr,status);
    g_done.push_back({v.obj,status});
}
static uint32_t FramesOf(const Voice&, uint32_t bytes) { return bytes/2; }
static double AudioPlaybackStep(const Voice&) { return 1; }
static float Sample(Voice&, uint32_t, uint32_t, uint32_t) { return 0.25f; }
static void Levels(const Voice&, float g[2][2]) {
    g[0][0]=g[1][1]=1;g[0][1]=g[1][0]=0;
}
// GENERATED_RENDER
// GENERATED_FLUSH
using FixtureArgs=std::array<uint32_t,4>;
#define STREAM(name,n) static uint32_t str_##name(Voice& v,const FixtureArgs& a)
// GENERATED_STREAM

[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*) { std::abort(); }
static bool old_status=false;
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target) {
    assert(target==0x4245e9);
    uint32_t object=R32(ctx.esp+4),output=R32(ctx.esp+8);
    assert(object==0x12000);
    auto& v=g_voices.at(1);
    if(old_status){W32(output,v.packets.size()<v.max_packets?1u:0u);ctx.eax=0;}
    else ctx.eax=str_GetStatus(v,{object,output});
    ctx.esp+=12;
}
// GENERATED_ORIGINAL
// GENERATED_TITLE
static uint32_t Retail(uint16_t flags,bool capacity) {
    X86Context ctx{};ctx.esp=0x60000;ctx.ecx=0x10000;
    W16(ctx.ecx+0x12,flags);W32(ctx.ecx+0xb0,ctx.ecx+0xb0+(capacity?8:0));
    W32(ctx.esp,0xeeeeeeee);W32(ctx.esp+4,0x11000);
    __imp__sub_004293B8(ctx,g_base);
    assert(ctx.esp==0x60008&&ctx.eax==0);
    return R32(0x11000);
}
static bool TitleGate() {
    X86Context ctx{};ctx.esp=0x60000;ctx.ecx=0x13000;
    ctx.ebx=11;ctx.esi=12;ctx.edi=13;ctx.ebp=14;
    W32(ctx.esp,0xeeeeeeee);W32(ctx.esp+4,0xabcdef01);
    __imp__sub_00251E10(ctx,g_base);
    assert(ctx.esp==0x60004&&R32(ctx.esp)==0xabcdef01);
    assert(ctx.ebx==11&&ctx.esi==12&&ctx.edi==13&&ctx.ebp==14);
    return uint8_t(ctx.eax)!=0;
}
static uint32_t Status() {
    assert(str_GetStatus(g_voices.at(1),{0x12000,0x11000})==0);
    return R32(0x11000);
}
static void PacketIn() {
    W32(0x15000,0x16000);W32(0x15004,4); // two PCM16 samples
    W32(0x15008,0x17000);W32(0x1500c,0x17004);W32(0x15010,0);
    assert(str_Process(g_voices.at(1),{0x12000,0x15000})==0);
    assert(R32(0x17000)==0&&R32(0x17004)==kPacketPending);
}
int main() {
    for(uint32_t flags=0;flags<65536;++flags)
        for(bool capacity:{false,true})
            assert(uc_stream::Status(uint16_t(flags),capacity)==Retail(uint16_t(flags),capacity));
    // All original flags include pending hardware stop and envelope completion.
    // The host intentionally reports neither until it actually models them.
    assert(Retail(0x8001,true)==0x10001);
    assert(Retail(0x2000,true)==0x80001);
    W32(0x13028,0x14000);W32(0x14030,0x12000);
    W32(0x12000,0x12100);W32(0x1210c,0x4245e9);
    auto& v=g_voices[1];
    assert(Status()==1&&!TitleGate());
    // Queued-but-unstarted is deliberately not claimed as playing. Existing
    // Process activates immediately, but a future staged-start path may not.
    v.packets.push_back({0x16000,4,0,0,0});v.paused=true;
    assert(Status()==1&&!TitleGate());v.paused=false;v.sync=true;
    assert(Status()==1&&!TitleGate());v.sync=false;v.packets.clear();
    str_Pause(v,{0,1});assert(Status()==1&&!TitleGate());
    PacketIn();assert(Status()==0x20001&&!TitleGate());
    str_Pause(v,{0,0});assert(Status()==0x10001&&TitleGate());
    // This exact title gate never opens with the pre-fix capacity-only reply.
    old_status=true;assert(!TitleGate());old_status=false;
    PacketIn();assert(Status()==0x10000&&TitleGate()); // no free packet slots
    str_Pause(v,{0,2});assert(Status()==0x20000&&!TitleGate());
    float samples[16]{};Render(samples,8);
    assert(v.packets.size()==2&&v.pos==0); // synchronous hold preserves queue
    v.sync=false; // IDirectSound_SynchPlayback releases this same host flag
    Render(samples,5);
    assert(v.packets.empty()&&v.active);
    assert(Status()==0x60001&&!TitleGate()); // starvation retains capacity
    assert(R32(0x17004)==0&&R32(0x17000)==4);
    PacketIn();assert(Status()==0x10001&&TitleGate());
    str_Discontinuity(v,{});assert(v.ending&&Status()==0x10001);
    Render(samples,3);
    assert(!v.active&&!v.ending&&Status()==1&&!TitleGate());
    PacketIn();str_Flush(v,{});
    assert(Status()==1&&!TitleGate()&&R32(0x17004)==kFlushed);
    assert(!t_flushed.empty());
    // A discontinuity after underrun also ends the active stream immediately.
    PacketIn();Render(samples,3);assert(Status()==0x60001);
    str_Discontinuity(v,{});assert(Status()==1&&!TitleGate());
    // Existing non-stream title paths retain their original fallback behavior.
    W32(0x14030,0);assert(!TitleGate());
    W32(0x13028,0);assert(TitleGate());
    std::puts("stream status: 131072 retail states, actual mixer/queue lifecycle and title251E10 gate passed");
}
