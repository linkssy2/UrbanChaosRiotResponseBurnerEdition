#undef NDEBUG
#include <cassert>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <mutex>
#include "cpu/x86_ops.h"
#include "xbe_frontend.h"
using std::isfinite;
static uint8_t storage[0x800000],*ram=storage;
static X86Context context,*regs=&context;
size_t g_xbox_total_ram=sizeof(storage);
#define g_ecx (regs->ecx)
#define g_esi (regs->esi)
#define g_edi (regs->edi)
#define g_ebx (regs->ebx)
#define g_esp (regs->esp)
#define MEM8(a) (ram[uint32_t(a)])
#define MEM32(a) (*(uint32_t*)(ram+uint32_t(a)))
#define MEMF(a) (*(float*)(ram+uint32_t(a)))
static bool TrialGuestReadable(uint32_t p,uint32_t n){return p>=0x10000&&p<sizeof(storage)&&n<=sizeof(storage)-p;}
static bool FrontendGuestOwns(uint32_t,uint32_t);
static TrialFrontendFrame published;
static TrialFrontendPresentation presentation;
static bool name_keyboard_mode=true;
int xbox_KeyboardMode(){return name_keyboard_mode;}
bool TrialFrontendPresents(uint32_t owner,uint32_t kind){return presentation.Presents(owner,kind);}
static UcHybridFrontendAction queued;
static TrialFrontendNameActionData name_queued;
bool TrialFrontendTakeName(TrialFrontendNameActionData& a){if(!name_queued.identity.kind)return false;a=name_queued;name_queued={};return true;}
static unsigned pointer_reads;
int ucrr_host_menu_pointer(float*,float*,int*){++pointer_reads;return 0;}
void ucrr_host_menu_mode(int){}
uint64_t ucrr_host_menu_milliseconds(){return 1000;}
void TrialFrontendPublish(const TrialFrontendFrame& f){
    published=f;
    presentation.Publish(f,TrialFrontendSupported(f)&&
        (f.delete_dialog||name_keyboard_mode));
}
bool TrialFrontendTakeAction(UcHybridFrontendAction& a){if(!queued.kind)return false;a=queued;queued={};return true;}
bool TrialFrontendOwns(uint32_t p,uint32_t k){return (published.delete_dialog||name_keyboard_mode)&&TrialFrontendSupported(published)&&published.source.owner==p&&published.source.kind==k;}
void TrialGraphicsMenuContext(uint32_t,uint32_t,uint32_t,uint32_t){}
struct Draw {uint32_t function,texture,color;float x,y,right,bottom;};
static std::vector<Draw> draws;
static std::vector<Draw> sidebar_quads;
static bool sidebar_draw_path;
X86_EXTERN_FUNC(__imp__sub_000656D0);
X86_EXTERN_FUNC(__imp__sub_00065550);
X86_EXTERN_FUNC(__imp__sub_00065170);
X86_EXTERN_FUNC(__imp__sub_002858A0);
X86_EXTERN_FUNC(__imp__sub_002830A0);
static unsigned name_validations,name_sounds,name_frees;
static unsigned profile_hover_sounds,profile_back_sounds;
static bool profile_child_active;
static bool delete_fixture;
static bool pause_fixture;
static unsigned pause_texts,pause_separators,pause_backdrops;
static unsigned deleted_profiles,deleted_index,profile_rebuilds;
X86_EXTERN_FUNC(__imp__sub_0025A310);
X86_EXTERN_FUNC(__imp__sub_00258BC0);
X86_EXTERN_FUNC(__imp__sub_00258D40);
X86_EXTERN_FUNC(__imp__sub_00258DF0);
X86_EXTERN_FUNC(__imp__sub_00284580);
void x86_resume(X86Context&,uint8_t*,const X86Resume&){assert(false);}
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t function,std::initializer_list<uint32_t> args){
    std::vector<uint32_t>a(args);
    if(function==0x2858A0){
        assert(a.size()==3);const uint32_t stack=c.esp;
        for(auto i=a.rbegin();i!=a.rend();++i){c.esp-=4;MEM32(c.esp)=*i;}
        c.esp-=4;MEM32(c.esp)=0xFEEDFACE;
        __imp__sub_002858A0(c,ram);assert(c.esp==stack);return c.eax;
    }
    if(function==0x17B30){assert(c.ecx==0x604000);assert(a.size()==1);return 0x605000;}
    assert(function==0x65760||function==0x656D0);
    const unsigned offset=function==0x65760?1:0;
    assert(a.size()==(offset?5:3));
    if(offset)assert(a[4]==0);
    draws.push_back({function,offset?a[0]:0,MEM32(a[offset+2]),MEMF(a[offset]),MEMF(a[offset]+4),MEMF(a[offset+1]),MEMF(a[offset+1]+4)});
    if(function==0x656D0){
        // Execute the production rectangle path, including material lookup,
        // corner rounding and the original half-pixel transform.
        const uint32_t stack=c.esp;
        for(auto i=a.rbegin();i!=a.rend();++i){c.esp-=4;MEM32(c.esp)=*i;}
        c.esp-=4;MEM32(c.esp)=0xFEEDFACE;
        sidebar_draw_path=true;__imp__sub_000656D0(c,ram);sidebar_draw_path=false;
        assert(c.esp==stack-12);c.esp=stack;
    }
    // Production helpers must restore every register, including temporary ESP.
    c.eax=0xBAD;c.esp-=40;c.ecx=0xBAD;c.xmm[2].u[1]=0xBAD;
    return 0;
}
}
X86_FUNC(sub_00387C82){
    const uint32_t source=X86_MEM32(ctx.esp+4);assert(std::strlen((char*)base+source)<=15);
    std::strcpy((char*)base+0x650000,(char*)base+source);ctx.eax=0x650000;ctx.esp+=4;
}
X86_FUNC(sub_00057FC0){
    const uint32_t sound=X86_MEM32(ctx.esp+4);
    if(sound==0x450958)++name_sounds;
    else if(sound==0x450960)++profile_hover_sounds;
    else {assert(sound==0x4509A8);++profile_back_sounds;}
    ctx.esp+=4;
}
X86_FUNC(sub_002D73A4){assert(X86_MEM32(ctx.esp+4)==0x650000);++name_frees;ctx.esp+=4;}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target){
    if(target==0x660400){assert(ctx.ecx==0x660000);ctx.eax=7;ctx.esp+=8;return;}
    if(target==0x660404){assert(ctx.ecx==0x660000);ctx.eax=0x660200;ctx.esp+=4;return;}
    if(target==0x660408){
        assert(ctx.ecx==0x660200&&X86_MEM32(ctx.esp+16)==7);
        assert(!X86_MEM32(ctx.esp+28)&&!X86_MEM32(ctx.esp+32));
        const uint32_t p=X86_MEM32(ctx.esp+4),q=X86_MEM32(ctx.esp+8);
        sidebar_quads.push_back({0x65170,7,X86_MEM32(ctx.esp+12),
            X86_MEMF32(p),X86_MEMF32(p+4),X86_MEMF32(q),X86_MEMF32(q+4)});
        ctx.esp+=36;return;
    }
    if(target==0x641200){ctx.eax=profile_child_active;ctx.esp+=4;return;}
    if(target==0x641208){
        assert(delete_fixture&&ctx.ecx==0x641000&&X86_MEM32(ctx.esp+4)==0x646000&&X86_MEM32(ctx.esp+8)==1);
        profile_child_active=true;ctx.esp+=12;return;
    }
    if(target==0x641204){
        if(delete_fixture){ctx.ecx=0x646000;__imp__sub_0025A310(ctx,base);return;}
        ctx.eax=0;ctx.esp+=12;return;
    }
    assert(target==0x2830A0);++name_validations;__imp__sub_002830A0(ctx,base);
}
// ORIGINAL_NAME_VALIDATION
#include "menu_pointer_guest.h"
bool TrialControlsProfile(uint32_t*,bool){return false;}
#include "xbe_graphics_menu_context.h"
#include "xbe_frontend_guest.inc"

