#include "xbe_coop_ko_rewards.h"
#include "xbe_coop_rewards.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include <array>
#include <cstring>

namespace {
uint32_t Read(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
struct Award {uint32_t world=0,victim=0,victim_id=0,player=0,player_id=0;};
thread_local Award incoming;
// Retail delays INCAPACITATED by two NPC updates. Freeze the successful event's
// owner, rather than consulting the current player or a later damage source.
// shortcut: At most 64 concurrent delayed KO notices; overflow keeps native
// scoring but suppresses that notice. Expand if a verified scene exceeds 64.
std::array<Award,64> pending{};
unsigned pending_count=0;
struct Damage {Award award{};uint32_t controller=0;};
// shortcut: The same 64-source bound applies before native taser dispatch;
// overflow omits only its personal notice. Expand if a scene exceeds the bound.
std::array<Damage,64> damage{};
bool Active(const uint8_t* b){
    return b&&trial_coop::Active()&&TrialGuestReadable(0x51c4dc,4)&&
        TrialGuestReadable(Read(b,0x51c4dc),8);
}
bool Victim(const uint8_t* b,uint32_t actor){
    if(!TrialGuestReadable(actor,0x19c)||(Read(b,actor+0x5c)&1)||
       Read(b,actor+0x54)==0xffffffffu)return false;
    // Both original NPC classes inheriting E2100's delayed award update.
    return Read(b,actor)==0x45c770u||Read(b,actor)==0x47bf50u;
}
bool Current(const uint8_t* b,const Award& a){
    return a.world==Read(b,0x51c4dc)&&Victim(b,a.victim)&&
        Read(b,a.victim+0x54)==a.victim_id;
}
bool CurrentDamage(const uint8_t* b,const Damage& d){
    return d.controller&&Current(b,d.award)&&TrialGuestReadable(d.controller,0xbc)&&
        !(Read(b,d.controller+0x5c)&1)&&Read(b,d.controller+0x88)==d.award.victim&&
        (Read(b,d.controller+0x9c)&0x42000)==0x42000;
}
void Owner(const uint8_t* b,Award& award,uint32_t source){
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t player=TrialCoopPlayer(slot);
        if(TrialGuestReadable(player,0x36c)&&Read(b,player+0x54)==source&&
           TrialCoopRewardManager(b,player)){
            award.player=player;award.player_id=source;return;
        }
    }
}
void Forget(uint32_t actor){
    if(!pending_count||!actor)return;
    for(auto& a:pending)if(a.victim==actor){a={};--pending_count;}
}
}
void TrialCoopKORewardsReset(){pending={};pending_count=0;damage={};incoming={};}
void TrialCoopKOForget(const X86Context& c){
    Forget(c.ecx);for(auto& d:damage)if(d.award.victim==c.ecx)d={};
}
void TrialCoopKODamage(const X86Context& c,const uint8_t* b){
    if(!Active(b)||!TrialGuestReadable(c.esp,8)||!TrialGuestReadable(c.ecx,0xbc)||
       (Read(b,c.ecx+0x5c)&1))return;
    // Incoming taser hits can also carry head/limb bits; 36800 normalizes them
    // before 37D10 dispatch. Preserve the first taser, not the latest body hit.
    if((Read(b,c.esp+4)&0x42000)!=0x42000)return;
    const uint32_t victim=Read(b,c.ecx+0x88);
    if(!Victim(b,victim))return;
    Damage* available=nullptr;
    for(auto& d:damage){
        if(d.controller&&!CurrentDamage(b,d))d={};
        if(d.controller==c.ecx)return;
        if(!d.controller&&!available)available=&d;
    }
    if(!available)return;
    *available={{Read(b,0x51c4dc),victim,Read(b,victim+0x54),0,0},c.ecx};
    Owner(b,available->award,c.eax);
}
void TrialCoopKOCycleFinished(const X86Context& c,const uint8_t*){
    if(!trial_coop::Active())return;
    for(auto& d:damage)if(d.controller==c.esi){d={};return;}
}
void TrialCoopKODirectPlayer(X86Context& c,const uint8_t* b){
    if(!Active(b)||!Victim(b,c.esi)||!TrialGuestReadable(c.edi,0x24)||
       Read(b,c.edi+0x10)!=c.ebx)return;
    bool registered=false;uint32_t source=0;
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t player=TrialCoopPlayer(slot);
        if(!TrialCoopRewardManager(b,player))continue;
        const uint32_t id=Read(b,player+0x54);
        registered|=c.eax==id;if(c.ebx==id)source=player;
    }
    // Only admit the actual impact owner at E1570's player comparison. The
    // original range, damage, hostility, hostage, and incapacitation gates run.
    if(registered&&source)c.eax=c.ebx;
}
TrialCoopKOUpdateScope::TrialCoopKOUpdateScope(const X86Context& c,const uint8_t* b)
    :base_(b),victim_(Active(b)?c.ecx:0){}
