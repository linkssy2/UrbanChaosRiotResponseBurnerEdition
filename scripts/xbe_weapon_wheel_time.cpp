#include "xbe_weapon_wheel_time.h"
#include "xbe_weapon_wheel.h"
#include "xbe_coop.h"
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>
#include <xmmintrin.h>

namespace {
thread_local bool inside=false;
uint32_t Word(const uint8_t* base,uint32_t address){uint32_t value;std::memcpy(&value,base+address,4);return value;}
}
TrialWeaponWheelTime::TrialWeaponWheelTime(const X86Context& c,uint8_t* b){
    if(inside)return;inside=true;owner_=true;
    if(!b||trial_coop::Active()||trial_coop::Requested()||
       !TrialGuestReadable(c.esp,12)||Word(b,c.esp)!=0x5d214u||
       !TrialGuestReadable(0x51c4dc,4)||!TrialGuestReadable(0x509a60,4))return;
    const uint32_t world=Word(b,0x51c4dc);
    if(c.ecx!=world||!TrialGuestReadable(world,0x1c)||Word(b,world)!=0x450c48u||
       (Word(b,world+0x10)&0x880u)!=0x80u)return;
    const uint32_t level=Word(b,world+4),game=Word(b,0x509a60);
    if(!TrialGuestReadable(level,12)||!TrialGuestReadable(game,4))return;
    const uint32_t first=Word(b,level+4),vtable=Word(b,game);
    // A single local view alone is insufficient: the original gameplay
    // controller's Multiplayer query must be the pinned false-return leaf.
    if(first>0xfffffffbu||Word(b,level+8)!=first+4||!TrialGuestReadable(first,4)||
       !TrialGuestReadable(vtable,0x24)||Word(b,vtable+0x20)!=0x276e0u||
       !trial_weapon_wheel::SlowTimeActive(world))return;
    const uint32_t bits=Word(b,c.esp+4);
    if(!bits||bits>=0x7f800000u)return; // Zero, negative and nonfinite deltas stay original.
    float delta;std::memcpy(&delta,&bits,4);
    const unsigned csr=_mm_getcsr();delta*=.2f;
    argument_=b+c.esp+4;original_=bits;std::memcpy(argument_,&delta,4);
    _mm_setcsr(csr);
}
TrialWeaponWheelTime::~TrialWeaponWheelTime(){
    if(argument_)std::memcpy(argument_,&original_,4);
    if(owner_)inside=false;
}
