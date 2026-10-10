/* Original list selection: 002659C0, +04 selected-index pointer, +08 rows,
 * +58 count. Only rendered, enabled command rows are eligible. No guessed rows. */
#include "menu_pointer_targets.h"
extern size_t g_xbox_total_ram;
static int menu_pointer_readable(uint32_t address,uint32_t bytes){
 return address>=0x10000u&&(size_t)address<g_xbox_total_ram&&
  (size_t)bytes<=g_xbox_total_ram-(size_t)address;
}
extern int ucrr_host_menu_pointer(float*,float*,int*);
extern void ucrr_host_menu_mode(int);
static UCRR_MENU_TARGET menu_targets[80];
static unsigned menu_target_count,menu_render_count;
static uint32_t menu_screen,menu_owner,menu_click_row,menu_manager,menu_click_action,menu_draw_row;
static int menu_seen,menu_have_pointer;
static UCRR_MENU_TARGET level_targets[32];
static unsigned level_target_count,level_render_count,level_draw_index;
static uint32_t level_owner,level_click_owner;
extern uint64_t ucrr_host_menu_milliseconds(void);
static int level_pointer_valid,level_hover_pending;
static float level_anchor_x,level_anchor_y;
static unsigned level_hover_index,level_hover_selection;
static uint64_t level_hover_since;
static float menu_last_x,menu_last_y;
static int menu_special_active(void);
#include "hybrid_frontend_guest.inc"
#include "menu_dialog_guest.inc"
static void menu_pointer_reset(void){
 hf_reset();
 menu_special_reset();
 level_pointer_valid=level_hover_pending=0;menu_target_count=menu_render_count=0;level_target_count=level_render_count=0;level_owner=level_click_owner=0;menu_have_pointer=menu_seen=0;
 menu_screen=menu_owner=menu_click_row=menu_manager=menu_click_action=menu_draw_row=0;
 ucrr_host_menu_mode(0);
}
void ucrr_menu_manager_destroy(void){
 if(g_ecx==menu_manager){hf_pause_manager=0;menu_pointer_reset();}
}
static void menu_pointer_begin(uint32_t manager){
 hf_begin();
 menu_special_begin_frame();
 menu_manager=manager;
 uint32_t screen=menu_pointer_readable(manager,0x15)?MEM32(manager+4):0;
 if(screen!=menu_screen){hf_reset();menu_special_reset();level_pointer_valid=level_hover_pending=0;level_target_count=0;level_owner=level_click_owner=0;menu_target_count=0;menu_have_pointer=0;menu_screen=screen;}
 menu_seen=0;level_click_owner=0;menu_owner=menu_click_row=menu_click_action=menu_draw_row=0;
}
static void menu_pointer_end(void){
 /* The update may have replaced/deleted the screen after rendering its rows.
  * Do not publish the outgoing screen's cursor mode for its successor. */
 if(!menu_pointer_readable(menu_manager,0x15)||MEM32(menu_manager+4)!=menu_screen){
  menu_pointer_reset();return;
 }
 if(!menu_seen||!menu_render_count){menu_target_count=0;menu_have_pointer=0;}
 if(menu_special_active()&&hf_pending.kind!=UC_HF_DIALOG&&hf_pending.kind!=UC_HF_PROFILES)memset(&hf_pending,0,sizeof(hf_pending));
 hf_publish();menu_owner=menu_click_row=menu_click_action=menu_draw_row=0;ucrr_host_menu_mode(menu_seen);
}
void ucrr_menu_list_enter(void){menu_owner=g_ecx;menu_click_row=menu_click_action=menu_draw_row=0;menu_render_count=0;menu_seen=1;}
void ucrr_menu_list_leave(void){hf_capture_list(menu_owner);menu_owner=menu_click_row=menu_click_action=menu_draw_row=0;}
void ucrr_menu_list_hover(void){
 uint32_t owner=g_esi;
 if(owner!=menu_owner||!menu_pointer_readable(owner,0x5c))return;
 uint32_t count=MEM32(owner+0x58),selection=MEM32(owner+4);
 if(count>20||!menu_pointer_readable(selection,4))return;
 hf_eligible_owner=owner;int native_changed;if(hf_input(owner,UC_HF_LIST,&native_changed))return;
 float x,y;int click=0;
 if(!ucrr_host_menu_pointer(&x,&y,&click))return;
 if(menu_have_pointer&&x==menu_last_x&&y==menu_last_y&&!click)return;
 menu_have_pointer=1;menu_last_x=x;menu_last_y=y;
 for(unsigned i=0;i<menu_target_count;++i){
  UCRR_MENU_TARGET*t=&menu_targets[i];
  if(t->owner!=owner||t->index>=count||MEM32(owner+8+t->index*4)!=t->row||
    !menu_pointer_readable(t->row,0x20)||!MEM8(t->row+4)||
    (t->table&&MEM32(t->row)!=t->table)||!ucrr_menu_target_hit(t,x,y))continue;
  if((!t->action||t->action==0x16)&&(!MEM32(t->row+0xc)||MEM32(t->row+0xc)==0xffffffffu))continue;
  MEM32(selection)=t->index;
  if(click){menu_click_row=t->row;menu_click_action=t->action?t->action:0x16;}
  break;
 }
}
/* Called after the row's original font-width measurement. ESP is S-24. */
void ucrr_menu_row_bounds(void){
 if(!menu_owner||!menu_pointer_readable(menu_owner,0x5c)||!menu_pointer_readable(g_edi,0x20)||
   !menu_pointer_readable(g_esp,0x30))return;
 uint32_t count=MEM32(menu_owner+0x58);if(count>20)return;
 unsigned index;for(index=0;index<count;++index)if(MEM32(menu_owner+8+index*4)==g_edi)break;
 if(index==count)return;
 uint32_t font=MEM32(0x509704);if(!menu_pointer_readable(font,0x30))return;
 int face=(signed char)MEM8(font+0x2c);if(face<0||face>3)return;
 uint32_t metrics=MEM32(font+0x14+face*4);if(!menu_pointer_readable(metrics,4))return;
 float height=MEMF(metrics),width=MEMF(g_esp+0x24);
 if(!isfinite(height)||height<=0||height>120||!isfinite(width)||width<=0||width>640)return;
 if(!menu_render_count)menu_target_count=0;
 ++menu_render_count;menu_seen=1;
 if(menu_target_count<80){
  UCRR_MENU_TARGET t={menu_owner,g_edi,index,MEMF(g_esp+0x2c),MEMF(g_esp+0x20),width,height};
  menu_targets[menu_target_count++]=t;
 }
}
/* Options/sliders draw the original arrow primitives. Their x/y and size are
 * the same values used by 00259860; no inferred screen rows or value writes. */
