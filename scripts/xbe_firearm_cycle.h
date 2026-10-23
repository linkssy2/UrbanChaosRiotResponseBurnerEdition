#pragma once
#include <cstring>
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"

extern int TrialFirearmDirection();
namespace trial_firearm_cycle {
static thread_local uint32_t inventory=0;
static uint32_t Read(const uint8_t* base,uint32_t address){uint32_t value;std::memcpy(&value,base+address,4);return value;}
struct Scope {
    uint32_t previous=inventory;
    Scope(const X86Context& ctx,const uint8_t* base){
        inventory=0;
        if(!TrialGuestReadable(ctx.esp,8)||ctx.ecx<0x498)return;
        const uint32_t caller=Read(base,ctx.esp),player=ctx.ecx-0x498;
        if((caller!=0xC4DCD&&caller!=0xC4E03)||Read(base,ctx.esp+4)!=0||
           !TrialGuestReadable(player,0x694)||Read(base,player+0x368)!=0||TrialFirearmDirection()!=-1)return;
        inventory=ctx.ecx;
    }
    ~Scope(){inventory=previous;}
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
};
static void BeforeIncrement(X86Context& ctx,const uint8_t* base){
    if(!inventory||ctx.edi!=inventory||ctx.edx!=inventory+0x7c||!TrialGuestReadable(ctx.edx,12))return;
    const uint32_t begin=Read(base,ctx.edx+4),end=Read(base,ctx.edx+8);
    if(begin!=ctx.ecx||end<=begin||(end-begin)%4||end-begin>27*4||!TrialGuestReadable(begin,end-begin))return;
    const uint32_t count=(end-begin)/4;
    if(ctx.ebp>=count)return;
    // The retail INC and following eligibility loop still execute unchanged.
    ctx.ebp=(ctx.ebp?ctx.ebp:count)-2;
}
}
