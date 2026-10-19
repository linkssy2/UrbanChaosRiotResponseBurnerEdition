#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using std::isfinite;

static uint8_t ram[0x800000];
size_t g_xbox_total_ram=sizeof(ram);
#define MEM8(a) (ram[uint32_t(a)])
#define MEM32(a) (*(uint32_t*)(ram+uint32_t(a)))
#define MEMF(a) (*(float*)(ram+uint32_t(a)))
static uint32_t g_ecx,g_esi,g_edi,g_ebx,g_esp=0x700000;
static bool TrialGuestReadable(uint32_t address,uint32_t bytes){
    return address>=0x10000u&&address<sizeof(ram)&&bytes&&bytes<=sizeof(ram)-address;
}
static int pointer_available,pointer_click,menu_mode;
static float pointer_x,pointer_y;
static unsigned frontend_publishes,frontend_actions;
uint64_t ucrr_host_menu_milliseconds(){return 1000;}
void ucrr_host_menu_mode(int mode){menu_mode=mode;}
int ucrr_host_menu_pointer(float* x,float* y,int* click){
    *x=pointer_x;*y=pointer_y;*click=pointer_click;pointer_click=0;return pointer_available;
}
#include "xbe_frontend.h"
extern "C" int ucrr_native_frontend_enabled(){return 0;}
extern "C" void ucrr_native_frontend_publish(const UcHybridFrontendSnapshot*){++frontend_publishes;}
extern "C" int ucrr_native_frontend_take_action(UcHybridFrontendAction*){++frontend_actions;return 0;}
static bool FrontendGuestOwns(uint32_t,uint32_t){return false;}
// This fixture exercises graphics-context discovery with the native frontend
// disabled. Profile deletion and pause confirmation use their real guest
// adapters in test_frontend; keep those unrelated actions unavailable here.
static bool FrontendProfileDelete(uint32_t,unsigned){return false;}
static bool FrontendProfileDeleteQuery(){return false;}
static bool FrontendDeleteChoice(uint32_t,unsigned){return false;}
static bool FrontendDeleteParent(){return false;}
static void FrontendPauseDialogEnter(){}
#include "menu_pointer_guest.h"
#include "xbe_graphics_menu_context.h"
static TrialMenuContext observed;
void TrialGraphicsMenuContext(uint32_t manager,uint32_t screen,uint32_t owner,uint32_t kind){
    observed={manager,screen,owner,kind};
}

#define CHECK(value) do{if(!(value)){std::printf("FAIL %d: %s\n",__LINE__,#value);return 1;}}while(0)
enum : uint32_t {Manager=0x600000,Screen=0x601000,Owner=0x602000,Row=0x603000,
                 Font=0x604000,Metrics=0x605000,Dialog=0x606000};
static void Begin(){
    menu_pointer_begin(Manager);GraphicsMenuBegin();
    g_ecx=g_esi=Owner;ucrr_menu_list_enter();
}
static void Draw(){
    for(unsigned i=0;i<2;++i){
        g_edi=Row+i*0x40;MEMF(g_esp+0x24)=160;
        MEMF(g_esp+0x2c)=100;MEMF(g_esp+0x20)=100+i*30;
        ucrr_menu_row_bounds();
    }
}
static void Leave(){GraphicsMenuCapture();ucrr_menu_list_leave();}
static void End(){menu_pointer_end();GraphicsMenuEnd();}
static void Frame(){Begin();ucrr_menu_list_hover();Draw();Leave();End();}

