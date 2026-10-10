#include <stdint.h>
/* UI messages retain real clicks; guest thread consumes them at list update. */
#define UCRR_MENU_CURSOR_MESSAGE (WM_APP+71)
static LONG menu_cursor_active;
static SRWLOCK menu_click_lock=SRWLOCK_INIT;
static int menu_click_pending;
static float menu_click_x,menu_click_y;
static int pc_menu_active(void){return InterlockedCompareExchange(&menu_cursor_active,0,0)!=0;}
static int pc_menu_position(float*x,float*y){
 if(xbox_InputIsBlocked()||GetForegroundWindow()!=pc_mouse_window||!xbox_output_window)return 0;
 RECT r;POINT p;if(!GetClientRect(xbox_output_window,&r)||!GetCursorPos(&p)||!ScreenToClient(xbox_output_window,&p))return 0;
 if(r.right<=0||r.bottom<=0||p.x<0||p.y<0||p.x>=r.right||p.y>=r.bottom)return 0;
 *x=(float)p.x*640.0f/r.right;*y=(float)p.y*480.0f/r.bottom;return 1;
}
static void pc_menu_click(void){
 float x,y;if(!pc_menu_active()||!pc_menu_position(&x,&y))return;
 AcquireSRWLockExclusive(&menu_click_lock);menu_click_x=x;menu_click_y=y;menu_click_pending=1;ReleaseSRWLockExclusive(&menu_click_lock);
}
static void pc_menu_clear(void){AcquireSRWLockExclusive(&menu_click_lock);menu_click_pending=0;ReleaseSRWLockExclusive(&menu_click_lock);}
static void pc_menu_apply_capture(void){
 if(pc_menu_active())ucrr_host_release_mouse();
 else pc_mouse_capture(); /* existing focus/overlay gates still apply */
}
void ucrr_host_menu_mode(int enabled){
 if(enabled)ucrr_host_pause_cancel();
 if(InterlockedExchange(&menu_cursor_active,!!enabled)!=!!enabled){pc_menu_clear();PostMessageA(pc_mouse_window,UCRR_MENU_CURSOR_MESSAGE,0,0);}
}
int ucrr_host_menu_pointer(float*x,float*y,int*click){
 *click=0;if(!pc_menu_active()||!pc_menu_position(x,y)){pc_menu_clear();return 0;}
 AcquireSRWLockExclusive(&menu_click_lock);
 if(menu_click_pending){*x=menu_click_x;*y=menu_click_y;*click=1;menu_click_pending=0;}
 ReleaseSRWLockExclusive(&menu_click_lock);return 1;
}

uint64_t ucrr_host_menu_milliseconds(void){return (uint64_t)GetTickCount64();}
