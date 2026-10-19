// Real movie update and query bodies, the production hooks and host latch.
// Decoder/draw boundaries are substituted; this does not claim live playback QA.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include "kernel/kernel.h"
#include "../common/xbe_intro_skip.h"
#include "../common/xbe_menu_key_taps.h"
namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
namespace mem {
static uint32_t denied=0xffffffff;
bool IsCommitted(uint32_t page){return page<kernel::memory.size()&&page!=denied;}
}
namespace {
std::recursive_mutex input_lock;
trial_intro::Gate intro_gate;
trial_input::MenuKeyTaps menu_key_taps;
bool click_pending=false,test_focused=true,test_graphics=false;
bool focused(){return test_focused;}
bool TrialGraphicsOpen(){return test_graphics;}
}
// GENERATED_HOST
#include "../common/xbe_intro_skip.cpp"

static constexpr uint32_t screen=0x600000,stack=0x700000;
static unsigned opens,closes,draws,original_queries;
static bool eof,original_held;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& ctx,uint8_t*,uint32_t address){
    assert(address==0xf0001&&R32(ctx.esp+12)==1);
    assert(R32(ctx.esp+4)<4);++original_queries;
    x87_push(ctx,original_held?1.:0.);ctx.esp+=16;
}
X86_FUNC(sub_000643E0){ctx.eax=0x601100;ctx.esp+=4;}
X86_FUNC(sub_00087960){assert(R32(ctx.esp+4)==screen+0x24);++opens;ctx.esp+=4;}
X86_FUNC(sub_000877A0){ctx.eax=eof;ctx.esp+=4;}
X86_FUNC(sub_000877B0){++closes;ctx.esp+=4;}
X86_FUNC(sub_000656D0){++draws;ctx.esp+=4;}
// Optional movie clock overlay is absent in startup screens used by this test.
X86_FUNC(sub_00017060){std::abort();}
X86_FUNC(sub_00017160){std::abort();}
X86_FUNC(sub_00017E40){std::abort();}
X86_FUNC(sub_00018CE0){std::abort();}
X86_FUNC(sub_002D6322){std::abort();}
// GENERATED_QUERY
// GENERATED_MOVIE

