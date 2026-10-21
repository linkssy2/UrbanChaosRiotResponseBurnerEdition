// Execute the retail detector with the production host sampler and guest seams.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
using namespace kernel;
static std::recursive_mutex input_lock;
static bool keyboard=true,focus=true,gameplay=true,menu_mode=false,ui=false,button=false;
static unsigned mode=0,spins=0;static uint64_t now=100;
static long spin_dx=0,spin_dy=0;static bool spin_button_held=true,spin_button=false;
static float spin_x=0,spin_y=0;static uint64_t spin_tick=0;
static int last_pad[4]{};
static bool focused(){return focus;}static bool TrialGraphicsOpen(){return ui;}
static int xbox_KeyboardMode(){return keyboard;}static uint64_t GetTickCount64(){return now;}
namespace trial_controls {unsigned SpinMode(){return mode;}bool SpinButtonDown(bool,int){return button;}}
// PRODUCTION_HOST_SAMPLE
static bool menu_pointer_readable(uint32_t a,uint32_t n){return a>=0x10000&&uint64_t(a)+n<=memory.size();}
static bool MouseObject(uint32_t a){return menu_pointer_readable(a,0x694)&&G<uint32_t>(a+0x368)==0;}
#define g_esi ctx.esi
#define g_esp ctx.esp
#define MEMF(a) G<float>(a)
static int TrialGuestHook(X86Context& ctx,uint8_t*,unsigned op){int result=0;switch(op){
// PRODUCTION_GUEST_SAMPLE
}return result;}
static float pad_x=0,pad_y=0;
static bool blocked=false,pistol=true;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& c,uint8_t*,uint32_t address){
    if(address==0xf0001){const unsigned action=R32(c.esp+8);x87_push(c,action==3?pad_x:pad_y);c.esp+=16;return;}
    assert(address==0xf0002);c.eax=blocked;c.esp+=4;
}
X86_FUNC(sub_0004A840){ctx.eax=0x610000;ctx.esp+=4;}
X86_FUNC(sub_000643E0){ctx.eax=0x620000;ctx.esp+=4;}
X86_FUNC(sub_00043150){ctx.eax=pistol;ctx.esp+=4;}
X86_FUNC(sub_000BFE00){assert(R32(ctx.esp+4)==2&&R32(ctx.esp+8)==0);++spins;ctx.esp+=12;}
X86_FUNC(sub_0005E660){x87_push(ctx,std::remainder(double(G<float>(ctx.esp+8))-G<float>(ctx.esp+4),6.283185307179586));ctx.esp+=4;}
// ORIGINAL_SPIN_DETECTOR
static constexpr uint32_t player=0x600000,stack=0x700000;
static void reset(){
    memory.fill(0);W32(0x51c4dc,0x630000);W32(0x630000,0x630100);W32(0x630108,0xf0001);
    W32(0x610000,0x610100);W32(0x610120,0xf0002);G<uint8_t>(0x620034)=1;
    G<float>(0x44e548)=.9f;G<float>(0x4583dc)=12.566370964f;
    mode=0;keyboard=focus=gameplay=true;menu_mode=ui=button=blocked=false;pistol=true;
    spin_dx=spin_dy=0;spin_tick=0;spin_button_held=true;spins=0;pad_x=pad_y=0;
}
static void tick(){
    now+=8;X86Context c{};c.ecx=player;c.esp=stack;c.esi=0x12345678;c.ebx=0x87654321;
    W32(stack,0xabcdef01);__imp__sub_000C64D0(c,g_base);
    assert(c.esp==stack+4&&c.esi==0x12345678&&c.ebx==0x87654321);
}
int main(){
    for(bool mouse:{true,false})for(int direction:{-1,1}){
        reset();keyboard=mouse;
        for(unsigned i=0;i<67;++i){
            const double angle=direction*i*6.283185307179586/32;
            if(mouse){spin_dx=long(std::cos(angle)*1000);spin_dy=long(-std::sin(angle)*1000);}
            else{pad_x=float(std::cos(angle));pad_y=float(std::sin(angle));}
            tick();if(i<=32)assert(!G<uint8_t>(0x51bdf4));
        }
        assert(G<uint8_t>(0x51bdf4)&&spins>0);
    }
    reset();mode=1;tick();button=true;tick();assert(spins==1&&G<uint8_t>(0x51bdf4));
    tick();assert(spins==1);button=false;tick();button=true;tick();assert(spins==2);
    for(unsigned guard=0;guard<5;++guard){
        reset();mode=1;tick();button=true;
        if(guard==0)blocked=true;if(guard==1)G<uint8_t>(player+0x2ac)=1;
        if(guard==2)W32(player+0x4fc,0xffffffff);if(guard==3)menu_mode=true;if(guard==4)focus=false;
        tick();assert(spins==0&&!G<uint8_t>(0x51bdf4));
    }
    reset();mode=1;tick();pistol=false;button=true;tick();assert(!spins&&G<uint8_t>(0x51bdf4));
    reset();mode=1;tick();G<uint8_t>(0x620034)=0;button=true;tick();assert(!G<uint8_t>(0x51bdf4));
    std::puts("pistol spin: retail two-circle detector, both devices/directions, button edges, gameplay gates and original reward passed");
}