// The resolved source text is a fixture boundary; both real presentation
// wrappers execute their original stack/argument/glyph-sink dispatch paths.
static std::string warning_sink;
static uint32_t warning_key;
X86_FUNC(sub_00017B30){
    assert(ctx.ecx==0x604000&&X86_MEM32(ctx.esp+4)==warning_key);
    ctx.eax=0x605000;ctx.esp+=8;
}
X86_FUNC(sub_000171A0){
    assert(ctx.ecx==0x604000);warning_sink=(char*)base+X86_MEM32(ctx.esp+4);
    ctx.esp+=16;
}
X86_FUNC(sub_00018A70){
    assert(ctx.ecx==0x604000);warning_sink=(char*)base+X86_MEM32(ctx.esp+4);
    ctx.esp+=28;
}
// Prompt metadata has its own production fixture; retain the composable scope
// while this fixture exercises profile warning text and original stack paths.
struct TrialPromptTextScope{
    TrialPromptTextScope(uint8_t*,uint32_t,uint32_t,uint32_t=0,uint32_t=0){}
};
// ORIGINAL_PROFILE_WARNING_PRESENTATION
static void CheckProfileWarnings(){
    struct WarningCase{const char* key;const char* source;const char* expected;};
    // ORIGINAL_PROFILE_WARNING_CASES
    auto present=[](const char* key,const char* source,const char* expected){
        warning_key=0x606000;
        std::strcpy((char*)ram+warning_key,key);std::strcpy((char*)ram+0x605200,source);
        MEM8(0x605031)=1;MEM32(0x605034)=0x605100;MEM32(0x605100)=0x605200;
        for(bool draw:{false,true}){
            context={};context.esp=0x700000;context.ecx=0x604000;
            context.esi=0x12345678;context.edi=0x87654321;context.ebx=0xCAFEBABE;
            const std::vector<uint32_t> args=draw?
                std::vector<uint32_t>{warning_key,0x42C80000,0x43480000,0x40,0,0,0}:
                std::vector<uint32_t>{warning_key,0x3F800000,0x40,0};
            for(auto i=args.rbegin();i!=args.rend();++i){context.esp-=4;MEM32(context.esp)=*i;}
            context.esp-=4;MEM32(context.esp)=0xFEEDFACE;
            warning_sink.clear();
            if(draw)__imp__sub_00018CE0(context,ram);else __imp__sub_00017E40(context,ram);
            assert(warning_sink==expected&&context.esp==0x700000);
            assert(context.esi==0x12345678&&context.edi==0x87654321);
            if(draw)assert(context.ebx==0xCAFEBABE);
            assert(!std::strcmp((char*)ram+warning_key,key));
            assert(!std::strcmp((char*)ram+0x605200,source));
        }
    };
    for(const auto& c:warning_cases)present(c.key,c.source,c.expected);
    for(const char* key:{"newfrontend-pressstart-1","NewFrontEnd-PRESSSTART-2"}){
        present(key,"PRESS START BUTTON","PRESS ANY KEY");
        present(key,"APPUYER SUR START","APPUYEZ SUR UNE TOUCHE");
        present(key,"PREMI START","PREMI UN TASTO");
        present(key,"START DR\xDC" "CKEN","BELIEBIGE TASTE DR\xDC" "CKEN");
        name_keyboard_mode=false;present(key,"PRESS START BUTTON","PRESS START BUTTON");
        name_keyboard_mode=true;
    }
    present("XBoxTRC-MemCard-1",warning_cases[0].source,warning_cases[0].expected);
    // Unrelated Xbox references, neighboring records and custom wording stay
    // untouched. Missing storage/undersized scratch falls back to the source.
    for(const char* key:{"xboxtrc-liveerror-1","xboxtrc-memcard-10","legal-text-9"})
        present(key,warning_cases[0].source,warning_cases[0].source);
    present("xboxtrc-memcard-1","Custom profile message","Custom profile message");
    std::strcpy((char*)ram+warning_key,warning_cases[0].key);
    std::strcpy((char*)ram+0x605200,warning_cases[0].source);
    assert(TrialFrontendWarningText(ram,warning_key,0x605200,0x607000,4)==0x605200);
    assert(TrialFrontendWarningText(ram,warning_key,0x605200,0,512)==0x605200);
    assert(TrialFrontendWarningText(ram,0,0x605200,0x607000,512)==0x605200);
    assert(TrialFrontendWarningText(ram,warning_key,0,0x607000,512)==0);
    std::memset(ram+0x605200,'X',512);
    assert(TrialFrontendWarningText(ram,warning_key,0x605200,0x607000,512)==0x605200);
}

// Execute the actual original profile update, rectangle producer and draw
// helper. Only font/GPU sinks, child-state queries and details refresh are
// fixture boundaries; bounds, guest action branches and callbacks are real.
X86_EXTERN_FUNC(__imp__sub_002840C0);
X86_EXTERN_FUNC(__imp__sub_00284330);
X86_EXTERN_FUNC(__imp__sub_00259690);
static std::vector<Draw> profile_quads;
static unsigned profile_refreshes;
static float profile_post_fade;
int TrialGuestHook(X86Context& ctx,uint8_t* base,unsigned op){
    assert(&ctx==regs&&base==ram);
    switch(op){
    case 16:ucrr_menu_dialog_enter();return 0;
    case 17:ucrr_menu_dialog_hover();return 0;
    case 18:ucrr_menu_dialog_bounds();return 0;
    case 19:ucrr_menu_dialog_leave();return 0;
    case 20:ucrr_menu_profile_enter();return 0;
    case 21:return ucrr_menu_profile_hover();
    case 22:ucrr_menu_profile_bounds();return 0;
    case 23:ucrr_menu_profile_leave();return 0;
    default:assert(false);return 0;
    }
}
X86_FUNC(sub_00017060){ctx.esp+=8;}
X86_FUNC(sub_00017160){ctx.esp+=12;}
X86_FUNC(sub_00018CE0){if(pause_fixture){assert(!TrialFrontendHideListSprites());++pause_texts;}ctx.esp+=32;}
X86_FUNC(sub_00018DE0){if(pause_fixture){assert(!TrialFrontendHideListSprites());++pause_separators;}ctx.esp+=24;}
static void ProfileQuad(X86Context& ctx,uint8_t* base,uint32_t fn){
    const uint32_t p=X86_MEM32(ctx.esp+8),q=X86_MEM32(ctx.esp+12);
    profile_quads.push_back({fn,X86_MEM32(ctx.esp+4),0,X86_MEMF32(p),X86_MEMF32(p+4),X86_MEMF32(q),X86_MEMF32(q+4)});
    ctx.esp+=4;
}
X86_FUNC(sub_00065760){ProfileQuad(ctx,base,0x65760);}
X86_FUNC(sub_00065550){if(sidebar_draw_path)__imp__sub_00065550(ctx,base);else ProfileQuad(ctx,base,0x65550);}
X86_FUNC(sub_00065170){__imp__sub_00065170(ctx,base);}
// Material services and the standard floor routine are fixture boundaries;
// all rectangle/UV/color argument construction executes original instructions.
X86_FUNC(sub_0022B210){ctx.eax=0x660500;ctx.esp+=4;}
X86_FUNC(sub_002E0E00){assert(ctx.ecx==0x660500);ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_002A7B50){assert(X86_MEM32(ctx.esp+4)==0x452428);ctx.eax=ctx.ecx;ctx.esp+=12;}
X86_FUNC(sub_002D666E){x87_push(ctx,std::floor(X86_MEMF64(ctx.esp+4)));ctx.esp+=4;}
// ORIGINAL_SIDEBAR_DRAW
X86_FUNC(sub_00259690){__imp__sub_00259690(ctx,base);profile_post_fade=X86_MEMF32(ctx.esp+12);}
X86_FUNC(sub_00259790){ctx.esp+=4;}
X86_FUNC(sub_00284330){__imp__sub_00284330(ctx,base);}
X86_FUNC(sub_002745F0){ctx.eax=menu_special_query()?1:0;ctx.esp+=4;}
X86_FUNC(sub_00274300){ctx.esp+=4;}
X86_FUNC(sub_00274320){ctx.esp+=8;}
X86_FUNC(sub_002835E0){
    assert(ctx.ecx==0x640000&&X86_MEM32(ctx.esp+4)==MEM32(0x51D710));
    ++profile_refreshes;ctx.esp+=8;
}
// Only allocation, visual sinks and the final save service are intercepted.
// Profile update, original modal factories/constructor and Yes/No consumer run.
X86_FUNC(sub_000656D0){assert(delete_fixture||pause_fixture);if(pause_fixture)++pause_backdrops;ctx.esp+=4;}
X86_FUNC(sub_00017180){ctx.esp+=12;}
X86_FUNC(sub_00017EE0){ctx.eax=1;ctx.esp+=16;}
X86_FUNC(sub_000176D0){x87_push(ctx,20.);ctx.esp+=8;}
X86_FUNC(sub_00017E40){x87_push(ctx,40.);ctx.esp+=20;}
X86_FUNC(sub_000871A0){X86_MEM32(ctx.ecx)=0xffffffff;ctx.eax=ctx.ecx;ctx.esp+=8;}
X86_FUNC(sub_00086FD0){
    const uint32_t p=X86_MEM32(ctx.esp+4);for(unsigned i=0;i<4;++i)X86_MEMF32(p+4*i)=1;
    ctx.eax=p;ctx.esp+=8;
}
X86_FUNC(sub_002D57FB){assert((delete_fixture||pause_fixture)&&X86_MEM32(ctx.esp+4)==0x8C);ctx.eax=0x646000;ctx.esp+=4;}
X86_FUNC(sub_00253FF0){
    assert((delete_fixture||pause_fixture)&&X86_MEM32(ctx.esp+8)==0x646000&&X86_MEM32(ctx.esp+12)==1);
    const uint32_t p=X86_MEM32(ctx.esp+4);X86_MEM32(p)=0x646000;X86_MEM32(p+4)=1;
    ctx.eax=p;ctx.esp+=4;
}
X86_FUNC(sub_000641F0){assert(delete_fixture);++deleted_profiles;deleted_index=X86_MEM32(ctx.esp+4);ctx.esp+=4;}
X86_FUNC(sub_0027EA20){
    assert(delete_fixture&&X86_MEM32(ctx.esp+8)==0x284490&&X86_MEM32(ctx.esp+12)==1);
    ++profile_rebuilds;ctx.esp+=4;
}
X86_FUNC(sub_00258BC0){__imp__sub_00258BC0(ctx,base);}
X86_FUNC(sub_00258D40){__imp__sub_00258D40(ctx,base);}
X86_FUNC(sub_00258DF0){assert(delete_fixture||pause_fixture);__imp__sub_00258DF0(ctx,base);}
// ORIGINAL_PROFILE_PATH
// ORIGINAL_PROFILE_DELETE_PATH

