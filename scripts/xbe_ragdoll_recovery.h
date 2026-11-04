#pragma once
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace trial_ragdoll_recovery {
inline uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
inline float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
inline bool Ragdoll(const uint8_t* b,uint32_t body){
    if(!TrialGuestReadable(body,0x4c))return false;
    const uint32_t entries=Word(b,body+0x44),count=Word(b,body+0x48);
    if(count>256||!TrialGuestReadable(entries,count*16))return false;
    for(uint32_t i=0;i<count;++i){
        const uint32_t p=entries+i*16;
        if(Word(b,p)==1)return Word(b,p+8)==4&&Word(b,p+12)==0;
    }
    return false;
}
class Scope {
    uint8_t* base_;uint32_t slot_=0,bits_=0;
public:
    Scope(const X86Context& c,uint8_t* b,uint32_t argument):base_(b){
        struct Restore {unsigned csr=_mm_getcsr();~Restore(){_mm_setcsr(csr);}} restore;
        if(!TrialGuestReadable(c.esp,argument+4))return;
        const uint32_t context=Word(b,c.esp+argument);
        if(!TrialGuestReadable(context,0x2c))return;
        const float dt=Real(b,context+8),inverse=Real(b,context+4),inverse_steps=Real(b,context+16);
        if(!std::isfinite(dt)||dt<=0||dt>=1.f/60.f||!std::isfinite(inverse)||inverse<=0)return;
        if(!std::isfinite(inverse_steps)||inverse_steps<=0||inverse_steps>1)return;
        const uint32_t constraint=Word(b,context+0x24);
        if(!TrialGuestReadable(constraint,0x18)||
           !(Ragdoll(b,Word(b,constraint+0x10))||Ragdoll(b,Word(b,constraint+0x14))))return;
        // Keep 60 Hz positional recovery, including the solver's substep count.
        const float limited=60.f/inverse_steps;
        if(!std::isfinite(limited)||limited<=0||limited>=inverse)return;
        slot_=context+4;bits_=Word(b,slot_);std::memcpy(b+slot_,&limited,4);
    }
    ~Scope(){if(slot_)std::memcpy(base_+slot_,&bits_,4);}
};
}
