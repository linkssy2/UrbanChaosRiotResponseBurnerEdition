#include <atomic>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_graphics_settings.h"
#include "xbe_fmv_layout.h"

namespace {
std::atomic<trial_fmv::Kind> movie_kind{trial_fmv::Kind::Background};
struct Arithmetic {unsigned csr=_mm_getcsr();Arithmetic(){_mm_setcsr(0x1f80);}~Arithmetic(){_mm_setcsr(csr);}};
uint32_t FmvWord(uint8_t* base,uint32_t address) {
    uint32_t value;std::memcpy(&value,base+address,4);return value;
}
}
void TrialFmvClose(){movie_kind=trial_fmv::Kind::Background;}
void TrialFmvOpen(X86Context& ctx,uint8_t* base) {
    movie_kind=trial_fmv::Kind::Preserve;
    if(!TrialGuestReadable(ctx.esp,8))return;
    uint32_t address=FmvWord(base,ctx.esp+4);
    char name[64]{};
    for(unsigned i=0;i<sizeof(name);++i){
        if(uint64_t(address)+i>UINT32_MAX||!TrialGuestReadable(address+i,1))return;
        char ch=char(base[address+i]);
        if(!ch){movie_kind=trial_fmv::Classify(std::string_view(name,i));return;}
        name[i]=ch>='A'&&ch<='Z'?char(ch+'a'-'A'):ch;
    }
}
trial_fmv::Layout TrialFmvLayout(uint8_t* base) {
    // Only the verified 640x480 movie quad; the alternate retail 448-line path stays authored.
    if(!TrialGuestReadable(0x519e20,9)||!FmvWord(base,0x519e20)||base[0x519e28])return {};
    return trial_fmv::Geometry(movie_kind.load(),trial_graphics::Widescreen(),trial_graphics::FmvScale());
}
void TrialFmvVertex(X86Context& ctx,uint8_t* base,trial_fmv::Layout layout) {
    if(layout.x==1&&layout.y==1)return;
    if(!TrialGuestReadable(ctx.esp,20)||FmvWord(base,ctx.esp)!=0xffffffffu)return;
    float xy[2];std::memcpy(xy,base+ctx.esp+4,sizeof(xy));
    if(!std::isfinite(xy[0])||!std::isfinite(xy[1]))return;
    Arithmetic arithmetic;
    // Correct only this film's submitted positions; its UVs, other draws and queued state stay authored.
    xy[0]=320+(xy[0]-320)*layout.x;xy[1]=240+(xy[1]-240)*layout.y;
    std::memcpy(base+ctx.esp+4,xy,sizeof(xy));
}
void TrialFmvBorders(X86Context& ctx,uint8_t* base,trial_fmv::Layout layout) {
    if(layout.x>=1||ctx.esp<0x10200)return;
    const uint32_t scratch=(ctx.esp-128)&~15u;
    if(!TrialGuestReadable(scratch-32,160))return;
    Arithmetic arithmetic;
    const X86Context saved=ctx;const uint32_t edge=uint32_t(320*(1-layout.x));
    // Movie frames skip the normal color clear; cover both newly exposed side bands.
    const uint32_t bounds[]={0,0,edge,480,640-edge,0,640,480};
    std::memcpy(base+scratch,bounds,sizeof(bounds));ctx.esp=scratch;
    kernel::CallGuest(ctx,0x3e4270,{2,scratch,0xf0,0xff000000u,0x3f800000u,0});
    ctx=saved;
}
