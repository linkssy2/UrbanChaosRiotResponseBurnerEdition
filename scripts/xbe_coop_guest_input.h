#pragma once
#include <cstring>
#include <array>
#include <xmmintrin.h>
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_coop.h"
#include "xbe_coop_input.h"
#include "xbe_coop_pause.h"
#include "xbe_input_memory.h"

namespace trial_coop {
inline InputMapping guest_input_mapping;
inline uint32_t pause_world=0;
inline void UpdateGuestInputMapping(uint8_t* base){
    uint32_t owner=0;
    if(base&&TrialGuestReadable(0x5096e0,4))std::memcpy(&owner,base+0x5096e0,4);
    uint8_t* record=owner&&TrialGuestReadable(owner,0x19)?base+owner+0xc:nullptr;
    guest_input_mapping.Update(owner,record,Active());
    uint32_t world=0;
    if(base&&TrialGuestReadable(0x51c4dc,4))std::memcpy(&world,base+0x51c4dc,4);
    if(!Active()||!record||world!=pause_world)ResetPauseOwner();
    pause_world=world;
}
inline unsigned PauseInputPlayer(){const int owner=PauseOwner();return owner<0?0u:unsigned(owner);}
inline void PauseFrontendBegin(){if(Active())pause_state.Begin();else ResetPauseOwner();}
inline void PauseFrontendSample(X86Context& ctx,uint8_t* base){
    if(!Active()||PauseOwner()>=0||ctx.ebp!=0x1d||ctx.edi!=0x509a70||
       !TrialGuestReadable(0x51c4dc,4)||ctx.esp<0x11000||!TrialGuestReadable(ctx.esp-4096,4096))return;
    const uint32_t world=X86_MEM32(0x51c4dc);
    if(!TrialGuestReadable(world,4))return;
    const uint32_t table=X86_MEM32(world);
    if(!TrialGuestReadable(table,12))return;
    const uint32_t query=X86_MEM32(table+8);
    if(!query)return;
    // Match the original frontend's press-edge query, including connection
    // and logical-port admission; do not infer ownership from host activity.
    struct Probe {
        X86Context c;uint8_t* base;uint32_t low;std::array<uint8_t,4096> saved;unsigned csr;
        Probe(const X86Context& context,uint8_t* b):c(context),base(b),low(c.esp-4096),csr(_mm_getcsr()){
            std::memcpy(saved.data(),base+low,saved.size());c.esp-=512;
        }
        ~Probe(){std::memcpy(base+low,saved.data(),saved.size());_mm_setcsr(csr);}
    } probe(ctx,base);
    probe.c.ecx=world;
    kernel::CallGuest(probe.c,query,{1,0x1d,3});
    const bool second=probe.c.st[probe.c.top]!=0;
    const int pressed=pause_state.Sample(X87_ST(0)!=0,second);
    if(pressed>=0){
        // Feed the edge through the original 4C3F0/4C610 consumer. A held P1
        // Start must not hide P2's press (or transfer the accepted owner).
        base[ctx.edi+0x42]=1;
        X87_ST(0)=1;
    }
}
struct PauseConsumeScope {
    X86Context& c;bool pause;
    PauseConsumeScope(X86Context& context,uint8_t* base):c(context),pause(Active()&&context.ecx==0x509a70&&
        TrialGuestReadable(context.esp,8)&&X86_MEM32(context.esp+4)==3){}
    ~PauseConsumeScope(){if(pause)pause_state.Consume((c.eax&255)!=0);}
};
inline bool PauseMenuState(const X86Context& ctx,uint8_t* base){
    return Active()&&TrialGuestReadable(ctx.ecx,0x14)&&X86_MEM32(ctx.ecx)==0x487818&&
        X86_MEM32(ctx.ecx+4)==0x267620&&base[ctx.ecx+0xc]!=0;
}
inline void PauseMenuEnter(const X86Context& ctx,uint8_t* base){
    if(PauseMenuState(ctx,base)&&!X86_MEM32(ctx.ecx+0x10)){
        pause_state.Enter();
        // The shared frontend's previous levels belong to its old reader.
        // Discard them before handing navigation to the accepted owner.
        if(TrialGuestReadable(0x509a78,0x54))std::memset(base+0x509a78,0,0x54);
    }
}
struct PauseMenuExitScope {
    bool pause;
    PauseMenuExitScope(const X86Context& ctx,uint8_t* base):pause(PauseMenuState(ctx,base)){}
    ~PauseMenuExitScope(){if(pause)ResetPauseOwner();}
};
}