int main(){
    // The real screen can be a composite containing the list: owner != screen.
    MEM32(Manager+4)=Screen;MEM32(Screen)=0x4877FC;
    MEM32(Owner)=0x4877B4;MEM32(Owner+0x58)=2;
    MEM32(0x509704)=Font;MEM32(Font+0x14)=Metrics;MEMF(Metrics)=24;
    for(unsigned i=0;i<2;++i){
        const uint32_t row=Row+i*0x40;MEM32(Owner+8+i*4)=row;
        MEM32(row)=0x487608;MEM8(row+4)=1;MEM32(row+0xc)=0x272010;
    }
    const uint32_t selectors[]={0x51D29C,0x51CFA4,0x51D310,0x51D304,0x51D30C};
    for(unsigned i=0;i<5;++i){
        MEM32(Owner+4)=selectors[i];Frame();
        CHECK(observed.kind==i+1&&observed.manager==Manager&&observed.screen==Screen&&observed.owner==Owner);
        CHECK(menu_mode&&menu_target_count==2);
    }
    // Controller/audio/general submenus, arbitrary lists, bad counts/selections
    // and unsupported base classes must not expose the host graphics page.
    for(uint32_t selector:{0x51D300u,0x51D308u,0x600100u}){
        MEM32(Owner+4)=selector;Frame();CHECK(!observed.kind);
    }
    MEM32(Owner+4)=0x51D310;
    for(uint32_t count:{0u,21u}){MEM32(Owner+0x58)=count;Frame();CHECK(!observed.kind);}
    MEM32(Owner+0x58)=2;MEM32(0x51D310)=2;Frame();CHECK(!observed.kind);MEM32(0x51D310)=0;
    MEM32(Owner)=0x123456;Frame();CHECK(!observed.kind);MEM32(Owner)=0x4877B4;
    Frame();CHECK(observed.kind==3);
    Begin();Draw();Leave();End();CHECK(!observed.kind); // original input gate did not run
    Begin();ucrr_menu_list_hover();Leave();End();CHECK(!observed.kind); // no rendered bounds

    // Stable frames keep identity; entering a fade revokes before the next draw.
    Frame();const auto identity=observed;Begin();CHECK(observed.kind==identity.kind);
    ucrr_menu_list_hover();Draw();Leave();End();CHECK(observed.owner==identity.owner);
    MEM8(Manager+0x14)=1;Begin();CHECK(!observed.kind);Draw();Leave();End();CHECK(!observed.kind);
    MEM8(Manager+0x14)=0;Frame();CHECK(observed.kind==3);
    Begin();ucrr_menu_list_hover();Draw();Leave();MEM8(Manager+0x14)=1;End();CHECK(!observed.kind);
    MEM8(Manager+0x14)=0;Frame();CHECK(observed.kind==3);
    Begin();ucrr_menu_list_hover();Draw();Leave();MEM32(Manager+4)=Screen+0x100;End();CHECK(!observed.kind);
    MEM32(Manager+4)=Screen;Frame();CHECK(observed.kind==3);

    // A child modal immediately revokes the previously published parent and
    // cannot be covered by the parent's later leave/end hooks.
    MEM32(Dialog)=0x485B78;MEM32(Dialog+0xc)=2;
    Begin();ucrr_menu_list_hover();Draw();g_ecx=Dialog;
    ucrr_menu_dialog_enter();GraphicsMenuSpecial();CHECK(!observed.kind);
    ucrr_menu_dialog_leave();Leave();End();CHECK(!observed.kind);
    Frame();CHECK(observed.kind==3);
    GraphicsMenuDestroy(Manager+0x100);CHECK(observed.kind==3);
    GraphicsMenuDestroy(Manager);g_ecx=Manager;ucrr_menu_manager_destroy();
    CHECK(!observed.kind&&!menu_mode&&!graphics_pending.kind);

    // This signal never enables the full frontend or consumes original clicks.
    Frame();pointer_available=pointer_click=1;pointer_x=110;pointer_y=135;
    Begin();ucrr_menu_list_hover();CHECK(MEM32(0x51D310)==1&&menu_click_row==Row+0x40);
    g_esi=Row+0x40;MEM32(g_esp)=0x263452;MEM32(g_esp+4)=0x16;
    CHECK(ucrr_menu_click_query()&&ucrr_menu_click_query());Draw();Leave();End();
    CHECK(observed.kind==3&&!frontend_publishes&&!frontend_actions);
    std::puts("graphics menu context: qualified source menus, transitions, modals, destruction and original clicks passed");
}
