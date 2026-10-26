#include "xbe_coop_pickups.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include <cmath>
#include <cstring>

bool TrialCoopPickupAllowed(const X86Context& c,const uint8_t* b){
    if(!b||!trial_coop::Active())return true;
    const auto word=[b](uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;};
    const uint32_t actor=c.ecx;
    if(!TrialGuestReadable(actor,0x890)||word(actor)!=0x46fbe8)return true;
    const uint32_t slot=word(actor+0x368);
    if(slot>1||TrialCoopPlayer(slot)!=actor)return true;
    float health;std::memcpy(&health,b+actor+0x9c,4);
    if(word(actor+0x54)==0xffffffffu||(word(actor+0x5c)&1)||
       word(actor+0x170)==12||!std::isfinite(health)||health<=0)return false;
    if(!TrialGuestReadable(c.esp,8)||!TrialGuestReadable(0x51c4dc,4))return false;
    const uint32_t world=word(0x51c4dc);
    if(!TrialGuestReadable(world,8))return false;
    const uint32_t state=word(world+4),id=word(c.esp+4);
    if(id==0xffffffffu||!TrialGuestReadable(state,0x203c))return false;
    // 207D70 keeps pending-deletion actors in this ID table until world cleanup.
    // 20A140 marks +5C immediately, before a second local player can collect.
    const uint32_t entry=state+0x3c+(id&0x3ff)*8;
    if(word(entry)!=id)return false;
    const uint32_t item=word(entry+4);
    return TrialGuestReadable(item,0x60)&&word(item+0x54)==id&&!(word(item+0x5c)&1);
}
