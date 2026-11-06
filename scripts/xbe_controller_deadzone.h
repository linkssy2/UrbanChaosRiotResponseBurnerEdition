#pragma once
#include "xbe_controls.h"
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>

// 22EDF0 applies a mapping descriptor's inner deadzone, then its remaining
// accumulation/clamp/edge rules. Replace only that inner range for the sampled
// gameplay controller; 31, menus and keyboard contributions take the original.
inline bool TrialControllerDeadzone(X86Context& ctx,uint8_t* base){
    if(!base||!TrialGuestReadable(ctx.ecx,0x18))return false;
    const auto read=[&](uint32_t at){uint32_t v;std::memcpy(&v,base+at,4);return v;};
    // Only the signed continuous axis calibration used by WorldInput.
    // Do not change button/edge conversions that share the generic evaluator.
    const uint32_t flags=read(ctx.ecx);
    if((flags&~0x1au)!=0x25u)return false;
    float minimum,gain;
    std::memcpy(&gain,base+ctx.ecx+0x10,4);
    std::memcpy(&minimum,base+ctx.ecx+0x14,4);
    float output;
    if(!trial_controls::NativeControllerResponse(read(ctx.ecx+4),minimum,gain,output))return false;
    ctx.xmm[0].f[0]=(flags&2)?-output:output;
    return true;
}
