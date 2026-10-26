// Extracted retail caller, death branch, expiry and reset/health operations.
// Physics creation, skin resources and scene collision still require live tests.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_death.cpp"
static constexpr uint32_t kPlayers[]={0x610000,0x612000};
static constexpr uint32_t kBodies[]={0x650000,0x652000};
static uint32_t current_body[2];
static unsigned body_calls=0,queries=0,ragdoll_branches=0,animation_branches=0,expires=0;
static unsigned resets=0,recreates=0,collision_releases=0,collision_creates=0;
static bool fail_recreate=false;
static float physical_position[2][3]{},placed_physics[3]{};
static unsigned placements=0;
static bool valid_physics=true;
static unsigned presentation=1,fail_requests=0,failure_cleanup=0;
uint8_t* B(){return kernel::memory.data();}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
unsigned TrialCoopPresentation(){return presentation;}
uint32_t TrialCoopPlayer(unsigned i){return i<2?kPlayers[i]:0;}
bool TrialRagdollWorldPosition(const uint8_t*,uint32_t handle,float (&position)[3]){
    if(!valid_physics||handle<11||handle>12)return false;
    std::memcpy(position,physical_position[handle-11],sizeof position);return true;
}
uint32_t TrialCoopBodyForPlayer(const uint8_t* b,uint32_t owner){
    for(unsigned i=0;i<2;++i)if(owner==kPlayers[i]&&Read(b,owner+0x54)==100+i){
        const auto body=current_body[i];
        if(Read(b,body)==0x45d040&&Read(b,body+0x58)==100+i&&!(Read(b,body+0x5c)&1))return body;
    }
    return 0;
}
bool TrialCoopBodyRecreate(X86Context&,uint8_t* b,uint32_t owner){
    if(fail_recreate)return false;
    const unsigned i=owner==kPlayers[0]?0:1;assert(owner==kPlayers[i]);
    ++recreates;current_body[i]+=0x400;
    std::memset(b+current_body[i],0,0x270);
    Put(b,current_body[i],0x45d040);Put(b,current_body[i]+0x54,300+i);Put(b,current_body[i]+0x58,100+i);
    Put(b,current_body[i]+0x194,11+i);
    return true;
}
X86_FUNC(sub_000E5310);
X86_FUNC(sub_0004E4C0);
float TrialRagdollSeconds(){return 1.f;}
// ORIGINAL_DEATH_REGIONS
X86_FUNC(sub_000E5310){
    ++body_calls;assert(ctx.ecx==current_body[0]||ctx.ecx==current_body[1]);
    // These are the six retail C7F40 arguments, not a fabricated death event.
    assert(Read(base,ctx.esp+4)==0x7e0034);
    assert(Read(base,ctx.esp+12)==2&&Read(base,ctx.esp+20)==0x68000c&&Read(base,ctx.esp+24)==1);
    const auto saved=ctx;ctx.ebp=ctx.esp-4;ctx.esi=ctx.ecx;
    CoopRagdollBranch(ctx,base);ctx=saved;ctx.esp+=28;
}
// ORIGINAL_000B5E80
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    if(fn==0x2c2b60){++queries;c.eax=0;c.esp+=4;return;}
    if(fn==0x2c2b40){c.eax=0;c.esp+=4;return;}
    if(fn==0xb5e80){__imp__sub_000B5E80(c,b);return;}
    if(fn==0x218b30){++expires;c.esp+=4;return;}
    if(fn==0x700001){assert(Read(b,c.esp+4)==1);++collision_releases;c.esp+=8;return;}
    if(fn==0x700002){assert(Read(b,c.esp+4)==0&&Read(b,c.esp+8)==0);c.eax=0;c.esp+=12;return;}
    // Original campaign B4/A4/reset update service boundaries.
    assert(fn==0xbdb00||fn==0x11030||fn==0xd0080);c.esp+=4;
}
X86_FUNC(sub_0004AB90){assert(false);}
// ORIGINAL_00069280
X86_FUNC(sub_0004E4C0){assert(Read(base,ctx.esp+4)==0);++failure_cleanup;ctx.esp+=8;}
// ORIGINAL_FAILURE_CONSUMER
// ORIGINAL_DEAD_FAILURE_CONSUMER
X86_FUNC(sub_0005AAB0){ctx.eax=0x6e0000;ctx.esp+=4;}
X86_FUNC(sub_0005B140){ctx.eax=0x6e1000;ctx.esp+=4;}
X86_FUNC(sub_000643E0){ctx.eax=0x6e2000;ctx.esp+=8;}
X86_FUNC(sub_00064400){ctx.eax=2;ctx.esp+=4;}
// ORIGINAL_000BDAB0
// ORIGINAL_002180A0
X86_FUNC(sub_000BC6A0){assert(false);}
X86_FUNC(sub_000BC640){assert(false);}
X86_FUNC(sub_000BC760){assert(false);}
X86_FUNC(sub_000BECB0){assert(false);}
X86_FUNC(sub_000BED30){assert(false);}
// ORIGINAL_000BFE00
X86_FUNC(sub_000BFE00){__imp__sub_000BFE00(ctx,base);}
X86_FUNC(sub_0028A150){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_00287570){assert(false);}
X86_FUNC(sub_0022D360){assert(false);}
X86_FUNC(sub_000BD250){
    ++collision_creates;Put(base,ctx.ecx+0x1b0,0x6a0000);ctx.esp+=4;
}
X86_FUNC(sub_00044320){ctx.esp+=16;}
X86_FUNC(sub_000C8930){ctx.esp+=4;}
X86_FUNC(sub_000C30D0){ctx.esp+=4;}
X86_FUNC(sub_000C1E60){assert(Read(base,ctx.esp+4)==0);ctx.esp+=16;}
X86_FUNC(sub_000416F0){ctx.esp+=4;}
X86_FUNC(sub_00041C30){ctx.esp+=8;}
X86_FUNC(sub_000D2860){ctx.esp+=4;}
// ORIGINAL_000D35A0
// ORIGINAL_00049660
X86_FUNC(sub_00049660){__imp__sub_00049660(ctx,base);}
X86_FUNC(sub_001F8FC0){assert(false);}
X86_FUNC(sub_00041F80){ctx.eax=Read(base,ctx.ecx);ctx.esp+=4;}
X86_FUNC(sub_000442D0){ctx.eax=0x6f8000;ctx.esp+=4;}
// ORIGINAL_VISIBILITY_REGIONS
// ORIGINAL_002A2910
X86_FUNC(sub_002A2910){__imp__sub_002A2910(ctx,base);}
X86_FUNC(sub_002E5CF0){
    assert(ctx.ecx==0x6a2000);++placements;
    std::memcpy(placed_physics,base+Read(base,ctx.esp+4),sizeof placed_physics);ctx.esp+=8;
}
// ORIGINAL_002A42B0
X86_FUNC(sub_002A42B0){__imp__sub_002A42B0(ctx,base);}
// ORIGINAL_000BD150
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t fn,const uint32_t* args,size_t n){
    const auto saved=c;
    const uint32_t before=c.esp;
    for(size_t i=n;i;--i){c.esp-=4;Put(B(),c.esp,args[i-1]);}
    c.esp-=4;Put(B(),c.esp,0);
    if(fn==0xbdab0){assert(!n);__imp__sub_000BDAB0(c,B());}
    else if(fn==0xd35a0){assert(n==2&&!args[0]&&!args[1]);++resets;__imp__sub_000D35A0(c,B());}
    else if(fn==0xbd150){assert(n==1);__imp__sub_000BD150(c,B());}
    else if(fn==0x2180a0){assert(n==1);__imp__sub_002180A0(c,B());}
    else{assert(fn==0x69280&&n==3&&args[0]==1&&!args[1]&&!args[2]);++fail_requests;__imp__sub_00069280(c,B());}
    assert(c.esp==before);const auto result=c.eax;
    // Production CallGuest returns EAX separately while restoring integer regs.
    c.eax=saved.eax;c.ebx=saved.ebx;c.ecx=saved.ecx;c.edx=saved.edx;
    c.esi=saved.esi;c.edi=saved.edi;c.ebp=saved.ebp;c.esp=saved.esp;
    return result;
}
}
void Setup(){
    kernel::memory.fill(0);trial_coop::Reset();TrialCoopDeathReset();
    body_calls=queries=ragdoll_branches=animation_branches=expires=0;
    resets=recreates=collision_releases=collision_creates=0;fail_recreate=false;
    placements=0;valid_physics=true;std::memset(physical_position,0,sizeof physical_position);
    presentation=1;fail_requests=failure_cleanup=0;
    std::memset(placed_physics,0,sizeof placed_physics);
    trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
    Put(B(),0x51c4dc,0x620000);
    Put(B(),0x620010,0x80);Put(B(),0x620004,0x6c6000);
    Put(B(),0x6c6030,0x6c7000);
    Put(B(),0x509a60,0x6c2000);Put(B(),0x6c2000,0x4526f0);
    Put(B(),0x45270c,0x69280);
    // Retail event-1 jump table, pinned together with the receiver.
    B()[0x693d8]=0;Put(B(),0x693a8,0x69385);
    Put(B(),0x46fbe8+0xc0,0x2c2b60);Put(B(),0x46fbe8+0xc4,0x2c2b60);
    Put(B(),0x46fbe8+0xc8,0x2c2b40);Put(B(),0x457880+0x10,0xb5e80);
    Put(B(),0x46fbe8+0xb4,0xbdb00);Put(B(),0x46fbe8+0xa4,0x11030);Put(B(),0x46fbe8+0x34,0xd0080);
    Put(B(),0x45d040+0x24,0x218b30);
    Put(B(),0x6a0000,0x6a1000);Put(B(),0x6a1004,0x700001);Put(B(),0x6a1000,0x700002);
    Put(B(),0x6a0008,0x6a2000);Put(B(),0x44d024,0x3c23d70a);
    Put(B(),0x44bf94,0x3f800000);Put(B(),0x457cec,0x42c80000);
    for(unsigned i=0;i<2;++i){
        const auto p=kPlayers[i],body=kBodies[i];current_body[i]=body;
        Put(B(),p,0x46fbe8);Put(B(),p+0x54,100+i);Put(B(),p+0x368,i);Put(B(),p+0x9c,0x42c80000);
        for(unsigned diagonal=0;diagonal<4;++diagonal)Put(B(),p+0x10+diagonal*20,0x3f800000);
        Put(B(),p+0x1b0,0x6a0000);Put(B(),p+0x1b8,0x6b0000+i*0x1000);
        Put(B(),p+0x7d8,0x6b1000+i*0x1000);Put(B(),p+0x664,0x6b1800+i*0x1000);
        Put(B(),body,0x45d040);Put(B(),body+0x54,200+i);Put(B(),body+0x58,100+i);
        Put(B(),body+0x194,11+i);
        // Gun/arms, offhand and optional shield have separate render wrappers.
        for(unsigned part=0;part<3;++part){
            const uint32_t wrapper=0x6d0000+i*0x1000+part*0x200;
            Put(B(),p+(part==0?0x74:part==1?0x2b8:0x660),wrapper);
            Put(B(),wrapper,wrapper+0x80);Put(B(),wrapper+0x80,0x457880);
        }
        Put(B(),p+0x498,0x6f0000+i*0x1000);Put(B(),p+0x4fc,2+i);Put(B(),p+0x500,2+i);
        Put(B(),0x6f0004+i*0x1000,2+i);Put(B(),0x6f0010+i*0x1000,15+i);
        X86Context c{};c.esi=p;OriginalSpawnVisibility(c,B());
    }
}
X86Context Caller(unsigned slot){
    X86Context c{};c.esp=0x7e0000;c.esi=kPlayers[slot];c.ebp=0x7e1000;
    Put(B(),c.ebp+12,0x680000);Put(B(),0x680000,2);Put(B(),0x680004,6);B()[0x680034]=1;
    Put(B(),c.esi+0x9c,0);return c;
}
void DownPlayer(unsigned slot){
    auto c=Caller(slot);CoopDeathBodyCaller(c,B());
    assert(body_calls&&B()[current_body[slot]+0x198]);
    Put(B(),kPlayers[slot]+0x170,12);
}
void TestRevivedVisibility(){
    for(unsigned slot=0;slot<2;++slot)for(bool shield:{false,true}){
        Setup();const uint32_t owner=kPlayers[slot],partner=kPlayers[1-slot];
        B()[owner+0x4c5]=B()[owner+0x4c6]=shield;
        const auto visibility=[&](uint32_t actor,unsigned part)->uint8_t&{
            return B()[Read(B(),Read(B(),actor+(part==0?0x74:part==1?0x2b8:0x660)))+0x66];
        };
        const auto render=[&](uint32_t actor){
            X86Context c{};c.esi=actor;c.esp=0x7e0000;c.ebx=0x11223344;c.ebp=0x55667788;
            OriginalFirstPersonVisibility(c,B());
            assert(c.esp==0x7e0000&&c.esi==actor&&c.ebp==0x55667788);
        };
        render(owner);assert(visibility(owner,0)&&visibility(owner,1)&&bool(visibility(owner,2))==shield);
        // A dead update, not death entry, clears the persistent show latches.
        DownPlayer(slot);X86Context dead{};dead.esi=owner;OriginalDeadVisibility(dead,B());
        for(unsigned part=0;part<3;++part)visibility(owner,part)=0;
        X86Context c{};c.esp=0x7e0000;c.ecx=owner;
        kernel::CallGuest(c,0xd35a0,{0,0});render(owner);
        assert(Read(B(),owner+0x170)==0&&!B()[owner+0x2d0]&&!B()[owner+0x2d1]);
        assert(!visibility(owner,0)&&!visibility(owner,1)&&!visibility(owner,2));
        // The old reset leaves fireable equipment intact but never shows it.
        assert(Read(B(),owner+0x4fc)==2+slot&&Read(B(),0x6f0010+slot*0x1000)==15+slot);
        Put(B(),owner+0x170,12);
        for(unsigned round=0;round<3;++round){
            if(round){DownPlayer(slot);dead.esi=owner;OriginalDeadVisibility(dead,B());}
            const uint8_t partner_visibility[]={B()[partner+0x2d0],B()[partner+0x2d1]};
            std::array<uint8_t,0x60> items{};std::memcpy(items.data(),B()+0x6f0000+slot*0x1000,items.size());
            for(unsigned part=0;part<3;++part)visibility(owner,part)=0;
            fail_recreate=true;assert(!TrialCoopRevivePlayer(c,B(),owner));
            assert(!B()[owner+0x2d0]&&!B()[owner+0x2d1]);fail_recreate=false;
            const auto saved=c;assert(TrialCoopRevivePlayer(c,B(),owner));assert(!std::memcmp(&c,&saved,sizeof c));
            render(owner);
            assert(visibility(owner,0)&&visibility(owner,1)&&bool(visibility(owner,2))==shield);
            assert(B()[partner+0x2d0]==partner_visibility[0]&&B()[partner+0x2d1]==partner_visibility[1]);
            assert(Read(B(),owner+0x4fc)==2+slot&&Read(B(),owner+0x500)==2+slot);
            assert(!std::memcmp(items.data(),B()+0x6f0000+slot*0x1000,items.size()));
        }
        // Normal presentation still suppresses a shield while scoped/blocked.
        visibility(owner,2)=0;Put(B(),owner+0x35c,1);render(owner);assert(!visibility(owner,2));
        // No continuous forcing: subsequent original hide requests remain valid.
        dead.esi=owner;OriginalDeadVisibility(dead,B());visibility(owner,0)=visibility(owner,1)=0;
        render(owner);assert(!visibility(owner,0)&&!visibility(owner,1));
    }
}
unsigned PolicyDestination(unsigned slot){
    auto* base=B();X86Context ctx{};ctx.esi=kPlayers[slot];
    // DEATH_DISPOSITION_HOOK
    return 0;
loc_000C8461:return 2;
loc_000C84EB:return 1;
}
void Tick(float delta){
    std::memcpy(B()+0x620014,&delta,4);
    X86Context c{};c.esp=0x7e0000;c.ebp=0x620000;c.eax=0x12340000;
    CoopDeathTick(c,B());
    assert(c.esp==0x7e0000&&c.ebp==0x620000);
}
bool FailureSelected(bool primary_dead=false){
    X86Context c{};c.esp=0x7d0000;c.esi=0x6c27c0;c.ebx=0x6c8000;c.edi=0x6c9000;
    Put(B(),c.esp,10);Put(B(),c.esp+4,11);Put(B(),c.esp+8,12);
    if(primary_dead)OriginalDeadFailureConsumer(c,B());else OriginalFailureConsumer(c,B());
    if(c.eax!=0x4504d0)return false;
    assert(c.esp==0x7d0014&&c.edi==10&&c.esi==11&&c.ebx==12);
    assert(Read(B(),0x6c9000)==0xffffffffu&&Read(B(),0x6c9004)==0xffffffffu);
    return true;
}
void TestCountdown(){
    for(unsigned rate:{30u,60u,120u}){
        Setup();DownPlayer(0);Tick(0.f);
        for(unsigned frame=1;frame<rate*10;++frame){Tick(1.f/rate);assert(!fail_requests);}
        Tick(1.f/rate);assert(fail_requests==1&&!TrialCoopReviveSeconds(0)&&FailureSelected());
    }
    for(unsigned slot=0;slot<2;++slot){
        Setup();DownPlayer(slot);assert(!TrialCoopReviveSeconds(slot)&&!FailureSelected());
        Tick(0.25f);assert(TrialCoopReviveSeconds(slot)==10&&!TrialCoopReviveSeconds(1-slot));
        for(unsigned i=0;i<35;++i)Tick(0.25f);
        auto duplicate=Caller(slot);TrialCoopDeathBody(duplicate,B());
        Tick(0.25f);assert(TrialCoopReviveSeconds(slot)==1);
        Put(B(),0x620010,0x880);for(unsigned i=0;i<20;++i)Tick(1.f);
        assert(TrialCoopReviveSeconds(slot)==1&&!fail_requests);
        Put(B(),0x620010,0x80);presentation=2;Tick(20.f);presentation=1;
        for(float invalid:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})Tick(invalid);
        assert(TrialCoopReviveSeconds(slot)==1&&!fail_requests);
        Tick(.5f);assert(TrialCoopReviveSeconds(slot)==1&&!fail_requests);
        X86Context c{};c.esp=0x7e0000;c.ebp=0x620000;c.eax=0x12345678;c.ebx=42;
        c.ecx=43;c.edx=44;c.edi=45;c.esi=46;c.st[0]=1.25;c.top=2;c.xmm[3].u[1]=47;c.cf=1;
        const auto saved=c;const unsigned csr=_mm_getcsr();_mm_setcsr(0x3f80);
        float delta=.5f;std::memcpy(B()+0x620014,&delta,4);TrialCoopDeathUpdate(c,B());
        assert(!std::memcmp(&c,&saved,sizeof c)&&_mm_getcsr()==0x3f80);_mm_setcsr(csr);
        assert(team_failed&&failure_sent&&!TrialCoopReviveSeconds(slot));
        assert(fail_requests==1&&FailureSelected(slot==0)&&failure_cleanup==1);
        assert(PositiveHealth(B(),kPlayers[1-slot]));
        assert(!TrialCoopRevivePlayer(c,B(),kPlayers[slot])); // Queued injection is too late.
        for(unsigned i=0;i<20;++i)Tick(1.f);
        assert(fail_requests==1);
    }
    Setup();DownPlayer(0);Tick(.25f);for(unsigned i=0;i<38;++i)Tick(.25f);
    X86Context c{};c.esp=0x7e0000;
    assert(TrialCoopRevivePlayer(c,B(),kPlayers[0])&&!TrialCoopReviveSeconds(0));
    Tick(20.f);assert(!fail_requests&&!team_failed&&!FailureSelected());
    // Both down uses the same proven terminal queue immediately, once.
    Setup();DownPlayer(0);Tick(.25f);DownPlayer(1);assert(PolicyDestination(1)==1);
    Tick(.25f);assert(!TrialCoopReviveSeconds(0)&&!TrialCoopReviveSeconds(1)&&fail_requests==1&&FailureSelected());
    Tick(20.f);assert(fail_requests==1);
    for(unsigned invalid=0;invalid<6;++invalid){
        Setup();DownPlayer(0);Tick(.25f);assert(TrialCoopReviveSeconds(0)==10);
        if(invalid==0)trial_coop::SetActive(false);
        if(invalid==1)Put(B(),kPlayers[0]+0x54,999);
        if(invalid==2)Put(B(),kBodies[0]+0x54,999);
        if(invalid==3)Put(B(),kPlayers[1]+0x5c,1);
        if(invalid==4)Put(B(),0x51c4dc,0x621000);
        if(invalid==5)TrialCoopDeathReset();
        Tick(20.f);assert(!TrialCoopReviveSeconds(0)&&!fail_requests);
    }
    // Invalid dispatch state fails closed for revival; a valid mode can retry.
    for(unsigned invalid=0;invalid<3;++invalid){
        Setup();DownPlayer(0);Tick(.25f);
        if(invalid==0)Put(B(),0x509a60,0);
        if(invalid==1)Put(B(),0x6c2000,0x44bf20);
        if(invalid==2)Put(B(),0x45270c,0x12fd0);
        Tick(10.f);assert(team_failed&&!failure_sent&&!fail_requests&&!TrialCoopRevivePlayer(c,B(),kPlayers[0]));
        Put(B(),0x509a60,0x6c2000);Put(B(),0x6c2000,0x4526f0);Put(B(),0x45270c,0x69280);
        Tick(.25f);Tick(.25f);assert(fail_requests==1&&failure_sent&&FailureSelected());
    }
    assert(!TrialCoopReviveSeconds(2));
}

