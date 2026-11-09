#pragma once
#include <cmath>
#include <cstring>
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"

extern int TrialFirearmDirection();
namespace trial_firearm_cycle {
inline thread_local uint32_t inventory=0;
struct HudCycle {uint32_t inventory=0,target=0;};
inline thread_local HudCycle hud_cycle;
inline uint32_t Read(const uint8_t* base,uint32_t address){uint32_t value;std::memcpy(&value,base+address,4);return value;}
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
static void Selection(const X86Context& ctx,const uint8_t* base){
    if(!TrialGuestReadable(ctx.esp,8)||!TrialGuestReadable(ctx.ecx,0x70))return;
    const uint32_t target=Read(base,ctx.esp+4);
    if(target==Read(base,ctx.ecx+0x68))return;
    if(ctx.ecx==hud_cycle.inventory)hud_cycle={};
    if(inventory&&ctx.ecx==inventory&&target<27)hud_cycle={inventory,target};
}
static bool StepHud(X86Context& ctx,uint8_t* base){
    if(!hud_cycle.inventory||!TrialGuestReadable(ctx.esp,0x1c)||Read(base,ctx.esp+0xc)!=0||
       ctx.ecx<0x16c||!TrialGuestReadable(ctx.ecx-0x16c,0x254)||
       Read(base,ctx.ecx-0x164)+0x498!=hud_cycle.inventory||
       !TrialGuestReadable(hud_cycle.inventory,0x70))return false;
    if(Read(base,hud_cycle.inventory+0x68)!=hud_cycle.target){hud_cycle={};return false;}
    const uint32_t items=Read(base,hud_cycle.inventory),item=items+hud_cycle.target*0x5c;
    if(!TrialGuestReadable(item,8)||Read(base,item+4)!=Read(base,ctx.esp+0x10)||
       !ctx.edi||ctx.edi>27||ctx.ebx>=ctx.edi||
       Read(base,ctx.ecx+0xc+ctx.ebx*8)!=Read(base,ctx.esp+0x10))return false;
    float position;std::memcpy(&position,base+ctx.ecx,4);
    const float count=float(ctx.edi),target=float(ctx.ebx),step=ctx.xmm[2].f[0]*ctx.xmm[5].f[0];
    if(!std::isfinite(position)||!std::isfinite(step)||step<0)return false;
    position=std::fmod(position,count);if(position<0)position+=count;
    float distance=position-target;if(distance<0)distance+=count;
    // Keep the accepted backward direction after the key is released; fade still runs retail code.
    if(step>=distance){position=target;hud_cycle={};}
    else {position-=step;if(position<0)position+=count;}
    std::memcpy(base+ctx.ecx,&position,4);
    return true;
}
}
