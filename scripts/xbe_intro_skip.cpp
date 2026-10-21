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
// Literal startup/mission movies only. Looping backgrounds and Press Start stay untouched.
uint32_t IntroKind(uint32_t owner,uint8_t* base) {
    if(!TrialGuestReadable(owner,0x48)||IntroWord(base,owner)!=0x489020||base[owner+0x15])return 0;
    constexpr const char* names[]={"dolby","EidosLogo","RockLogo","intro_12_edit",
        "intro_1","intro_2","intro_3","intro_4","intro_5","intro_6","intro_8","intro_9","intro_11","intro_12","intro_13"};
    for(unsigned i=0;i<sizeof(names)/sizeof(*names);++i)
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
    // At272BD6 two saved registers and24 bytes of locals precede the dt argument.
    if(kind&&TrialGuestReadable(ctx.esp+0x24,4))TrialIntroAdvance(IntroFloat(base,ctx.esp+0x24));
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

static bool TitleOwner(uint32_t owner,uint8_t* base) {
    return TrialGuestReadable(owner,0x20)&&IntroWord(base,owner)==0x489974&&
        IntroWord(base,owner+8)==0x2717b0&&
        (IntroWord(base,owner+4)==0x16||IntroWord(base,owner+4)==0x1d);
}
void TrialTitleUpdate(X86Context& ctx,uint8_t* base) {
    const bool title=TitleOwner(ctx.ecx,base);
    TrialTitleContext(title&&std::isfinite(IntroFloat(base,ctx.ecx+0x1c))&&
                      IntroFloat(base,ctx.ecx+0x1c)<=0);
}
void TrialTitleAccept(X86Context& ctx,uint8_t* base) {
    if(!TitleOwner(ctx.edi,base))return;
    if(TrialTitleTake()){
        ctx.ebx=(ctx.ebx&~255u)|1u;
        const uint32_t port=0;std::memcpy(base+0x51d2f4,&port,4);
    }
    // Continue through the retail sound and successor; consume held inputs
    // so this same press cannot select a profile on the following screen.
    if(ctx.ebx&255u)TrialTitleFinish();
}
