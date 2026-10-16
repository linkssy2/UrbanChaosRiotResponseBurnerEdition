// Actual guest sprite renderer/colour packer and production bridge. The final
// batched-quad submission is captured; this does not claim live pixel acceptance.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include "kernel/kernel.h"
#include "native/hud_health.h"
#include "native/hud_heartbeat.h"
namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
namespace mem {
static uint32_t denied=0xffffffff;
bool IsCommitted(uint32_t page){return page<kernel::memory.size()&&page!=denied;}
}
#include "../common/xbe_health_pulse.cpp"

// Accepted scalar owners previously qualified against unchanged71960 and its
// shared retail RNG. Production only reads their guest results; it never runs
// another waveform generator, advances a timer or consumes RNG for drawing.
using std::isfinite;
#include "native/game_random.c"
#include "native/hud_heartbeat.c"

X86_EXTERN_FUNC(sub_0007B180);
X86_EXTERN_FUNC(sub_000871A0);
X86_EXTERN_FUNC(sub_000718A0);
X86_EXTERN_FUNC(sub_002D50C0);
X86_FUNC(sub_001F7A70){assert(false&&"player fixture has no NPC label");}
X86_FUNC(sub_00087030){assert(false&&"player fixture has no NPC label");}
X86_FUNC(sub_000658B0){assert(false&&"player fixture has no NPC label");}
X86_FUNC(sub_002D666E) {
    double value;std::memcpy(&value,base+ctx.esp+4,8);
    x87_push(ctx,std::floor(value));ctx.esp+=4;
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
// GENERATED_ORIGINAL
// GENERATED_CALL_GUEST

static constexpr uint32_t beat=0x600000,health=0x601000,actor=0x602000,fill=0x603000;
static constexpr uint32_t samples=0x600100,colors=0x600200;
static constexpr uint32_t sprites=0x604000,sprite=0x604100,svtable=0x604200,descriptor=0x604300;
static constexpr uint32_t engine=0x605000,evtable=0x605100,quad=0x605200,qvtable=0x605300;
static constexpr uint32_t background=0x606000,bvtable=0x606100,fillchild=0x606200,fvtable=0x606300,dimensions=0x606400,dvtable=0x606500;
struct Draw {float left,top,right,bottom;uint32_t color;};
static std::vector<Draw> draws;
static std::vector<std::array<float,17>> requests,backgrounds,fills;
static unsigned calls;
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t address) {
    if(address==0x7b180){
        assert(ctx.ecx==sprite);++calls;std::array<float,17> d{};
        std::memcpy(d.data(),base+R32(ctx.esp+4),sizeof(d));requests.push_back(d);sub_0007B180(ctx,base);return;
    }
    if(address==0xf0003){
        assert(ctx.ecx==background);std::array<float,17> d{};std::memcpy(d.data(),base+R32(ctx.esp+4),sizeof(d));
        backgrounds.push_back(d);ctx.esp+=8;return;
    }
    if(address==0xf0004){assert(ctx.ecx==fillchild);ctx.eax=dimensions;ctx.esp+=4;return;}
    if(address==0xf0005){
        assert(ctx.ecx==dimensions);ctx.eax=R32(ctx.esp+4);G<float>(ctx.eax)=1;G<float>(ctx.eax+4)=3;ctx.esp+=8;return;
    }
    if(address==0xf0006){
        assert(ctx.ecx==fillchild);std::array<float,17> d{};std::memcpy(d.data(),base+R32(ctx.esp+4),sizeof(d));
        fills.push_back(d);ctx.esp+=8;return;
    }
    if(address==0xf0001){assert(ctx.ecx==engine);ctx.eax=quad;ctx.esp+=4;return;}
    if(address==0xf0002){
        assert(ctx.ecx==quad&&R32(ctx.esp+16)==123&&R32(ctx.esp+28)==0&&R32(ctx.esp+32)==0);
        const uint32_t a=R32(ctx.esp+4),b=R32(ctx.esp+8),u=R32(ctx.esp+20),v=R32(ctx.esp+24);
        draws.push_back({G<float>(a),G<float>(a+4),G<float>(b),G<float>(b+4),R32(ctx.esp+12)});
        assert(G<float>(u)==0&&G<float>(u+4)==0&&G<float>(v)==1&&G<float>(v+4)==1);
        ctx.esp+=36;
        // Real CallGuest does not save XMM/flags or host MXCSR. The bridge must.
        ctx.xmm[7].u[2]=0xdeadbeef;ctx.cf=1;_mm_setcsr(0x7f80);return;
    }
    std::abort();
}