static void Seed(const char* name="dolby",unsigned flags=4){
    std::memset(g_base+screen,0,0x48);W32(screen,0x489020);
    W32(screen+4,0x2723b0);G<float>(screen+8)=2;W32(screen+12,flags);
    g_base[screen+0x14]=1;std::strcpy(reinterpret_cast<char*>(g_base+screen+0x24),name);
    W32(0x5096e0,0x601000);W32(0x601000+0x28,0x601100);g_base[0x601100+0x3b]=15;
    W32(0x51c4dc,0x601200);W32(0x601200,0x601300);W32(0x601300+8,0xf0001);
    // GENERATED_CONSTANTS
    eof=false;
}
static uint32_t Tick(float dt){
    X86Context ctx{};ctx.ecx=screen;ctx.esp=stack;
    ctx.esi=0x11111111;ctx.edi=0x22222222;ctx.ebp=0x33333333;
    W32(stack,0xabcdef01);G<float>(stack+4)=dt;W32(stack+8,0);W32(stack+12,0x12345678);
    __imp__sub_00272BB0(ctx,g_base);
    assert(ctx.esp==stack+12&&ctx.esi==0x11111111&&ctx.edi==0x22222222&&ctx.ebp==0x33333333);
    assert(R32(stack)==0xabcdef01&&R32(stack+12)==0x12345678);
    return ctx.eax;
}
static void Ready(const char* name="dolby",unsigned flags=4){
    Seed(name,flags);const unsigned before=opens;
    assert(Tick(0)==0&&opens==before+1&&g_base[screen+0x16]);
    assert(Tick(3)==0&&!g_base[screen+0x18]);
}
static void Finish(){
    const unsigned before=closes;
    for(unsigned i=0;i<10&&closes==before;++i)Tick(1.f/60);
    assert(closes==before+1&&!intro_gate.owner);
}
static void Physical(unsigned source,bool down){
    if(source==0)intro_gate.Key(1,down,true);
    else if(source==1)intro_gate.Key(2,down,true);
    else{std::array<uint8_t,8> analog{};intro_gate.Pad(int(source-2),down?16u:0u,analog,true);}
}
static void MenuTapHandoff(){
    intro_gate={};menu_key_taps.Clear();
    menu_key_taps.Event('F',true,false,true);menu_key_taps.Event('F',false,false,true);
    TrialIntroMovie(0,0,false,false); // An ownerless context must preserve menu input.
    menu_key_taps.Begin();assert(menu_key_taps.Retained('F'));
    menu_key_taps.Event('W',true,false,true);menu_key_taps.Event('W',false,false,true);
    TrialIntroMovie(screen,1,true,false); // Clear both published F and queued W.
    assert(!menu_key_taps.Retained('F'));
    TrialIntroClose(screen);menu_key_taps.Begin();
    assert(!menu_key_taps.Retained('F')&&!menu_key_taps.Retained('W'));menu_key_taps.End();
    menu_key_taps.Event('F',true,false,true);menu_key_taps.Event('F',false,false,true);
    menu_key_taps.Begin();assert(menu_key_taps.Retained('F'));
    menu_key_taps.GuestPortPolled(0);menu_key_taps.End();menu_key_taps.Clear();
}
static void Lifecycle(){
    const char* names[]={"dolby","EidosLogo","RockLogo","intro_12_edit"};
    for(unsigned physical=0;physical<6;++physical){
        intro_gate={};opens=closes=draws=original_queries=0;original_held=true;
        for(const char* name:names){
            Ready(name);
            if(opens>1){for(unsigned i=0;i<8;++i){Physical(physical,true);Tick(.016f);}
                assert(!g_base[screen+0x18]&&!g_base[screen+0x17]);}
            Physical(physical,false);Physical(physical,true);Tick(.016f);
            assert(g_base[screen+0x18]&&!g_base[screen+0x17]);
            const unsigned original_fade=R32(screen+0x20);
            assert(original_fade==48); // original fast-fade flag, untouched
            Finish();assert(intro_gate.OwnsPad(physical<2?0:int(physical-2)));
        }
        assert(opens==4&&closes==4&&original_queries==0&&draws==28);
        Physical(physical,false);assert(!intro_gate.OwnsPad(physical<2?0:int(physical-2)));
    }
    // A mouse down/up between controller polls is still exactly one skip.
    intro_gate={};Ready();intro_gate.Key(2,true,true);intro_gate.Key(2,false,true);
    Tick(.016f);assert(g_base[screen+0x18]);Finish();Ready("EidosLogo");
    for(unsigned i=0;i<10;++i)Tick(.016f);
    assert(!g_base[screen+0x18]);
    // No input before or exactly at the retail minimum-delay boundary is queued.
    intro_gate={};Seed();Tick(0);intro_gate.Key(1,true,true);Tick(2);Tick(.001f);
    assert(!g_base[screen+0x18]);intro_gate.Key(1,false,true);intro_gate.Key(1,true,true);
    Tick(.016f);assert(g_base[screen+0x18]);Finish();
    // A fresh press during an existing fade cannot be queued for its successor.
    intro_gate.Key(1,false,true);Ready();intro_gate.Key(1,true,true);Tick(.016f);
    intro_gate.Key(1,false,true);intro_gate.Key(1,true,true);Finish();Ready("EidosLogo");
    Tick(.016f);assert(!g_base[screen+0x18]);
    // EOF still closes naturally and discards an unconsumed pending tap.
    intro_gate={};Ready();g_base[screen+0x14]=0;Physical(3,true);eof=true;
    assert(Tick(.016f)==0x2723b0&&!intro_gate.owner&&intro_gate.OwnsPad(1));
    Ready("RockLogo");Tick(.016f);assert(!g_base[screen+0x18]);Physical(3,false);
    // Focus loss requires neutral input before accepting a new physical press.
    intro_gate.FocusLost();intro_gate.Key(1,true,false);intro_gate.Key(1,true,true);
    Tick(.016f);assert(!g_base[screen+0x18]);intro_gate.Key(1,false,true);
    intro_gate.Key(1,true,true);Tick(.016f);assert(g_base[screen+0x18]);Finish();
}
static void Guards(){
    intro_gate={};original_held=false;Ready();
    X86Context ctx{};ctx.esi=screen;ctx.esp=stack;ctx.eax=0x12345678;
    W32(stack,0x272c5c);W32(stack+4,0);W32(stack+8,0x16);
    intro_gate.Key(1,true,true);const auto saved=ctx;
    assert(TrialIntroQuery(ctx,g_base)&&ctx.esp==stack+4&&ctx.eax==0x12345601);
    ctx=saved;assert(TrialIntroQuery(ctx,g_base)&&ctx.eax==0x12345600);
    for(unsigned variant=0;variant<8;++variant){
        Seed();ctx=saved;mem::denied=0xffffffff;W32(stack,0x272c5c);W32(stack+4,0);W32(stack+8,0x16);
        switch(variant){
            case 0:W32(stack,0x276702);break; // separate Press Start widget
            case 1:W32(stack+8,0x1d);break;
            case 2:W32(stack+4,4);break;
            case 3:W32(screen,0x489024);break;
            case 4:g_base[screen+0x15]=1;break;
            case 5:std::strcpy(reinterpret_cast<char*>(g_base+screen+0x24),"intro_1");break;
            case 6:mem::denied=screen;break;
            case 7:ctx.esp=0xfffffff8;break;
        }
        const auto before=ctx;assert(!TrialIntroQuery(ctx,g_base));assert(!std::memcmp(&ctx,&before,sizeof(ctx)));
    }
    mem::denied=0xffffffff;
    // Unqualified gameplay movie/title inputs still invoke the actual original query.
    for(const char* name:{"intro_1","gamelogo","splash","fe_bk1"}){
        intro_gate={};original_held=false;Ready(name);assert(!intro_gate.owner);
        const unsigned before=original_queries;original_held=true;Tick(.016f);
        assert(original_queries>before&&g_base[screen+0x18]);Finish();
    }
    // One-port retail screens cannot consume another controller's edge.
    intro_gate={};Ready();g_base[screen+0x14]=0;
    Physical(3,true);Tick(.016f);assert(!g_base[screen+0x18]);
    Physical(0,true);Tick(.016f);assert(g_base[screen+0x18]);Finish();
    // Immediate-skip flag and original alternative successor retain their ABI.
    intro_gate={};Ready("dolby",0);Physical(0,true);
    assert(Tick(.016f)==0x2723b0&&g_base[screen+0x17]&&!intro_gate.owner);
}
int main(){MenuTapHandoff();Lifecycle();Guards();std::puts("intro skip: original delays/fades/EOF/ABI, all startup movies, keyboard/mouse/four pads and menu tap/release handoff passed");}
