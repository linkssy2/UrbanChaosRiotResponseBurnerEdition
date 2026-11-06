// Real BC4B0 axis shaping, BEF80 movement excerpts, 44350 smoothers, 1F9010
// clock and BC930 impulse decay. Only physical input and equipment lookup stubbed.
#undef NDEBUG
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <xmmintrin.h>
namespace movement_test {
std::array<uint8_t,0x800000> memory{};
constexpr uint32_t P=0x600000,Q=0x602000,W=0x610000,L=0x620000,V=0x630000,M=0x640000,S=0x7f0000;
unsigned inertia[2]{1,1};
uint32_t second=Q,unmapped=0;
bool equipment=false;
float axes[2]{};
uint8_t* Data(){return memory.data();}
uint32_t Word(uint32_t at){uint32_t value;std::memcpy(&value,Data()+at,4);return value;}
void Put(uint32_t at,uint32_t value){std::memcpy(Data()+at,&value,4);}
void Real(uint32_t at,float value){std::memcpy(Data()+at,&value,4);}
float Float(uint32_t at){float value;std::memcpy(&value,Data()+at,4);return value;}
}
namespace mem {bool IsCommitted(uint32_t page){return page<movement_test::memory.size()&&page!=movement_test::unmapped;}}
namespace trial_controls {unsigned MovementInertia(unsigned slot){assert(slot<2);return movement_test::inertia[slot];}}
uint32_t TrialCoopPlayer(unsigned slot){return slot==1?movement_test::second:movement_test::P;}
#include "../common/xbe_movement_inertia.h"
#include "../common/xbe_player_actions.h"
#include "cpu/x86_ops.h"
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_000643E0){ctx.eax=movement_test::M;ctx.esp+=4;}
X86_FUNC(sub_00041F50){
    using namespace movement_test;assert(ctx.ecx==P+0x498||ctx.ecx==Q+0x498);
    ctx.eax=equipment?1u:0u;ctx.esp+=4;
}
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target){
    using namespace movement_test;assert(target==0x567f0&&ctx.ecx==W);
    const auto slot=Word(ctx.esp+4),action=Word(ctx.esp+8),mode=Word(ctx.esp+12);
    assert(slot<2&&action<=2&&mode==3);
    x87_push(ctx,axes[action==1?0:1]);ctx.esp+=16;
}
// ORIGINAL_MOVEMENT_BODIES
namespace movement_test {
void Seed(bool coop=false){
    memory.fill(0);inertia[0]=inertia[1]=1;second=Q;unmapped=0;equipment=false;
    trial_coop::SetActive(coop);trial_player_actions::walking[0]=trial_player_actions::walking[1]=false;
    Put(0x51c4dc,W);Put(W+4,L);Put(W,V);Put(V+8,0x567f0);Real(W+0x14,1.f/60);
    Data()[M+0x31]=1;
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t owner=slot?Q:P,id=100+slot;
        Put(owner,0x46fbe8);Put(owner+0x54,id);Put(owner+0x368,slot);Real(owner+0x9c,100);
        Put(L+0x3c+id*8,id);Put(L+0x40+id*8,owner);
        for(uint32_t offset:{0x1d0u,0x1dcu,0x738u,0x744u})Real(owner+offset+8,6);
        Real(owner+0x1ac,.6f);Put(owner+0x25c,owner+0x1000);Real(owner+0x24c,125);
    }
    // ORIGINAL_CONSTANTS
}
X86Context Step(unsigned slot,float sideways,float forward,bool immediate=true){
    axes[0]=sideways;axes[1]=forward;
    X86Context ctx{};ctx.edi=slot?Q:P;ctx.esi=slot;ctx.esp=S-12;ctx.ebx=0x12345678;ctx.ebp=0x76543210;
    ctx.fcw=0x37f;Put(S-12,0xbadcafe);Put(S-8,S+0x14);Put(S-4,S+0x18);
    Put(S-128,0x12345678);Put(S+0x100,0x87654321);
    ResolvedMovement(ctx,Data());assert(ctx.esp==S-8&&ctx.top==0);
    if(immediate)ImmediateMovement(ctx,Data());else OriginalMovement(ctx,Data());
    assert(ctx.esp==S&&ctx.top==0&&ctx.edi==(slot?Q:P));
    assert(Word(S-128)==0x12345678&&Word(S+0x100)==0x87654321);
    return ctx;
}
void DefaultParity(){
    unsigned cases=0;
    for(unsigned route=0;route<3;++route)for(float dt:{0.f,1.f/120,1.f/30,.5f})
    for(bool crouch:{false,true})for(bool walk:{false,true}){
        const unsigned slot=route==2;const uint32_t owner=slot?Q:P;
        Seed(route!=0);Real(W+0x14,dt);Data()[owner+0x434]=crouch;trial_player_actions::walking[slot]=walk;
        Real(owner+0x1d0,-.4f);Real(owner+0x1dc,.6f);equipment=bool(cases&1);
        const std::vector<uint8_t> before(memory.begin(),memory.end());const unsigned csr=_mm_getcsr();
        const auto native=Step(slot,1.5f,-.75f,false);const unsigned result_csr=_mm_getcsr();
        const std::vector<uint8_t> expected(memory.begin(),memory.end());
        std::memcpy(Data(),before.data(),before.size());_mm_setcsr(csr);
        const auto hooked=Step(slot,1.5f,-.75f);assert(!std::memcmp(&native,&hooked,sizeof native));
        assert(!std::memcmp(Data(),expected.data(),expected.size())&&_mm_getcsr()==result_csr);++cases;
    }
    std::printf("Movement Inertia On: %u complete original context/RAM comparisons passed\n",cases);
}
void Immediate(){
    for(unsigned route=0;route<3;++route)for(float dt:{1.f/120,1.f/60,1.f/30})
    for(bool crouch:{false,true})for(bool walk:{false,true})for(bool shield:{false,true}){
        const unsigned slot=route==2;const uint32_t owner=slot?Q:P,other=slot?P:Q;
        Seed(route!=0);inertia[slot]=0;Real(W+0x14,dt);equipment=shield;
        Data()[owner+0x434]=crouch;trial_player_actions::walking[slot]=walk;
        const std::vector<uint8_t> untouched(Data()+other,Data()+other+0x1100);
        for(const auto& target:std::array<std::array<float,2>,5>{{{1,1},{-.4f,.7f},{-1,-1},{0,0},{1.5f,-1.5f}}}){
            // Execute an unmodified copy first: every smoother, presentation blend,
            // vertical impulse and non-output memory must still match it exactly.
            const std::vector<uint8_t> before(memory.begin(),memory.end());
            Step(slot,target[0],target[1],false);const std::vector<uint8_t> expected(memory.begin(),memory.end());
            std::memcpy(Data(),before.data(),before.size());Step(slot,target[0],target[1]);
            const float scale=(walk?.35f:1.f)*(shield?.6f:1.f);
            assert(std::fabs(Float(owner+0x730)-std::clamp(target[0],-1.f,1.f)*scale*(crouch?.5f:1.f))<1e-6f);
            assert(std::fabs(Float(owner+0x734)-std::clamp(target[1],-1.f,1.f)*scale*(crouch?.3f:1.f))<1e-6f);
            auto actual=std::vector<uint8_t>(memory.begin(),memory.end());
            std::memcpy(actual.data()+owner+0x730,expected.data()+owner+0x730,8);
            assert(actual==expected&&!std::memcmp(Data()+other,untouched.data(),untouched.size()));
        }
    }
    // Per-player choices and changes during motion. Original histories are live
    // throughout: switching back to On resumes the ordinary tracked ramp.
    Seed(true);inertia[0]=0;Step(0,1,0);Step(1,0,1);
    assert(Float(P+0x730)==1&&std::fabs(Float(Q+0x734)-.1f)<1e-6f);
    inertia[0]=1;inertia[1]=0;Step(0,0,0);Step(1,0,-1);
    assert(std::fabs(Float(P+0x730)-.09f)<1e-6f&&Float(Q+0x734)==-1);
    inertia[0]=0;Step(0,0,0);assert(Float(P+0x730)==0);
    // Reload and the ordinary firing/equipment states retain immediate movement.
    for(uint32_t state:{0u,1u,2u,4u,5u,6u,7u,8u,9u,10u}){
        Seed();inertia[0]=0;Put(P+0x170,state);Step(0,1,-1);
        assert(Float(P+0x730)==1&&Float(P+0x734)==-1);
    }
}
void Impulses(){
    for(unsigned slot=0;slot<2;++slot)for(float x:{0.f,.05f,2.f,-2.f})for(float y:{0.f,3.f}){
        Seed(true);inertia[slot]=0;const uint32_t owner=slot?Q:P;
        Real(owner+0x244,x);Real(owner+0x248,y);Step(slot,1,-1);
        const float forward=Float(owner+0x244),sideways=Float(owner+0x248);
        if(forward||sideways){assert(Float(owner+0x730)==sideways&&Float(owner+0x734)==forward);}
        else assert(Float(owner+0x730)==1&&Float(owner+0x734)==-1);
        assert(Float(owner+0x24c)==124.5f); // Actual BC930 vertical decay is retained.
    }
}
void Guards(){
    for(unsigned test=0;test<27;++test){
        Seed(true);inertia[0]=inertia[1]=0;X86Context ctx{};ctx.edi=P;
        Real(P+0x1d4,1);Real(P+0x1e0,-1);Real(P+0x730,.2f);Real(P+0x734,-.3f);
        switch(test){
        case 0:Put(P,0);break;case 1:Put(P+0x5c,1);break;case 2:Data()[P+0x304]=1;break;
        case 3:Put(P+0x368,2);break;case 4:Put(P+0x54,0xffffffffu);break;
        case 5:Put(L+0x3c+100*8,99);break;case 6:Put(L+0x40+100*8,Q);break;
        case 7:Real(P+0x9c,0);break;case 8:Real(P+0x9c,-1);break;
        case 9:Put(P+0x170,3);break;case 10:Put(P+0x170,11);break;
        case 11:Put(P+0x170,12);break;case 12:Put(P+0x170,13);break;
        case 13:Put(W+0x10,0x800);break;case 14:Real(W+0x14,0);break;
        case 15:Real(W+0x14,-1);break;case 16:Put(W+0x14,0x7f800000);break;
        case 17:Put(P+0x1d4,0x7fc00000);break;case 18:Real(P+0x1e0,1.01f);break;
        case 19:Put(P+0x730,0x7f800000);break;case 20:unmapped=P;break;
        case 21:unmapped=W;break;case 22:unmapped=L;break;
        case 23:ctx.edi=0xfffffff0;break;
        case 24:ctx.edi=Q;Real(Q+0x1d4,1);trial_coop::SetActive(false);break;
        case 25:ctx.edi=Q;Real(Q+0x1d4,1);second=P;break;
        case 26:inertia[0]=1;break;
        }
        const auto original=ctx;const std::vector<uint8_t> before(memory.begin(),memory.end());const unsigned csr=_mm_getcsr();
        TrialMovementInertia(ctx,Data());
        assert(!std::memcmp(&ctx,&original,sizeof ctx)&&!std::memcmp(Data(),before.data(),before.size())&&_mm_getcsr()==csr);
    }
    // An admitted call changes exactly eight output bytes, never CPU/stack/FP.
    Seed();inertia[0]=0;Real(P+0x1d4,.35f);Put(P+0x1e0,0x80000000);
    X86Context ctx{};ctx.edi=P;ctx.esp=0xfffffff0;ctx.st[3]=1.25;ctx.top=3;
    const auto saved=ctx;const unsigned csr=_mm_getcsr();auto expected=std::vector<uint8_t>(memory.begin(),memory.end());
    std::memcpy(expected.data()+P+0x730,Data()+P+0x1d4,4);std::memcpy(expected.data()+P+0x734,Data()+P+0x1e0,4);
    TrialMovementInertia(ctx,Data());assert(!std::memcmp(&ctx,&saved,sizeof ctx)&&expected==std::vector<uint8_t>(memory.begin(),memory.end())&&_mm_getcsr()==csr);
    TrialMovementInertia(ctx,nullptr);
    // Zero elapsed time and alternate movement preserve the original branch.
    for(bool alternate:{false,true}){
        Seed();inertia[0]=0;Real(W+0x14,alternate?1.f/60:0);Data()[P+0x304]=alternate;
        const auto before=std::vector<uint8_t>(memory.begin(),memory.end());Step(0,1,-1,false);
        const auto original=std::vector<uint8_t>(memory.begin(),memory.end());
        std::memcpy(Data(),before.data(),before.size());Step(0,1,-1);assert(original==std::vector<uint8_t>(memory.begin(),memory.end()));
    }
}
}
int main(){
    movement_test::DefaultParity();movement_test::Immediate();movement_test::Impulses();movement_test::Guards();
    std::puts("Movement inertia: original ramp, immediate locomotion, Walk/crouch, per-player settings and guarded overrides passed");
}
