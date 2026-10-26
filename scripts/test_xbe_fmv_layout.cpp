// Execute the retail movie draw and its patched counterpart; SDK submission is captured, not rasterized.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "kernel/kernel.h"
#include "xbe_graphics_settings.h"
namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
namespace mem {
static uint32_t denied=0xffffffff;
bool IsCommitted(uint32_t page){return page<kernel::memory.size()&&page!=denied;}
}
namespace trial_graphics {
static bool wide=true;
static uint32_t mode=0;
bool Widescreen(){return wide;}
uint32_t FmvScale(){return mode;}
}
#include "../common/xbe_fmv_layout.cpp"
struct Point {float x,y,z,w,u,v;};
static std::vector<Point> points;
static std::vector<uint32_t> calls;
static float u,v;
static unsigned clears;
static constexpr uint32_t stack=0x700000,name_address=0x600000;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context&,uint8_t*,uint32_t){std::abort();}
namespace kernel {
uint32_t CallGuest(X86Context& ctx,uint32_t function,const uint32_t* args,size_t count){
    assert(function==0x3e4270&&count==6&&args[0]==2&&args[2]==0xf0);
    assert(args[3]==0xff000000u&&args[4]==0x3f800000u&&args[5]==0);
    const uint32_t expected[]={0,0,80,480,560,0,640,480};
    assert(std::memcmp(g_base+args[1],expected,sizeof(expected))==0);
    assert(points.size()==4);++clears;calls.push_back(function);
    ctx.eax=0x12345678;ctx.edx=0xabcdef01;ctx.esp+=28;ctx.xmm[0].u[0]=7;
    return ctx.eax;
}
}
X86_FUNC(sub_003E7770){calls.push_back(0x3e7770);ctx.esp+=8;}
X86_FUNC(sub_003E82D0){calls.push_back(0x3e82d0);ctx.esp+=8;}
X86_FUNC(sub_003E8470){calls.push_back(0x3e8470);ctx.esp+=8;}
X86_FUNC(sub_003E3840){assert(R32(ctx.esp+4)==0&&R32(ctx.esp+8)==0x123400);calls.push_back(0x3e3840);ctx.esp+=12;}
X86_FUNC(sub_003E6EE0){assert(R32(ctx.esp+4)==0x104);calls.push_back(0x3e6ee0);ctx.esp+=8;}
X86_FUNC(sub_003EF700){assert(R32(ctx.esp+4)==8);calls.push_back(0x3ef700);ctx.esp+=8;}
X86_FUNC(sub_003EF5C0){assert(R32(ctx.esp+4)==9);u=G<float>(ctx.esp+8);v=G<float>(ctx.esp+12);calls.push_back(0x3ef5c0);ctx.esp+=16;}
X86_FUNC(sub_003EF600){
    assert(R32(ctx.esp+4)==0xffffffffu);
    points.push_back({G<float>(ctx.esp+8),G<float>(ctx.esp+12),G<float>(ctx.esp+16),G<float>(ctx.esp+20),u,v});
    calls.push_back(0x3ef600);ctx.esp+=24;
}
X86_FUNC(sub_003EF740){calls.push_back(0x3ef740);ctx.esp+=4;}
// GENERATED_ORIGINAL_DRAW
// GENERATED_DRAW

