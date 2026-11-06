#pragma once
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

inline uint32_t TrialHealthUpgradeColor(const X86Context& c,const uint8_t* b){
    struct Restore {unsigned csr=_mm_getcsr();~Restore(){_mm_setcsr(csr);}} restore;
    if(!b||!TrialGuestReadable(c.ebp,12))return 0;
    uint32_t descriptor;std::memcpy(&descriptor,b+c.ebp+8,4);
    if(!TrialGuestReadable(descriptor,0x20))return 0;
    float alpha;std::memcpy(&alpha,b+descriptor+0x1c,4);
    _mm_setcsr(0x1f80);
    if(!std::isfinite(alpha)||alpha<=0)return 0;
    // Retail 733E0's black upgrade dividers bypass the descriptor's HUD fade.
    return (alpha>=1?85u:uint32_t(85.f*alpha))<<24;
}