static X86Context Reset() {
    memory.fill(0);draws.clear();requests.clear();backgrounds.clear();fills.clear();calls=0;mem::denied=0xffffffff;trace={};
    W32(beat,0x452f90);W32(beat+4,53);W32(beat+8,44);W32(beat+0x10,health+0x10);
    G<float>(beat+0x14)=1000;G<float>(beat+0x60)=.15f;W32(beat+0x6c,sprites);
    W32(beat+0x64,samples);W32(beat+0x68,colors);
    W32(beat+0xc,background);W32(background,bvtable);W32(bvtable+8,0xf0003);
    W32(health,0x453550);W32(health+4,actor);W32(health+0xc,fill);G<float>(health+0x4c)=1;
    const float rgba[]={157.f/255,219.f/255,64.f/255,1};std::memcpy(HostPtr(health+0x10),rgba,16);
    W32(actor+0x54,44);W32(fill,0x4531c0);G<float>(fill+8)=220;
    for(unsigned i=0;i<53;++i)W32(sprites+i*4,sprite);
    W32(sprite,svtable);W32(svtable+8,0x7b180);W32(sprite+4,123);
    W32(fill+4,fillchild);G<float>(fill+0x10)=1;G<float>(fill+0x14)=1;
    W32(fillchild,fvtable);W32(fvtable+8,0xf0006);W32(fvtable+12,0xf0004);
    W32(dimensions,dvtable);W32(dvtable+20,0xf0005);
    G<float>(sprite+8)=G<float>(sprite+12)=G<float>(sprite+24)=G<float>(sprite+28)=1;
    const float desc[]={49,34,640,480,1,1,1,1,0,0,0,0,0,0,0,1,1};
    std::memcpy(HostPtr(descriptor),desc,sizeof(desc));
    W32(0x5150b0,engine);W32(engine,evtable);W32(evtable+0x48,0xf0001);
    W32(quad,qvtable);W32(qvtable+4,0xf0002);
    // GENERATED_CONSTANTS
    X86Context ctx{};ctx.esi=beat;ctx.esp=0x700000;ctx.ebp=0x700200;W32(ctx.ebp+8,descriptor);
    std::memcpy(HostPtr(ctx.esp+0x90),desc,sizeof(desc));
    ctx.eax=11;ctx.ecx=12;ctx.edx=13;ctx.ebx=14;ctx.edi=15;ctx.fcw=0x37f;ctx.mxcsr=0x1f80;
    ctx.xmm[7].u[2]=0x87654321;ctx.st[7]=42;return ctx;
}
static void Invoke(X86Context& ctx) {
    const auto saved=ctx;std::array<uint8_t,0x5400> owners{};
    std::memcpy(owners.data(),HostPtr(beat),owners.size());
    std::array<uint8_t,0x210> stack{};std::memcpy(stack.data(),HostPtr(ctx.esp),stack.size());
    _mm_setcsr(0x5f80);draws.clear();requests.clear();calls=0;TrialHealthPulse(ctx,g_base);
    assert(!std::memcmp(&ctx,&saved,sizeof(ctx))&&_mm_getcsr()==0x5f80);
    assert(!std::memcmp(owners.data(),HostPtr(beat),owners.size()));
    assert(!std::memcmp(stack.data(),HostPtr(ctx.esp),stack.size()));
    _mm_setcsr(0x1f80);
    assert(calls==draws.size()&&calls<=1);
}

