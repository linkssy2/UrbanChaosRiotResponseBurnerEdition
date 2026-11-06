#include "xbe_coop_hazards.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>

namespace {
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
}

void TrialCoopHazardPlayer(X86Context& c,const uint8_t* b){
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(c.esp,8))return;
    const uint32_t caller=Word(b,c.esp),slot=Word(b,c.esp+4);
    // FXFire, FXElectricity and FireDamageSphere already iterate the player
    // count. Retail 4ACD0 ignores their index: P2's startup overwrites +834,
    // otherwise hitting P2 twice (including its burn cooldown) and missing P1.
    // Other campaign callers intentionally use the original singleton lookup.
    if((caller!=0x18ef41u&&caller!=0x191247u&&caller!=0x1a5c92u)||slot>=2||
       !TrialGuestReadable(0x509a60,4)||c.ecx!=Word(b,0x509a60)||
       !TrialGuestReadable(c.ecx,0x838)||Word(b,c.ecx+0x10)!=2||
       c.eax!=Word(b,c.ecx+0x834)||!TrialGuestReadable(0x51c4dc,4))return;
    const uint32_t world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(world,8))return;
    const uint32_t state=Word(b,world+4);
    if(!TrialGuestReadable(state,12))return;
    const uint32_t first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return;
    uint32_t ids[2];
    for(unsigned i=0;i<2;++i){
        const uint32_t actor=TrialCoopPlayer(i),layer=Word(b,first+i*4);
        if(!TrialGuestReadable(actor,0x36c)||Word(b,actor)!=0x46fbe8u||
           (Word(b,actor+0x5c)&1)||Word(b,actor+0x368)!=i||
           !TrialGuestReadable(layer,0x28)||Word(b,layer+4)!=actor||Word(b,layer+0x24)!=i)return;
        ids[i]=Word(b,actor+0x54);if(ids[i]==0xffffffffu)return;
    }
    if(ids[0]==ids[1]||(c.eax!=ids[0]&&c.eax!=ids[1]))return;
    c.eax=ids[slot];
}
