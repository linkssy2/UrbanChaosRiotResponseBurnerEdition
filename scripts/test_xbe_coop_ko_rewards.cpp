// Retail event forwarding, knockout scheduler, delayed counter/award branches
// and final popup consumer. Generic event-registry dispatch, localized strings,
// animation setup, and allocated HUD queue storage are explicit boundaries.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x1000000> memory{};}
namespace mem {unsigned checks=0;bool IsCommitted(uint32_t p){++checks;return p<kernel::memory.size();}}
#include "../common/xbe_coop_ko_rewards.cpp"
constexpr uint32_t P[2]={0x610000,0x612000},W=0x620000,C=0x640000;
uint32_t V=0x630000;
constexpr uint32_t EVENT=0x650000,IMPACT=0x651000,PROFILE=0x660000,STATS=0x661000;
constexpr uint32_t TEXT=0x670000,SP=0xfe0000,FS=0x680000,M[2]={0x690000,0x691000};
unsigned delivered[2]{},goal=3,forwarded=0;bool missing_manager=false;
uint32_t last_caller=0,last_kind=0;
uint8_t* B(){return kernel::memory.data();}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
uint32_t TrialCoopPlayer(unsigned i){return i<2?P[i]:0;}
uint32_t TrialCoopRewardManager(const uint8_t* b,uint32_t player){
    if(!trial_coop::Active()||missing_manager||Read(b,0x51c4dc)!=W)return 0;
    for(unsigned i=0;i<2;++i)if(player==P[i]&&Read(b,player)==0x46fbe8u&&
        Read(b,player+0x54)==100+i&&!(Read(b,player+0x5c)&1))return M[i];
    return 0;
}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){assert(false);}
X86_FUNC(sub_000159C0){ctx.esp+=12;}
X86_FUNC(sub_001F8EA0){ctx.esp+=8;}
X86_FUNC(sub_001F8DE0){ctx.esp+=16;}
X86_FUNC(sub_001F8BA0){ctx.esp+=4;}
X86_FUNC(sub_001F92B0){const auto out=Read(base,ctx.esp+4);Put(out,Read(base,ctx.esp+8));ctx.eax=out;ctx.esp+=4;}
X86_FUNC(sub_0020A260){ctx.eax=0;ctx.esp+=16;}
X86_FUNC(sub_00032CB0){ctx.esp+=8;}
X86_FUNC(sub_001F8FC0){ctx.eax=Read(base,ctx.esp+4)==Read(base,V+0x54)?V:0;ctx.esp+=4;}
X86_FUNC(sub_0004A840){ctx.eax=PROFILE;ctx.esp+=4;}
X86_FUNC(sub_0025CE20){assert(Read(base,ctx.esp+4)==0);ctx.eax=goal;ctx.esp+=4;}
X86_FUNC(sub_00262160){ctx.esp+=4;}
X86_FUNC(sub_002D6322){ctx.esp+=4;}
X86_FUNC(sub_000DEFC0){ctx.esp+=12;}
X86_FUNC(sub_00218C40){ctx.esp+=4;}
X86_FUNC(sub_00077E60){
    assert(ctx.ecx==M[0]||ctx.ecx==M[1]);++delivered[ctx.ecx==M[1]];
    last_kind=Read(base,ctx.esp+4);ctx.esp+=24;
}
// ORIGINAL_0004ACD0
X86_FUNC(sub_0004ACD0){__imp__sub_0004ACD0(ctx,base);}
// ORIGINAL_IMPACT_GATES
// ORIGINAL_000DDCA0
// ORIGINAL_00036800
// ORIGINAL_000DF380
// ORIGINAL_001F90A0
X86_FUNC(sub_001F90A0){__imp__sub_001F90A0(ctx,base);}
// ORIGINAL_001998B0
// ORIGINAL_00073060
X86_FUNC(sub_00073060){last_caller=Read(base,ctx.esp);__imp__sub_00073060(ctx,base);}
// ORIGINAL_DELAYED_AWARD
// COOP_DAMAGE_END
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    // The real entity registry found the victim, and the original forwarding
    // wrapper supplied event, payload, size=8, and source entity descriptor.
    assert(fn==0xabcdefu&&c.ecx==V&&Read(b,c.esp+4)==Read(b,0x4f9dc8));
    assert(Read(b,c.esp+8)==EVENT&&Read(b,c.esp+12)==8);
    const auto source=Read(b,c.esp+16);assert(TrialGuestReadable(source,4));
    ++forwarded;auto callback=c;callback.esp=SP-0x2000;callback.ecx=V;
    Put(callback.esp,0x1234);Put(callback.esp+4,EVENT);Put(callback.esp+8,source);
    __imp__sub_000DF380(callback,b);assert(callback.esp==SP-0x2000+12);
    c.eax=1;c.esp+=20;
}
X86Context Context(){X86Context c{};c.esp=SP;c.fs_base=FS;c.ecx=V;c.esi=V;return c;}
void Setup(){
    V=0x630000;
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetActive(true);TrialCoopKORewardsReset();
    delivered[0]=delivered[1]=forwarded=0;goal=3;last_caller=last_kind=0;missing_manager=false;
    Put(0x51c4dc,W);Put(0x51c398,0x622000);Put(0x4f9dc8,73);Put(0x51613c,M[1]);
    Put(V,0x45c770);Put(0x45c770,0xabcdef);Put(V+0x54,700);Put(C+0x54,701);Put(C+0x88,V);
    Put(PROFILE+0xc,STATS);Put(PROFILE+0x834,101);Put(EVENT,700);B()[EVENT+5]=1;
    for(unsigned i=0;i<2;++i){Put(P[i],0x46fbe8);Put(P[i]+0x54,100+i);Put(P[i]+0x368,i);}
}
void DamageHit(uint32_t owner,uint32_t flags=0x42000){
    auto c=Context();c.ecx=C;Put(SP+4,flags);Put(SP+8,0x3f800000);Put(SP+12,2);Put(SP+16,owner);
    __imp__sub_000DDCA0(c,B());assert(c.esp==SP+20&&Read(B(),C+0xb8)==owner);
}
void CycleEnd(){auto c=Context();c.esi=C;CoopDamageEnd(c,B());}
unsigned DamagePending(){unsigned n=0;for(const auto& d:damage)n+=bool(d.controller);return n;}
void Event(uint32_t owner,bool direct=false,uint32_t caller=0,bool seed=true){
    auto c=Context();if(!direct&&seed)DamageHit(owner);
    if(!direct){
        auto normalize=Context();normalize.ecx=C;Put(SP+4,EVENT+0x20);Put(SP+8,EVENT+0x24);Put(EVENT+0x20,0);
        __imp__sub_00036800(normalize,B());assert(normalize.esp==SP+12&&(Read(B(),EVENT+0x20)&0x1f0fe)==0x2000);
    }
    c.ecx=c.esi=direct?V:C;c.edi=IMPACT;Put(IMPACT+0x10,owner);
    if(direct){CoopDirectKO(c,B());assert(c.esp==SP);if(!c.eax)return;c.ecx=c.esi;}
    Put(SP,caller?caller:direct?0xe190c:0x38266);Put(SP+4,Read(B(),V+0x54));Put(SP+8,73);Put(SP+12,EVENT);
    __imp__sub_001998B0(c,B());assert(c.esp==SP+16&&!incoming.victim);
    if(!direct){CycleEnd();assert(!DamagePending());}
}
void Step(){
    auto c=Context();c.esp=SP-0x4c;c.esi=V;c.ecx=V;
    Put(c.esp,0);Put(c.esp+4,0);Put(c.esp+8,0);DelayedAward(c,B());assert(c.esp==SP+4);
}
unsigned Pending(){unsigned n=0;for(const auto& a:pending)n+=bool(a.victim);assert(n==pending_count);return n;}
void CheckPendingOccupancy(){
    Setup();assert(!Pending());auto c=Context();unsigned checks=0;
    {TrialCoopKOUpdateScope update(c,B());checks=mem::checks;}
    assert(mem::checks==checks); // Empty updates do not qualify victim memory.
    Put(SP,0xe25b6);c.ecx=M[1];TrialCoopKORecipient(c,B());assert(!c.ecx&&!Pending());
    c.ecx=0;TrialCoopKOForget(c);assert(!Pending());
    // An update can enqueue an award after its scope was constructed empty.
    c=Context();{TrialCoopKOUpdateScope update(c,B());Event(100);assert(Pending()==1);Put(V+0x110,0);}
    assert(!Pending());
    Setup();Event(100);assert(Pending()==1);c=Context();c.esi=0;Put(SP,0xe25b6);
    TrialCoopKORecipient(c,B());assert(!c.ecx&&Pending()==1);
    c.ecx=0;TrialCoopKOForget(c);assert(Pending()==1);
    c=Context();TrialCoopKOForget(c);TrialCoopKOForget(c);assert(!Pending());
    // A reused address with a different ID replaces, rather than adds to, ownership.
    Setup();Event(100);Put(V+0x54,701);Put(EVENT,701);Put(V+0x198,0);Put(V+0x110,0);
    Event(101);assert(Pending()==1);Step();Step();assert(!Pending()&&!delivered[0]&&delivered[1]==1);
    for(unsigned stale=0;stale<3;++stale){
        Setup();for(unsigned i=0;i<64;++i){
            V=0x630000+i*0x200;Put(V,0x45c770);Put(V+0x54,700+i);Put(C+0x88,V);Put(EVENT,700+i);Event(100);
            assert(Pending()==i+1);
        }
        if(stale==0)Put(0x630000+0x54,999);
        if(stale==1)Put(0x630000+0x110,0);
        if(stale==2)Put(0x630000+0x5c,1);
        V=0x638000;Put(V,0x45c770);Put(V+0x54,764);Put(C+0x88,V);Put(EVENT,764);
        Event(101);assert(Pending()==64);Step();Step();assert(Pending()==63&&delivered[1]==1);
        c=Context();for(unsigned i=0;i<64;++i){c.ecx=0x630000+i*0x200;TrialCoopKOForget(c);}
        assert(!Pending());
    }
}
int main(){
    CheckPendingOccupancy();
    for(unsigned slot=0;slot<2;++slot)for(uint32_t flags:{0x42000u,0x42040u,0x42080u}){
        Setup();DamageHit(100+slot,flags);DamageHit(101-slot,0x40008);DamageHit(0xffffffffu,0x40002);
        Event(0,false,0,false);Step();Step();assert(delivered[slot]==1&&!delivered[1-slot]&&Read(B(),STATS)==1);
        Setup();DamageHit(100+slot,flags);DamageHit(101-slot,0x42000);
        Event(0,false,0,false);Step();Step();assert(delivered[slot]==1&&!delivered[1-slot]);
    }
    Setup();DamageHit(900);DamageHit(100,0x40008);Event(0,false,0,false);Step();Step();assert(!delivered[0]&&!delivered[1]);
    Setup();DamageHit(100);assert(DamagePending()==1);CycleEnd();assert(!DamagePending());
    DamageHit(101);Event(0,false,0,false);Step();Step();assert(!delivered[0]&&delivered[1]==1);
    // Exercise the actual original singleton comparisons before the direct KO
    // event, including a negative witness for Player 1 and unrelated AI hits.
    for(bool impact:{false,true})for(uint32_t owner:{100u,101u,900u,0xffffffffu}){
        Setup();Put(IMPACT+0x10,owner);auto c=Context();c.edi=IMPACT;
        if(impact)OriginalImpactPlayer(c,B());else OriginalDirectKO(c,B());
        assert(c.esp==SP&&c.eax==(owner==101));
        c=Context();c.edi=IMPACT;
        if(impact)CoopImpactPlayer(c,B());else CoopDirectKO(c,B());
        assert(c.esp==SP&&c.eax==(owner==100||owner==101));
    }
    for(unsigned invalid=0;invalid<3;++invalid){
        Setup();Put(IMPACT+0x10,100);auto c=Context();c.edi=IMPACT;
        if(invalid==0)Put(P[0]+0x54,999);
        if(invalid==1)missing_manager=true;
        if(invalid==2)trial_coop::SetActive(false);
        CoopDirectKO(c,B());assert(c.esp==SP&&!c.eax);
    }
    Setup();Event(900,true);assert(!forwarded&&!Pending()&&!Read(B(),V+0x110));
    for(bool direct:{false,true})for(unsigned slot=0;slot<2;++slot)for(unsigned threshold:{0u,1u,3u}){
        Setup();goal=threshold;Event(100+slot,direct);assert(forwarded==1&&Read(B(),V+0x110)==2&&Pending()==1);
        Step();assert(Read(B(),V+0x110)==1&&delivered[0]+delivered[1]==0&&Read(B(),STATS)==0);
        // A later player/environment hit cannot steal a successful knockout.
        DamageHit(100+(1-slot));DamageHit(0xffffffffu);Step();
        assert(delivered[slot]==1&&delivered[1-slot]==0&&Read(B(),STATS)==1&&!Pending());
        assert(last_caller==(threshold==1?0xe25b6u:threshold==3?0xe261bu:0xe2677u));
        assert(last_kind==(threshold==1?3u:threshold==3?1u:2u));
        Step();assert(Read(B(),STATS)==1&&delivered[slot]==1);
    }
    Setup();Event(100);Event(101);assert(Pending()==1);Step();Step();assert(delivered[0]==1&&!delivered[1]);
    // Original early gate refuses re-incapacitating an already downed actor.
    for(uint32_t flags:{1u,4u}){Setup();Put(V+0x198,flags);Event(100);assert(!Pending()&&!Read(B(),V+0x110));}
    for(uint32_t owner:{0xffffffffu,900u}){Setup();Event(owner);Step();Step();assert(!delivered[0]&&!delivered[1]&&Read(B(),STATS)==1);}
    Setup();Event(100,false,0x10767b);Step();Step();assert(!delivered[0]&&!delivered[1]); // scripted event is not a player action
    for(unsigned invalid=0;invalid<6;++invalid){
        Setup();Event(100);
        if(invalid==0)Put(V+0x54,999);
        if(invalid==1)Put(V+0x5c,1);
        if(invalid==2)Put(P[0]+0x54,999);
        if(invalid==3)Put(0x51c4dc,W+0x100);
        if(invalid==4)missing_manager=true;
        if(invalid==5){auto c=Context();TrialCoopKOForget(c);Put(V+0x198,0);} // reconstructed same pointer/id
        Step();Step();assert(!delivered[0]&&!delivered[1]&&!Pending());
    }
    Setup();Event(100);Put(V+0x110,0);Step();assert(!Pending()&&!delivered[0]&&!Read(B(),STATS));
    for(unsigned i=0;i<130;++i){Put(V+0x198,0);Event(100+(i&1));Put(V+0x110,0);Step();assert(!Pending());}
    Setup();for(unsigned i=0;i<65;++i){
        V=0x630000+i*0x200;Put(V,0x45c770);Put(V+0x54,700+i);Put(C+0x88,V);Put(EVENT,700+i);Event(100);
    }
    assert(Pending()==64);Step();Step();assert(!delivered[0]&&!delivered[1]&&Pending()==64); // bounded overflow cannot choose P2
    V=0x630000;Step();Step();assert(delivered[0]==1&&Pending()==63);
    Setup();Event(100);TrialCoopKORewardsReset();assert(!Pending());Step();Step();assert(!delivered[0]&&!delivered[1]&&!Pending());
    Setup();trial_coop::SetActive(false);Event(100);Step();Step();assert(!delivered[0]&&delivered[1]==1); // retail single-player path retained
    Setup();auto c=Context();Put(SP,0x3768c);c.ecx=M[1];TrialCoopKORecipient(c,B());assert(c.ecx==M[1]);
    std::puts("KO rewards: retail event forwarding, successful scheduling, both delayed updates, all three award branches and unchanged count, each causal player/direct impact, later damage and repeated-event isolation, unknown/scripted source suppression, stale/despawn/reused actor, empty-scan gates, exact pending occupancy/replacement/overflow, cancellation/reset and single-player preservation passed; live pixels remain untested");
}