static void CopyHistory(const UCHudHeartbeat& h){
    G<float>(beat+0x14)=h.previous_health;G<float>(beat+0x5c)=h.sweep;
    G<float>(beat+0x60)=h.pulse;G<float>(beat+0x74)=h.elapsed;
    std::memcpy(HostPtr(samples),h.samples,sizeof(h.samples));
    std::memcpy(HostPtr(colors),h.colors,sizeof(h.colors));
}
static void NoSound(void*){}
static UCHudHeartbeat Source(UCGameRandom& rng){
    UCHudHeartbeat h;assert(uc_hud_heartbeat_initialize(&h));assert(!uc_game_random_init(&rng,0));
    const UCHudHeartbeatActor a{1000,1000,1,1};
    assert(uc_hud_heartbeat_step(&h,&a,0,&rng,NoSound,nullptr));return h;
}
static void Step(UCHudHeartbeat& h,UCGameRandom& rng,float dt){
    const UCHudHeartbeatActor a{1000,1000,1,1};
    assert(uc_hud_heartbeat_step(&h,&a,dt,&rng,NoSound,nullptr));CopyHistory(h);
}
static PulseSnapshot Snapshot(X86Context& ctx){PulseSnapshot s;assert(CapturePulse(ctx,g_base,s));return s;}
static std::vector<std::array<float,17>> Policy(const PulseSnapshot& s,TraceHistory& h){
    std::vector<std::array<float,17>> out;LifelinePulse(s,h,[&](const auto& d){out.push_back(d);});
    assert(out.size()<=1);return out;
}

static void ClockChecks(){
    for(unsigned hz:{30u,60u})for(float extent:{20.f,220.f,2580.f})for(float fraction:{.1f,.5f,1.f,1.4f}){
        auto ctx=Reset();G<float>(fill+8)=extent;G<float>(health+0x4c)=fraction;
        UCGameRandom rng;auto h=Source(rng);CopyHistory(h);
        TraceHistory history;Policy(Snapshot(ctx),history);
        float previous_sweep=h.sweep;unsigned wraps=0;bool bar_seen=false;
        const float end=std::floor(103+extent*std::min(fraction,1.f));
        for(unsigned frame=0;frame<hz*7;++frame){
            Step(h,rng,1.f/hz);if(h.sweep<previous_sweep)++wraps;previous_sweep=h.sweep;
            const auto s=Snapshot(ctx);const auto rng_before=rng;auto result=Policy(s,history);
            assert(!std::memcmp(&rng,&rng_before,sizeof(rng)));
            assert(history.sweep==h.sweep&&history.bar_started==(wraps>0));
            assert(result.size()==(wraps>0?1u:0u));
            for(const auto& d:result){
                bar_seen=true;const float length=std::min(6.f,end-103);
                assert(std::fabs(d[0]-(103+(end-103-length)*h.sweep))<.001f);
                assert(d[0]>=103&&d[2]<=end&&d[2]-d[0]<=6.001f&&d[1]==47&&d[3]==50);
            }
            if(frame==hz||frame==hz*3){const auto saved=history;auto repeat=Policy(s,history);
                assert(!std::memcmp(&saved,&history,sizeof(history))&&result==repeat);}
        }
        assert(wraps>=3&&bar_seen);
    }
}

