#include <cmath>
#include <cstring>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_intro_skip.h"

namespace {
uint32_t IntroWord(uint8_t* base,uint32_t address) {
    uint32_t value;std::memcpy(&value,base+address,4);return value;
}
float IntroFloat(uint8_t* base,uint32_t address) {
    float value;std::memcpy(&value,base+address,4);return value;
}
// These four literal names are the startup chain 272780 -> 2723B0 -> 272380
// -> 2722F0. Gameplay intro_N, looping backgrounds and Press Start are excluded.
uint32_t IntroKind(uint32_t owner,uint8_t* base) {
    if(!TrialGuestReadable(owner,0x48)||IntroWord(base,owner)!=0x489020||base[owner+0x15])return 0;
    constexpr const char* names[]={"dolby","EidosLogo","RockLogo","intro_12_edit"};
    for(unsigned i=0;i<4;++i)
        if(!std::memcmp(base+owner+0x24,names[i],std::strlen(names[i])+1))return i+1;
    return 0;
}
}

// Hooks are observations of the original update/close, not movie playback owners.
void TrialIntroUpdate(X86Context& ctx,uint8_t* base) {
    // 272BD6 follows the retail ADDSS/store of elapsed time. ESI is the movie
    // screen here; no second timer or floating-point addition is introduced.
    const uint32_t owner=ctx.esi,kind=IntroKind(owner,base);
    bool ready=false;
    if(kind){
        const float elapsed=IntroFloat(base,owner+0x1c),delay=IntroFloat(base,owner+8);
        ready=std::isfinite(elapsed)&&std::isfinite(delay)&&
            base[owner+0x16]&&!base[owner+0x17]&&!base[owner+0x18]&&elapsed>delay;
    }
    TrialIntroMovie(kind?owner:0,kind,kind&&!base[owner+0x16],ready);
}

bool TrialIntroQuery(X86Context& ctx,uint8_t* base) {
    if(!TrialGuestReadable(ctx.esp,12))return false;
    const uint32_t caller=IntroWord(base,ctx.esp),port=IntroWord(base,ctx.esp+4),action=IntroWord(base,ctx.esp+8);
    if(!((caller==0x272c5c&&action==0x16)||(caller==0x272c6b&&action==0x1d))||
       !IntroKind(ctx.esi,base)||port>=4)return false;
    // Retail only inspects AL, and its cdecl caller removes the two arguments.
    ctx.eax=(ctx.eax&~255u)|unsigned(TrialIntroTake(ctx.esi,port));
    ctx.esp+=4;
    return true;
}

void TrialIntroEnd(X86Context& ctx,uint8_t*) { TrialIntroClose(ctx.esi); }