namespace controller_fixture {
// The production pad function runs unchanged. Only its shared UI state and
// the panel-state query are supplied by this fixture; no ImGui input is mocked.
static std::mutex context_lock;
static std::atomic<bool> frontend_active=false;
static std::atomic<unsigned> frontend_events=0;
static std::atomic<unsigned> settings_page=0;
static std::atomic<uint64_t> navigation_generation=1;
static bool panel_open=false;
static bool PanelOpen(){return panel_open;}
static bool ControllerSupplement(){return false;} // Supplemental screens are exercised by test_menu_style.
// PRODUCTION_FRONTEND_PAD

static void Sample(unsigned expected,uint32_t buttons=0,uint8_t confirm=0,uint8_t back=0,int16_t x=0,int16_t y=0){
    TrialFrontendPad(buttons,confirm,back,x,y);
    assert(frontend_events.exchange(0)==expected);
}
static void Check(){
    // Inactive menus discard queued and held input. Entry requires neutral.
    frontend_events=15;Sample(0,1,255,255);
    frontend_active=true;Sample(0,1,255,255);Sample(0,1,255,255);
    Sample(0);Sample(4,0,255);Sample(0,0,255);Sample(0);
    Sample(8,0,0,255);Sample(0,0,0,255);Sample(0);
    // Start is still the original pause control, never host Confirm or Back.
    Sample(0,16);Sample(0,16);Sample(0);
    Sample(1,1);Sample(0,1);Sample(0);Sample(2,8);Sample(0);
    Sample(0,0,0,0,-16000,16000);Sample(1,0,0,0,-16001,0);Sample(0);
    Sample(2,0,0,0,16001,0);Sample(0);

    // Opening Graphics clears queued Confirm and requires release of the
    // activating gesture, even without a navigation-generation change.
    TrialFrontendPad(0,255,0,0,0);assert(frontend_events==4);
    panel_open=true;Sample(0,0,255);Sample(0,0,255,255);Sample(0);
    Sample(8,0,0,255);Sample(0,0,0,255);Sample(0);
    // Fresh A/directions belong to the settings widgets, not parent rows.
    Sample(21,1,255,0,-20000,20000);Sample(0);
    Sample(38,10,255,0,20000,-20000);Sample(0);
    Sample(0,16);Sample(0);Sample(0,0,0,30);Sample(8,0,0,31);
    Sample(0);Sample(29,1,255,255,-20000,0);Sample(0);

    // Controls owns distinct directions and confirmation for its binding
    // rows, pager and footer, with the same opening-input neutral gate.
    settings_page=1;++navigation_generation;
    Sample(0,0,255);Sample(0);Sample(4,0,255);Sample(0);
    Sample(1,1);Sample(0);Sample(2,2);Sample(0);
    Sample(16,4);Sample(0);Sample(32,8);Sample(0);
    Sample(8,0,0,255);Sample(0);
    settings_page=2;++navigation_generation;
    Sample(0,0,255);Sample(0);Sample(4,0,255);Sample(0);
    Sample(1,1);Sample(0);Sample(2,2);Sample(0);Sample(16,4);Sample(0);Sample(32,8);Sample(0);
    Sample(8,0,0,255);Sample(0);
    settings_page=0;++navigation_generation;Sample(0);

    // Close clears events and bumps this shared generation in production.
    // Held Back/A/direction must stay suppressed on the restored parent.
    TrialFrontendPad(0,0,255,0,0);assert(frontend_events==8);
    panel_open=false;frontend_events=0;++navigation_generation;
    Sample(0,0,0,255);Sample(0,1,255,255);Sample(0,1,255,0);
    Sample(0);Sample(4,0,255);Sample(0);Sample(1,1);Sample(0);
    Sample(8,0,0,255);Sample(0);

    // A replacement screen also discards stale queued navigation and holds.
    TrialFrontendPad(8,0,0,0,0);assert(frontend_events==2);
    ++navigation_generation;Sample(0,8);Sample(0,8);Sample(0);Sample(2,8);
    frontend_active=false;Sample(0,0,255);frontend_active=true;
    Sample(0,0,255);Sample(0);Sample(4,0,255);
}
}

