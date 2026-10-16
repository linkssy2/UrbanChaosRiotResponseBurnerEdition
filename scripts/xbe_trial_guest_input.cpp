#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <xmmintrin.h>
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "kernel/kernel.h"
using std::isfinite;
static thread_local X86Context* regs;
static thread_local uint8_t* ram;
size_t g_xbox_total_ram=0x08000000;
#define g_eax (regs->eax)
#define g_ecx (regs->ecx)
#define g_edx (regs->edx)
#define g_ebx (regs->ebx)
#define g_esp (regs->esp)
#define g_ebp (regs->ebp)
#define g_esi (regs->esi)
#define g_edi (regs->edi)
#define MEM8(a) (*(uint8_t*)(ram+uint32_t(a)))
#define MEM32(a) (*(x86_u32u*)(ram+uint32_t(a)))
#define MEMF(a) (*(x86_f32u*)(ram+uint32_t(a)))
#include "hybrid_frontend_guest.h"
#include "xbe_frontend.h"
static bool FrontendGuestOwns(uint32_t owner,uint32_t kind);
#include "xbe_input_memory.h"
#include "menu_pointer_guest.h"
#include "pc_mouse_rotation.h"
extern int TrialMouseActive();
extern void TrialGameplay();
extern void TrialMouseTake(long*,long*,unsigned*);
extern void TrialInputPublish();
#include "xbe_graphics_menu_context.h"
#include "xbe_frontend_guest.inc"
static bool MouseObject(uint32_t object){return menu_pointer_readable(object,0x694)&&MEM32(object+0x368)==0;}
int TrialGuestHook(X86Context& ctx,uint8_t* base,unsigned op){
    regs=&ctx;ram=base;unsigned csr=_mm_getcsr();int result=0;
    switch(op){
    case 0:menu_pointer_begin(g_ecx);GraphicsMenuBegin();break;
    case 1:menu_pointer_end();GraphicsMenuEnd();break;
    case 2:ucrr_menu_list_enter();FrontendListEnter();break;case 3:GraphicsMenuCapture();ucrr_menu_list_leave();break;
    case 4:ucrr_menu_adjust_enter();break;case 5:ucrr_menu_adjust_leave();break;
    case 6:ucrr_menu_arrow_bounds();break;case 7:ucrr_menu_list_hover();break;
    case 8:ucrr_menu_row_bounds();break;case 9:result=FrontendNameQuery()||ucrr_menu_click_query();break;
    case 10:ucrr_level_enter();break;case 11:ucrr_level_leave();break;
    case 12:result=ucrr_level_hover();break;case 13:ucrr_level_thumb_begin();break;
    case 14:ucrr_level_thumb_bounds();break;
    case 15:GraphicsMenuDestroy(g_ecx);ucrr_menu_manager_destroy();break;
    case 16:ucrr_menu_dialog_enter();GraphicsMenuSpecial();break;case 17:ucrr_menu_dialog_hover();break;
    case 18:ucrr_menu_dialog_bounds();break;case 19:ucrr_menu_dialog_leave();break;
    case 20:ucrr_menu_profile_enter();GraphicsMenuSpecial();break;case 21:result=ucrr_menu_profile_hover();break;
    case 22:ucrr_menu_profile_bounds();break;case 23:ucrr_menu_profile_leave();break;
    case 24:if(MEM32(g_esp+4)==0)TrialInputPublish();break;
    case 25:result=MouseObject(g_edi)&&TrialMouseActive();break;
    case 26:{
        // D30BE receives EBX in ECX; BEF80 preserves that nonvolatile receiver.
        uint32_t object=g_ebx;
        if(MouseObject(object)){
            TrialGameplay();
            if(TrialMouseActive()){
                long dx,dy;unsigned sensitivity;TrialMouseTake(&dx,&dy,&sensitivity);
                float pitch=MEMF(object+0x688),yaw=MEMF(object+0x68c);
                if(ucrr_pc_rotate(&pitch,&yaw,MEMF(object+0x37c),dx,dy,sensitivity)){
                    MEMF(object+0x688)=pitch;MEMF(object+0x68c)=yaw;
                }
            }
        }break;}
    case 27:result=FrontendBadge();break;
    case 28:result=FrontendLevelReady()&&MEM32(g_esp)==0x27094E;break;
    case 29:FrontendNameEnter();break;
    case 30:result=FrontendNameInput();break;
    case 31:FrontendNameLeave();break;
    case 33:FrontendRewardSidebar();break;
    case 32:{
        // Retain the retail panel builder and all unlock/save decisions.
        const X86Context saved=ctx;
        if(ucrr_rewards_hover()){
            ctx.ecx=saved.esi;kernel::CallGuest(ctx,0x25B460,{});
        }
        ctx=saved;break;
    }
    }
    _mm_setcsr(csr);return result;
}
