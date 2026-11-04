#pragma once
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>

namespace trial_daniels_spawn {
inline void Correct(uint8_t* base,uint32_t actor){
    if(!TrialGuestReadable(actor,0x4c))return;
    uint32_t type;std::memcpy(&type,base+actor,4);
    constexpr uint32_t position[]={0x45900b6d,0x42f4bef0,0xc4204a85};
    if(type!=0x47bf50||std::memcmp(base+actor+0x40,position,12))return;
    // This unique Angel Heights spawn straddles two wall layers; clear both before creating the capsule.
    constexpr float corridor_x=4558.f;
    std::memcpy(base+actor+0x40,&corridor_x,4);
}
}
