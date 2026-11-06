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
bool menu_right_pending=false;
bool focused(){return test_focused;}
bool TrialGraphicsOpen(){return test_graphics;}
trial_intro::TitleGate title_gate;
unsigned title_finishes=0;
}
void TrialTitleContext(bool eligible){title_gate.Context(eligible&&test_focused&&!test_graphics);}
bool TrialTitleTake(){return title_gate.Take()&&test_focused&&!test_graphics;}
void TrialGraphicsInputChanged(){++title_finishes;menu_key_taps.Clear();}
// GENERATED_HOST
#include "../common/xbe_intro_skip.cpp"
#ifdef UC_TEST_PREVIOUS_STARTUP
// Link the saved producer against this same generated original-factory fixture.
bool TrialStartupProfile(X86Context&,uint8_t*){return false;}
#endif

static constexpr uint32_t screen=0x600000,stack=0x700000;
static unsigned opens,closes,draws,original_queries;
static bool eof,original_held;
static constexpr uint32_t controller=0x603000,title_root=0x604000,profile_root=0x605000;
static unsigned bank_calls,controller_resets,successors,title_parts,title_widgets;
static unsigned profile_checks,profile_builds,alternate_builds,profile_refreshes;
static unsigned title_presentations,profile_presentations,movie_deletes;
static std::array<unsigned,64> factory_events{};
static unsigned factory_event_count;
static uint32_t allocation=0x610000;
static bool controller_valid,profile_available=true,transition_ready=true;
static void FactoryEvent(unsigned event){assert(factory_event_count<factory_events.size());factory_events[factory_event_count++]=event;}
X86_FUNC(sub_002717B0);
X86_FUNC(sub_00271C00);
X86_FUNC(sub_00271E80);
X86_FUNC(sub_00271810);
X86_FUNC(sub_0027CA20);
X86_FUNC(sub_002845B0);
X86_FUNC(sub_00064E30);
static void FixturePair(X86Context& ctx,uint32_t object,uint32_t kind=1){
    const auto out=R32(ctx.esp+4);W32(out,object);W32(out+4,kind);ctx.eax=out;ctx.esp+=4;
}
static void TitlePart(X86Context& ctx){++title_parts;FixturePair(ctx,title_root);}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t address){
    switch(address){
    case 0x271e80:sub_00271E80(ctx,base);return;
    case 0x271810:sub_00271810(ctx,base);return;
    case 0x27ca20:sub_0027CA20(ctx,base);return;
    case 0x2831a0:++alternate_builds;FactoryEvent(6);FixturePair(ctx,profile_root,7);return;
    case 0xf0002:ctx.eax=controller_valid;ctx.esp+=4;return;
    case 0xf0003:++title_presentations;ctx.eax=0;ctx.esp+=12;return;
    case 0xf0004:++profile_presentations;ctx.eax=0;ctx.esp+=12;return;
    case 0xf0005:{
        // The real outgoing movie update and original transition-completion byte.
        const uint32_t context=R32(ctx.esp+8);g_base[R32(context+0x10)]=transition_ready;
        extern void FixtureMovie(X86Context&,uint8_t*);FixtureMovie(ctx,base);return;
    }
    case 0xf0006:{
        ++movie_deletes;extern void FixtureMovieDestroy(X86Context&,uint8_t*);
        FixtureMovieDestroy(ctx,base);return;
    }
    }
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
void FixtureMovie(X86Context& ctx,uint8_t* base){__imp__sub_00272BB0(ctx,base);}
// GENERATED_DESTROY
void FixtureMovieDestroy(X86Context& ctx,uint8_t* base){__imp__sub_00272FE0(ctx,base);ctx.esp+=4;}
X86_FUNC(sub_002745F0){ctx.eax=original_held;ctx.esp+=4;}
X86_FUNC(sub_00058000){std::abort();}
X86_FUNC(sub_00057FC0){ctx.esp+=4;}
// GENERATED_TITLE

X86_FUNC(sub_00208980){
    assert(R32(ctx.esp+4)==0x485f00&&R32(ctx.esp+8)==0);++bank_calls;FactoryEvent(1);
    ctx.eax=0x601600;ctx.esp+=12;
}
X86_FUNC(sub_00258800){TitlePart(ctx);}
X86_FUNC(sub_00273030){TitlePart(ctx);}
X86_FUNC(sub_00273700){TitlePart(ctx);}
X86_FUNC(sub_00266EE0){TitlePart(ctx);}
X86_FUNC(sub_00258B60){TitlePart(ctx);}
X86_FUNC(sub_00266EA0){TitlePart(ctx);}
X86_FUNC(sub_00284860){TitlePart(ctx);}
X86_FUNC(sub_00259CD0){TitlePart(ctx);}
X86_FUNC(sub_002D57FB){
    assert(R32(ctx.esp+4)==0x20);
    if(R32(ctx.esp+20)==0x2717b0)++title_widgets;
    ctx.eax=allocation;allocation+=0x40;ctx.esp+=4;
}
X86_FUNC(sub_0027E2B0){ctx.esp+=4;}
X86_FUNC(sub_00258430){++profile_refreshes;ctx.esp+=4;}
X86_FUNC(sub_00064A50){
    assert(R32(ctx.esp+4)==0x51d498);++profile_checks;FactoryEvent(4);
    W32(0x51d498,0);ctx.eax=profile_available;ctx.esp+=4;
}
X86_FUNC(sub_00285B80){
    assert(R32(ctx.esp+8)==0x271810&&R32(ctx.esp+12)==0x284490);
    ++profile_builds;FactoryEvent(5);FixturePair(ctx,profile_root,7);
}
X86_FUNC(sub_002743B0){ctx.esp+=4;}
X86_FUNC(sub_002743D0){ctx.esp+=4;}
// GENERATED_FACTORIES

static void Seed(const char* name="dolby",unsigned flags=4){
    std::memset(g_base+screen,0,0x48);W32(screen,0x489020);
    W32(screen+4,0x2723b0);G<float>(screen+8)=2;W32(screen+12,flags);
    g_base[screen+0x14]=1;std::strcpy(reinterpret_cast<char*>(g_base+screen+0x24),name);
    W32(0x5096e0,0x601000);W32(0x601000+0x28,0x601100);g_base[0x601100+0x3b]=15;
    W32(0x51c4dc,0x601200);W32(0x601200,0x601300);W32(0x601300+8,0xf0001);
    // GENERATED_CONSTANTS
    W32(0x4e836c,2);
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
    if(source<2&&down)intro_gate.Advance(.65f);
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
    // A quick tap is cancelled; holding must complete before release.
    intro_gate={};Ready();intro_gate.Key(2,true,true);intro_gate.Key(2,false,true);
    Tick(.016f);assert(!g_base[screen+0x18]);Physical(1,true);Tick(.016f);assert(g_base[screen+0x18]);Finish();Ready("EidosLogo");
    for(unsigned i=0;i<10;++i)Tick(.016f);
    assert(!g_base[screen+0x18]);
    // No input before or exactly at the retail minimum-delay boundary is queued.
    intro_gate={};Seed();Tick(0);intro_gate.Key(1,true,true);Tick(2);Tick(.001f);
    assert(!g_base[screen+0x18]);intro_gate.Key(1,false,true);intro_gate.Key(1,true,true);intro_gate.Advance(.65f);
    Tick(.016f);assert(g_base[screen+0x18]);Finish();
    // A fresh press during an existing fade cannot be queued for its successor.
    intro_gate.Key(1,false,true);Ready();intro_gate.Key(1,true,true);intro_gate.Advance(.65f);Tick(.016f);
    intro_gate.Key(1,false,true);intro_gate.Key(1,true,true);Finish();Ready("EidosLogo");
    Tick(.016f);assert(!g_base[screen+0x18]);
    // EOF still closes naturally and discards an unconsumed pending tap.
    intro_gate={};Ready();g_base[screen+0x14]=0;Physical(3,true);eof=true;
    assert(Tick(.016f)==0x2723b0&&!intro_gate.owner&&intro_gate.OwnsPad(1));
    Ready("RockLogo");Tick(.016f);assert(!g_base[screen+0x18]);Physical(3,false);
    // Focus loss requires neutral input before accepting a new physical press.
    intro_gate.FocusLost();intro_gate.Key(1,true,false);intro_gate.Key(1,true,true);
    Tick(.016f);assert(!g_base[screen+0x18]);intro_gate.Key(1,false,true);
    intro_gate.Key(1,true,true);intro_gate.Advance(.65f);Tick(.016f);assert(g_base[screen+0x18]);Finish();
}
static void Guards(){
    intro_gate={};original_held=false;Ready();
    X86Context ctx{};ctx.esi=screen;ctx.esp=stack;ctx.eax=0x12345678;
    W32(stack,0x272c5c);W32(stack+4,0);W32(stack+8,0x16);
    intro_gate.Key(1,true,true);intro_gate.Advance(.65f);const auto saved=ctx;
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
            case 5:std::strcpy(reinterpret_cast<char*>(g_base+screen+0x24),"intro_7");break;
            case 6:mem::denied=screen;break;
            case 7:ctx.esp=0xfffffff8;break;
        }
        const auto before=ctx;assert(!TrialIntroQuery(ctx,g_base));assert(!std::memcmp(&ctx,&before,sizeof(ctx)));
    }
    mem::denied=0xffffffff;
    // Unqualified gameplay movie/title inputs still invoke the actual original query.
    for(const char* name:{"intro_7","gamelogo","splash","fe_bk1"}){
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
static void HoldChecks(){
    for(const char* name:{"dolby","intro_1","intro_13"}){
        intro_gate={};Ready(name);intro_gate.Key(2,true,true);
        Tick(.3f);assert(!g_base[screen+0x18]&&TrialIntroProgress()>0&&TrialIntroProgress()<1);
        intro_gate.Key(2,false,true);Tick(.4f);assert(!g_base[screen+0x18]&&TrialIntroProgress()==0);
        intro_gate.Key(2,true,true);Tick(.3f);intro_gate.FocusLost();Tick(.4f);
        assert(!g_base[screen+0x18]&&TrialIntroProgress()==0);
        intro_gate.Key(2,false,true);intro_gate.Key(2,true,true);Tick(.3f);Tick(.36f);
        assert(g_base[screen+0x18]&&TrialIntroProgress()==0);Finish();
        Ready("EidosLogo");Tick(1);assert(!g_base[screen+0x18]);
    }
}
static void Title(){
    Seed();original_held=false;title_gate={};
    W32(screen,0x489974);W32(screen+4,0x1d);W32(screen+8,0x2717b0);
    W32(screen+12,0);W32(screen+16,0);g_base[screen+0x14]=1;G<float>(screen+0x1c)=1;
    auto tick=[](){
        X86Context ctx{};ctx.ecx=screen;ctx.esp=stack;ctx.ebx=0x12345678;ctx.edi=0x87654321;
        W32(stack,0xabcdef01);G<float>(stack+4)=1;W32(stack+8,0);
        __imp__sub_00276690(ctx,g_base);
        assert(ctx.esp==stack+12&&ctx.ebx==0x12345678&&ctx.edi==0x87654321);
        return ctx.eax;
    };
    title_gate.Press(true,false);assert(tick()==0); // No press queued before title readiness.
    assert(tick()==0);title_gate.Press(true,true);assert(tick()==0); // Held-key repeat.
    title_gate.Press(false,false);assert(tick()==0);
    title_gate.Press(true,false);assert(tick()==0x2717b0&&R32(0x51d2f4)==0&&title_finishes==1);
    assert(tick()==0); // Consumed event cannot start twice.
    title_gate.Press(true,false);test_focused=false;assert(tick()==0);test_focused=true;
    W32(screen+8,0x284580);title_gate.Press(true,false);assert(tick()==0); // Other screen.
    W32(screen+8,0x2717b0);original_held=true;
    assert(tick()==0x2717b0&&title_finishes==2); // Retail controller action still works.
}
static void Startup(){
    assert(!StartupOption(nullptr));
    for(const char* value:{"","0","true","yes","01","1 "," 1","10","-1"})assert(!StartupOption(value));
    assert(StartupOption("1"));
    startup_pending=true;intro_gate={};title_gate={};original_held=false;
    opens=closes=draws=original_queries=title_finishes=0;
    for(auto& owner:skipped_startup)owner=0;
    constexpr const char* names[]={"dolby","EidosLogo","RockLogo","intro_12_edit"};
    constexpr uint32_t next[]={0x2723b0,0x272380,0x2722f0,0x271e80};
    intro_gate.Key(1,true,true);std::array<uint8_t,8> analog{};intro_gate.Pad(1,16,analog,true);
    for(unsigned i=0;i<4;++i){
        Seed(names[i]);W32(screen+4,next[i]);W32(0x5154bc,0x12345678);W32(0x519e20,0x87654321);
        menu_key_taps.Event('F',true,false,true);menu_key_taps.Event('F',false,false,true);click_pending=true;
        std::array<uint8_t,0x48> before;std::memcpy(before.data(),g_base+screen,before.size());
        assert(Tick(0)==next[i]&&!std::memcmp(before.data(),g_base+screen,before.size()));
        assert(!opens&&!closes&&!draws&&!original_queries&&!intro_gate.owner&&!click_pending);
        assert(intro_gate.OwnsPad(0)&&intro_gate.OwnsPad(1));
        menu_key_taps.Begin();assert(!menu_key_taps.Retained('F'));menu_key_taps.End();
        X86Context c{};c.ecx=screen;c.esp=stack;c.esi=0x11223344;c.fs_base=0x602000;
        W32(stack,0xabcdef01);W32(stack+4,0x13572468);W32(c.fs_base,0x24681357);
        __imp__sub_00272FE0(c,g_base);
        assert(c.esp==stack+4&&c.esi==0x11223344&&R32(c.fs_base)==0x24681357);
        assert(R32(stack)==0xabcdef01&&R32(stack+4)==0x13572468&&R32(screen)==0x450708);
        assert(!closes&&!skipped_startup[i]&&R32(0x5154bc)==0x12345678&&R32(0x519e20)==0x87654321);
    }
    // Strict movie identity and state guards leave the guest and input untouched.
    for(unsigned variant=0;variant<13;++variant){
        Seed();X86Context c{};c.ecx=screen;c.esp=stack;c.eax=0x12345678;
        mem::denied=0xffffffff;
        switch(variant){
        case 0:W32(screen+4,0x271e80);break;
        case 1:g_base[screen+0x15]=1;break;
        case 2:g_base[screen+0x16]=1;break;
        case 3:g_base[screen+0x17]=1;break;
        case 4:g_base[screen+0x18]=1;break;
        case 5:W32(screen+0x10,1);break;
        case 6:W32(screen+0x44,0x271e80);break;
        case 7:W32(screen,0x489024);break;
        case 8:std::strcpy(reinterpret_cast<char*>(g_base+screen+0x24),"intro_1");break;
        case 9:mem::denied=screen;break;
        case 10:c.esp=0xfffffff8;break;
        case 11:mem::denied=stack;break;
        }
        const auto saved=c;const auto gate=intro_gate;
        assert(!TrialStartupMovie(c,variant==12?nullptr:g_base));
        assert(!std::memcmp(&c,&saved,sizeof(c))&&!std::memcmp(&gate,&intro_gate,sizeof(gate)));
    }
    mem::denied=0xffffffff;
    for(const char* name:{"intro_1","intro_13","fe_bk1","gamelogo"}){Seed(name);assert(Tick(0)==0&&g_base[screen+0x16]);}
    // Only a skipped instance omits close; normal/opened movies keep retail cleanup.
    Seed();X86Context destroy{};destroy.ecx=screen;destroy.esp=stack;destroy.fs_base=0x602000;
    const unsigned before_close=closes;__imp__sub_00272FE0(destroy,g_base);assert(closes==before_close+1);
    intro_gate={};Seed();assert(Tick(0)==0x2723b0);g_base[screen+0x16]=1;
    destroy.ecx=screen;destroy.esp=stack;__imp__sub_00272FE0(destroy,g_base);
    assert(closes==before_close+2&&!skipped_startup[0]);
    startup_pending=false;
    Seed();assert(Tick(0)==0&&g_base[screen+0x16]); // Later matching movies play normally.
    std::puts("startup movies: strict opt-in, exact unopened chain and original destructor/stack guards passed");
}

static void FactorySeed(bool enabled=true){
    std::memset(g_base+0x601000,0,0x800);std::memset(g_base+controller,0,0x28);
    Seed();startup_pending=enabled;intro_gate={};title_gate={};menu_key_taps.Clear();
    for(auto& owner:skipped_startup)owner=0;
    opens=closes=draws=original_queries=title_finishes=0;
    bank_calls=controller_resets=successors=title_parts=title_widgets=0;
    profile_checks=profile_builds=alternate_builds=profile_refreshes=0;
    title_presentations=profile_presentations=movie_deletes=factory_event_count=0;
    controller_valid=false;profile_available=true;transition_ready=true;allocation=0x610000;
    W32(0x5150b4,0x601400);W32(0x601400+0x144,0);
    W32(0x601500,0x601580);W32(0x601580+0xec,0xf0002);
    W32(title_root,0x604100);W32(0x604104,0xf0003);
    W32(profile_root,0x605100);W32(0x605104,0xf0004);
    W32(0x489020,0xf0006);W32(0x489024,0xf0005);
    W32(0x51d2f4,3);g_base[0x51d5a4]=1;g_base[0x51d5a5]=0;g_base[0x5010e8]=0;
    g_base[0x601018]=9;g_base[0x51d295]=1;W32(0x4e8358,1);
    // GENERATED_FACTORY_DATA
}
static void ArmProfile(){
    Seed("intro_12_edit");W32(screen+4,0x271e80);
    assert(Tick(0)==0x271e80&&!opens&&!draws);
}
static void Construct(uint32_t factory){
    X86Context c{};c.ecx=controller;c.esp=stack;c.fs_base=0x602000;
    c.ebx=0x12345678;c.esi=0x23456789;c.edi=0x3456789a;c.ebp=0x456789ab;
    W32(c.fs_base,0x87654321);W32(stack,0xabcdef01);W32(stack+4,factory);W32(stack+8,0x13572468);
    sub_002727E0(c,g_base);
    assert(c.eax==controller&&c.esp==stack+8&&c.ebx==0x12345678&&c.esi==0x23456789&&c.edi==0x3456789a&&c.ebp==0x456789ab);
    assert(R32(c.fs_base)==0x87654321&&R32(stack)==0xabcdef01&&R32(stack+8)==0x13572468);
    assert(R32(controller)==0x489008&&!R32(controller+0xc)&&R32(controller+0x10)==1&&!g_base[controller+0x14]);
}
static void ControllerTick(){
    X86Context c{};c.ecx=controller;c.esp=stack;c.fs_base=0x602000;
    c.ebx=0x12345678;c.esi=0x23456789;c.edi=0x3456789a;c.ebp=0x456789ab;
    W32(c.fs_base,0x87654321);W32(stack,0xabcdef01);G<float>(stack+4)=.016f;W32(stack+8,0x13572468);
    sub_00272870(c,g_base);
    assert(c.esp==stack+8&&c.ebx==0x12345678&&c.esi==0x23456789&&c.edi==0x3456789a&&c.ebp==0x456789ab);
    assert(R32(c.fs_base)==0x87654321&&R32(stack)==0xabcdef01&&R32(stack+8)==0x13572468);
}
static void DirectProfile(){
    // The first installed/presented tree is the original profile result. These
    // assertions fail against the saved old producer before any title update.
    for(unsigned device=0;device<3;++device)for(unsigned session=0;session<2;++session){
        FactorySeed();ArmProfile();g_base[0x601000]=uint8_t(session);
        if(device)W32(0x601400+0x144,0x601500);controller_valid=device==2;
        menu_right_pending=true;menu_key_taps.Event('F',true,false,true);menu_key_taps.Event('F',false,false,true);
        Construct(0x271e80);
        assert(title_parts==0&&title_widgets==0&&title_presentations==0);
        assert(bank_calls==1&&controller_resets==unsigned(device!=2)&&successors==1);
        assert(R32(controller+4)==profile_root&&R32(controller+8)==7);
        assert(!startup_pending&&title_finishes==1&&!menu_right_pending&&R32(0x51d2f4)==0);
        assert(!g_base[0x51d295]&&g_base[0x5010e8]==1&&g_base[0x601018]==1);
        assert(profile_checks==unsigned(!session)&&profile_builds==unsigned(!session)&&alternate_builds==session);
        assert(profile_refreshes==session&&g_base[0x5154bc]==unsigned(!session));
        unsigned at=0;assert(factory_events[at++]==1);
        if(device!=2)assert(factory_events[at++]==2);
        assert(factory_events[at++]==3);
        if(session)assert(factory_events[at++]==6);
        else{assert(factory_events[at++]==4);assert(factory_events[at++]==5);}
        assert(at==factory_event_count);
        menu_key_taps.Begin();assert(!menu_key_taps.Retained('F'));menu_key_taps.End();
        ControllerTick();assert(profile_presentations==1&&!title_presentations);
        // Back from profiles still constructs both authored Start/A widgets.
        Construct(0x271810);
        assert(title_parts>0&&title_widgets==2&&successors==1&&R32(controller+4)==title_root);
        ControllerTick();assert(title_presentations==1);
    }
    FactorySeed(false);Construct(0x271e80); // Disabled setting keeps retail startup.
    assert(title_widgets==2&&title_parts>0&&!successors&&!profile_builds&&!title_finishes);
    FactorySeed();ArmProfile();Construct(0x271810); // Armed, but wrong factory caller.
    assert(title_widgets==2&&title_parts>0&&!successors&&startup_pending&&!title_finishes);
    // Original storage admission can produce its warning instead of profiles.
    FactorySeed();ArmProfile();profile_available=false;Construct(0x271e80);
    assert(profile_checks==1&&!profile_builds&&profile_refreshes==1&&successors==1&&!title_widgets);
    assert(g_base[0x51d5a5]==1); // Original one-time storage check state retained.
    std::puts("direct profile: original init/order, output-pair, storage branches, zero title creation, disabled/later title passed");
}
static void StartupTransition(){
    FactorySeed();Seed("intro_12_edit");W32(screen+4,0x271e80);
    std::array<uint8_t,0x48> before;std::memcpy(before.data(),g_base+screen,before.size());
    for(unsigned state:{0u,1u,4u}){
        W32(0x4e836c,state);
        assert(Tick(.016f)==0&&startup_pending&&!opens&&!closes&&!draws);
        assert(!std::memcmp(before.data(),g_base+screen,before.size()));
    }
    mem::denied=0x4e8000;assert(Tick(0)==0);mem::denied=0xffffffff;
    W32(0x4e836c,3);
    W32(controller,0x489008);W32(controller+4,screen);W32(controller+8,1);W32(controller+0x10,1);
    transition_ready=false;ControllerTick();
    assert(!startup_pending&&successors==1&&R32(controller+4)==screen&&R32(controller+0xc)==profile_root);
    assert(g_base[controller+0x14]&&!title_parts&&!title_widgets&&!opens&&!closes&&!movie_deletes);
    for(unsigned frame=0;frame<5;++frame)ControllerTick();
    assert(successors==1&&!opens&&!closes&&!draws&&R32(controller+4)==screen);
    // A reopened or mutated instance cannot inherit the outgoing-movie grace.
    for(unsigned variant=0;variant<3;++variant){
        X86Context c{};c.ecx=screen;c.esp=stack;
        if(variant==0)g_base[screen+0x16]=1;
        if(variant==1)W32(screen+4,0x2723b0);
        if(variant==2)W32(screen,0x489024);
        const auto saved=c;assert(!TrialStartupMovie(c,g_base)&&!std::memcmp(&c,&saved,sizeof(c)));
        std::memcpy(g_base+screen,before.data(),before.size());
    }
    transition_ready=true;ControllerTick();
    assert(movie_deletes==1&&!closes&&!skipped_startup[3]&&R32(controller+4)==profile_root&&R32(controller+8)==7);
    ControllerTick();assert(profile_presentations==1&&!title_presentations&&!opens);
    Seed("intro_12_edit");W32(screen+4,0x271e80);assert(Tick(0)==0&&opens==1); // Reused address is a new movie.
    std::puts("startup transition: original readiness gate, pending/current lifecycle and exact outgoing-instance cleanup passed");
}
int main(int argc,char** argv){
    assert(startup_pending==StartupOption(std::getenv("UCRR_SKIP_STARTUP")));startup_pending=false;
    if(argc==2&&!std::strcmp(argv[1],"--direct-profile")){DirectProfile();return 0;}
    if(argc==2&&!std::strcmp(argv[1],"--startup-transition")){StartupTransition();return 0;}
    MenuTapHandoff();Lifecycle();Guards();HoldChecks();Title();Startup();
    DirectProfile();StartupTransition();
    std::puts("intro/title: original delays/fades/ABI, fresh title presses, controller action and menu handoff passed");
}
