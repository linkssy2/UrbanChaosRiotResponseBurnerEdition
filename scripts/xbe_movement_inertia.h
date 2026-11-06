#pragma once
#include "xbe_controls.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>

// BEF80: only the ordinary locomotion output, after presentation blend and before
// crouch/equipment/impulse overrides. Keep all four original smoothers running.
inline void TrialMovementInertia(const X86Context& ctx,uint8_t* base){
    const uint32_t owner=ctx.edi;
    if(!base||!TrialGuestReadable(owner,0x890))return;
    const auto read=[&](uint32_t at){uint32_t v;std::memcpy(&v,base+at,4);return v;};
    if(read(owner)!=0x46fbe8||(read(owner+0x5c)&1)||base[owner+0x304])return;
    const uint32_t slot=read(owner+0x368),id=read(owner+0x54),state=read(owner+0x170);
    if(slot>1||trial_controls::MovementInertia(slot)||id==0xffffffffu||
       (slot&&(!trial_coop::Active()||TrialCoopPlayer(1)!=owner))||
       state==3||state>=11)return;
    const uint32_t health=read(owner+0x9c);
    if(!health||health>=0x7f800000u||!TrialGuestReadable(0x51c4dc,4))return;
    const uint32_t world=read(0x51c4dc);
    if(!TrialGuestReadable(world,0x18)||(read(world+0x10)&0x800u))return;
    const uint32_t delta=read(world+0x14),level=read(world+4);
    if(!delta||delta>=0x7f800000u||!TrialGuestReadable(level,0x203c))return;
    const uint32_t entry=level+0x3c+(id&1023)*8;
    if(read(entry)!=id||read(entry+4)!=owner)return;
    const uint32_t sideways=read(owner+0x1d4),forward=read(owner+0x1e0);
    // BC4B0 has already clamped the resolved axes and applied this player's Walk.
    // Integer bit checks and copies preserve guest flags, x87, SSE and host MXCSR.
    if((sideways&0x7fffffffu)>0x3f800000u||(forward&0x7fffffffu)>0x3f800000u||
       (read(owner+0x730)&0x7fffffffu)>=0x7f800000u||
       (read(owner+0x734)&0x7fffffffu)>=0x7f800000u)return;
    std::memcpy(base+owner+0x730,&sideways,4);
    std::memcpy(base+owner+0x734,&forward,4);
}