static void Open(const char* name){
    X86Context c{};c.esp=stack;W32(stack,0x12345678);W32(stack+4,name_address);
    std::strcpy(reinterpret_cast<char*>(g_base+name_address),name);
    const X86Context before=c;TrialFmvOpen(c,g_base);assert(std::memcmp(&c,&before,sizeof(c))==0);
}
static X86Context Draw(bool original=false,bool live=true,bool alternate=false){
    points.clear();calls.clear();clears=0;
    W32(0x519e20,live?0x123400:0);g_base[0x519e28]=alternate;
    G<float>(0x455ad0)=448;G<float>(0x4523d0)=480;
    X86Context c{};c.esp=stack;c.esi=0x12345678;c.ecx=0x89abcdef;c.edx=0xf00d;
    W32(stack,0xaabbccdd);W32(stack+4,0x44556677);
    if(original)OriginalMovieDraw(c,g_base);else __imp__sub_00087810(c,g_base);
    assert(c.esp==stack+4&&c.esi==0x12345678);
    assert(c.ecx==(live?(alternate?0x43e00000u:0x43f00000u):0x89abcdefu));
    assert(R32(stack)==0xaabbccdd&&R32(stack+4)==0x44556677);
    assert(points.size()==(live?4:0));return c;
}
static bool Near(float a,float b){return std::abs(a-b)<.001f;}
static void Corners(float x0,float x1,float y0,float y1){
    const Point expected[]={{x1,y0,0,1,640,0},{x0,y0,0,1,0,0},{x0,y1,0,1,0,480},{x1,y1,0,1,640,480}};
    assert(points.size()==4);
    for(unsigned i=0;i<4;++i){
        const auto& a=points[i];const auto& b=expected[i];
        assert(Near(a.x,b.x)&&Near(a.y,b.y)&&a.z==b.z&&a.w==b.w&&a.u==b.u&&a.v==b.v);
    }
}
int main(int argc,char**){
    Open("intro_1");const auto original=Draw(true);const auto original_calls=calls;
    Corners(0,640,0,480);assert(!clears);
    // This independent source rectangle is the released Black Fill behavior at16:9.
    Draw(argc>1);Corners(0,640,-80,560);assert(!clears&&calls==original_calls);
    assert(Near(480*(0-points[0].y)/(points[2].y-points[0].y),60));
    assert(Near(480*(480-points[0].y)/(points[2].y-points[0].y),420));
    for(auto size: {std::array<float,2>{1280,720}, {1920,1080}, {2560,1440}}){
        assert(Near((points[0].x-points[1].x)*size[0]/640/640,
                    (points[2].y-points[1].y)*size[1]/480/480));
    }
    trial_graphics::mode=1;const auto fitted=Draw();Corners(80,560,0,480);
    assert(clears==1&&std::memcmp(&original,&fitted,sizeof(original))==0);
    assert(calls.size()==original_calls.size()+1&&calls.back()==0x3e4270);
    trial_graphics::mode=0;
    for(const char* name: {"EidosLogo","rocklogo","dolby","unknown"}){
        Open(name);Draw();Corners(80,560,0,480);assert(clears==1);
    }
    for(const char* name: {"fe_bk1","fe_mp","gamelogo","splash"}){
        Open(name);Draw();Corners(0,640,0,480);assert(!clears);
    }
    for(const char* name: {"intro_12_edit","outro_13","PAL_INTRO_1","intro_26","credits","reward","attract","complete"}){
        Open(name);Draw();Corners(0,640,-80,560);assert(!clears);
    }
    trial_graphics::wide=false;
    for(unsigned mode: {0u,1u}){trial_graphics::mode=mode;Draw();Corners(0,640,0,480);assert(!clears);}
    trial_graphics::wide=true;trial_graphics::mode=0;
    Draw(false,true,true);Corners(0,640,0,448);assert(!clears);
    Draw(false,false);assert(calls.empty()&&!clears);
    TrialFmvClose();Draw();Corners(0,640,0,480);assert(!clears);
    // A malformed open cannot retain a previous movie's fill policy.
    Open("intro_1");X86Context c{};c.esp=stack;W32(stack+4,name_address);
    std::memset(g_base+name_address,'x',64);TrialFmvOpen(c,g_base);Draw();Corners(80,560,0,480);
    Open("intro_1");c.esp=0xfffffffc;TrialFmvOpen(c,g_base);Draw();Corners(80,560,0,480);
    Open("intro_1");c.esp=stack;W32(stack+4,name_address);mem::denied=name_address;
    TrialFmvOpen(c,g_base);mem::denied=0xffffffff;Draw();Corners(80,560,0,480);
    // Native math must not leak changed floating-point flags back into guest code.
    Open("intro_1");const unsigned previous=_mm_getcsr();_mm_setcsr(0x7f80);
    Draw();assert(_mm_getcsr()==0x7f80);_mm_setcsr(previous);Corners(0,640,-80,560);
    std::puts("FMV: retail quad, Fill crop, 4:3 bands, UV/ABI preservation, logos, backgrounds, invalid names and lifecycle passed");
}
