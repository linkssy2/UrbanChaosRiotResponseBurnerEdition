#pragma once
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <xmmintrin.h>

namespace trial_blink {
struct Clock { double fraction=0; uint32_t stamp=0; bool sampled=false; };
inline std::unordered_map<uint32_t,Clock> clocks;
inline uint32_t world=0;
inline uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
inline void Forget(uint32_t p){clocks.erase(p);}
inline unsigned Steps(const X86Context& c,const uint8_t* b){
    struct Restore {unsigned csr=_mm_getcsr();~Restore(){_mm_setcsr(csr);}} restore;
    if(!TrialGuestReadable(c.ecx,0x24)||Word(b,c.ecx)!=0x48f1c4||
       !TrialGuestReadable(c.esp,8)||!TrialGuestReadable(0x51c4dc,4))return 1;
    float delta;std::memcpy(&delta,b+c.esp+4,4);
    const uint32_t w=Word(b,0x51c4dc);
    if(!TrialGuestReadable(w,0x18)||!std::isfinite(delta)||delta<0||delta>.25f)return 1;
    if(world!=w){world=w;clocks.clear();}
    auto& clock=clocks[c.ecx];
    const uint32_t stamp=Word(b,w+8);
    // B9830 forwards animation delta; world time deduplicates repeated pose/view passes.
    if(clock.sampled&&clock.stamp==stamp)return 0;
    clock.stamp=stamp;clock.sampled=true;
    if(b[c.ecx+0x20]||delta==0)return 0;
    if(Word(b,c.ecx+0x1c)==0){clock.fraction=0;return 1;}
    clock.fraction+=double(delta)*30.0;
    const unsigned steps=unsigned(clock.fraction+1.e-6);
    clock.fraction-=steps;
    return steps;
}
}
