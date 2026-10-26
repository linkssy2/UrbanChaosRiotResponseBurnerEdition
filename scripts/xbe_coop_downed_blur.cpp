#include "xbe_coop_downed_blur.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_cinematics.h"
#include "xbe_coop_death.h"
#include "xbe_trial_graphics_ui.h"
#include "xbe_intro_skip.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include <cstring>
#include <xmmintrin.h>

namespace d3d {void TrialDownedSceneBlur(unsigned mask);}
namespace {
uint32_t ReadBlur(const uint8_t* base,uint32_t address){
    uint32_t value;std::memcpy(&value,base+address,4);return value;
}
bool Counting(unsigned slot){
    const auto seconds=TrialCoopReviveSeconds(slot);return seconds&&seconds<=10;
}
}

void TrialCoopDownedSceneBlur(const X86Context& c,const uint8_t* b){
    if(!trial_coop::Active()||!trial_coop::Requested()||
       (!Counting(0)&&!Counting(1))||TrialCoopPresentation()!=1||
       TrialGraphicsOpen()||TrialIntroPlaying()||!b||
       !TrialGuestReadable(c.esp,4)||ReadBlur(b,c.esp)!=0x557e8u||
       !TrialGuestReadable(0x51c4dc,4))return;
    const uint32_t world=ReadBlur(b,0x51c4dc);
    if(c.ecx!=world||!TrialGuestReadable(world,0x18)||ReadBlur(b,world)!=0x450c48u||
       (ReadBlur(b,world+0x10)&0x880u)!=0x80u)return;
    const auto state=ReadBlur(b,world+4);
    if(!TrialGuestReadable(state,12))return;
    const auto first=ReadBlur(b,state+4);
    if(first>0xfffffff7u||ReadBlur(b,state+8)!=first+8||!TrialGuestReadable(first,8))return;
    uint32_t actors[2];
    for(unsigned slot=0;slot<2;++slot){
        actors[slot]=TrialCoopPlayer(slot);const auto layer=ReadBlur(b,first+slot*4);
        if(!TrialGuestReadable(layer,0x28)||ReadBlur(b,layer+4)!=actors[slot]||
           ReadBlur(b,layer+0x24)!=slot)return;
    }
    if(!actors[0]||actors[0]==actors[1])return;
    unsigned mask=0;
    for(unsigned slot=0;slot<2;++slot){
        // The death owner validates actor IDs, current ragdoll and world reuse.
        // Re-read its snapshot afterward: validation can revoke stale timers.
        if(Counting(slot)&&TrialCoopPlayerDowned(b,actors[slot])&&Counting(slot))mask|=1u<<slot;
    }
    // 215060 has finished both scene views; 215380 has not emitted either HUD.
    // Drain only queued guest draws, then enqueue one immutable scene snapshot.
    if(mask){
        struct Restore {unsigned csr=_mm_getcsr();~Restore(){_mm_setcsr(csr);}} restore;
        d3d::TrialDownedSceneBlur(mask);
    }
}