void ucrr_menu_adjust_enter(void){menu_draw_row=menu_owner?g_ecx:0;}
void ucrr_menu_adjust_leave(void){menu_draw_row=0;}
void ucrr_menu_arrow_bounds(void){
 if(!menu_owner||!menu_draw_row||!menu_pointer_readable(menu_owner,0x5c)||
    !menu_pointer_readable(menu_draw_row,0x20)||!menu_pointer_readable(g_esp,0x18))return;
 uint32_t direction=MEM32(g_esp+4),count=MEM32(menu_owner+0x58);
 if((direction!=2&&direction!=3)||count>20)return;
 unsigned index;for(index=0;index<count;++index)if(MEM32(menu_owner+8+index*4)==menu_draw_row)break;
 if(index==count)return;
 float x=MEMF(g_esp+8),y=MEMF(g_esp+12),scale=MEMF(g_esp+20);
 float width=scale*MEMF(0x4859ec),half=width*scale*MEMF(0x44c1f8);
 if(!isfinite(x)||!isfinite(y)||!isfinite(width)||!isfinite(half)||width<=0||width>120||half<=0||half>120)return;
 if(!menu_render_count)menu_target_count=0;
 ++menu_render_count;menu_seen=1;
 if(menu_target_count<80){UCRR_MENU_TARGET target={menu_owner,menu_draw_row,index,direction==2?x-width:x,y-half,width,2*half,direction==2?0x20:0x21,MEM32(menu_draw_row)};menu_targets[menu_target_count++]=target;}
}
int ucrr_menu_click_query(void){
 if(menu_special_query())return 1;
 if(hf_back_query())return 1;
 if(level_owner&&level_click_owner==level_owner&&g_esi==level_owner&&menu_pointer_readable(g_esp,8)&&MEM32(g_esp)==0x00270B2Eu&&MEM32(g_esp+4)==0x16)return 1;
 /* Only the selected row's original action query sees its own click. */
 if(!menu_owner||!menu_click_row||g_esi!=menu_click_row||!menu_pointer_readable(g_esp,8)||MEM32(g_esp+4)!=menu_click_action)return 0;
 uint32_t caller=MEM32(g_esp);
 if(menu_click_action==0x16)return caller==0x00263452u;
 if(menu_click_action==0x20)return caller==0x00263CDBu||caller==0x002637d2u||caller==0x002648dcu||caller==0x0026512cu;
 if(menu_click_action==0x21)return caller==0x00263D08u||caller==0x002637feu||caller==0x00264962u||caller==0x002651b5u;
 return 0;
}

