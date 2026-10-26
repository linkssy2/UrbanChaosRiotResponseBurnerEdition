#include "xbe_coop_death.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_body.h"
#include "xbe_coop_cinematics.h"
#include "xbe_ragdoll_settings.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace {
uint32_t Read(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
struct Downed {
    uint32_t owner=0,id=0,body=0,body_id=0;
    double remaining=10.0;
    bool counting=false;
};
Downed downed[2];
std::atomic<unsigned> revive_seconds[2]{};
uint32_t world=0,owners[2]{},ids[2]{};
bool team_failed=false,failure_sent=false;
unsigned expired_slot=0;
void ClearSeconds(){for(auto& value:revive_seconds)value.store(0,std::memory_order_relaxed);}
bool PositiveHealth(const uint8_t* b,uint32_t p){const auto v=Read(b,p+0x9c);return v>0&&v<0x7f800000u;}
bool Pair(const uint8_t* b){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(0x51c4dc,4))return false;
    const uint32_t w=Read(b,0x51c4dc);
    if(!TrialGuestReadable(w,0x18))return false;
    uint32_t p[2],id[2];
    for(unsigned i=0;i<2;++i){
        p[i]=TrialCoopPlayer(i);
        if(!TrialGuestReadable(p[i],0x890)||Read(b,p[i])!=0x46fbe8||
           Read(b,p[i]+0x368)!=i||(Read(b,p[i]+0x5c)&1))return false;
        id[i]=Read(b,p[i]+0x54);if(id[i]==0xffffffffu)return false;
    }
    if(p[0]==p[1]||id[0]==id[1])return false;
    if(w!=world||p[0]!=owners[0]||p[1]!=owners[1]||id[0]!=ids[0]||id[1]!=ids[1]){
        world=w;for(unsigned i=0;i<2;++i){owners[i]=p[i];ids[i]=id[i];downed[i]={};}
        team_failed=failure_sent=false;ClearSeconds();
    }
    return true;
}
int Slot(uint32_t owner){return owner==owners[0]?0:owner==owners[1]?1:-1;}
bool Matches(const uint8_t* b,const Downed& d){
    return d.owner&&Read(b,d.owner+0x54)==d.id&&
        TrialCoopBodyForPlayer(b,d.owner)==d.body&&TrialGuestReadable(d.body,0x270)&&
        Read(b,d.body+0x54)==d.body_id;
}
bool Down(const uint8_t* b,unsigned i){
    return Matches(b,downed[i])&&Read(b,owners[i]+0x170)==12&&
        !PositiveHealth(b,owners[i])&&b[downed[i].body+0x198];
}
bool DownedPosition(const uint8_t* b,unsigned i,float (&position)[3]){
    return Down(b,i)&&TrialRagdollWorldPosition(b,Read(b,downed[i].body+0x194),position);
}
struct HostRestore {unsigned csr=_mm_getcsr();~HostRestore(){_mm_setcsr(csr);}};
bool NotifyFailure(X86Context& c,uint8_t* b){
    if(c.esp<0x18000||!TrialGuestReadable(c.esp-0x7000,0x7000)||
       !TrialGuestReadable(0x509a60,4))return false;
    const auto manager=Read(b,0x509a60);
    if(!TrialGuestReadable(manager,0x7e8)||Read(b,manager)!=0x4526f0||
       !TrialGuestReadable(0x45270c,4)||Read(b,0x45270c)!=0x69280)return false;
    // Original campaign event 1 queues playerDied (+7C7); normal-play's
    // 4E5E0 then selects NewFrontEnd-PlayerDied-1. No death animation, damage
    // pointer, second actor kill or direct UI-state mutation is needed.
    HostRestore restore;X86Context temp=c;
    temp.esp=(c.esp-0x600)&~15u;temp.ecx=manager;
    kernel::CallGuest(temp,0x69280,{1,0,0});
    return b[manager+0x7c7]!=0;
}
}

