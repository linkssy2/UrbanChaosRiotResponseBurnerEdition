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
static std::vector<std::array<uint32_t,4>> rectangles;
static float u,v;
static unsigned clears;
static constexpr uint32_t stack=0x700000,name_address=0x600000;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context&,uint8_t*,uint32_t){std::abort();}
namespace kernel {
uint32_t CallGuest(X86Context& ctx,uint32_t function,const uint32_t* args,size_t count){
    assert(function==0x3e4270&&count==6&&(args[0]==2||args[0]==4)&&args[2]==0xf0);
    assert(args[3]==0xff000000u&&args[4]==0x3f800000u&&args[5]==0);
    for(unsigned i=0;i<args[0];++i){
        std::array<uint32_t,4> rect;std::memcpy(rect.data(),g_base+args[1]+16*i,16);
        rectangles.push_back(rect);
    }
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
    points.clear();calls.clear();rectangles.clear();clears=0;
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
static void Corners(float x0,float x1,float y0,float y1,float top=0,float bottom=0){
    const Point expected[]={{x1,y0,0,1,640,top},{x0,y0,0,1,0,top},{x0,y1,0,1,0,480-bottom},{x1,y1,0,1,640,480-bottom}};
    assert(points.size()==4);
    for(unsigned i=0;i<4;++i){
        const auto& a=points[i];const auto& b=expected[i];
        assert(Near(a.x,b.x)&&Near(a.y,b.y)&&a.z==b.z&&a.w==b.w&&a.u==b.u&&a.v==b.v);
    }
}
int main(int argc,char**){
    Open("intro_1");const auto original=Draw(true);const auto original_calls=calls;
    Corners(0,640,0,480);assert(!clears);
    // Independent audited movie cases; black-row bounds include a four-pixel safety margin.
    const struct {const char* name;unsigned top,bottom;} movies[]={
        {"attract",92,92},{"complete",60,92},{"credits",92,92},{"dolby",44,44},
        {"EidosLogo",92,92},{"rocklogo",92,92},{"intro_12_edit",92,92},
        {"intro_21",60,92},{"intro_22",60,92},{"intro_23",60,92},
        {"intro_24",60,92},{"intro_25",60,92},{"intro_26",60,92},{"reward",60,92},
        {"intro_1",0,0},{"intro_2",0,0},{"intro_3",0,0},{"intro_4",0,0},
        {"intro_5",0,0},{"intro_6",0,0},{"intro_8",0,0},{"intro_9",0,0},
        {"intro_11",0,0},{"intro_12",0,0},{"intro_13",0,0},{"outro_13",0,0},
        {"PAL_INTRO_1",0,0},{"pal_intro_2",0,0},{"pal_intro_3",0,0},{"pal_intro_4",0,0},
        {"pal_intro_5",0,0},{"pal_intro_6",0,0},{"pal_intro_8",0,0},{"pal_intro_9",0,0},
        {"pal_intro_11",0,0},{"pal_intro_12",0,0},{"pal_intro_13",0,0},{"pal_outro_13",0,0},
        {"unknown",0,0},{"pal_dolby",0,0},
    };
    unsigned cases=0;
    for(const auto& movie:movies)for(bool wide:{false,true})for(unsigned mode:{0u,1u,2u}){
        trial_graphics::wide=wide;trial_graphics::mode=mode;Open(movie.name);
        const float top=mode==0?float(movie.top):0,bottom=mode==0?float(movie.bottom):0;
        const double source_height=480-top-bottom,display=wide?16./9:4./3;
        const double scale=(std::min)(480*display/640,480/source_height);
        const float w=float(640*scale*640/(480*display)),h=float(source_height*scale);
        const float x0=(640-w)/2,y0=(480-h)/2;
        const auto fitted=Draw(argc>1);Corners(x0,640-x0,y0,480-y0,top,bottom);
        assert(std::memcmp(&original,&fitted,sizeof(original))==0);
        assert(calls.size()==original_calls.size()+clears);
        assert(std::equal(original_calls.begin(),original_calls.end(),calls.begin()));
        std::vector<std::array<uint32_t,4>> expected;
        const uint32_t x=uint32_t(points[1].x),y=uint32_t(points[1].y);
        if(x){expected.push_back({0,0,x,480});expected.push_back({640-x,0,640,480});}
        if(y){expected.push_back({0,0,640,y});expected.push_back({0,480-y,640,480});}
        assert(rectangles==expected&&clears==unsigned(!expected.empty()));
        for(float screen_h:{480.f,720.f,1080.f,1440.f}){
            const float screen_w=screen_h*float(display);
            const float sx=(points[0].x-points[1].x)*screen_w/640/640;
            const float sy=(points[2].y-points[1].y)*screen_h/480/float(source_height);
            assert(Near(sx,sy));
        }
        ++cases;
    }
    trial_graphics::wide=true;trial_graphics::mode=0;
    Open("unknown");Draw();Corners(80,560,0,480);assert(clears==1);
    for(const char* name: {"fe_bk1","fe_mp","gamelogo","splash"}){
        Open(name);
        for(unsigned mode:{0u,1u}){trial_graphics::mode=mode;Draw();Corners(0,640,0,480);assert(!clears);}
    }
    trial_graphics::wide=true;trial_graphics::mode=0;
    Open("dolby");
    Draw(false,true,true);Corners(0,640,0,448);assert(!clears);
    Draw(false,false);assert(calls.empty()&&!clears);
    TrialFmvClose();Draw();Corners(0,640,0,480);assert(!clears);
    // A malformed open cannot retain a previous movie's fill policy.
    Open("dolby");X86Context c{};c.esp=stack;W32(stack+4,name_address);
    std::memset(g_base+name_address,'x',64);TrialFmvOpen(c,g_base);Draw();Corners(80,560,0,480);
    Open("dolby");c.esp=0xfffffffc;TrialFmvOpen(c,g_base);Draw();Corners(80,560,0,480);
    Open("dolby");c.esp=stack;W32(stack+4,name_address);mem::denied=name_address;
    TrialFmvOpen(c,g_base);mem::denied=0xffffffff;Draw();Corners(80,560,0,480);
    // Native math must not leak changed floating-point flags back into guest code.
    Open("dolby");const unsigned previous=_mm_getcsr();_mm_setcsr(0x7f80);
    Draw();assert(_mm_getcsr()==0x7f80);_mm_setcsr(previous);
    trial_graphics::mode=1;_mm_setcsr(0x7f80);Draw();assert(_mm_getcsr()==0x7f80);_mm_setcsr(previous);
    // Invalid texture coordinates and unreadable storage must remain untouched.
    const auto layout=trial_fmv::Geometry(trial_fmv::Classify("dolby"),true,0);
    for(unsigned invalid=0;invalid<5;++invalid){
        X86Context c{};c.esp=stack;W32(stack,invalid==0?8:9);
        G<float>(stack+4)=invalid==1?17:640;G<float>(stack+8)=invalid==2?239:0;
        if(invalid==3)W32(stack+8,0x7fc00000);
        if(invalid==4)mem::denied=stack;
        std::array<uint8_t,12> before;std::memcpy(before.data(),g_base+stack,12);
        const X86Context saved=c;TrialFmvTexcoord(c,g_base,layout);mem::denied=0xffffffff;
        assert(std::memcmp(before.data(),g_base+stack,12)==0&&std::memcmp(&saved,&c,sizeof(c))==0);
    }
    std::printf("FMV: %u original-guest draw cases; proportional Fill/Original, 4:3/16:9, four output sizes, UV/ABI, clear ordering, backgrounds, guards and lifecycle passed\n",cases);
}