/* Campaign carousel: 270320 owns selection +1c and count +ec. Bounds are
 * captured from the actual thumbnail rectangle produced at 26F44D. */
void ucrr_level_enter(void){
 if(level_owner!=g_ecx){level_pointer_valid=level_hover_pending=0;level_target_count=0;menu_have_pointer=0;}
 level_owner=g_ecx;level_click_owner=0;level_render_count=0;level_draw_index=32;
 menu_seen=1;menu_render_count=1;
}
void ucrr_level_leave(void){hf_capture_levels(level_owner);level_click_owner=0;level_draw_index=32;}
int ucrr_level_hover(void){
 if(level_owner!=g_esi||!menu_pointer_readable(level_owner,0x118))return 0;
 unsigned count=MEM32(level_owner+0xec);if(!count||count>17)return 0;
 hf_eligible_owner=level_owner;int native_changed;if(hf_input(level_owner,UC_HF_LEVELS,&native_changed))return native_changed;
 float x,y;int click=0;
 if(!ucrr_host_menu_pointer(&x,&y,&click)){level_pointer_valid=level_hover_pending=0;return 0;}
 /* A moving carousel must not select successive tiles under a resting mouse.
  * Small pointer jitter is ignored; deliberate hover settles for 160 ms.
  * Clicks still select/confirm immediately through the original handler. */
 int moved=!level_pointer_valid||fabsf(x-level_anchor_x)>=4.0f||fabsf(y-level_anchor_y)>=4.0f;
 if(moved){level_pointer_valid=1;level_anchor_x=x;level_anchor_y=y;}
 unsigned selected=MEM32(level_owner+0x1c);
 if(level_hover_pending&&selected!=level_hover_selection)level_hover_pending=0;
 for(unsigned i=0;i<level_target_count;++i){
  UCRR_MENU_TARGET*t=&level_targets[i];
  if(t->owner!=level_owner||t->index>=count||MEM32(level_owner+0x20+12*t->index)!=t->row||!ucrr_menu_target_hit(t,x,y))continue;
  if(click){level_hover_pending=0;MEM32(level_owner+0x1c)=t->index;level_click_owner=level_owner;return selected!=t->index;}
  if(selected==t->index){level_hover_pending=0;return 0;}
  uint64_t now=ucrr_host_menu_milliseconds();
  if(!level_hover_pending||level_hover_index!=t->index){
   level_hover_pending=0;if(!moved)return 0;
   level_hover_pending=1;level_hover_index=t->index;level_hover_selection=selected;level_hover_since=now;return 0;
  }
  if(now-level_hover_since<160)return 0;
  level_hover_pending=0;level_anchor_x=x;level_anchor_y=y;
  MEM32(level_owner+0x1c)=t->index;return 1;
 }
 level_hover_pending=0;
 return 0;
}
void ucrr_level_thumb_begin(void){
 if(g_esi==level_owner&&menu_pointer_readable(g_esp,0x18))level_draw_index=MEM32(g_esp+0x14);
}
void ucrr_level_thumb_bounds(void){
 if(!level_owner||level_draw_index>=17||!menu_pointer_readable(level_owner,0x118)||!menu_pointer_readable(g_esp,0x30))return;
 float x=MEMF(g_esp+0x28),y=MEMF(g_esp+0x2c),w=MEMF(g_esp+0x20)-x,h=MEMF(g_esp+0x24)-y;
 if(!isfinite(x)||!isfinite(y)||!isfinite(w)||!isfinite(h)||w<=0||h<=0||w>640||h>480)return;
 if(!level_render_count++)level_target_count=0;
 if(level_target_count<32){UCRR_MENU_TARGET t={level_owner,MEM32(level_owner+0x20+12*level_draw_index),level_draw_index,x,y,w,h};level_targets[level_target_count++]=t;}
 hf_capture_levels(level_owner);
}
