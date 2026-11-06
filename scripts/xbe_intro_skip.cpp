#include <cmath>
#include <cstdlib>
#include <cstring>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_intro_skip.h"

namespace {
bool StartupOption(const char* value){return value&&value[0]=='1'&&!value[1];}
bool startup_pending=StartupOption(std::getenv("UCRR_SKIP_STARTUP"));
bool startup_profile=false;
uint32_t skipped_startup[4]{};
uint32_t IntroWord(uint8_t* base,uint32_t address) {
    uint32_t value;std::memcpy(&value,base+address,4);return value;
}
float IntroFloat(uint8_t* base,uint32_t address) {
    float value;std::memcpy(&value,base+address,4);return value;
}
// Literal startup/mission movies only. Looping backgrounds stay untouched.
uint32_t IntroKind(uint32_t owner,uint8_t* base) {
    if(!base||!TrialGuestReadable(owner,0x48)||IntroWord(base,owner)!=0x489020||base[owner+0x15])return 0;
    constexpr const char* names[]={"dolby","EidosLogo","RockLogo","intro_12_edit",
        "intro_1","intro_2","intro_3","intro_4","intro_5","intro_6","intro_8","intro_9","intro_11","intro_12","intro_13"};
    for(unsigned i=0;i<sizeof(names)/sizeof(*names);++i)
        if(!std::memcmp(base+owner+0x24,names[i],std::strlen(names[i])+1))return i+1;
    return 0;
}
uint32_t StartupKind(uint32_t owner,uint8_t* base){
    const uint32_t kind=IntroKind(owner,base);
    constexpr uint32_t next[]={0x2723b0,0x272380,0x2722f0,0x271e80};
    return kind&&kind<=4&&IntroWord(base,owner+4)==next[kind-1]&&
        !base[owner+0x16]&&!base[owner+0x17]&&!base[owner+0x18]&&
        !IntroWord(base,owner+0x10)&&!IntroWord(base,owner+0x44)?kind:0;
}
}

bool TrialStartupMovie(X86Context& ctx,uint8_t* base){
    if(ctx.esp>0xfffffff3u||!TrialGuestReadable(ctx.esp,12))return false;
    const uint32_t owner=ctx.ecx,kind=StartupKind(owner,base);
    if(!kind||(!startup_pending&&skipped_startup[kind-1]!=owner))return false;
    skipped_startup[kind-1]=owner;
    TrialIntroMovie(owner,kind,true,false);TrialIntroClose(owner);
    ctx.eax=IntroWord(base,owner+4);
    if(!startup_pending)ctx.eax=0; // Outgoing skipped movie may still be fading.
    else if(kind==4){
        // The title's original 64E30 gate reads this object's +8 state. Keep
        // that gate before the startup factory, without constructing a title.
        const bool readable=TrialGuestReadable(0x4e836c,4);
        const uint32_t state=readable?IntroWord(base,0x4e836c):0;
        startup_profile=readable&&state!=0&&state!=1&&state!=4;
        if(!startup_profile)ctx.eax=0;
    }
    ctx.esp+=12;
    return true;
}
bool TrialStartupMovieClose(const X86Context& ctx,uint8_t* base){
    // The retail destructor closes the global decoder even for an unopened movie.
    for(unsigned i=0;i<4;++i)if(skipped_startup[i]&&skipped_startup[i]==ctx.esi){
        skipped_startup[i]=0;return StartupKind(ctx.esi,base)==i+1;
    }
    return false;
}

extern "C" void sub_002717B0(X86Context&,uint8_t*);
bool TrialStartupProfile(X86Context& ctx,uint8_t* base){
    // 271CCD: the retail bank/controller initialization has completed. Only
    // 271E80's startup call may omit the title tree; 271810 returns to title.
    if(!startup_pending||!startup_profile||!base||ctx.esp>0xffffff9fu||
       !TrialGuestReadable(ctx.esp,0x60)||IntroWord(base,ctx.esp+0x58)!=0x271e92||
       !TrialGuestReadable(IntroWord(base,ctx.esp+0x5c),8))return false;
    const uint32_t port=0;std::memcpy(base+0x51d2f4,&port,4);
    startup_pending=startup_profile=false;
    TrialTitleContext(false);TrialTitleFinish();
    // Tail-call the original successor with the existing output pair and
    // return address. Restore exactly 271C00's three saves and 0x4c locals.
    ctx.edi=IntroWord(base,ctx.esp);ctx.esi=IntroWord(base,ctx.esp+4);
    ctx.ebx=IntroWord(base,ctx.esp+8);ctx.esp+=0x58;
    sub_002717B0(ctx,base);
    return true;
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
    if(ctx.ebx&255u){startup_pending=startup_profile=false;TrialTitleFinish();}
}