TrialCoopKOUpdateScope::~TrialCoopKOUpdateScope(){
    if(pending_count&&victim_&&(!Victim(base_,victim_)||!Read(base_,victim_+0x110)))Forget(victim_);
}

TrialCoopKOEventScope::TrialCoopKOEventScope(const X86Context& c,const uint8_t* b)
    :world_(incoming.world),victim_(incoming.victim),victim_id_(incoming.victim_id),
     player_(incoming.player),player_id_(incoming.player_id){
    incoming={};
    if(!Active(b)||!TrialGuestReadable(c.esp,16)||
       !TrialGuestReadable(0x4f9dc8,4)||Read(b,c.esp+8)!=Read(b,0x4f9dc8))return;
    const uint32_t caller=Read(b,c.esp),payload=Read(b,c.esp+12);
    uint32_t victim=0,source=0xffffffffu;
    if(caller==0x38266u){
        // Native pending flags combine damage. +B8 may already refer to a later
        // body/environment hit, so use the first qualifying taser's source.
        if(!TrialGuestReadable(c.ecx,0xbc)||c.ecx!=c.esi)return;
        victim=Read(b,c.ecx+0x88);
        for(const auto& d:damage)if(d.controller==c.ecx&&CurrentDamage(b,d)){
            incoming=d.award;break;
        }
    }else if(caller==0xe190cu){
        // E1570's direct knockout event precedes the ordinary damage update;
        // the impact payload already contains its causal actor at +10.
        if(c.ecx!=c.esi||!TrialGuestReadable(c.edi,0x24))return;
        victim=c.esi;source=Read(b,c.edi+0x10);
    }else return;
    if(!Victim(b,victim)||Read(b,c.esp+4)!=Read(b,victim+0x54)||
       !TrialGuestReadable(payload,8)||Read(b,payload)!=Read(b,victim+0x54)||!b[payload+5]){incoming={};return;}
    if(caller==0xe190cu){
        incoming={Read(b,0x51c4dc),victim,Read(b,victim+0x54),0,0};
        Owner(b,incoming,source);
    }
}
TrialCoopKOEventScope::~TrialCoopKOEventScope(){
    incoming={world_,victim_,victim_id_,player_,player_id_};
}
void TrialCoopKOCommitted(const X86Context& c,const uint8_t* b){
    if(!Active(b)||!Victim(b,c.esi))return;
    Forget(c.esi);
    if(incoming.victim!=c.esi||!Current(b,incoming)||!incoming.player)return;
    for(auto& a:pending)if(!a.victim||!Current(b,a)||!Read(b,a.victim+0x110)){
        if(!a.victim)++pending_count;
        a=incoming;return;
    }
}
void TrialCoopKORecipient(X86Context& c,const uint8_t* b){
    if(!Active(b)||!TrialGuestReadable(c.esp,4))return;
    const uint32_t caller=Read(b,c.esp);
    if(caller!=0xe25b6u&&caller!=0xe261bu&&caller!=0xe2677u)return;
    // A missing or expired source must never borrow the last-created P2 HUD.
    c.ecx=0;
    if(!pending_count||!c.esi)return;
    for(auto& a:pending)if(a.victim==c.esi){
        const auto award=a;a={};--pending_count;
        if(Current(b,award)&&TrialGuestReadable(award.player,0x58)&&
           Read(b,award.player+0x54)==award.player_id)
            c.ecx=TrialCoopRewardManager(b,award.player);
        return;
    }
}