static void Frame(uint32_t selector){
    MEM32(0x602004)=selector;menu_pointer_begin(0x600000);GraphicsMenuBegin();
    g_ecx=g_esi=0x602000;ucrr_menu_list_enter();FrontendListEnter();ucrr_menu_list_hover();
    menu_render_count=1;menu_target_count=1;menu_targets[0]={0x602000,0x603000,0,40,400,180,24};
    GraphicsMenuCapture();ucrr_menu_list_leave();menu_pointer_end();GraphicsMenuEnd();
}
static void CheckManagerHandoff(){
    // Exercise the real shared guest manager/reset/publication sequence, not
    // merely a synthetic host transition bit. The outgoing native frame is
    // still visible, but its callbacks stop owning input at the first reset.
    for(uint32_t selector:{0x51D29Cu,0x51CFA4u,0x51D310u,0x51D304u,0x51D30Cu,0x51D300u,0x51CFB8u}){
        MEM8(0x600014)=0;Frame(selector);const auto stable=published;
        assert(presentation.visible&&!presentation.transition);
        if(selector==0x51CFB8u)assert(stable.list_kind==7&&(stable.source.source_kind&0x80000000u));
        MEM8(0x600014)=1;menu_pointer_begin(0x600000);GraphicsMenuBegin();
        assert(published.source.source_kind&UC_HF_TRANSITION);
        assert(!TrialFrontendSupported(published)&&presentation.visible&&presentation.transition);
        assert(!std::memcmp(&presentation.frame,&stable,sizeof(stable)));
        assert(!TrialFrontendOwns(stable.source.owner,stable.source.kind));
        g_ecx=stable.source.owner;ucrr_menu_list_enter();
        assert(TrialFrontendHideListSprites());
        dialog_running=1;
        assert(!TrialFrontendHideListSprites()); // Restart/checkpoint and other original modal children remain visible.
        dialog_running=0;
        const auto stack=context.esp;const auto quads=sidebar_quads.size();
        __imp__sub_00065550(context,ram);
        assert(context.esp==stack+4&&sidebar_quads.size()==quads);context.esp=stack;
        __imp__sub_00065170(context,ram);
        assert(context.esp==stack+4&&sidebar_quads.size()==quads);context.esp=stack;
        // First-frame and incoming-owner draws precede native publication.
        const auto saved_presentation=presentation;presentation={};
        assert(TrialFrontendHideListSprites());
        presentation=saved_presentation;
        std::memcpy(ram+0x67E000,ram+stable.source.owner,0xE8);
        menu_owner=0x67E000;assert(TrialFrontendHideListSprites());
        MEM32(0x67E004)=0x51D308;assert(!TrialFrontendHideListSprites());
        menu_owner=stable.source.owner;
        name_keyboard_mode=false;assert(!TrialFrontendHideListSprites());name_keyboard_mode=true;
        menu_owner=0;assert(!TrialFrontendHideListSprites());
        // Repeated fade frames may have new epochs, but never resurrect a
        // callback identity or discard the copied presentation prematurely.
        for(unsigned i=0;i<3;++i){hf_publish();assert(presentation.transition&&presentation.visible);}
        MEM8(0x600014)=0;Frame(selector);
        assert(TrialFrontendSupported(published)&&presentation.visible&&!presentation.transition);
        // View Controls and the original child/modal screens
        // remain source-owned. A settled unsupported child clears the cover.
        Frame(0x51D308);assert(!TrialFrontendSupported(published)&&!presentation.visible);
        MEM8(0x600014)=1;hf_reset();assert(!presentation.visible);MEM8(0x600014)=0;
    }
    Frame(0x51D29C);MEM32(0x600004)=0;menu_pointer_end();
    assert(!presentation.visible&&!TrialFrontendSupported(published));
    MEM32(0x600004)=0x601000;
}
static uint32_t ProfileFrame(){
    menu_pointer_begin(0x600000);g_ecx=0x640000;g_esp=0x700000;profile_quads.clear();
    g_esp-=4;MEM32(g_esp)=0x652000;
    g_esp-=4;MEMF(g_esp)=1.f/60;
    g_esp-=4;MEM32(g_esp)=0xFEEDFACE;
    __imp__sub_002840C0(context,ram);assert(g_esp==0x700000);
    const uint32_t callback=context.eax;menu_pointer_end();return callback;
}
static void CheckProfiles(){
    MEM8(0x600014)=0;MEM32(0x640000)=0x48CB30;MEM32(0x640004)=0x51D710;
    MEM32(0x640008)=0x283250;MEM32(0x64000C)=0x283500;MEM32(0x640014)=0x271810;
    MEM32(0x640688)=2;MEM32(0x51D710)=0;MEM32(0x485A18)=0x17;
    MEM32(0x64068C)=0x641000;MEM32(0x641000)=0x641100;
    MEM32(0x64110C)=0x641200;MEM32(0x641108)=0x641204;
    std::strcpy((char*)ram+0x64001C,"Alice");
    std::strcpy((char*)ram+0x64001C+0x19C,"Bob");
    std::strcpy((char*)ram+0x48CB70,"New profile");
    // ORIGINAL_PROFILE_CONSTANTS
    std::memcpy(ram+0x503224,profile_constants,sizeof(profile_constants));
    MEMF(0x44BF94)=1;MEMF(0x4859F0)=50;
    assert(ProfileFrame()==0);const auto s=published.source;
    assert(TrialFrontendSupported(published)&&s.kind==UC_HF_PROFILES&&s.count==4);
    assert(profile_quads.size()==8&&profile_post_fade==275);
    TrialFrontendProfileDrawOwner=s.owner;assert(TrialFrontendHideListSprites());
    name_keyboard_mode=false;assert(!TrialFrontendHideListSprites());name_keyboard_mode=true;
    TrialFrontendProfileDrawOwner=0;
    for(unsigned i=0;i<4;++i){
        const auto& b=published.profile_bounds[i];const auto& solid=profile_quads[2*i];const auto& fade=profile_quads[2*i+1];
        assert(solid.function==0x65760&&fade.function==0x65550);
        assert(solid.x==0&&solid.right==275&&fade.x==275&&fade.right==350);
        assert(solid.y==80+i*45&&solid.bottom==115+i*45&&fade.y==solid.y&&fade.bottom==solid.bottom);
        assert(b.x==solid.x&&b.y==solid.y&&b.w==fade.right-solid.x&&b.h==solid.bottom-solid.y);
        // A click on the actual source label (x50) was outside the old
        // post-draw target beginning at275. It must hit the complete bar.
        const float label_x=MEMF(0x4859F0),label_y=b.y+10;
        assert(label_x<profile_post_fade&&label_x>=b.x&&label_x<b.x+b.w&&label_y<b.y+b.h);
    }
    assert(!std::strcmp(s.rows[1].label,"Bob")&&!std::strcmp(s.rows[3].label,"New profile"));
    for(unsigned i:{1u,3u,0u}){
        const auto view=published.source;
        queued={view.epoch,view.screen,view.owner,view.rows[i].row,i,UC_HF_SELECT};
        const unsigned sounds=profile_hover_sounds,refreshes=profile_refreshes;
        assert(ProfileFrame()==0&&MEM32(0x51D710)==i&&MEM32(0x51D704)==i);
        assert(profile_hover_sounds==sounds+1&&profile_refreshes==refreshes+1&&published.source.epoch==s.epoch);
        const auto selected=published.source;
        queued={selected.epoch,selected.screen,selected.owner,selected.rows[i].row,i,UC_HF_ACTIVATE};
        assert(ProfileFrame()==(i<2?0x283250u:0x283500u));
        assert(profile_hover_sounds==sounds+1&&profile_refreshes==refreshes+1);
    }
    queued={s.epoch-1,s.screen,s.owner,s.rows[0].row,0,UC_HF_ACTIVATE};
    assert(ProfileFrame()==0&&MEM32(0x51D710)==0);
    queued={s.epoch,s.screen,s.owner,s.rows[0].row,0,UC_HF_ACTIVATE};
    MEM32(0x640008)=0xDEAD;assert(ProfileFrame()==0&&MEM32(0x51D710)==0);
    MEM32(0x640008)=0x283250;ProfileFrame();
    auto view=published.source;queued={view.epoch,view.screen,view.owner,0,0,UC_HF_BACK};
    assert(ProfileFrame()==0x271810&&profile_back_sounds==1);
    // The actual source child-active gate must revoke profile ownership.
    view=published.source;queued={view.epoch,view.screen,view.owner,view.rows[1].row,1,UC_HF_ACTIVATE};
    profile_child_active=true;assert(ProfileFrame()==0&&!TrialFrontendSupported(published)&&MEM32(0x51D710)==0);
    profile_child_active=false;queued={};ProfileFrame();
    MEM8(0x600014)=1;assert(ProfileFrame()==0&&!TrialFrontendSupported(published));MEM8(0x600014)=0;
    name_sounds=0;
}
static void CheckDeleteProfile(){
    delete_fixture=true;profile_child_active=false;queued={};
    MEM32(0x640010)=0x284580;MEM32(0x641104)=0x641208;
    MEM32(0x509704)=0x604000;MEM32(0x604020)=0x604100;MEMF(0x604100)=20;
    MEMF(0x4E8394)=640;MEMF(0x4E8398)=480;MEMF(0x4E839C)=320;MEMF(0x4E83A0)=240;
    MEMF(0x44C1F8)=.5f;MEMF(0x46C320)=1;MEMF(0x47DF9C)=1;
    std::strcpy((char*)ram+0x48CB3C,"Delete saved game?");
    std::strcpy((char*)ram+0x48573C,"Yes");std::strcpy((char*)ram+0x485750,"No");
    auto request=[&](unsigned i){
        const auto s=published.source;queued={s.epoch,s.screen,s.owner,s.rows[i].row,i,TRIAL_HF_DELETE};
    };
    auto open=[&](){
        profile_child_active=false;queued={};ProfileFrame();request(1);
        assert(ProfileFrame()==0&&profile_child_active);
        assert(MEM32(0x646000)==0x485B78&&MEM32(0x646008)==1); // original default No
        assert(MEM32(0x646030)==0x284580&&MEM32(0x646034)==0x284310);
        ProfileFrame();assert(TrialFrontendSupported(published)&&published.delete_dialog);
        assert(!std::strcmp(published.delete_name,"Bob")&&published.source.rows[0].enabled);
        const auto epoch=published.source.epoch;
        ProfileFrame();assert(published.source.epoch==epoch); // parent does not revoke child
    };
    ProfileFrame();assert(published.profile_delete_mask==3);
    request(3);assert(ProfileFrame()==0&&!profile_child_active&&deleted_profiles==0); // empty
    auto s=published.source;queued={s.epoch-1,s.screen,s.owner,s.rows[1].row,1,TRIAL_HF_DELETE};
    assert(ProfileFrame()==0&&!profile_child_active);
    request(1);MEM32(0x640010)=0;assert(ProfileFrame()==0&&!profile_child_active);MEM32(0x640010)=0x284580;
    open();s=published.source;
    queued={s.epoch,s.screen,s.owner,s.rows[1].row,1,UC_HF_ACTIVATE};
    assert(ProfileFrame()==0&&deleted_profiles==0); // original No callback swallowed by parent
    open();s=published.source;queued={s.epoch,s.screen,s.owner,0,0,UC_HF_BACK};
    assert(ProfileFrame()==0&&deleted_profiles==0);
    open();s=published.source;
    // Selecting a different slot or replacing the same slot record while the
    // dialog is open cannot redirect its Yes to another profile.
    MEM32(0x51D710)=0;queued={s.epoch,s.screen,s.owner,s.rows[0].row,0,UC_HF_ACTIVATE};
    assert(ProfileFrame()==0&&published.delete_dialog&&!published.source.rows[0].enabled);
    MEM32(0x51D710)=1;MEM8(0x640018+0x19C)^=1;
    s=published.source;queued={s.epoch,s.screen,s.owner,s.rows[0].row,0,UC_HF_ACTIVATE};
    assert(ProfileFrame()==0&&published.delete_dialog&&!published.source.rows[0].enabled);
    MEM8(0x640018+0x19C)^=1;
    open();s=published.source;
    queued={s.epoch-1,s.screen,s.owner,s.rows[0].row,0,UC_HF_ACTIVATE};assert(ProfileFrame()==0);
    s=published.source;queued={s.epoch,s.screen,s.owner,s.rows[0].row,0,UC_HF_ACTIVATE};
    assert(ProfileFrame()==0x284580&&deleted_profiles==0);
    // Execute that actual returned callback; only its disk service and screen
    // rebuilding service are in-memory counters, never real user save files.
    context.esp=0x700000;context.esp-=4;MEM32(context.esp)=0x647000;
    context.esp-=4;MEM32(context.esp)=0xFEEDFACE;
    __imp__sub_00284580(context,ram);
    assert(context.esp==0x6FFFFC&&context.eax==0x647000&&deleted_profiles==1&&deleted_index==1&&profile_rebuilds==1);
    context.esp=0x700000;profile_child_active=false;ProfileFrame();
    assert(!frontend_delete.owner&&!published.delete_dialog&&published.source.kind==UC_HF_PROFILES);
    delete_fixture=false;queued={};name_sounds=0;
}
static uint32_t PauseDialogFrame(){
    menu_pointer_begin(0x600000);GraphicsMenuBegin();
    g_ecx=0x602000;ucrr_menu_list_enter();FrontendListEnter();
    g_ecx=0x646000;g_esp=0x700000;
    g_esp-=4;MEM32(g_esp)=0x652000;
    g_esp-=4;MEMF(g_esp)=1.f/60;
    g_esp-=4;MEM32(g_esp)=0xFEEDFACE;
    __imp__sub_0025A310(context,ram);assert(g_esp==0x700000);
    const uint32_t callback=context.eax;
    // The list parent completes after its modal child, as in2659C0.
    GraphicsMenuCapture();ucrr_menu_list_leave();menu_pointer_end();GraphicsMenuEnd();
    return callback;
}
static void CheckPauseQuit(uint32_t callback=0x267400){
    pause_fixture=true;queued={};name_keyboard_mode=true;
    MEM32(0x600004)=0x601000;MEM32(0x600008)=0x10;MEM8(0x600014)=0;
    std::strcpy((char*)ram+0x473368,"NewFrontEnd-LooseProgress-1");
    std::strcpy((char*)ram+0x48573C,"NewFrontEnd-Yes-1");
    std::strcpy((char*)ram+0x485750,"NewFrontEnd-No-1");
    std::strcpy((char*)ram+0x605200,"You will lose your progress, are you sure?");
    auto open=[&]{
        hf_quit_handoff=0;queued={};Frame(0x51CFA4);
        assert(hf_pause_manager==menu_manager);
        // Execute the original shared factory with the exact265D9E arguments.
        context.esp=0x700000;
        const uint32_t args[]={0x647000,0x473368,0,callback,0x284310,3};
        for(unsigned i=6;i;--i){context.esp-=4;MEM32(context.esp)=args[i-1];}
        context.esp-=4;MEM32(context.esp)=0xFEEDFACE;
        __imp__sub_00258DF0(context,ram);assert(context.esp==0x700000-24);context.esp+=24;
        assert(MEM32(0x647000)==0x646000&&MEM32(0x646008)==1&&FrontendPauseDialog(0x646000));
        pause_texts=pause_separators=pause_backdrops=0;
    };
    auto action=[&](unsigned kind,unsigned index=0){
        const auto& s=published.source;
        queued={s.epoch,s.screen,s.owner,index<s.count?s.rows[index].row:0,index,kind};
    };
    open();assert(PauseDialogFrame()==0);
    assert(published.pause_dialog&&TrialFrontendSupported(published));
    assert(!std::strcmp(published.dialog_message,"You will lose your progress, are you sure?"));
    assert(pause_texts==0&&pause_separators==0&&pause_backdrops==1); // first frame
    const auto epoch=published.source.epoch;
    assert(PauseDialogFrame()==0&&published.source.epoch==epoch&&pause_backdrops==2);
    auto s=published.source;
    for(unsigned invalid=0;invalid<4;++invalid){
        action(UC_HF_ACTIVATE,0);
        if(invalid==0)--queued.epoch;
        if(invalid==1)++queued.screen;
        if(invalid==2)++queued.owner;
        if(invalid==3)++queued.row;
        assert(PauseDialogFrame()==0&&MEM32(0x646008)==1&&!hf_quit_handoff);
    }
    const unsigned confirm_sounds=name_sounds,back_sounds=profile_back_sounds;
    action(UC_HF_ACTIVATE,1);assert(PauseDialogFrame()==0x284310&&name_sounds==confirm_sounds+1);
    action(UC_HF_BACK);assert(PauseDialogFrame()==0x284310&&profile_back_sounds==back_sounds+1);
    action(UC_HF_ACTIVATE,0);assert(PauseDialogFrame()==callback&&bool(hf_quit_handoff)==(callback==0x267400));
    assert(name_sounds==confirm_sounds+2); // original confirmation sound still runs

    // Every exact factory field is required; another warning/callback keeps
    // its own rendering and cannot consume a queued native Quit action.
    for(uint32_t offset:{0u,4u,0xCu,0x10u,0x14u,0x30u,0x34u,0x50u}){
        open();const uint32_t original=MEM32(0x646000+offset);
        MEM32(0x646000+offset)=original^1u;
        assert(!FrontendPauseDialog(0x646000));
        MEM32(0x646000+offset)=original;
    }
    open();PauseDialogFrame();action(UC_HF_ACTIVATE,0);MEM32(0x646030)=0x284580;
    assert(PauseDialogFrame()==0&&pause_texts==3&&pause_separators==1&&!published.pause_dialog);
    queued={};
    open();name_keyboard_mode=false;
    assert(PauseDialogFrame()==0&&pause_texts==3&&pause_separators==1&&pause_backdrops==1);
    assert(!TrialFrontendOwns(0x646000,UC_HF_DIALOG)&&!presentation.visible);
    name_keyboard_mode=true;open();PauseDialogFrame();
    MEM8(0x600014)=1;pause_texts=pause_separators=0;
    assert(PauseDialogFrame()==0&&pause_texts==0&&pause_separators==0);
    assert(presentation.visible&&presentation.transition&&!TrialFrontendSupported(published));
    // An unmatched transition and another manager never suppress original UI.
    presentation={};g_ecx=0x646000;ucrr_menu_dialog_enter();
    assert(!TrialFrontendHidePauseDialogText(0x646000));dialog_running=0;
    MEM8(0x600014)=0;hf_pause_manager=0;assert(!FrontendPauseDialog(0x646000));
    pause_fixture=false;queued={};name_sounds=0;MEM32(0x600008)=0;Frame(0x51D29C);
}
static void CheckDifficultyDialog(){
    const uint32_t owner=0x646000;
    const char* keys[]={"NewFrontEnd-EMH-1","NewFrontEnd-EMH-2","NewFrontEnd-EMH-3","NewFrontEnd-EMH-4","NewFrontEnd-EMH-5"};
    for(unsigned i=0;i<5;++i)std::strcpy(reinterpret_cast<char*>(ram+0x660000+128*i),keys[i]);
    for(unsigned count:{3u,4u}){
        std::memset(ram+owner,0,0x8C);MEM32(owner)=0x485B78;MEM32(owner+4)=0x660000;
        MEM32(owner+0xC)=count;MEM32(owner+8)=1;MEM32(owner+0x50)=0x284310;
        for(unsigned i=0;i<count;++i){MEM32(owner+0x10+4*i)=0x660080+128*i;MEM32(owner+0x30+4*i)=0x280000+16*i;}
        assert(FrontendDifficultyDialog(owner));
        const uint32_t parent=0x647000,wrapper=0x648000,shade=0x649000;
        MEM32(parent)=0x4885FC;MEM32(parent+0x10C)=wrapper;
        MEM32(wrapper)=0x485C84;MEM32(wrapper+4)=shade;
        MEM32(shade)=0x4846A4;MEM32(shade+4)=owner;
        menu_pointer_begin(0x600000);
        // First-frame parent suppression publishes its actual difficulty child early.
        presentation={};hf_view={};dialog_owner=0;dialog_running=0;menu_owner=0;
        assert(!TrialFrontendHideListSprites());
        TrialFrontendLevelDrawHidden=TrialFrontendLevelDrawBegin(parent);
        assert(TrialFrontendLevelDrawHidden&&TrialFrontendHideListSprites());
        assert(dialog_owner==0&&dialog_running==0);
        name_keyboard_mode=false;assert(!TrialFrontendHideListSprites());name_keyboard_mode=true;
        g_ecx=owner;ucrr_menu_dialog_enter();
        assert(TrialFrontendDifficulty(published.source)&&TrialFrontendOwns(owner,UC_HF_DIALOG));
        assert(TrialFrontendHidePauseDialogText(owner));
        assert(!TrialFrontendHideListSprites()); // Keep the modal's own backdrop.
        ucrr_menu_dialog_leave();
        TrialFrontendLevelDrawHidden=false;
        assert(!TrialFrontendHideListSprites()); // The movie and other parent scopes remain visible.
        // The level parent finishes after its child; it must not erase the modal.
        const auto modal_epoch=hf_view.epoch;
        hf_capture_levels(0x647000);
        menu_pointer_end();
        assert(TrialFrontendDifficulty(published.source)&&published.source.owner==owner);
        assert(hf_view.epoch==modal_epoch&&TrialFrontendOwns(owner,UC_HF_DIALOG));
        name_keyboard_mode=false;menu_pointer_begin(0x600000);g_ecx=owner;ucrr_menu_dialog_enter();
        assert(!TrialFrontendLevelDrawBegin(parent));
        assert(!TrialFrontendHidePauseDialogText(owner));
        ucrr_menu_dialog_leave();menu_pointer_end();name_keyboard_mode=true;
        // A closed wrapper, a stale accepted dialog or an unrelated owner cannot hide the browser.
        MEM32(wrapper+4)=0;assert(!TrialFrontendLevelDrawBegin(parent));MEM32(wrapper+4)=shade;
        MEM32(wrapper)=0;assert(!TrialFrontendLevelDrawBegin(parent));MEM32(wrapper)=0x485C84;
        MEM32(shade)=0;assert(!TrialFrontendLevelDrawBegin(parent));MEM32(shade)=0x4846A4;
        MEM32(shade+4)=0;assert(!TrialFrontendLevelDrawBegin(parent));MEM32(shade+4)=owner;
        MEM32(wrapper+4)=owner;assert(!TrialFrontendLevelDrawBegin(parent));MEM32(wrapper+4)=shade;
        MEM32(parent)=0;assert(!TrialFrontendLevelDrawBegin(parent));MEM32(parent)=0x4885FC;
        MEM32(owner+0x30)=0;assert(!FrontendDifficultyDialog(owner));
        assert(!TrialFrontendLevelDrawBegin(parent));
        MEM32(parent+0x10C)=0;assert(!TrialFrontendLevelDrawBegin(parent));
    }
    Frame(0x51D29C);
}
static void NameFrame(){
    menu_pointer_begin(0x600000);g_ecx=g_esi=0x644000;
    FrontendNameEnter();FrontendNameLeave();menu_pointer_end();
}
static void NameRequest(unsigned kind,const char* text){
    const auto& s=published.source;name_queued={};
    name_queued.identity={s.epoch,s.screen,s.owner,s.rows[0].row,0,kind};
    assert(std::strlen(text)<16);std::strcpy(name_queued.text,text);
    g_ecx=g_esi=0x644000;g_esp=0x700000;MEM32(g_esp+0x24)=0;
}
static void CheckName(){
    menu_pointer_reset();MEM32(0x600004)=0x601000;MEM8(0x600014)=0;
    constexpr uint32_t owner=0x644000,validator=0x642000;
    MEM32(owner)=0x48CD08;MEM32(owner+8)=0x51D71C;MEM32(owner+0xC)=15;
    MEM32(owner+0x10)=0x283250;MEM32(owner+0x14)=validator;MEM32(owner+0x18)=0x5A;
    MEM32(validator)=0x48CAD8;MEM32(0x48CAD8)=0x2830A0;MEM32(validator+0x674)=1;
    std::strcpy((char*)ram+validator+8,"Alice");MEM8(0x51D71C)=0;
    std::strcpy((char*)ram+0x4FF1E0,"Duplicate profile");
    NameFrame();const uint64_t epoch=published.source.epoch;
    assert(TrialFrontendSupported(published)&&published.source.kind==TRIAL_HF_NAME&&published.name_capacity==15);
    // Edits preserve the screen identity, clear an old error and do not invoke
    // validation/saving until Create. Q remains ordinary permitted name text.
    MEM32(owner+0x34)=0x4FF1E0;NameRequest(UC_HF_SELECT,"Q Alice9");
    assert(FrontendNameInput()&&!name_validations&&!name_frees);
    assert(!std::strcmp((char*)ram+0x51D71C,"Q Alice9")&&MEM32(owner+0x34)==0&&MEM32(owner+0x18)==0x52);
    NameFrame();assert(published.source.epoch==epoch&&!std::strcmp(published.name,"Q Alice9"));
    // Run the actual source2858A0+2830A0, including normalized duplicate
    // rejection and the empty-name route to the original default-name callback.
    struct Case{const char* text;bool accepted;uint32_t error;};
    for(const Case& c:{Case{"Alice",false,0x4FF1E0},Case{" Alice ",false,0x4FF1E0},
            Case{"Unique9",true,0},Case{"",true,0x4FF1C4},Case{"   ",true,0x4FF1C4},
            Case{"123456789012345",true,0}}){
        NameRequest(UC_HF_ACTIVATE,c.text);const auto saved=context;
        const unsigned validates=name_validations,sounds=name_sounds,frees=name_frees;
        assert(FrontendNameInput()&&!std::memcmp(&saved,&context,sizeof(context)));
        assert(MEM32(g_esp+0x24)==(c.accepted?0x283250u:0u)&&MEM32(owner+0x34)==c.error);
        assert(name_validations==validates+1&&name_frees==frees+1&&name_sounds==sounds+unsigned(c.accepted));
        assert(!std::strcmp((char*)ram+0x51D71C,c.text));
        NameFrame();assert(published.source.epoch==epoch);
    }
    NameRequest(UC_HF_SELECT,"Replacement");--name_queued.identity.epoch;
    assert(FrontendNameInput()&&!std::strcmp((char*)ram+0x51D71C,"123456789012345"));
    NameRequest(UC_HF_ACTIVATE,"Bad!");assert(FrontendNameInput()&&MEM32(g_esp+0x24)==0);
    char too_long[16];std::memset(too_long,'a',16);assert(!TrialFrontendNameText(too_long));
    // Controller mode gets the source on-screen keyboard; no queued host edit
    // is consumed. Cancel routes only the exact original parent Back trigger.
    name_keyboard_mode=false;NameRequest(UC_HF_SELECT,"Controller");assert(!FrontendNameInput());
    name_keyboard_mode=true;name_queued={};
    NameRequest(UC_HF_BACK,"");assert(FrontendNameInput());
    g_edi=0x646000;MEM32(g_edi)=0x489974;MEM32(g_edi+4)=0x17;MEM32(g_edi+8)=0x271810;
    MEM32(g_esp)=0x276723;MEM32(g_esp+4)=0x16;assert(!FrontendNameQuery());
    MEM32(g_esp+4)=0x17;assert(FrontendNameQuery()&&!FrontendNameQuery());
    MEM8(owner+4)=1;NameFrame();assert(!TrialFrontendSupported(published));
}
static void CheckObjectives(){
    constexpr uint32_t manager=0x660000,screen=0x661000,owner=0x662000;
    MEM32(manager+4)=screen;MEM8(manager+0x14)=0;MEM32(screen)=0x489974;
    MEM32(owner)=0x487588;MEM32(owner+4)=0x51CF9C;MEM32(owner+0x58)=20;
    MEM32(owner+0xAC)=0x267400;MEM8(owner+0xB0)=0;MEM32(0x485A18)=0x17;
    name_keyboard_mode=true;menu_pointer_begin(manager);g_ecx=owner;
    FrontendObjectivesEnter();FrontendObjectivesLeave();menu_pointer_end();
    const auto s=published.source;assert(s.kind==UC_HF_OBJECTIVES&&TrialFrontendSupported(published));
    const auto before=std::vector<uint8_t>(ram+owner,ram+owner+0xFC);
    for(auto kind:{UC_HF_BACK,UC_HF_LEFT,UC_HF_RIGHT}){
        menu_pointer_begin(manager);g_ecx=owner;FrontendObjectivesEnter();g_edi=owner;g_esp=0x700000;
        queued={s.epoch,s.screen,owner,owner,0,uint32_t(kind)};FrontendObjectivesInput();
        MEM32(g_esp)=kind==UC_HF_BACK?0x26316A:kind==UC_HF_LEFT?0x26319C:0x2631DE;
        MEM32(g_esp+4)=kind==UC_HF_BACK?0x17:kind==UC_HF_LEFT?0x1E:0x1F;
        assert(FrontendObjectivesQuery());++MEM32(g_esp);assert(!FrontendObjectivesQuery());
        --MEM32(g_esp);name_keyboard_mode=false;assert(!FrontendObjectivesQuery());name_keyboard_mode=true;
        FrontendObjectivesLeave();assert(!FrontendObjectivesQuery());menu_pointer_end();
    }
    menu_pointer_begin(manager);g_ecx=owner;FrontendObjectivesEnter();g_edi=owner;
    queued={s.epoch-1,s.screen,owner,owner,0,UC_HF_BACK};FrontendObjectivesInput();assert(!frontend_objectives_action);
    FrontendObjectivesLeave();menu_pointer_end();
    assert(!std::memcmp(before.data(),ram+owner,before.size()));
    MEM32(owner)=0;g_ecx=owner;FrontendObjectivesEnter();assert(!frontend_objectives_owner);
}
int main(){
    g_esp=0x700000;
    MEMF(0x44BF94)=1;MEMF(0x44C1F8)=.5f;
    MEM32(0x5150B0)=0x660000;MEM32(0x660000)=0x660100;
    MEM32(0x6601B4)=0x660400;MEM32(0x660148)=0x660404;
    MEM32(0x660200)=0x660300;MEM32(0x660304)=0x660408;
    // Reproduce the observed crack with the old input through the exact same
    // original rectangle path: x=3 becomes 2.5, exposing guest columns 0..2.
    MEMF(0x661000)=3;MEMF(0x661004)=73;
    MEMF(0x661008)=47;MEMF(0x66100C)=379;MEM32(0x661010)=0xff000000;
    const auto initial=context;
    kernel::CallGuest(context,0x656D0,{0x661000,0x661008,0x661010});
    context=initial;assert(sidebar_quads.size()==1);
    const auto old_cover=sidebar_quads.back();
    assert(old_cover.x==2.5f&&old_cover.right==46.5f&&old_cover.y==72.5f&&old_cover.bottom==378.5f);
    sidebar_quads.clear();draws.clear();
    MEM32(0x600004)=0x601000;MEM32(0x601000)=0x4877FC;
    MEM32(0x602000)=0x4877B4;MEM32(0x602058)=2;
    for(unsigned i=0;i<2;++i){const uint32_t row=0x603000+i*0x40;
        MEM32(0x602008+i*4)=row;MEM32(row)=0x487608;MEM8(row+4)=1;
        MEM32(row+8)=0x606000;MEM32(row+12)=0x272010;}
    std::strcpy((char*)ram+0x606000,"NewFrontEnd-Options-1");
    MEM32(0x509704)=0x604000;MEM8(0x605031)=1;MEM32(0x605034)=0x605100;MEM32(0x605100)=0x605200;
    // The original lookup remains unchanged; only the copied host text is
    // encoded for ImGui, preserving source bytes and the full guest context.
    {
        const auto before=context;char decoded[128]{},key[128]="Original-Language-Key";
        std::strcpy((char*)ram+0x605200,"Fran\xe7" "ais, qualit\xe9, pi\xf9, gr\xf6\xdf" "er \x93PC\x94");
        FrontendText(key,decoded);
        assert(!std::strcmp(decoded,"Fran\xc3\xa7" "ais, qualit\xc3\xa9, pi\xc3\xb9, gr\xc3\xb6\xc3\x9f" "er \xe2\x80\x9cPC\xe2\x80\x9d"));
        assert(!std::memcmp(&before,&context,sizeof(context)));
        assert(MEM8(0x605204)==0xe7);
        char small[4]{};TrialFrontendUtf8("\xe9\xe9",small);assert(!std::strcmp(small,"\xc3\xa9"));
        TrialFrontendUtf8("\x81X",small);assert(!std::strcmp(small,"?X"));
    }
    std::strcpy((char*)ram+0x605200,"Original localized row");
    for(uint32_t selector:{0x51D29Cu,0x51CFA4u,0x51D310u}){
        const auto before=context;Frame(selector);
        assert(TrialFrontendSupported(published)&&published.source.count==2);
        assert(!std::strcmp(published.labels[0],"Original localized row"));
        assert(g_esp==before.esp);
        const auto s=published.source;
        queued={s.epoch,s.screen,s.owner,s.rows[1].row,1,UC_HF_ACTIVATE};
        menu_pointer_begin(0x600000);g_ecx=g_esi=0x602000;ucrr_menu_list_enter();ucrr_menu_list_hover();
        assert(MEM32(selector)==1&&menu_click_row==s.rows[1].row&&menu_click_action==0x16);
        // A stale screen action cannot run a callback or rewrite selection.
        queued={s.epoch-1,s.screen,s.owner,s.rows[0].row,0,UC_HF_ACTIVATE};ucrr_menu_list_hover();assert(MEM32(selector)==1);
    }
    CheckManagerHandoff();
    Frame(0x51D308);assert(!TrialFrontendSupported(published));
    unsigned reads=pointer_reads;g_ecx=g_esi=0x602000;ucrr_menu_list_enter();ucrr_menu_list_hover();assert(pointer_reads==reads+1);
    Frame(0x51D29C);MEM8(0x600014)=1;menu_pointer_begin(0x600000);GraphicsMenuBegin();
    assert(!TrialFrontendSupported(published));MEM8(0x600014)=0;

    // Restore an original unlocked assignment list, with real art/check slots.
    menu_manager=0x600000;menu_screen=0x601000;level_owner=0x607000;
    MEM32(level_owner)=0x4885FC;MEM32(level_owner+0xEC)=17;MEM32(level_owner+0x1C)=0;
    // ORIGINAL_COMPLETION_TABLE
    std::memcpy(ram+0x26F688,completion_table,16);
    MEM32(0x51D0DC)=0x610000;MEM32(0x51D0E0)=0x610100;MEM32(0x51D0E4)=0x610200;
    for(unsigned i=0;i<17;++i){
        MEM32(level_owner+0x20+12*i)=i<11?i:0xffffffffu;
        MEM32(level_owner+0x24+12*i)=i<11?0:i-11;MEM8(level_owner+0x28+12*i)=1;
    }
    for(unsigned i=0;i<12;++i){uint32_t art=i<11?0x500A98+24*i:0x500BA0;MEM32(art+16)=0x620000+i*0x100;MEM32(art+20)=0x630000+i*0x100;}
    for(unsigned index=0;index<17;++index)for(uint32_t completed:{0xffffffffu,0u,1u,2u,3u}){
        level_draw_index=index;g_esp=0x700000;MEM32(g_esp)=0x27092C;MEM32(g_esp+4)=index<11?0x500A98+24*index:0x500BA0;MEM32(g_esp+28)=completed;
        const auto saved=context;draws.clear();sidebar_quads.clear();assert(FrontendBadge());assert(!std::memcmp(&context,&saved,sizeof(context)));
        const auto r=TrialLevelBounds(index,17);assert(r.y>=94&&r.y+r.h<=371);
        if(index==0){
            const auto& cover=draws[0];const auto& border=draws[1];
            assert(cover.function==0x656D0&&cover.color==0xff000000&&
                cover.x==0&&cover.y==73&&cover.right==47&&cover.bottom==379);
            assert(border.x==46&&border.right==47&&border.y==cover.y&&border.bottom==cover.bottom);
            assert(sidebar_quads.size()>=2);
            const auto& emitted=sidebar_quads[0];const auto& divider=sidebar_quads[1];
            assert(emitted.x==-.5f&&emitted.right==old_cover.right&&emitted.color==0xff000000);
            assert(emitted.y==old_cover.y&&emitted.bottom==old_cover.bottom);
            assert(divider.x==45.5f&&divider.right==46.5f&&divider.color==0xff858b89);
            // Coverage of pixel centers after scale, expressed in the original
            // Xbox half-pixel coordinate space. This is a geometry oracle;
            // GPU composition and visual acceptance remain separate checks.
            for(unsigned width:{640u,1280u,2560u,3840u}){
                const float scale=width/640.f;
                for(unsigned pixel=0;pixel<unsigned(3*scale);++pixel){
                    const float center=(pixel+.5f)/scale-.5f;
                    assert(center<old_cover.x&&center>=emitted.x&&center<emitted.right);
                }
            }
        }
        const auto& badge=draws[draws.size()-(completed<4?2:1)];assert(badge.function==0x65760&&badge.x==r.x+2&&badge.y==r.y+1);
        assert(badge.texture==MEM32(MEM32(g_esp+4)+(completed<4?20:16)));
        if(completed<4){uint32_t branch=completion_table[completed];uint32_t slot=branch==0x26F621?0x51D0DC:branch==0x26F653?0x51D0E0:0x51D0E4;assert(draws.back().texture==MEM32(slot));}
    }
    name_keyboard_mode=false;draws.clear();assert(!FrontendLevelReady()&&!FrontendBadge()&&draws.empty());
    name_keyboard_mode=true;
    draws.clear();MEM32(g_esp)=0x27591E;assert(!FrontendBadge()&&draws.empty());
    MEM32(g_esp)=0x27092C;MEM8(level_owner+0x28+12*level_draw_index)=0;assert(!FrontendBadge());
    MEM8(level_owner+0x28+12*level_draw_index)=1;MEM8(0x600014)=1;assert(!FrontendBadge());
    // The reward browser retains the title's filtered records and locked labels.
    MEM8(0x600014)=0;constexpr uint32_t reward=0x650000,records=0x651000,text=0x652000;
    MEM32(reward)=0x485F14;MEM32(reward+0x18)=2;MEM32(reward+0x2C)=records;
    MEM32(reward+0x14)=0x2715D0;MEM32(0x485A18)=0x17;
    std::strcpy(reinterpret_cast<char*>(ram+text),"Reward fixture");
    for(unsigned i=0;i<2;++i){MEM32(records+44*i+0x18)=text;MEM32(records+44*i+0x1C)=text;}
    MEM32(0x600004)=reward;menu_pointer_begin(0x600000);g_esi=reward;
    assert(!ucrr_rewards_hover());menu_pointer_end();assert(published.source.kind==UC_HF_REWARD_BROWSER);
    MEM32(0x4FF4BC+16)=0x620100;MEM8(records+0x28)=1;MEM32(records+0x14)=0x653000;MEM32(0x653010)=0x620200;
    menu_pointer_begin(0x600000);g_esi=reward;g_esp=0x700000;draws.clear();
    const auto reward_context=context;FrontendRewardSidebar();
    assert(!std::memcmp(&context,&reward_context,sizeof(context))&&draws.size()==5);
    assert(draws[0].x==0&&draws[0].right==47&&draws[0].bottom==379);
    assert(draws[3].texture==0x620200&&draws[4].texture==0x620100);
    assert(draws[3].right-draws[3].x==24&&draws[3].bottom-draws[3].y==24);
    for(unsigned i=0;i<TrialRewardsPerPage;++i){
        const auto reward_bounds=TrialLevelBounds(i,TrialRewardsPerPage),level_bounds=TrialLevelBounds(i,4);
        assert(reward_bounds.x==level_bounds.x&&reward_bounds.y==level_bounds.y&&reward_bounds.h==level_bounds.h);
    }
    assert(draws[3].x==10&&draws[3].y==95&&draws[4].y==125);
    name_keyboard_mode=false;draws.clear();FrontendRewardSidebar();assert(draws.empty());name_keyboard_mode=true;
    ucrr_rewards_hover();menu_pointer_end();
    auto reward_snapshot=published.source;
    queued={reward_snapshot.epoch,reward_snapshot.screen,reward,reward_snapshot.rows[1].row,1,UC_HF_ACTIVATE};
    menu_pointer_begin(0x600000);g_esi=reward;assert(ucrr_rewards_hover()&&MEM32(reward+4)==1);menu_pointer_end();
    reward_snapshot=published.source;
    queued={reward_snapshot.epoch,reward_snapshot.screen,reward,0,0,UC_HF_BACK};
    menu_pointer_begin(0x600000);g_esi=reward;ucrr_rewards_hover();MEM32(g_esp)=0x25CCEE;MEM32(g_esp+4)=0x17;
    assert(hf_back_query());menu_pointer_end();
    // Entry/return clicks and Escape remain suppressed until a neutral frame.
    TrialFrontendReleaseGate gate;assert(!gate.Accept(1,true));assert(!gate.Accept(1,true));
    assert(!gate.Accept(1,false));assert(gate.Accept(1,true));
    assert(!gate.Accept(2,true));assert(!gate.Accept(2,false));assert(gate.Accept(2,false));
    controller_fixture::Check();
    CheckProfiles();CheckDeleteProfile();CheckPauseQuit();CheckPauseQuit(0x267330);CheckPauseQuit(0x2672A0);CheckDifficultyDialog();CheckName();CheckProfileWarnings();CheckObjectives();
    std::puts("frontend: original profile bounds/actions, typed-name source validation/cancel, source-owned actions, badge/completion, manager visual handoff, controller gates and localized PC profile warnings passed");
}
