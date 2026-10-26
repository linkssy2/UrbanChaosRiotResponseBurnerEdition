#include "xbe_coop_controls.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_controls.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>

namespace {
uint32_t Read(const uint8_t* base,uint32_t at){uint32_t v;std::memcpy(&v,base+at,4);return v;}
uint32_t World(const uint8_t* base){
    if(!base||!TrialGuestReadable(0x51c4dc,4))return 0;
    const uint32_t world=Read(base,0x51c4dc);
    return TrialGuestReadable(world,0x910)?world:0;
}
bool Second(const uint8_t* base,uint32_t actor){
    if(!base||!trial_coop::Active()||!actor||actor!=TrialCoopPlayer(1)||
       !TrialGuestReadable(actor,0x36c)||Read(base,actor)!=0x46fbe8||
       Read(base,actor+0x368)!=1||(Read(base,actor+0x5c)&1))return false;
    const uint32_t world=World(base);if(!world)return false;
    const uint32_t state=Read(base,world+4),id=Read(base,actor+0x54);
    if(id==0xffffffffu||!TrialGuestReadable(state,0x203c))return false;
    const uint32_t entry=state+0x3c+(id&1023)*8;
    return Read(base,entry)==id&&Read(base,entry+4)==actor;
}
}
unsigned TrialCoopControlValue(const uint8_t* base,uint32_t actor,unsigned setting,unsigned original){
    return Second(base,actor)?trial_controls::SecondProfileValue(setting,original):original;
}
unsigned TrialCoopControlSlotValue(const uint8_t* base,unsigned slot,unsigned setting,unsigned original){
    return slot==1?TrialCoopControlValue(base,TrialCoopPlayer(1),setting,original):original;
}
unsigned TrialCoopVibrationValue(const uint8_t* base,uint32_t manager,unsigned slot,unsigned original){
    if(slot!=1||!base||!trial_coop::Active()||!TrialGuestReadable(manager,0x58)||Read(base,manager)!=0x452640)return original;
    const uint32_t world=World(base);
    if(!world||Read(base,world+0x90c)!=manager)return original;
    // The original 682F0 calls this gate only for vibration (type 2), not shake.
    return TrialCoopControlSlotValue(base,slot,0,original);
}
