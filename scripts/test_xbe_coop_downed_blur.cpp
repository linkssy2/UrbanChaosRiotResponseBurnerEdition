// Production ownership/gating and actual lifted pre-HUD caller instructions.
// Body lifecycle is supplied by the separately tested cooperative death owner.
// GPU blur filtering is exercised by the renderer's dedicated test.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_downed_blur.cpp"

constexpr uint32_t kWorld=0x600000,kState=0x601000,kVector=0x602000;
constexpr uint32_t kActors[]={0x610000,0x612000},kLayers[]={0x620000,0x621000};
static unsigned seconds[2]{},presentation=1,mask=0,calls=0,owner_queries=0;
static bool menu=false,intro=false,down[2]{},revoke=false;
static std::vector<unsigned> order;
uint8_t* B(){return kernel::memory.data();}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
uint32_t TrialCoopPlayer(unsigned i){return i<2?kActors[i]:0;}
unsigned TrialCoopReviveSeconds(unsigned i){return i<2?seconds[i]:0;}
unsigned TrialCoopPresentation(){return presentation;}
bool TrialGraphicsOpen(){return menu;}
bool TrialIntroPlaying(){return intro;}
bool TrialCoopPlayerDowned(const uint8_t* b,uint32_t p){
    assert(b==B());++owner_queries;
    const auto slot=p==kActors[0]?0u:1u;assert(p==kActors[slot]);
    if(revoke)seconds[slot]=0;
    return down[slot];
}
namespace d3d {void TrialDownedSceneBlur(unsigned value){
    assert(value&&!(value&~3u));mask=value;++calls;order.push_back(2);
    _mm_setcsr(_mm_getcsr()^0x6000u);
}}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t fn){
    assert(base==B()&&fn==0x700000);order.push_back(1);ctx.esp+=4;
}
X86_FUNC(sub_00215380){
    const auto before=ctx;
    TrialCoopDownedSceneBlur(ctx,base);
    assert(!std::memcmp(&before,&ctx,sizeof ctx));
    order.push_back(3);ctx.esp+=4;
}
// ORIGINAL_DOWNED_BLUR_BOUNDARIES

X86Context Setup(){
    kernel::memory.fill(0);trial_coop::requested=true;trial_coop::active=true;
    presentation=1;menu=intro=revoke=false;seconds[0]=seconds[1]=0;down[0]=down[1]=false;
    calls=mask=owner_queries=0;order.clear();
    Put(0x51c4dc,kWorld);Put(kWorld,0x450c48);Put(kWorld+4,kState);Put(kWorld+0x10,0x80);
    Put(kState+4,kVector);Put(kState+8,kVector+8);
    for(unsigned i=0;i<2;++i){Put(kVector+4*i,kLayers[i]);Put(kLayers[i]+4,kActors[i]);Put(kLayers[i]+0x24,i);}
    X86Context ctx{};ctx.ecx=kWorld;ctx.esp=0x7e0000;Put(ctx.esp,0x557e8);return ctx;
}
void CheckBlocked(X86Context& ctx){
    const auto before=ctx;const auto previous=calls;
    const std::vector<uint8_t> ram(kernel::memory.begin(),kernel::memory.end());
    TrialCoopDownedSceneBlur(ctx,B());
    assert(calls==previous&&!std::memcmp(&ctx,&before,sizeof ctx)&&
        std::equal(ram.begin(),ram.end(),kernel::memory.begin()));
}
int main(){
    auto c=Setup();CheckBlocked(c);assert(!owner_queries);
    for(unsigned slot=0;slot<2;++slot)for(unsigned remaining=1;remaining<=10;++remaining){
        c=Setup();seconds[slot]=remaining;down[slot]=true;
        const std::vector<uint8_t> ram(kernel::memory.begin(),kernel::memory.end());
        const auto csr=_mm_getcsr();
        TrialCoopDownedSceneBlur(c,B());
        assert(calls==1&&mask==(1u<<slot)&&csr==_mm_getcsr()&&
            std::equal(ram.begin(),ram.end(),kernel::memory.begin()));
        seconds[slot]=0;CheckBlocked(c); // revive/expiry clears immediately
    }
    c=Setup();seconds[0]=seconds[1]=5;down[0]=down[1]=true;
    TrialCoopDownedSceneBlur(c,B());assert(calls==1&&mask==3);
    c=Setup();seconds[0]=11;down[0]=true;CheckBlocked(c);assert(!owner_queries);
    c=Setup();seconds[0]=5;CheckBlocked(c);assert(owner_queries==1); // expired body rejected
    c=Setup();seconds[0]=5;down[0]=revoke=true;CheckBlocked(c); // world/actor reuse revoked snapshot
    for(unsigned gate=0;gate<13;++gate){
        c=Setup();seconds[0]=5;down[0]=true;
        switch(gate){
        case 0:trial_coop::active=false;break;
        case 1:trial_coop::requested=false;break;
        case 2:presentation=2;break;
        case 3:presentation=0;break;
        case 4:menu=true;break;
        case 5:intro=true;break;
        case 6:Put(kWorld+0x10,0x880);break;
        case 7:Put(kWorld+0x10,0);break;
        case 8:Put(c.esp,0x557f0);break;
        case 9:c.ecx+=4;break;
        case 10:Put(kState+8,kVector+4);break;
        case 11:Put(kLayers[0]+4,kActors[1]);break;
        case 12:Put(kLayers[1]+0x24,0);break;
        }
        CheckBlocked(c);assert(!owner_queries);
    }
    // Actual scene tail finalizes the scene; real HUD caller pushes557E8 before
    // entering the hook. The first native HUD service follows the queued blur.
    c=Setup();seconds[1]=7;down[1]=true;
    Put(0x5150b0,0x680000);Put(0x680000,0x681000);Put(0x6810a0,0x700000);
    OriginalSceneFinish(c,B());c.ecx=kWorld;
    const auto esp=c.esp,esi=c.esi;OriginalHudBegin(c,B());
    assert(calls==1&&mask==2&&order==std::vector<unsigned>({1,2,3}));
    assert(c.esp==esp-4&&ReadBlur(B(),c.esp)==esi);
    std::puts("Downed scene blur ownership, clearing, pause/cinematic and original pre-HUD ordering passed.");
}