int main(){
    TestRevivedVisibility();
    TestCountdown();
    for(unsigned slot=0;slot<2;++slot){
        Setup();auto c=Caller(slot);OriginalDeathBodyCaller(c,B());assert(!body_calls&&queries==1);
        c=Caller(slot);CoopDeathBodyCaller(c,B());assert(body_calls==1&&ragdoll_branches==1&&!animation_branches);
        assert(c.esp==0x7e0000&&c.esi==kPlayers[slot]);assert(PolicyDestination(slot)==1);
        Put(B(),kPlayers[slot]+0x170,12);assert(TrialCoopPlayerDowned(B(),kPlayers[slot]));
        c={};c.esi=kBodies[slot];c.esp=0x7e0000;c.xmm[0].f[0]=10;
        OriginalBodyExpiry(c,B());assert(expires==1);
        c.esi=kBodies[slot];c.esp=0x7e0000;c.xmm[0].f[0]=10;
        CoopBodyExpiry(c,B());assert(expires==1);
        assert(TrialCoopNearbyDownedPlayer(B(),kPlayers[1-slot])==kPlayers[slot]);
        float distance=201;std::memcpy(B()+kPlayers[1-slot]+0x40,&distance,4);
        assert(!TrialCoopNearbyDownedPlayer(B(),kPlayers[1-slot]));
        Put(B(),kPlayers[1-slot]+0x40,0);
        // The corpse slid far from the actor's death transform. Proximity must
        // follow the visible physics body and revival must place collision there.
        physical_position[slot][0]=1000;physical_position[slot][1]=200;physical_position[slot][2]=300;
        float contact[3]{};
        assert(TrialCoopDownedPosition(B(),kPlayers[slot],contact)&&!std::memcmp(contact,physical_position[slot],sizeof contact));
        assert(!TrialCoopDownedPosition(B(),kPlayers[1-slot],contact));
        assert(!TrialCoopNearbyDownedPlayer(B(),kPlayers[1-slot]));
        std::memcpy(B()+kPlayers[1-slot]+0x40,physical_position[slot],12);
        assert(TrialCoopNearbyDownedPlayer(B(),kPlayers[1-slot])==kPlayers[slot]);
        std::array<uint8_t,48> orientation{};std::memcpy(orientation.data(),B()+kPlayers[slot]+0x10,48);
        c={};c.esp=0x7e0000;c.edi=81;c.ebx=82;c.eax=83;c.edx=84;c.ecx=85;c.ebp=86;
        c.st[0]=3.25;c.top=2;c.mxcsr=0x3f80;c.cf=1;c.df=1;c.xmm[2].u[0]=91;
        const auto saved=c;const auto host=_mm_getcsr();_mm_setcsr(0x3f80);
        Put(B(),kPlayers[slot]+0x854,0x12345601);Put(B(),kPlayers[slot]+0x84c,0x660000);Put(B(),kPlayers[slot]+0x850,2);
        valid_physics=false;assert(!TrialCoopRevivePlayer(c,B(),kPlayers[slot])&&!resets&&!recreates);
        assert(!TrialCoopDownedPosition(B(),kPlayers[slot],contact));
        valid_physics=true;
        fail_recreate=true;assert(!TrialCoopRevivePlayer(c,B(),kPlayers[slot]));assert(!resets);
        fail_recreate=false;assert(TrialCoopRevivePlayer(c,B(),kPlayers[slot]));
        assert(resets==1&&recreates==1&&collision_releases==1&&collision_creates==1);
        assert(placements==1&&!std::memcmp(B()+kPlayers[slot]+0x40,physical_position[slot],12));
        assert(!std::memcmp(B()+kPlayers[slot]+0x10,orientation.data(),48));
        // Retail stores 0.01 as float, just below exact 1/100. This fixture
        // deliberately preserves round-down MXCSR, so 10/3/2 can be one ULP
        // lower. Keep the expected axis order and scale independently fixed.
        constexpr uint32_t expected_physics[]={0x41200000,0x40400000,0x40000000};
        for(unsigned axis=0;axis<3;++axis){
            uint32_t actual;std::memcpy(&actual,&placed_physics[axis],4);
            assert(actual>=expected_physics[axis]-1&&actual<=expected_physics[axis]);
        }
        assert(Read(B(),kPlayers[slot]+0x170)==0&&Read(B(),kPlayers[slot]+0x9c)==0x42c80000);
        assert(Read(B(),kPlayers[slot]+0x854)==0x12345601&&Read(B(),kPlayers[slot]+0x84c)==0x660000&&Read(B(),kPlayers[slot]+0x850)==2);
        assert(!std::memcmp(&c,&saved,sizeof c)&&_mm_getcsr()==0x3f80);_mm_setcsr(host);
        assert(!TrialCoopPlayerDowned(B(),kPlayers[slot]));
        assert(!TrialCoopDownedPosition(B(),kPlayers[slot],contact));
        assert(!TrialCoopRevivePlayer(c,B(),kPlayers[slot]));
    }
    Setup();DownPlayer(0);assert(PolicyDestination(0)==1);DownPlayer(1);assert(PolicyDestination(1)==1);
    X86Context c{};c.esp=0x7e0000;assert(!TrialCoopRevivePlayer(c,B(),kPlayers[0]));
    assert(!TrialCoopKeepDownedBody(B(),kBodies[0])&&!TrialCoopNearbyDownedPlayer(B(),kPlayers[0]));
    // Restart can reuse every address and entity ID. The authored-player
    // creation seam must reset policy even when pointer identity is unchanged.
    trial_coop::SetActive(false);TrialCoopDeathReset();trial_coop::SetActive(true);
    for(unsigned i=0;i<2;++i){Put(B(),kPlayers[i]+0x170,0);Put(B(),kPlayers[i]+0x9c,0x42c80000);B()[kBodies[i]+0x198]=0;}
    DownPlayer(0);assert(PolicyDestination(0)==1&&!team_failed);
    for(unsigned invalid=0;invalid<5;++invalid){
        Setup();DownPlayer(0);
        if(invalid==0)trial_coop::SetActive(false);
        if(invalid==1)Put(B(),kPlayers[0]+0x54,999);
        if(invalid==2)Put(B(),kBodies[0]+0x54,999);
        if(invalid==3)Put(B(),kPlayers[1]+0x5c,1);
        if(invalid==4)Put(B(),0x51c4dc,0x621000);
        assert(!TrialCoopPlayerDowned(B(),kPlayers[0]));assert(!TrialCoopRevivePlayer(c,B(),kPlayers[0]));
        assert(!TrialCoopKeepDownedBody(B(),kBodies[0]));
        float contact[3]{};assert(!TrialCoopDownedPosition(B(),kPlayers[0],contact));
    }
    Setup();c=Caller(0);trial_coop::SetActive(false);CoopDeathBodyCaller(c,B());assert(!body_calls);
    c.esi=kBodies[0];c.ebp=0x7e1000;Put(B(),c.ebp+12,6);OriginalRagdollBranch(c,B());assert(animation_branches==1);
    CoopRagdollBranch(c,B());assert(animation_branches==2&&!ragdoll_branches);
    puts("co-op downed: original death, first-person visibility and terminal playerDied receiver witnesses; repeat revival for both slots, shield gates, 10-second 30/60/120 FPS countdown, pause/cinematic freeze, identity reset, revival deadline, safe failure dispatch, context preservation and both-down checks passed (offline; rendered model pixels, physics and mission-fail presentation require live validation)");
}
