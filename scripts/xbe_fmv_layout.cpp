#include <atomic>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_graphics_settings.h"
#include "xbe_fmv_layout.h"

namespace {
std::atomic<trial_fmv::Movie> movie_info{trial_fmv::Movie{trial_fmv::Kind::Background}};
struct Arithmetic {unsigned csr=_mm_getcsr();Arithmetic(){_mm_setcsr(0x1f80);}~Arithmetic(){_mm_setcsr(csr);}};
uint32_t FmvWord(uint8_t* base,uint32_t address) {
    uint32_t value;std::memcpy(&value,base+address,4);return value;
}
}
void TrialFmvClose(){movie_info=trial_fmv::Movie{trial_fmv::Kind::Background};}
void TrialFmvOpen(X86Context& ctx,uint8_t* base) {
    movie_info=trial_fmv::Movie{};
    if(!TrialGuestReadable(ctx.esp,8))return;
    uint32_t address=FmvWord(base,ctx.esp+4);
    char name[64]{};
    for(unsigned i=0;i<sizeof(name);++i){
        if(uint64_t(address)+i>UINT32_MAX||!TrialGuestReadable(address+i,1))return;
        char ch=char(base[address+i]);
        if(!ch){movie_info=trial_fmv::Classify(std::string_view(name,i));return;}
        name[i]=ch>='A'&&ch<='Z'?char(ch+'a'-'A'):ch;
    }
}
trial_fmv::Layout TrialFmvLayout(uint8_t* base) {
    // Only the verified 640x480 movie quad; the alternate retail 448-line path stays authored.
    if(!TrialGuestReadable(0x519e20,9)||!FmvWord(base,0x519e20)||base[0x519e28])return {};
    Arithmetic arithmetic;
    return trial_fmv::Geometry(movie_info.load(),trial_graphics::AspectRatio(),trial_graphics::FmvScale());
}
void TrialFmvVertex(X86Context& ctx,uint8_t* base,trial_fmv::Layout layout) {
    if(layout.x==1&&layout.y==1)return;
    if(!TrialGuestReadable(ctx.esp,20)||FmvWord(base,ctx.esp)!=0xffffffffu)return;
    float xy[2];std::memcpy(xy,base+ctx.esp+4,sizeof(xy));
    if(!std::isfinite(xy[0])||!std::isfinite(xy[1]))return;
    Arithmetic arithmetic;
    // Fit this movie's active picture; other draws and queued state stay authored.
    xy[0]=320+(xy[0]-320)*layout.x;xy[1]=240+(xy[1]-240)*layout.y;
    std::memcpy(base+ctx.esp+4,xy,sizeof(xy));
}
void TrialFmvTexcoord(X86Context& ctx,uint8_t* base,trial_fmv::Layout layout) {
    if(layout.top==0&&layout.bottom==0)return;
    if(!TrialGuestReadable(ctx.esp,12)||FmvWord(base,ctx.esp)!=9)return;
    Arithmetic arithmetic;
    float uv[2];std::memcpy(uv,base+ctx.esp+4,sizeof(uv));
    if((uv[0]!=0&&uv[0]!=640)||(uv[1]!=0&&uv[1]!=480))return;
    const float v=uv[1]==0?layout.top:480-layout.bottom;
    std::memcpy(base+ctx.esp+8,&v,sizeof(v));
}
void TrialFmvBorders(X86Context& ctx,uint8_t* base,trial_fmv::Layout layout) {
    if((layout.x>=1&&layout.y>=1)||ctx.esp<0x10200)return;
    const uint32_t scratch=(ctx.esp-128)&~15u;
    if(!TrialGuestReadable(scratch-32,160))return;
    Arithmetic arithmetic;
    const X86Context saved=ctx;
    const uint32_t x=uint32_t(320-320*layout.x),y=uint32_t(240-240*layout.y);
    // Movie playback skips color clears; cover exposed bands after its queued quad.
    uint32_t bounds[16],count=0;
    if(x){const uint32_t side[]={0,0,x,480,640-x,0,640,480};std::memcpy(bounds,side,sizeof(side));count=2;}
    if(y){const uint32_t side[]={0,0,640,y,0,480-y,640,480};std::memcpy(bounds+4*count,side,sizeof(side));count+=2;}
    if(!count)return;
    std::memcpy(base+scratch,bounds,count*16);ctx.esp=scratch;
    kernel::CallGuest(ctx,0x3e4270,{count,scratch,0xf0,0xff000000u,0x3f800000u,0});
    ctx=saved;
}
