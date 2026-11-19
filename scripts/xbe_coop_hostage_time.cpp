#include "xbe_coop_hostage_time.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>
#include <xmmintrin.h>

namespace {
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
struct Pair {uint32_t actor[2]{},id[2]{},manager=0;unsigned native=2;};
thread_local uint32_t query_goal=0,query_player=0xffffffffu;
bool Alive(const uint8_t* b,uint32_t actor){
    const auto hp=Word(b,actor+0x9c);
    return hp>0&&hp<0x7f800000u&&Word(b,actor+0x170)!=12;
}
bool Players(const uint8_t* b,uint32_t goal,Pair& p){
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(goal,0x1a0)||Word(b,goal+0x54)==0xffffffffu||(Word(b,goal+0x5c)&1))return false;
    // All retail NewHostageTaker goal variants share admission/ramp/recovery.
    const auto type=Word(b,goal);
    if(type!=0x46d510u&&type!=0x46d5f8u&&type!=0x46d708u&&type!=0x46d7a8u&&type!=0x46d8a0u)return false;
    if(!TrialGuestReadable(0x509a60,4)||!TrialGuestReadable(0x51c4dc,4))return false;
    p.manager=Word(b,0x509a60);const auto world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(p.manager,0x838)||!TrialGuestReadable(world,8))return false;
    const auto state=Word(b,world+4);
    if(!TrialGuestReadable(state,12))return false;
    const auto first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return false;
    const auto selected=Word(b,p.manager+0x834);
    for(unsigned slot=0;slot<2;++slot){
        const auto actor=TrialCoopPlayer(slot),layer=Word(b,first+4*slot);
        if(!TrialGuestReadable(actor,0x36c)||Word(b,actor)!=0x46fbe8u||
           (Word(b,actor+0x5c)&1)||Word(b,actor+0x368)!=slot||
           !TrialGuestReadable(layer,0x28)||Word(b,layer+4)!=actor||Word(b,layer+0x24)!=slot)return false;
        p.actor[slot]=actor;p.id[slot]=Word(b,actor+0x54);
        if(p.id[slot]==0xffffffffu)return false;
        if(p.id[slot]==selected)p.native=slot;
    }
    return p.id[0]!=p.id[1]&&p.native<2;
}
struct ScopedGuestCall {
    uint32_t old_goal=query_goal,old_player=query_player;unsigned csr=_mm_getcsr();
    ScopedGuestCall(uint32_t goal,uint32_t player){query_goal=goal;query_player=player;}
    ~ScopedGuestCall(){query_goal=old_goal;query_player=old_player;_mm_setcsr(csr);}
};
}

void TrialCoopHostageSlowmoAdmission(X86Context& c,uint8_t* b){
    Pair p;if(query_goal||!Players(b,c.esi,p))return;
    if((c.eax&255)&&Alive(b,p.actor[p.native]))return;
    c.eax&=0xffffff00u;
    const auto partner=p.native^1;
    if(!Alive(b,p.actor[partner]))return;
    for(unsigned offset:{0x40u,0x44u,0x48u})
        if((Word(b,p.actor[partner]+offset)&0x7fffffffu)>=0x7f800000u)return;
    if(c.esp<0x10040u||!TrialGuestReadable(c.esp-0x40,0x40))return;
    // Retry the original LOS AND aiming-line admission for the other living
    // player. Only the two player lookups inside that predicate are substituted;
    // no global singleton, authored window, hit logic or time scale is rewritten.
    // Reuse the just-returned native query's stack position. Guest x86 has no
    // live red zone below ESP; only the caller frame at/above ESP is preserved.
    ScopedGuestCall restore(c.esi,p.id[partner]);X86Context temporary=c;
    temporary.ecx=c.esi;temporary.df=0;
    const auto accepted=kernel::CallGuest(temporary,0x1600d0,{})&255u;
    c.eax=(c.eax&0xffffff00u)|accepted;
}

void TrialCoopHostageSlowmoQueryPlayer(X86Context& c,const uint8_t* b){
    Pair p;if(!query_goal||c.esi!=query_goal||!Players(b,c.esi,p)||
       c.ecx!=p.manager||c.eax!=p.id[p.native])return;
    for(unsigned slot=0;slot<2;++slot)if(query_player==p.id[slot]&&Alive(b,p.actor[slot])){c.eax=query_player;return;}
}

void TrialCoopHostageSlowmoLivingPlayer(X86Context& c,const uint8_t* b){
    Pair p;if(!Players(b,c.esi,p)||c.ecx!=p.manager||c.eax!=p.id[p.native]||Alive(b,p.actor[p.native]))return;
    if(Alive(b,p.actor[p.native^1]))c.eax=p.id[p.native^1];
}

void TrialCoopHostageSlowmoFeedback(X86Context& c,uint8_t* b){
    Pair p;if(!Players(b,c.esi,p)||c.esp<0x10018u||
       !TrialGuestReadable(c.esp-24,44)||!TrialGuestReadable(0x51c398,4)||!TrialGuestReadable(0x4f9f6c,4))return;
    const auto services=Word(b,0x51c398);
    if(!TrialGuestReadable(services,12)||c.ecx!=Word(b,services+8)||
       Word(b,c.esp+4)!=Word(b,0x4f9f6c)||Word(b,c.esp+12)!=4||
       !TrialGuestReadable(Word(b,c.esp+8),4)||!TrialGuestReadable(Word(b,c.esp+16),4)||
       Word(b,Word(b,c.esp+16))!=Word(b,c.esi+0x54))return;
    const auto original=Word(b,c.esp);
    const auto partner=original==p.id[0]?p.id[1]:original==p.id[1]?p.id[0]:0xffffffffu;
    if(partner==0xffffffffu)return;
    // The original update still delivers its own event. Mirror the same native
    // view-feedback payload to the other admitted view, including a downed one.
    // MainPlayer consumes/resets +364 every tick, so ending the authored window
    // restores ordinary view behavior without a second timer or persistent state.
    ScopedGuestCall restore(0,0xffffffffu);X86Context temporary=c;temporary.df=0;
    kernel::CallGuest(temporary,0x1f90a0,{partner,Word(b,c.esp+4),Word(b,c.esp+8),4,Word(b,c.esp+16)});
}