void TrialCoopDeathReset(){
    world=0;team_failed=failure_sent=false;ClearSeconds();
    for(unsigned i=0;i<2;++i){owners[i]=ids[i]=0;downed[i]={};}
}
void TrialCoopDeathBody(X86Context& c,uint8_t* b){
    // Both hooks follow MainPlayer's null C0 getter inside C7F40. Its original
    // health-zero, collision stop and damage/velocity setup have already run.
    if(!Pair(b))return;
    const int slot=Slot(c.esi);if(slot<0||Read(b,c.esi+0x9c)!=0)return;
    const uint32_t body=TrialCoopBodyForPlayer(b,c.esi);
    if(!body)return;
    auto& d=downed[slot];
    if(!Matches(b,d)){
        if(Read(b,c.esi+0x170)==12||b[body+0x198])return;
        d={c.esi,ids[slot],body,Read(b,body+0x54)};

    }
    c.eax=body;
}
unsigned TrialCoopDeathDisposition(const uint8_t* b,uint32_t owner){
    if(!Pair(b))return 0;
    const int slot=Slot(owner);if(slot<0||!Matches(b,downed[slot])||
       !b[downed[slot].body+0x198]||PositiveHealth(b,owner))return 0;
    // Keep the native dead state/ragdoll, and queue one campaign failure only
    // after both actors finish updating. The old C8461 shortcut did not send
    // PlayerDeathSequence's earlier event and could leave both players down.
    if(!team_failed&&(!PositiveHealth(b,owners[1-slot])||Read(b,owners[1-slot]+0x170)==12)){
        team_failed=true;expired_slot=unsigned(slot);ClearSeconds();
    }
    return 1;
}
unsigned TrialCoopReviveSeconds(unsigned slot){
    return slot<2?revive_seconds[slot].load(std::memory_order_relaxed):0;
}
void TrialCoopDeathUpdate(X86Context& c,uint8_t* b){
    // 209258 is reached once after both actors update; the original 55610
    // caller skips this world update while paused (world flag 0x800).
    if(!Pair(b)){TrialCoopDeathReset();return;}
    if(c.ebp!=world)return;
    if(team_failed){
        if(!failure_sent&&Down(b,expired_slot))failure_sent=NotifyFailure(c,b);
        return;
    }
    const bool frozen=(Read(b,world+0x10)&0x880)!=0x80||TrialCoopPresentation()==2;
    float delta;std::memcpy(&delta,b+world+0x14,4);
    const bool advance=!frozen&&std::isfinite(delta)&&delta>0;
    HostRestore restore;
    for(unsigned i=0;i<2;++i){
        auto& d=downed[i];
        if(!Down(b,i)){d.counting=false;revive_seconds[i].store(0,std::memory_order_relaxed);continue;}
        if(!d.counting){d.remaining=10.0;d.counting=true;}
        else if(advance)d.remaining-=delta;
        if(d.remaining<=0){
            team_failed=true;expired_slot=i;ClearSeconds();
            failure_sent=NotifyFailure(c,b);return;
        }
        revive_seconds[i].store(unsigned(std::ceil(d.remaining)),std::memory_order_relaxed);
    }
}
bool TrialCoopImmediateRagdoll(const uint8_t* b,uint32_t body){
    if(!Pair(b))return false;
    for(const auto& d:downed)if(d.body==body&&Matches(b,d)&&!PositiveHealth(b,d.owner))return true;
    return false;
}
bool TrialCoopKeepDownedBody(const uint8_t* b,uint32_t body){
    if(!Pair(b)||team_failed)return false;
    for(unsigned i=0;i<2;++i)if(downed[i].body==body&&Down(b,i))return true;
    return false;
}
bool TrialCoopPlayerDowned(const uint8_t* b,uint32_t owner){
    if(!Pair(b))return false;const int slot=Slot(owner);return slot>=0&&Down(b,unsigned(slot));
}
bool TrialCoopDownedPosition(const uint8_t* b,uint32_t owner,float (&position)[3]){
    if(!Pair(b)||team_failed)return false;
    HostRestore restore;
    const int slot=Slot(owner);
    return slot>=0&&DownedPosition(b,unsigned(slot),position);
}
uint32_t TrialCoopNearbyDownedPlayer(const uint8_t* b,uint32_t healer){
    if(!Pair(b)||team_failed)return 0;
    const int slot=Slot(healer);
    if(slot<0||!PositiveHealth(b,healer)||Read(b,healer+0x170)==12||!Down(b,1-slot))return 0;
    HostRestore restore;
    // The visible corpse may slide away from the owner's death transform.
    // Use its native physical-body position; 200 units is the original medic
    // hand-off distance, without its NPC steering cone.
    float position[3];if(!DownedPosition(b,unsigned(1-slot),position))return 0;
    double distance=0;
    for(unsigned axis=0;axis<3;++axis){
        float a,z;std::memcpy(&a,b+healer+0x40+axis*4,4);
        z=position[axis];
        if(!std::isfinite(a)||!std::isfinite(z))return 0;
        const double delta=double(a)-z;distance+=delta*delta;
    }
    return distance<=200.0*200.0?owners[1-slot]:0;
}
bool TrialCoopRevivePlayer(X86Context& c,uint8_t* b,uint32_t owner){
    if(!Pair(b)||team_failed||c.esp<0x18000||!TrialGuestReadable(c.esp-0x7000,0x7000))return false;
    const int slot=Slot(owner);
    if(slot<0||!Down(b,slot)||TrialCoopNearbyDownedPlayer(b,owners[1-slot])!=owner)return false;
    // D35A0 touches these initialized animation/transition objects directly.
    for(uint32_t offset:{0x1b8u,0x7d8u,0x664u})
        if(!TrialGuestReadable(Read(b,owner+offset),offset==0x1b8?0x4c:8))return false;
    HostRestore restore;X86Context temp=c;const uint32_t sp=(c.esp-0x600)&~15u;
    float position[3],matrix[16];
    if(!DownedPosition(b,unsigned(slot),position))return false;
    std::memcpy(matrix,b+owner+0x10,sizeof matrix);
    for(float value:matrix)if(!std::isfinite(value))return false;
    std::memcpy(matrix+12,position,sizeof position);
    temp.ecx=owner;temp.esp=sp;kernel::CallGuest(temp,0xbdab0,{});
    const float maximum=float(temp.st[temp.top&7]);
    if(!std::isfinite(maximum)||maximum<=0)return false;
    // A new original body is needed: E5310 owns physics/animation resources and
    // has no inverse reset. Retire the old body through its native lifecycle;
    // never clear a live ragdoll handle or reuse its animation graph in place.
    if(!TrialCoopBodyRecreate(temp,b,owner))return false;
    temp.ecx=owner;temp.esp=sp;kernel::CallGuest(temp,0xd35a0,{0,0});
    // BDBC0 hides both first-person parts every dead-state update. D35A0 does
    // not undo that; restore the original D4F19 startup gates once on revival.
    // CCF80 still owns the model setters and all shield/equipment visibility.
    b[owner+0x2d1]=b[owner+0x2d0]=1;
    // D38E0 uses this same reset-then-placement ordering. BD150 preserves
    // orientation, updates yaw and moves the recreated collision through
    // 2A42B0; raw translation writes would leave collision at the old site.
    std::memcpy(b+sp+0x100,matrix,sizeof matrix);
    temp.ecx=owner;temp.esp=sp;kernel::CallGuest(temp,0xbd150,{sp+0x100});
    uint32_t health;std::memcpy(&health,&maximum,4);
    temp.ecx=owner;temp.esp=sp;kernel::CallGuest(temp,0x2180a0,{health});
    downed[slot]={};
    revive_seconds[slot].store(0,std::memory_order_relaxed);
    return Read(b,owner+0x170)!=12&&PositiveHealth(b,owner);
}
