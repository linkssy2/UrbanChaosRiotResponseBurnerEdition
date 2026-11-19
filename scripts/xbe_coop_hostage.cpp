#include "xbe_coop_hostage.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>
#include <xmmintrin.h>

namespace {
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
float Real(const uint8_t* b,uint32_t a){float v;std::memcpy(&v,b+a,4);return v;}
bool Position(const uint8_t* b,uint32_t actor){
    for(unsigned axis=0;axis<3;++axis)if((Word(b,actor+0x40+axis*4)&0x7fffffffu)>=0x7f800000u)return false;
    return true;
}
bool Alive(const uint8_t* b,uint32_t actor){
    const auto hp=Word(b,actor+0x9c);
    return hp>0&&hp<0x7f800000u&&Word(b,actor+0x170)!=12&&Position(b,actor);
}
struct Pair {uint32_t actor[2]{},id[2]{};};
bool Players(const uint8_t* b,const X86Context& c,uint32_t type,Pair& pair){
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(c.esi,0xf8)||Word(b,c.esi)!=type||
       Word(b,c.esi+0x54)==0xffffffffu||(Word(b,c.esi+0x5c)&1)||
       !TrialGuestReadable(0x509a60,4)||c.ecx!=Word(b,0x509a60)||
       !TrialGuestReadable(c.ecx,0x838)||c.eax!=Word(b,c.ecx+0x834)||
       !TrialGuestReadable(0x51c4dc,4))return false;
    const auto world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(world,8))return false;
    const auto state=Word(b,world+4);
    if(!TrialGuestReadable(state,12))return false;
    const auto first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return false;
    for(unsigned slot=0;slot<2;++slot){
        const auto actor=TrialCoopPlayer(slot),layer=Word(b,first+4*slot);
        if(!TrialGuestReadable(actor,0x36c)||Word(b,actor)!=0x46fbe8u||
           (Word(b,actor+0x5c)&1)||Word(b,actor+0x368)!=slot||
           !TrialGuestReadable(layer,0x28)||Word(b,layer+4)!=actor||Word(b,layer+0x24)!=slot)return false;
        pair.actor[slot]=actor;pair.id[slot]=Word(b,actor+0x54);
        if(pair.id[slot]==0xffffffffu)return false;
    }
    return pair.id[0]!=pair.id[1]&&(c.eax==pair.id[0]||c.eax==pair.id[1]);
}
struct HostRestore {unsigned csr=_mm_getcsr();~HostRestore(){_mm_setcsr(csr);}};
}

namespace {
void SelectTarget(X86Context& c,const uint8_t* b,uint32_t type,bool cached){
    Pair pair;
    if(!Players(b,c,type,pair))return;
    const auto owner=Word(b,c.esi+0xd4);
    if(!TrialGuestReadable(owner,0x60)||Word(b,owner+0x54)==0xffffffffu||
       (Word(b,owner+0x5c)&1)||!Position(b,owner))return;
    HostRestore restore;
    // Retail overwrites this target with the last registered player every tick.
    // Consider both living players; preserve the previous target on exact ties.
    // Original proximity, warnings, weapon eligibility and scripted NPC shots stay intact.
    auto preferred=cached?Word(b,c.esi+0xf4):c.eax;
    if(preferred!=pair.id[0]&&preferred!=pair.id[1])preferred=c.eax;
    unsigned best=2;double distance=0;
    for(unsigned slot=0;slot<2;++slot){
        if(!Alive(b,pair.actor[slot]))continue;
        double squared=0;
        for(unsigned axis=0;axis<3;++axis){
            const double delta=double(Real(b,pair.actor[slot]+0x40+axis*4))-Real(b,owner+0x40+axis*4);
            squared+=delta*delta;
        }
        if(best==2||squared<distance||(squared==distance&&pair.id[slot]==preferred)){
            best=slot;distance=squared;
        }
    }
    if(best<2)c.eax=pair.id[best];
}
}

void TrialCoopHostageTarget(X86Context& c,const uint8_t* b,bool dragging){
    SelectTarget(c,b,dragging?0x466368u:0x466f08u,!dragging);
}

void TrialCoopHostageIdleFacing(X86Context& c,const uint8_t* b){
    // Only reached inside the authored Look At Player branch. This behavior's
    // +F4 holds the hostage, so it must never be treated as a cached player ID.
    SelectTarget(c,b,0x466248u,false);
}

void TrialCoopHostageFacing(X86Context& c,const uint8_t* b){
    Pair pair;if(!Players(b,c,0x466f08u,pair))return;
    const auto selected=Word(b,c.esi+0xf4);
    // Face the same target already selected by this controller's update.
    for(unsigned slot=0;slot<2;++slot)if(selected==pair.id[slot]&&Alive(b,pair.actor[slot])){c.eax=selected;return;}
}
