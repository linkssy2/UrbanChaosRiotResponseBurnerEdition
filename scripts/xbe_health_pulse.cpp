#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"

namespace {
struct PulseSnapshot {
    float sweep=0, elapsed=0, health=0, extent=0, fraction=0;
    std::array<float,4> color{};
    std::array<float,17> descriptor{};
    float fill_x=0,fill_y=0;
    uint32_t owner=0,actor=0,sample_owner=0,color_owner=0,sprite=0;
};
struct TraceHistory {
    uint32_t owner=0,actor=0,sample_owner=0,color_owner=0;
    float elapsed=0,sweep=0;
    bool bar_started=false;
};
static thread_local TraceHistory trace;

// 76A50 supplies its own colour to the 53-column heartbeat. That borrowed
// pointer identifies the player health owner without a global/stale HUD cache.
static bool CapturePulse(X86Context& ctx,uint8_t* base,PulseSnapshot& out) {
    auto word=[base](uint32_t p){uint32_t v;std::memcpy(&v,base+p,4);return v;};
    auto real=[base](uint32_t p){float v;std::memcpy(&v,base+p,4);return v;};
    const uint32_t beat=ctx.esi;
    if(!TrialGuestReadable(beat,0x7c)||word(beat)!=0x452f90||word(beat+4)!=53) return false;
    const uint32_t color=word(beat+0x10);
    if(color<0x10010) return false;
    const uint32_t health=color-0x10;
    if(!TrialGuestReadable(health,0x50)||word(health)!=0x453550) return false;
    const uint32_t actor=word(health+4),fill=word(health+0xc),sprites=word(beat+0x6c);
    if(!TrialGuestReadable(actor,0x58)||word(actor+0x54)!=word(beat+8)||
       word(beat+8)==0xffffffff||!TrialGuestReadable(fill,0x18)||word(fill)!=0x4531c0||
       !TrialGuestReadable(sprites,8)||!TrialGuestReadable(ctx.ebp,12)) return false;
    const uint32_t sprite=word(sprites+4),descriptor=word(ctx.ebp+8);
    const uint32_t samples=word(beat+0x64),colors=word(beat+0x68);
    if(!TrialGuestReadable(sprite,0x20)||!TrialGuestReadable(word(sprite),12)||
       word(word(sprite)+8)!=0x7b180||!TrialGuestReadable(descriptor,0x44)||
       !TrialGuestReadable(samples,53*4)||!TrialGuestReadable(colors,53*16)||
       ctx.esp>0xffffff2bu||!TrialGuestReadable(ctx.esp+0x90,0x44)) return false;
    out.owner=beat;out.actor=word(beat+8);out.sample_owner=samples;out.color_owner=colors;
    out.sweep=real(beat+0x5c);out.elapsed=real(beat+0x74);out.health=real(beat+0x14);
    out.extent=real(fill+8);out.fraction=real(health+0x4c);out.sprite=sprite;
    out.fill_x=real(descriptor)+54;out.fill_y=real(descriptor+4)+14;
    std::memcpy(out.color.data(),base+color,16);
    // 71E04..71E59 already applied the monitor's own slide. The incoming
    // descriptor alone is not its displayed origin during HUD transitions.
    std::memcpy(out.descriptor.data(),base+ctx.esp+0x90,0x44);
    if(!std::isfinite(out.sweep)||out.sweep<0||out.sweep>=1||
       !std::isfinite(out.elapsed)||out.elapsed<0||out.elapsed>0x1p24f||
       !std::isfinite(out.health)||out.health<=0||
       !std::isfinite(out.extent)||out.extent<20||out.extent>2580||
       !std::isfinite(out.fraction)||out.fraction<=0||out.fraction>4||
       !std::isfinite(out.fill_x)||std::fabs(out.fill_x)>65536||
       !std::isfinite(out.fill_y)||std::fabs(out.fill_y)>65536) return false;
    for(unsigned i=0;i<4;++i) {
        if(!std::isfinite(out.color[i])||out.color[i]<0||out.color[i]>1||
           !std::isfinite(out.descriptor[i])||std::fabs(out.descriptor[i])>65536||
           !std::isfinite(out.descriptor[4+i])||out.descriptor[4+i]<0||out.descriptor[4+i]>1) return false;
    }
    return out.color[3]>0 && out.descriptor[7]>0;
}

static void AdvanceTrace(TraceHistory& h,const PulseSnapshot& s){
    if(h.owner!=s.owner||h.actor!=s.actor||h.sample_owner!=s.sample_owner||h.color_owner!=s.color_owner||
       s.elapsed<h.elapsed||s.elapsed-h.elapsed>1.f){
        h={};h.owner=s.owner;h.actor=s.actor;h.sample_owner=s.sample_owner;h.color_owner=s.color_owner;
    }else if(s.elapsed>h.elapsed&&s.sweep<h.sweep){
        // Start the small bar highlight when the original monitor reaches its
        // right edge. The same source sweep drives it; drawing owns no clock.
        h.bar_started=true;
    }
    h.sweep=s.sweep;h.elapsed=s.elapsed;
}

// Add only a short highlight: the original ECG, fill and damage layers draw
// unchanged. Original 7B180 owns pixel rounding, material and alpha blend.
template<class Emit>
static void LifelinePulse(const PulseSnapshot& s,TraceHistory& h,Emit emit) {
    const float x=std::floor(s.fill_x),end=std::floor(s.fill_x+s.extent*std::min(s.fraction,1.f));
    AdvanceTrace(h,s);
    if(h.bar_started&&end>x){
        const float length=std::min(6.f,end-x),left=x+(end-x-length)*s.sweep;
        auto d=s.descriptor;
        d[0]=left;d[1]=s.fill_y-1;d[2]=std::min(left+length,end);d[3]=s.fill_y+2;
        for(unsigned j=0;j<4;++j)d[4+j]=(j<3?s.color[j]*.65f+.35f:s.color[j])*s.descriptor[4+j];
        d[9]=d[10]=0;d[15]=d[16]=1;
        emit(d);
    }
}
}

// Called at7200D after the unchanged 53-column ECG; never replaces a draw.
void TrialHealthPulse(X86Context& ctx,uint8_t* base) {
    struct Restore {
        X86Context& ctx;X86Context saved;unsigned csr;
        ~Restore(){ctx=saved;_mm_setcsr(csr);}
    } restore{ctx,ctx,_mm_getcsr()};
    _mm_setcsr(0x1f80);
    PulseSnapshot s;
    if(!CapturePulse(ctx,base,s)||ctx.esp<0x10100){if(trace.owner==ctx.esi)trace={};return;}
    const uint32_t scratch=(ctx.esp-0x80)&~15u;
    if(!TrialGuestReadable(scratch-0x100,0x180))return;
    LifelinePulse(s,trace,[&](const std::array<float,17>& d){
        std::memcpy(base+scratch,d.data(),0x44);
        ctx.esp=scratch;ctx.ecx=s.sprite;
        kernel::CallGuest(ctx,0x7b180,{scratch});
        // A draw can drain the renderer and change the host arithmetic mode.
        _mm_setcsr(0x1f80);
    });
}
