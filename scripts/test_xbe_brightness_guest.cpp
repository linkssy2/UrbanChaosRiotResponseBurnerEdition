// Execute the original XBE sampling loop with full and reduced publications.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
using namespace kernel;
static bool reduce=false,admitted=false;
static constexpr uint32_t surface=0x20000,pixels=0x100000,viewport=0x30000;
static unsigned width=640,height=480;
X86_FUNC(sub_003E3250){ctx.eax=surface;ctx.esp+=8;}
X86_FUNC(sub_003E61F0){uint32_t out=R32(ctx.esp+8);W32(out,2560);W32(out+4,pixels);ctx.eax=0;ctx.esp+=20;}
X86_FUNC(sub_003E55B0){ctx.eax=0;ctx.esp+=8;}
X86_FUNC(sub_002D6780){ctx.esp+=4;}
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target){
    assert(target==0x24||target==0x28||target==8||target==4);
    ctx.eax=target==0x24?width:target==0x28?height:0;ctx.esp+=4;
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
extern "C" void TrialBrightnessComplete(X86Context& ctx){
    admitted=ctx.ebp==20&&ctx.edx==640&&ctx.ebx==480&&R32(ctx.esp+0x14)==0&&
        R32(ctx.esp+0x24)==2560&&R32(ctx.esp+0x28)==pixels;
    if(!reduce||!admitted)return;
    unsigned sum=0;
    for(unsigned y=15;y<480;y+=30)for(unsigned x=20;x<640;x+=40)
        for(unsigned c=0;c<3;++c)sum+=g_base[pixels+(y*640+x)*4+c];
    unsigned remainder=sum%768;
    for(unsigned y=15;y<480;y+=30)for(unsigned x=20;x<640;x+=40)
        for(unsigned c=0;c<3;++c){g_base[pixels+(y*640+x)*4+c]=sum/768+(remainder!=0);if(remainder)--remainder;}
}
/* ORIGINAL */
static unsigned invoke(){
    X86Context ctx{};ctx.ecx=0x10000;ctx.esp=0x70000;
    W32(ctx.esp,0);W32(ctx.esp+4,viewport);W32(viewport,viewport+0x100);
    for(unsigned offset:{4u,8u,0x24u,0x28u})W32(viewport+0x100+offset,offset);
    __imp__sub_0008BBE0(ctx,g_base);assert(ctx.esp==0x70008);
    return R32(0x10038);
}
int main(){
    for(unsigned trial=0;trial<32;++trial){
        unsigned seed=trial+7;
        for(unsigned i=0;i<640*480*4;++i){seed=seed*1664525u+1013904223u;g_base[pixels+i]=seed>>24;}
        W32(0x10038,trial*1021);reduce=false;unsigned expected=invoke();assert(admitted);
        W32(0x10038,trial*1021);reduce=true;assert(invoke()==expected&&admitted);
    }
    width=320;height=240;invoke();assert(!admitted);
    std::puts("Brightness XBE oracle: unchanged sampling loop produces identical full/reduced averages; subviewport rejected");
}