static void SpriteChecks(){
    auto ctx=Reset();UCGameRandom rng;auto h=Source(rng);CopyHistory(h);Invoke(ctx);assert(draws.empty());
    bool bar_line=false;
    for(unsigned frame=0;frame<800;++frame){
        Step(h,rng,1.f/60);Invoke(ctx);
        for(const auto& d:draws){
            bar_line=true;assert(d.left==std::floor(d.left)+.5f&&d.left>=102.5f&&d.right<=322.5f);
            assert(d.top==46.5f&&d.bottom==49.5f&&d.right-d.left<=6);
        }
    }
    assert(bar_line);
    // Damage/healing clips only the additive short line.
    assert(trace.bar_started);G<float>(health+0x4c)=.01f;
    Invoke(ctx);assert(draws.size()==1);for(const auto& d:draws)assert(d.right<=104.5f);
    G<float>(health+0x4c)=1;Invoke(ctx);
    // No permanent baseline is emitted before the original source wraps.
    ctx=Reset();G<float>(beat+0x5c)=52.5f/53;G<float>(beat+0x74)=2;
    Invoke(ctx);assert(draws.empty());
    G<float>(beat+0x5c)=0;G<float>(beat+0x74)=2.04f;Invoke(ctx);
    assert(trace.bar_started&&draws.size()==1&&draws[0].left==102.5f&&draws[0].right==108.5f);
    assert(draws[0].top==46.5f&&draws[0].bottom==49.5f);
    auto frozen=draws;Invoke(ctx);assert(draws.size()==frozen.size());
    assert(!std::memcmp(draws.data(),frozen.data(),draws.size()*sizeof(Draw)));
    G<float>(beat+0x60)=.27f;Invoke(ctx);assert(draws.size()==frozen.size());
    assert(!std::memcmp(draws.data(),frozen.data(),draws.size()*sizeof(Draw)));
    G<float>(ctx.esp+0x90)=39;G<float>(ctx.esp+0x90+28)=.25f;
    Invoke(ctx);assert(draws.size()==1&&draws[0].left==102.5f&&draws[0].right==108.5f);
    for(const auto& d:draws)assert((d.color>>24)<=63);
    // The original source, not the draw bridge, owns waveform fading.
    for(unsigned i=0;i<53;++i)G<float>(colors+i*16+12)=0;
    Invoke(ctx);assert(draws.size()==1);
    G<float>(beat+0x74)=0;G<float>(beat+0x5c)=0;std::memset(HostPtr(samples),0,53*4);
    Invoke(ctx);assert(!trace.bar_started&&draws.empty());
    W32(beat+8,45);W32(actor+0x54,45);Invoke(ctx);assert(trace.actor==45&&!trace.bar_started);
    std::memcpy(HostPtr(0x609000),HostPtr(beat),0x7c);ctx.esi=0x609000;
    Invoke(ctx);assert(trace.owner==0x609000&&!trace.bar_started);
}

static void OriginalDrawChecks(){
    // Execute the actual original and staged heartbeat functions, not a second
    // drawing model. Include zeros/faded samples and the original sliding entry.
    for(float elapsed:{.2f,2.04f})for(float fraction:{.1f,1.f})for(bool faded:{false,true}){
        auto initial=Reset();G<float>(beat+0x74)=elapsed;G<float>(beat+0x5c)=.02f;
        G<float>(health+0x4c)=fraction;G<float>(fill+0x14)=fraction;
        for(unsigned i=0;i<53;++i){
            G<float>(samples+i*4)=i%5==0?-7.f:(i%3==0?4.f:0.f);
            for(unsigned j=0;j<4;++j)G<float>(colors+i*16+j*4)=j==3&&faded?(i%2?.4f:0.f):1.f;
        }
        G<float>(descriptor+28)=.75f;
        std::array<uint8_t,0x7000> owners{};std::memcpy(owners.data(),HostPtr(beat),owners.size());
        auto draw_fill=[&]{
            auto c=initial;c.ecx=fill;W32(c.esp,0);W32(c.esp+4,descriptor);
            fills.clear();_mm_setcsr(0x1f80);sub_00074190(c,g_base);
            assert(c.esp==initial.esp+8&&c.ebp==initial.ebp&&fills.size()==1);
            assert(fills[0][0]==49&&fills[0][1]==34&&fills[0][2]==49+220*fraction&&fills[0][3]==37);
            for(unsigned j=4;j<8;++j)assert(fills[0][j]==G<float>(descriptor+j*4));
            return fills[0];
        };
        const auto original_fill=draw_fill();
        initial.ecx=beat;W32(initial.esp,0);W32(initial.esp+4,descriptor);
        std::array<uint8_t,0x1300> stack{};std::memcpy(stack.data(),HostPtr(0x6ff000),stack.size());
        draws.clear();requests.clear();backgrounds.clear();calls=0;_mm_setcsr(0x1f80);
        auto retail=initial;RetailHeartbeat(retail,g_base);const unsigned original_csr=_mm_getcsr();
        const auto original_requests=requests,original_backgrounds=backgrounds;const auto original_draws=draws;
        assert(calls==52&&requests.size()==52&&backgrounds.size()==1);
        assert(draws.size()==(faded?26u:52u));
        assert(retail.esp==initial.esp+8&&retail.ebp==initial.ebp);
        assert(!std::memcmp(owners.data(),HostPtr(beat),owners.size()));
        // Give the additive seam a real source wrap only in the settled case.
        trace={};if(elapsed>2){auto s=Snapshot(initial);s.elapsed=2;s.sweep=.99f;Policy(s,trace);}
        std::memcpy(HostPtr(0x6ff000),stack.data(),stack.size());
        draws.clear();requests.clear();backgrounds.clear();calls=0;_mm_setcsr(0x1f80);
        auto hooked=initial;HookedHeartbeat(hooked,g_base);
        const unsigned additional=elapsed>2?1u:0u;
        assert(requests.size()==52+additional&&draws.size()==original_draws.size()+additional);
        assert(!std::memcmp(requests.data(),original_requests.data(),52*sizeof(requests[0])));
        assert(!std::memcmp(draws.data(),original_draws.data(),original_draws.size()*sizeof(Draw)));
        assert(backgrounds==original_backgrounds);
        assert(!std::memcmp(&hooked,&retail,sizeof(hooked))&&_mm_getcsr()==original_csr);
        assert(!std::memcmp(owners.data(),HostPtr(beat),owners.size()));
        if(additional){const auto& d=requests.back();assert(d[0]>=103&&d[2]<=103+220*fraction&&d[2]-d[0]<=6.001f);}
        assert(draw_fill()==original_fill);
    }
    _mm_setcsr(0x1f80);
}

int main() {
    ClockChecks();SpriteChecks();OriginalDrawChecks();
    for(unsigned bad=0;bad<19;++bad){auto ctx=Reset();
        G<float>(beat+0x74)=2;G<float>(beat+0x5c)=.99f;Invoke(ctx);
        G<float>(beat+0x74)=2.04f;G<float>(beat+0x5c)=0;Invoke(ctx);assert(draws.size()==1);
        switch(bad){
        case 0:mem::denied=beat;break;
        case 1:W32(beat,0);break;
        case 2:W32(beat+4,52);break;
        case 3:W32(beat+0x10,0);break;
        case 4:W32(health,0);break;
        case 5:W32(actor+0x54,45);break;
        case 6:W32(fill,0);break;
        case 7:W32(svtable+8,0);break;
        case 8:G<float>(beat+0x14)=0;break;
        case 9:G<float>(health+0x4c)=0;break;
        case 10:G<float>(beat+0x5c)=std::numeric_limits<float>::quiet_NaN();break;
        case 11:G<float>(health+0x10)=std::numeric_limits<float>::infinity();break;
        case 12:G<float>(ctx.esp+0x90+28)=0;break;
        case 13:G<float>(health+0x1c)=0;break;
        case 14:mem::denied=(ctx.esp-0x80)&~0xfffu;break;
        case 15:W32(beat+0x64,0);break;
        case 16:W32(beat+0x64,0x7fff80);break;
        case 17:W32(beat+0x68,0x7fffe0);break;
        case 18:G<float>(beat+0x74)=std::numeric_limits<float>::infinity();break;
        }
        Invoke(ctx);assert(draws.empty()&&!calls);
    }
    std::puts("health trace: all 52 original ECG requests, frame and fill preserved; at most one additive source-synchronized highlight; damage, pause/reset, slide/alpha and original sprite/context ABI passed");
}
