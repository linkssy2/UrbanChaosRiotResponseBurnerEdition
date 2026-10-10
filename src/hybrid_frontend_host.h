/* Native presentation of CONTROL-owned screens. Guest callbacks/saves remain
 * authoritative; only copied rows and validated actions cross this boundary. */
#include "hybrid_frontend_guest.h"
#if defined(UCRR_HYBRID_PC_FRONTEND)
#include "hybrid_guest_cpu.h"
#include "migration_physics_ragdoll_pool.h"
#include "../native/runtime/control_frontend_bridge.h"
extern void d3d8_SetHostPresentation(void (*)(ID3D11Device*,ID3D11DeviceContext*,ID3D11RenderTargetView*,unsigned,unsigned));
extern void d3d8_SetHostPresentationSize(unsigned,unsigned);
extern unsigned d3d8_GetHostAnisotropy(void);
extern int d3d8_SaveHostAnisotropy(unsigned);
extern int uc_hybrid_scenery_lighting_visible(void);
extern void uc_hybrid_scenery_lighting_show(int);
extern int uc_hybrid_character_lighting_visible(void);
extern void uc_hybrid_character_lighting_show(int);
extern int uc_hybrid_character_lighting_quality(void);
extern void uc_hybrid_character_lighting_set_quality(int);
static UCNativeControlFrontend *hf_native;
static SRWLOCK hf_lock=SRWLOCK_INIT;
static UcHybridFrontendSnapshot hf_snapshot;
static UcHybridFrontendAction hf_actions[16];
static unsigned hf_action_read,hf_action_write;
static UCNativeFrontendInput hf_inputs[32];
static unsigned hf_input_read,hf_input_write;
static int hf_page,hf_list_page,hf_list_pages=1,hf_control_page,hf_device=1,hf_slot,hf_bind=-1,hf_bind_armed;
static unsigned hf_limit=60,hf_pad;
static uint64_t hf_identity,hf_pad_time,hf_menu_identity,hf_skip_until;
static unsigned hf_guest_selected=UINT32_MAX;
static LONG hf_published;
static int hf_failed,hf_movie,hf_movie_loop,hf_movie_title,hf_blocked,hf_presented_menu;
static char hf_status[512],hf_error[512];
enum { HF_GRAPHICS=100,HF_CONTROLS,HF_EFFECTS,HF_BACK,HF_PREVIOUS,HF_NEXT,
 HF_RESOLUTION,HF_FULLSCREEN,HF_ASPECT,HF_SCALE,HF_MSAA,HF_AF,HF_VSYNC,HF_LIMIT,HF_FPS,HF_LIGHTING,
 HF_DEVICE,HF_SLOT,HF_SENSITIVITY,HF_DEADZONE_L,HF_DEADZONE_R,HF_RESET,HF_RESET_YES,HF_RESET_NO,HF_CHARACTER_LIGHTING,HF_CHARACTER_QUALITY,
 HF_APPLY,HF_DISCARD,HF_QUIT_DESKTOP,HF_QUIT_YES,HF_QUIT_NO,HF_RAGDOLLS,HF_SAVE_LEAVE,HF_DISCARD_LEAVE,HF_CANCEL_LEAVE,
 HF_EFFECT_BASE=200,HF_BIND_BASE=300 };
/* Drafts are local to the PC settings pages. No setter runs until Apply. */
static unsigned hf_draft[207];static unsigned char hf_dirty[207];
static unsigned hf_bind_draft[2][25][2];static unsigned char hf_bind_dirty[2][25][2],hf_reset_pending[2];
static int hf_applying,hf_apply_ok,hf_leave_page;
static int hf_unsaved(void){for(unsigned i=0;i<207;i++)if(hf_dirty[i])return 1;for(unsigned d=0;d<2;d++){if(hf_reset_pending[d])return 1;for(unsigned a=0;a<25;a++)for(unsigned n=0;n<2;n++)if(hf_bind_dirty[d][a][n])return 1;}return 0;}
static unsigned hf_setting(unsigned id,unsigned current){return id<207&&hf_dirty[id]?hf_draft[id]:current;}
static unsigned hf_binding(unsigned device,unsigned action,unsigned slot){return hf_bind_dirty[device][action][slot]?hf_bind_draft[device][action][slot]:device?xbox_KeyboardBinding(action,slot):xbox_ControllerBinding(action,slot);}
static void hf_stage_binding(unsigned value){hf_bind_draft[hf_device][hf_bind][hf_slot]=value;hf_bind_dirty[hf_device][hf_bind][hf_slot]=1;snprintf(hf_status,sizeof(hf_status),"Pending. Press Apply to save.");}
static void hf_discard(void){memset(hf_dirty,0,sizeof(hf_dirty));memset(hf_bind_dirty,0,sizeof(hf_bind_dirty));memset(hf_reset_pending,0,sizeof(hf_reset_pending));hf_bind=-1;}
static int hf_configured(void){
 static int active=-1;if(active<0){const char *s=getenv("UCRR_HYBRID_PC_FRONTEND");active=s&&!strcmp(s,"1");}return active;
}
int ucrr_native_frontend_enabled(void){return hf_configured()&&!hf_failed;}
static void hf_fail(void){
 hf_failed=1;
 if(hf_native&&hf_movie)uc_native_control_frontend_movie_close(hf_native,NULL,0);
 hf_movie=0;hf_skip_until=0;hf_bind=-1;
 AcquireSRWLockExclusive(&hf_lock);memset(&hf_snapshot,0,sizeof(hf_snapshot));
 hf_input_read=hf_input_write=hf_action_read=hf_action_write=0;InterlockedExchange(&hf_published,0);ReleaseSRWLockExclusive(&hf_lock);
 if(hf_blocked){xbox_InputSetBlocked(FALSE);hf_blocked=0;pc_menu_apply_capture();}
 SetWindowTextA(pc_mouse_window,"Urban Chaos Hybrid - PC menu renderer unavailable; original menus active");
}
void ucrr_native_frontend_publish(const UcHybridFrontendSnapshot *s){
 if(!s||!ucrr_native_frontend_enabled())return;
 int released=0;
 /* Publish and unblock as one transaction; this acquire is balanced by the
  * unconditional release below, before posting to the window thread. */
 AcquireSRWLockExclusive(&hf_lock);
 if(hf_snapshot.epoch!=s->epoch||s->kind==UC_HF_NONE){hf_action_read=hf_action_write=0;hf_input_read=hf_input_write=0;}
 hf_snapshot=*s;InterlockedExchange(&hf_published,s->kind!=UC_HF_NONE);
 /* Revoke blocking before the next Present so original carousel clicks are
  * immediately usable. Cursor/capture changes stay on the window thread. */
 if(hf_snapshot.kind==UC_HF_NONE&&hf_blocked){hf_blocked=0;xbox_InputSetBlocked(FALSE);released=1;}
 ReleaseSRWLockExclusive(&hf_lock);
 if(released)PostMessageA(pc_mouse_window,UCRR_MENU_CURSOR_MESSAGE,0,0);
}
int ucrr_native_frontend_take_action(UcHybridFrontendAction *a){
 int ok=0;AcquireSRWLockExclusive(&hf_lock);
 if(a&&hf_action_read!=hf_action_write){*a=hf_actions[hf_action_read++%16];ok=1;}
 ReleaseSRWLockExclusive(&hf_lock);return ok;
}
static void hf_guest_action(const UcHybridFrontendSnapshot *s,unsigned index,unsigned kind){
 if(index>=s->count&&kind!=UC_HF_BACK)return;
 UcHybridFrontendAction a={s->epoch,s->screen,s->owner,index<s->count?s->rows[index].row:0,index,kind};
 AcquireSRWLockExclusive(&hf_lock);
 if(s->epoch==hf_snapshot.epoch){
  if(hf_action_write!=hf_action_read&&hf_actions[(hf_action_write-1)%16].kind==UC_HF_SELECT)
   hf_actions[(hf_action_write-1)%16]=a;
  else if(hf_action_write==hf_action_read)hf_actions[hf_action_write++%16]=a;
 }
 ReleaseSRWLockExclusive(&hf_lock);
}
static int hf_config_path(wchar_t path[MAX_PATH]){
 DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);wchar_t *p=n&&n<MAX_PATH?wcsrchr(path,L'\\'):NULL;
 if(!p||p-path+16>=MAX_PATH)return 0;wcscpy(p+1,L"xboxrecomp.ini");return 1;
}
static unsigned hf_get(const wchar_t *key,unsigned fallback){wchar_t p[MAX_PATH];return hf_config_path(p)?GetPrivateProfileIntW(L"Graphics",key,fallback,p):fallback;}
static int hf_put(const wchar_t *key,unsigned v){wchar_t p[MAX_PATH],text[24];swprintf(text,24,L"%u",v);return hf_config_path(p)&&WritePrivateProfileStringW(L"Graphics",key,text,p);}
static void hf_result(int ok,const char *success){snprintf(hf_status,sizeof(hf_status),"%s",ok?success:"Could not save this setting. Your previous setting is retained.");}
static unsigned hf_cycle(unsigned v,const unsigned *choices,unsigned count,int direction){
 unsigned i=0;while(i<count&&choices[i]!=v)++i;if(i==count)i=0;
 return choices[(i+count+(direction<0?-1:1))%count];
}
static void hf_presentation_size(void){
 d3d8_SetHostPresentationSize(xbox_fullscreen?xbox_sizes[xbox_resolution][0]:0,xbox_fullscreen?xbox_sizes[xbox_resolution][1]:0);
}
static void hf_display(void){
 hf_presentation_size();
 if(!xbox_fullscreen){RECT r={0,0,xbox_sizes[xbox_resolution][0],xbox_sizes[xbox_resolution][1]};
  AdjustWindowRect(&r,(DWORD)GetWindowLongPtrA(pc_mouse_window,GWL_STYLE),FALSE);
  SetWindowPos(pc_mouse_window,NULL,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER);}
 xbox_overlay_layout(pc_mouse_window);
}
static void hf_fullscreen(int on){
 if(on==xbox_fullscreen)return;xbox_fullscreen=on;hf_presentation_size();
 if(on){MONITORINFO m={sizeof(m)};GetWindowRect(pc_mouse_window,&xbox_windowed_rect);
  GetMonitorInfoA(MonitorFromWindow(pc_mouse_window,MONITOR_DEFAULTTONEAREST),&m);
  SetWindowLongPtrA(pc_mouse_window,GWL_STYLE,WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN);
  SetWindowPos(pc_mouse_window,NULL,m.rcMonitor.left,m.rcMonitor.top,m.rcMonitor.right-m.rcMonitor.left,m.rcMonitor.bottom-m.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOZORDER);
 }else{SetWindowLongPtrA(pc_mouse_window,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPCHILDREN);
  SetWindowPos(pc_mouse_window,NULL,xbox_windowed_rect.left,xbox_windowed_rect.top,xbox_windowed_rect.right-xbox_windowed_rect.left,xbox_windowed_rect.bottom-xbox_windowed_rect.top,SWP_FRAMECHANGED|SWP_NOZORDER);hf_display();}
}
static int hf_native_visible(void){return ucrr_native_frontend_enabled()&&!hf_failed&&InterlockedCompareExchange(&hf_published,0,0);}
static void hf_enqueue(unsigned key,int click){
 UCNativeFrontendInput input={0};input.key=key;input.click=!!click;
 RECT r;POINT p;if(!GetClientRect(xbox_output_window,&r)||r.right<=0||r.bottom<=0)return;
 input.client_width=r.right;input.client_height=r.bottom;
 if(!key){if(!GetCursorPos(&p)||!ScreenToClient(xbox_output_window,&p))return;input.x=(float)p.x;input.y=(float)p.y;}
 AcquireSRWLockExclusive(&hf_lock);
 if(!key&&!click&&hf_input_write!=hf_input_read&&!hf_inputs[(hf_input_write-1)%32].key&&!hf_inputs[(hf_input_write-1)%32].click)
  hf_inputs[(hf_input_write-1)%32]=input;
 else if(hf_input_write-hf_input_read<32)hf_inputs[hf_input_write++%32]=input;
 ReleaseSRWLockExclusive(&hf_lock);
}
/* Called before normal window input. Rebinding is polled after all activation
 * buttons release, so selecting a binding never binds the selecting click. */
static int hf_window_input(HWND w,UINT message,WPARAM key,LPARAM flags){
 (void)w;ucrr_host_pause_event(message,key,flags);if(!ucrr_native_frontend_enabled())return 0;
 if(message==WM_LBUTTONUP)xbox_KeyboardConsumeMouseClick(0);
 if(message==WM_SETCURSOR&&LOWORD(flags)==HTCLIENT&&!pc_mouse_captured){
  SetCursor(uc_native_control_frontend_cursor(pc_mouse_window));return 1;}
 if(hf_movie&&!hf_native_visible()){
  if(message==WM_KEYDOWN&&!(flags&(1LL<<30))&&(key==VK_RETURN||key==VK_SPACE||key==VK_ESCAPE))hf_skip_until=GetTickCount64()+500;
  if(message==WM_LBUTTONDOWN&&(!hf_movie_loop||hf_movie_title)){xbox_KeyboardConsumeMouseClick(1);hf_skip_until=GetTickCount64()+500;return 1;}
 }
 if(!hf_native_visible())return 0;
 if(message==WM_KILLFOCUS||message==WM_ACTIVATEAPP&&!key){
  hf_bind=-1;AcquireSRWLockExclusive(&hf_lock);hf_input_read=hf_input_write=0;hf_action_read=hf_action_write=0;ReleaseSRWLockExclusive(&hf_lock);return 0;}
 if(message==WM_KEYDOWN||message==WM_SYSKEYDOWN){
  if(flags&(1LL<<29))return 0;
  if(!(flags&(1LL<<30))){xbox_KeyboardNoteInput();
   if(hf_bind>=0){if(key==VK_ESCAPE)hf_bind=-1;else if(key==VK_DELETE&&hf_bind_armed){hf_stage_binding(0);hf_bind=-1;}}
   else hf_enqueue((unsigned)key,0);}
  return 1;
 }
 if(message==WM_MOUSEWHEEL&&hf_bind>=0&&hf_bind_armed&&hf_device){
  hf_stage_binding((SHORT)HIWORD(key)>0?256:257);hf_bind=-1;return 1;}
 if(message==WM_LBUTTONDOWN){xbox_KeyboardConsumeMouseClick(1);xbox_KeyboardNoteInput();if(hf_bind<0)hf_enqueue(0,1);return 1;}
 if(message==WM_MOUSEMOVE){if(hf_bind<0)hf_enqueue(0,0);return 1;}
 return message==WM_KEYUP||message==WM_SYSKEYUP||message==WM_RBUTTONDOWN||message==WM_MBUTTONDOWN;
}
static void hf_change(unsigned id,int direction){
 const unsigned af[]={1,2,4,8,16},limits[]={0,30,60,120};unsigned v=0;int ok=1;const char *saved="Saved.";
 if(!direction)direction=1;
 switch(id){
 case HF_DISCARD:hf_discard();saved="Pending changes discarded.";break;
 case HF_APPLY:
  int camera_restart=hf_dirty[HF_ASPECT]&&hf_draft[HF_ASPECT]!=(unsigned)xbox_aspect;hf_applying=1;hf_apply_ok=1;
  for(unsigned device=0;device<2;device++){
   if(hf_reset_pending[device]){int success=device?xbox_KeyboardReset():xbox_ControllerReset();if(success)hf_reset_pending[device]=0;else hf_apply_ok=0;}
   if(hf_reset_pending[device])continue;
   for(unsigned a=0;a<(device?25u:24u);a++)for(unsigned slot=0;slot<2;slot++)if(hf_bind_dirty[device][a][slot]){
    int success=device?xbox_KeyboardBind(a,slot,hf_bind_draft[device][a][slot]):xbox_ControllerBind(a,slot,hf_bind_draft[device][a][slot]);
    if(success)hf_bind_dirty[device][a][slot]=0;else hf_apply_ok=0;
   }
  }
  for(unsigned setting=0;setting<207;setting++)if(hf_dirty[setting])hf_change(setting,1);
  hf_applying=0;snprintf(hf_status,sizeof(hf_status),"%s",hf_apply_ok?(camera_restart?"Changes saved. Restart to update the game camera aspect.":"Changes applied and saved."):"Some changes could not be applied. Unapplied choices remain pending.");return;

 case HF_GRAPHICS:hf_page=1;break;case HF_CONTROLS:hf_page=2;break;case HF_EFFECTS:hf_page=3;break;
 case HF_SAVE_LEAVE:hf_change(HF_APPLY,0);if(hf_apply_ok)hf_page=hf_leave_page==3?1:0;return;
 case HF_DISCARD_LEAVE:hf_discard();hf_page=hf_leave_page==3?1:0;break;
 case HF_CANCEL_LEAVE:hf_page=hf_leave_page;break;
 case HF_BACK:
  hf_bind=-1;
  if(hf_page==6)hf_page=hf_leave_page;
  else if(hf_page==4)hf_page=2;
  else if(hf_page>=1&&hf_page<=3&&hf_unsaved()){hf_leave_page=hf_page;hf_page=6;}
  else hf_page=hf_page==3?1:0;
  break;
 case HF_QUIT_DESKTOP:hf_page=5;break;
 case HF_QUIT_NO:hf_page=0;break;
 case HF_QUIT_YES:if(hf_page==5&&!strcmp(hf_snapshot.title,"NewFrontEnd-MainMenu-1"))PostMessageA(pc_mouse_window,WM_CLOSE,0,0);break;
 case HF_PREVIOUS:if(hf_page==2)hf_control_page=(hf_control_page+4)%5;else hf_list_page=(hf_list_page+hf_list_pages-1)%hf_list_pages;break;
 case HF_NEXT:if(hf_page==2)hf_control_page=(hf_control_page+1)%5;else hf_list_page=(hf_list_page+1)%hf_list_pages;break;
 case HF_RESOLUTION:v=hf_applying?hf_draft[id]:(hf_setting(HF_RESOLUTION,xbox_resolution)+5+(direction<0?-1:1))%5;if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"WindowResolution",v);if(ok){xbox_resolution=v;hf_display();}break;
 case HF_FULLSCREEN:v=hf_applying?hf_draft[id]:!hf_setting(HF_FULLSCREEN,xbox_fullscreen);if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"Fullscreen",v);if(ok)hf_fullscreen(v);break;
 case HF_ASPECT:v=hf_applying?hf_draft[id]:(hf_setting(HF_ASPECT,xbox_aspect)+4+(direction<0?-1:1))%4;if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=d3d8_SaveHostAspect(v);if(ok){xbox_aspect=v;xbox_overlay_layout(pc_mouse_window);}saved="Saved. Restart to update the game's camera aspect.";break;
 case HF_SCALE:v=hf_applying?hf_draft[id]:(hf_setting(HF_SCALE,xbox_render_scale)+7+(direction<0?-1:1))%8+1;if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=d3d8_SaveHostRenderScale(v);if(ok)xbox_render_scale=v;saved="Internal resolution applied.";break;
 case HF_MSAA:{const unsigned samples[]={1,2,4};v=hf_applying?hf_draft[id]:hf_cycle(hf_setting(id,d3d8_GetHostMsaa()),samples,3,direction);if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=d3d8_SaveHostMsaa(v);break;}
 case HF_AF:v=hf_applying?hf_draft[id]:hf_cycle(hf_setting(HF_AF,d3d8_GetHostAnisotropy()),af,5,direction);if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=d3d8_SaveHostAnisotropy(v);break;
 case HF_VSYNC:v=hf_applying?hf_draft[id]:!hf_setting(HF_VSYNC,xbox_vsync);if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"VSync",v);if(ok){xbox_vsync=v;d3d8_SetHostVsync(v);}break;
 case HF_LIMIT:v=hf_applying?hf_draft[id]:hf_cycle(hf_setting(HF_LIMIT,hf_limit),limits,4,direction);if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"FrameLimit",v);if(ok)hf_limit=v;break;
 case HF_FPS:v=hf_applying?hf_draft[id]:!hf_setting(HF_FPS,xbox_fps);if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=d3d8_SaveFpsCounter(v);if(ok)xbox_fps=v;break;
 case HF_RAGDOLLS:v=hf_applying?hf_draft[id]:(hf_setting(id,uc_ragdoll_retention_get())+3+(direction<0?-1:1))%3;if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"RagdollRetention",v);if(ok)uc_ragdoll_retention_set(v);break;
 case HF_LIGHTING:v=hf_applying?hf_draft[id]:!hf_setting(HF_LIGHTING,uc_hybrid_scenery_lighting_visible());if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"NativeSceneryLighting",v);if(ok)uc_hybrid_scenery_lighting_show(v);break;
 case HF_CHARACTER_LIGHTING:v=hf_applying?hf_draft[id]:!hf_setting(HF_CHARACTER_LIGHTING,uc_hybrid_character_lighting_visible());if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"CharacterLighting",v);if(ok)uc_hybrid_character_lighting_show(v);break;
 case HF_CHARACTER_QUALITY:v=hf_applying?hf_draft[id]:!hf_setting(HF_CHARACTER_QUALITY,uc_hybrid_character_lighting_quality());if(!hf_applying){hf_draft[id]=v;hf_dirty[id]=1;break;}ok=hf_put(L"CharacterLightingQuality",v);if(ok)uc_hybrid_character_lighting_set_quality(v);break;
 case HF_DEVICE:hf_device=!hf_device;hf_control_page=0;break;case HF_SLOT:hf_slot=!hf_slot;break;
 case HF_SENSITIVITY:{int n=(int)hf_setting(id,xbox_MouseGetSensitivity())+(direction<0?-25:25);if(n<25)n=25;if(n>1600)n=1600;
  if(!hf_applying){hf_draft[id]=n;hf_dirty[id]=1;break;}n=hf_draft[id];ok=hf_put(L"MouseSensitivity",n);if(ok)xbox_MouseSetSensitivity(n);break;}
 case HF_DEADZONE_L:case HF_DEADZONE_R:{unsigned stick=id-HF_DEADZONE_L;int n=(int)hf_setting(id,xbox_ControllerDeadzone(stick))+(direction<0?-1:1);if(n<0)n=0;if(n>30)n=30;if(!hf_applying){hf_draft[id]=n;hf_dirty[id]=1;break;}n=hf_draft[id];ok=xbox_ControllerSetDeadzone(stick,n);break;}
 case HF_RESET:hf_page=4;break;case HF_RESET_NO:hf_page=2;break;
 case HF_RESET_YES:hf_reset_pending[hf_device]=1;memset(hf_bind_dirty[hf_device],0,sizeof(hf_bind_dirty[hf_device]));hf_page=2;saved="Defaults pending. Press Apply to restore.";break;
 default:
  if(id>=HF_BIND_BASE&&id<HF_BIND_BASE+25){hf_bind=(int)(id-HF_BIND_BASE);hf_bind_armed=0;saved="Release all buttons, then press an input. Esc cancels; Delete clears.";}
#ifdef UCRR_GRAPHICS_EFFECT_UI
  else if(id>=HF_EFFECT_BASE&&id<HF_EFFECT_BASE+7){v=(xbox_effects&(1u<<(id-HF_EFFECT_BASE)))?0:1;if(!hf_applying){hf_draft[id]=!hf_setting(id,!v);hf_dirty[id]=1;break;}v=hf_draft[id]?xbox_effects|(1u<<(id-HF_EFFECT_BASE)):xbox_effects&~(1u<<(id-HF_EFFECT_BASE));extern int uc_graphics_features_apply(unsigned);ok=uc_graphics_features_apply(v);if(ok)xbox_effects=v;saved="Effect visibility applied.";}
#endif
  break;
 }
 if(hf_applying){if(ok)hf_dirty[id]=0;else hf_apply_ok=0;}
 else if(id<207&&hf_dirty[id])saved="Pending. Press Apply to save.";
 hf_result(ok,saved);
 AcquireSRWLockExclusive(&hf_lock);hf_input_read=hf_input_write=0;ReleaseSRWLockExclusive(&hf_lock);
}
static void hf_menu(const UcHybridFrontendSnapshot *s){
 UCNativeFrontendRow rows[20]={0};char values[20][128]={{0}};unsigned count=0;
 UCNativeFrontendDetailText texts[32];UCNativeFrontendDetailIcon icons[32];
 UCNativeFrontendLevelDetails level={0};
#define HF_ROW(id_,name_,value_,help_,enabled_,adjust_) do{unsigned n=count++;rows[n]=(UCNativeFrontendRow){id_,enabled_,adjust_,0,name_,values[n],help_};snprintf(values[n],128,"%s",value_);}while(0)
#define HF_NUMBER(id_,name_,v_,help_) do{unsigned n=count;HF_ROW(id_,name_,"",help_,1,1);snprintf(values[n],128,"%u",(unsigned)(v_));}while(0)
 const char *title=s->title[0]?s->title:s->kind==UC_HF_LEVELS?"Select assignment":"Options";
 unsigned selected=UINT32_MAX;
 if(hf_page==1){
  title="Graphics";unsigned n=count;HF_ROW(HF_RESOLUTION,"Resolution","","Output resolution. Fullscreen scales this image to your monitor.",1,1);snprintf(values[n],128,"%d x %d",xbox_sizes[hf_setting(HF_RESOLUTION,xbox_resolution)][0],xbox_sizes[hf_setting(HF_RESOLUTION,xbox_resolution)][1]);
  HF_ROW(HF_FULLSCREEN,"Display mode",hf_setting(HF_FULLSCREEN,xbox_fullscreen)?"Fullscreen":"Windowed","Borderless fullscreen uses your monitor's resolution.",1,1);
  static const char *aspects[]={"Original 4:3","Widescreen 16:9","Stretch 16:9","Fill"};HF_ROW(HF_ASPECT,"Aspect ratio",aspects[hf_setting(HF_ASPECT,xbox_aspect)],"Camera changes apply after restart.",1,1);
  HF_NUMBER(HF_SCALE,"Internal resolution scale",hf_setting(HF_SCALE,xbox_render_scale),"Multiplies the original rendering resolution. Press Apply to activate.");
  HF_ROW(HF_MSAA,"MSAA",hf_setting(HF_MSAA,d3d8_GetHostMsaa())==1?"Off":hf_setting(HF_MSAA,d3d8_GetHostMsaa())==2?"2x":"4x","Off disables multisample anti-aliasing. Press Apply to activate.",1,1);
  HF_NUMBER(HF_AF,"Anisotropic filtering",hf_setting(HF_AF,d3d8_GetHostAnisotropy()),"Sharpens eligible world textures at oblique angles.");
  HF_ROW(HF_VSYNC,"VSync",hf_setting(HF_VSYNC,xbox_vsync)?"On":"Off","Synchronize presentation to the monitor.",1,1);
  HF_NUMBER(HF_LIMIT,"FPS limit",hf_setting(HF_LIMIT,hf_limit),"0 is uncapped presentation. The original simulation clock stays unchanged.");
  HF_ROW(HF_FPS,"FPS counter",hf_setting(HF_FPS,xbox_fps)?"On":"Off","Display the actual presentation rate.",1,1);
  HF_ROW(HF_LIGHTING,"Dynamic scenery lighting",hf_setting(HF_LIGHTING,uc_hybrid_scenery_lighting_visible())?"On":"Off","Scenery lights: native on Westside, original on other missions. The flashlight stays enabled.",1,1);
  HF_ROW(HF_CHARACTER_LIGHTING,"World character lighting",hf_setting(HF_CHARACTER_LIGHTING,uc_hybrid_character_lighting_visible())?"On":"Off","NPCs and movable objects. Your gun, hands and flashlight model keep original lighting in every setting.",1,1);
  {static const char *names[]={"Default","More","Forever"};HF_ROW(HF_RAGDOLLS,"Ragdoll amount",names[hf_setting(HF_RAGDOLLS,uc_ragdoll_retention_get())],"Default: 5 bodies / 30 seconds. More: 32 / 120 seconds. Forever: no age or count eviction until leaving the mission; may reduce performance.",1,1);}
  HF_ROW(HF_CHARACTER_QUALITY,"Character lighting quality",hf_setting(HF_CHARACTER_QUALITY,uc_hybrid_character_lighting_quality())?"Simple":"Full","Simple keeps base shading, directional light and flashlight on world characters and movable models. Removes local point-light glow from fires and explosions. Your gun and hands keep full lighting.",1,1);
  HF_ROW(HF_EFFECTS,"Effects","","Choose which original effects are rendered.",1,0);HF_ROW(HF_BACK,"Back","","",1,0);
 }else if(hf_page==2){
  title="Controls";HF_ROW(HF_DEVICE,"Input device",hf_device?"Keyboard and mouse":"Controller","Each action keeps two independent bindings.",1,1);
  HF_ROW(HF_SLOT,"Binding slot",hf_slot?"Alternate":"Primary","Select the binding slot to edit.",1,1);
  unsigned max=hf_device?25:24,start=hf_control_page*5;
  for(unsigned a=start;a<start+5&&a<max;a++){unsigned n=count,v=hf_binding(hf_device,a,hf_slot);
   HF_ROW(HF_BIND_BASE+a,xbox_action_names[a],hf_bind==(int)a?"Press input...":hf_reset_pending[hf_device]&&!hf_bind_dirty[hf_device][a][hf_slot]?"Default (pending)":hf_device?xbox_KeyboardSourceName(v):xbox_ControllerSourceName(v),xbox_action_descriptions[a],1,0);rows[n].keycap=hf_device&&v<=257&&!hf_reset_pending[hf_device]?v:0;}
  if(hf_device)HF_NUMBER(HF_SENSITIVITY,"Mouse sensitivity",hf_setting(HF_SENSITIVITY,xbox_MouseGetSensitivity()),"25 to 1600 percent.");
  else{HF_NUMBER(HF_DEADZONE_L,"Movement deadzone",hf_setting(HF_DEADZONE_L,xbox_ControllerDeadzone(0)),"Percent of stick travel ignored.");HF_NUMBER(HF_DEADZONE_R,"Look deadzone",hf_setting(HF_DEADZONE_R,xbox_ControllerDeadzone(1)),"Percent of stick travel ignored.");}
  HF_ROW(HF_PREVIOUS,"Previous controls","","",1,0);HF_ROW(HF_NEXT,"Next controls","","",1,0);
  HF_ROW(HF_RESET,"Restore defaults","","Restore this device's bindings after confirmation.",1,0);HF_ROW(HF_BACK,"Back","","",1,0);
 }else if(hf_page==3){
  title="Graphics - effects";
#ifdef UCRR_GRAPHICS_EFFECT_UI
  static const char *names[]={"All particles","Snow / smoke particles","Character blob shadows","Dynamic decals","Point-light stencil shadows","Hidden particle prep","Sky models"};
  static const char *help[]={
   "Enables the original particle rendering paths.",
   "Snow and smoke subset; All particles must also be on.",
   "Character blob shadows only; other shadow paths are unchanged.",
   "Original dynamic decal rendering.",
   "Original point-light stencil shadows only. Native scenery and flashlight shadows stay enabled.",
   "Only affects work when All particles is off. Disable both to skip hidden particle preparation.",
   "Original sky model rendering."};
  for(unsigned i=0;i<7;i++)HF_ROW(HF_EFFECT_BASE+i,names[i],hf_setting(HF_EFFECT_BASE+i,!!(xbox_effects&(1u<<i)))?"On":"Off",help[i],uc_graphics_features_enabled(),1);
#endif
  HF_ROW(HF_BACK,"Back","","",1,0);
 }else if(hf_page==6){title="Apply your changes?";HF_ROW(HF_SAVE_LEAVE,"Apply and go back","","Save your pending settings.",1,0);HF_ROW(HF_DISCARD_LEAVE,"Discard and go back","","Keep the previous settings.",1,0);HF_ROW(HF_CANCEL_LEAVE,"Cancel","","Return to your pending settings.",1,0);
 }else if(hf_page==5){title="Are you sure you want to quit?";HF_ROW(HF_QUIT_YES,"Yes","","Close Burner Edition and return to your desktop.",1,0);HF_ROW(HF_QUIT_NO,"No","","Stay in the game.",1,0);
  if(hf_menu_identity!=(s->epoch^((uint64_t)5<<56)^((uint64_t)hf_control_page<<48)^((uint64_t)hf_list_page<<40)))selected=1;
 }else if(hf_page==4){title="Restore controls?";HF_ROW(HF_RESET_YES,"Restore defaults","","Only the selected device's bindings will reset.",1,0);HF_ROW(HF_RESET_NO,"Cancel","","",1,0);
 }else if(s->kind==UC_HF_LEVELS){
  if(s->count>17||s->text_count<2||s->text_count>32||s->icon_count>32){hf_fail();return;}
  hf_list_page=0;
  for(unsigned i=0;i<s->count;i++){const UcHybridFrontendRow *r=&s->rows[i];HF_ROW(i,r->label,"","",r->enabled,0);rows[i].artwork_id=r->artwork_id;rows[i].completed=r->completed;}
  for(unsigned i=0;i<s->text_count;i++){const UcHybridFrontendDetailText *t=&s->texts[i];
   texts[i]=(UCNativeFrontendDetailText){t->flags,t->color,t->font,t->x,t->y,t->prefix,t->suffix};}
  for(unsigned i=0;i<s->icon_count;i++){const UcHybridFrontendDetailIcon *t=&s->icons[i];
   icons[i]=(UCNativeFrontendDetailIcon){t->kind,t->color,t->x,t->y};}
  level=(UCNativeFrontendLevelDetails){s->text_count,s->icon_count,texts,icons,s->level_actions,s->level_difficulty,s->level_medals};
  if(hf_guest_selected!=s->selected||hf_menu_identity!=s->epoch)selected=s->selected;
  hf_guest_selected=s->selected;
 }else{
  hf_list_pages=s->count?(s->count+11)/12:1;
  unsigned start=hf_list_page*12;if(start>=s->count)start=hf_list_page=0;
  for(unsigned i=start;i<s->count&&i<start+12;i++){
   const UcHybridFrontendRow *r=&s->rows[i];HF_ROW(i,r->label,r->value,"",r->enabled,!!(r->actions&(UC_HF_ROW_LEFT|UC_HF_ROW_RIGHT)));
  }
  if(s->count>12){HF_ROW(HF_PREVIOUS,"Previous","","",1,0);HF_ROW(HF_NEXT,"Next","","",1,0);}
  if(s->back_action&&(s->kind==UC_HF_PROFILES||s->kind==UC_HF_REWARD_BROWSER))HF_ROW(HF_BACK,"Back","","",1,0);
  if(!strcmp(s->title,"NewFrontEnd-MainMenu-1")||!strcmp(s->title,"NewFrontEnd-Paused-1")){
   HF_ROW(HF_GRAPHICS,"Graphics","","Display, frame rate and effects.",1,0);HF_ROW(HF_CONTROLS,"Controls","","Keyboard, mouse and controller bindings.",1,0);
  }
  if(!strcmp(s->title,"NewFrontEnd-MainMenu-1"))HF_ROW(HF_QUIT_DESKTOP,"Quit to Desktop","","Close the game after confirmation.",1,0);
  if(s->selected>=start&&s->selected<start+12&&
      (hf_guest_selected!=s->selected||hf_menu_identity!=(s->epoch^((uint64_t)hf_list_page<<40))))selected=s->selected-start;
  hf_guest_selected=s->selected;
 }
 if(hf_page>=1&&hf_page<=3){HF_ROW(HF_APPLY,"Apply","","Save and apply pending settings.",1,0);HF_ROW(HF_DISCARD,"Discard changes","","Keep the currently applied settings.",1,0);}
 UCNativeFrontendMenu menu={s->epoch^((uint64_t)hf_page<<56)^((uint64_t)hf_control_page<<48)^((uint64_t)hf_list_page<<40),
  (level.text_count?UC_NATIVE_FRONTEND_LEVEL_BROWSER:s->kind==UC_HF_REWARD_BROWSER&&!hf_page?UC_NATIVE_FRONTEND_REWARDS_BROWSER:0)|
  ((s->source_kind&0x80000000u)||(s->source_kind&0x18)==0x10||!strcmp(s->title,"NewFrontEnd-Paused-1")?UC_NATIVE_FRONTEND_OVER_GAME:0),selected,count,title,hf_status,
  level.text_count?"HOVER TO BROWSE   /   CLICK TO DEPLOY   /   Q OR ESC TO GO BACK":hf_page==3?"PRESS APPLY TO SAVE CHANGES   /   Q OR ESC TO GO BACK":"CLICK TO SELECT   /   ARROWS TO CHANGE   /   Q OR ESC TO GO BACK",rows,level.text_count?&level:NULL};
 if(!uc_native_control_frontend_menu(hf_native,&menu,hf_error,sizeof(hf_error)))hf_fail();
 hf_menu_identity=menu.identity;
#undef HF_NUMBER
#undef HF_ROW
}
static void hf_event(const UcHybridFrontendSnapshot *s,const UCNativeFrontendInput *input,unsigned confirm_port){
 UCNativeFrontendEvent event={0};if(!uc_native_control_frontend_input(hf_native,input,&event,hf_error,sizeof(hf_error)))return;
 if(event.kind==UC_NATIVE_FRONTEND_BACK){if(hf_page){hf_change(HF_BACK,0);}else if(s->back_action)hf_guest_action(s,0,UC_HF_BACK);}
 else if(event.kind==UC_NATIVE_FRONTEND_ACTIVATE){
  if(event.id==HF_BACK&&!hf_page&&s->back_action)hf_guest_action(s,0,UC_HF_BACK);
  else if(event.id>=100)hf_change(event.id,event.direction);
  else{
   /* Arm before publishing: the guest can consume this action immediately. */
   if(!event.direction&&confirm_port<4)xbox_ControllerConsumeConfirm(confirm_port);
   hf_guest_action(s,event.id,event.direction?(event.direction<0?UC_HF_LEFT:UC_HF_RIGHT):UC_HF_ACTIVATE);
  }
 }else if((event.kind==UC_NATIVE_FRONTEND_DIFFICULTY||event.kind==UC_NATIVE_FRONTEND_REWARDS)&&s->kind==UC_HF_LEVELS&&!hf_page){
  const unsigned mask=event.kind==UC_NATIVE_FRONTEND_DIFFICULTY?1:2;
  if(s->level_actions&mask)hf_guest_action(s,event.index,event.kind==UC_NATIVE_FRONTEND_DIFFICULTY?UC_HF_DIFFICULTY:UC_HF_REWARDS);
 }else if(event.kind==UC_NATIVE_FRONTEND_HOVER&&event.id<100)hf_guest_action(s,event.id,UC_HF_SELECT);
 if(event.kind==UC_NATIVE_FRONTEND_ACTIVATE||event.kind==UC_NATIVE_FRONTEND_BACK||event.kind==UC_NATIVE_FRONTEND_DIFFICULTY||event.kind==UC_NATIVE_FRONTEND_REWARDS){
  AcquireSRWLockExclusive(&hf_lock);hf_input_read=hf_input_write=0;ReleaseSRWLockExclusive(&hf_lock);
 }
}
static void hf_present_body(ID3D11Device *device,ID3D11DeviceContext *context,ID3D11RenderTargetView *view,unsigned width,unsigned height){
 if(!ucrr_native_frontend_enabled())return;
 hf_presentation_size(); /* Also follows the existing F11 fullscreen shortcut. */
 if(hf_failed){if(hf_blocked){xbox_InputSetBlocked(FALSE);hf_blocked=0;}return;}
 if(!hf_native){wchar_t game[MAX_PATH];if(!MultiByteToWideChar(CP_UTF8,0,UCRR_GAME_DIR,-1,game,MAX_PATH))return;
  hf_native=uc_native_control_frontend_create(device,context,game,hf_error,sizeof(hf_error));if(!hf_native){hf_fail();return;}}
 uc_native_control_frontend_pace(hf_native,hf_limit,hf_error,sizeof(hf_error));
 UcHybridFrontendSnapshot s;int released=0;
 AcquireSRWLockExclusive(&hf_lock);s=hf_snapshot;
 int active=s.kind!=UC_HF_NONE;
 if(active!=hf_blocked){hf_blocked=active;xbox_InputSetBlocked(active);released=!active;}
 ReleaseSRWLockExclusive(&hf_lock);
 if(released)PostMessageA(pc_mouse_window,UCRR_MENU_CURSOR_MESSAGE,0,0);
 const int transition=!active&&(s.source_kind&UC_HF_TRANSITION)&&hf_presented_menu==1;
 if(!active&&!transition&&hf_presented_menu&&!pc_menu_active()){UCNativeFrontendMenu closed={0};closed.title=closed.status=closed.footer="";closed.selected=UINT32_MAX;
  closed.flags=(s.source_kind&0x80000000u)?UC_NATIVE_FRONTEND_OVER_GAME:0;
  if(!uc_native_control_frontend_menu(hf_native,&closed,hf_error,sizeof(hf_error))){hf_fail();return;}hf_presented_menu=closed.flags?2:0;}
 if(active)hf_presented_menu=1;
 if(hf_identity!=s.epoch){hf_identity=s.epoch;hf_page=hf_list_page=0;hf_bind=-1;hf_status[0]=0;}
 if(active){
  hf_menu(&s);
  if(hf_failed)return;
  if(GetForegroundWindow()==pc_mouse_window){
   unsigned pad_port=UINT32_MAX,pad=xbox_ControllerCapturePort(&pad_port);
   if(hf_bind>=0){unsigned input=hf_device?xbox_KeyboardCapture():pad;
    if(!input)hf_bind_armed=1;else if(hf_bind_armed){hf_stage_binding(input);hf_bind=-1;}
   }else{
    if(pad&&(!hf_pad||pad!=hf_pad||pad>=13&&GetTickCount64()-hf_pad_time>240)){
     unsigned key=pad==1?VK_RETURN:pad==2?VK_ESCAPE:pad==13||pad==19?VK_UP:pad==14||pad==20?VK_DOWN:pad==15||pad==17?VK_LEFT:pad==16||pad==18?VK_RIGHT:0;
     if(!key&&s.kind==UC_HF_LEVELS&&!hf_page)key=pad==3?'R':pad==4?'D':0;
     if(key){UCNativeFrontendInput input={key};hf_event(&s,&input,pad==1?pad_port:UINT32_MAX);}hf_pad_time=GetTickCount64();}
    for(unsigned n=0;n<32;n++){UCNativeFrontendInput input;int have=0;AcquireSRWLockExclusive(&hf_lock);
     if(hf_input_read!=hf_input_write){input=hf_inputs[hf_input_read++%32];have=1;}ReleaseSRWLockExclusive(&hf_lock);
     if(!have)break;hf_event(&s,&input,UINT32_MAX);}
   }hf_pad=pad;
  }
  hf_menu(&s);
  if(hf_failed)return;
 }
 /* Keep the last native presentation through the source fade, without
  * processing input or retaining the outgoing guest callback ownership. */
 if(active||transition){RECT client;GetClientRect(xbox_output_window,&client);D3D11_VIEWPORT viewport={0,0,(float)width,(float)height,0,1};
  if(client.right>0&&client.bottom>0&&uc_native_control_frontend_draw(hf_native,view,&viewport,client.right,client.bottom,transition?3:1,hf_error,sizeof(hf_error))!=1)hf_fail();}
}
static void hf_present(ID3D11Device *device,ID3D11DeviceContext *context,ID3D11RenderTargetView *view,unsigned width,unsigned height){
 HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);
 hf_present_body(device,context,view,width,height);restore_cpu(&cpu);
}
static void hf_initialize(void){
 if(!ucrr_native_frontend_enabled())return;
 unsigned resolution=hf_get(L"WindowResolution",2);xbox_resolution=resolution<5?resolution:2;
 unsigned limit=hf_get(L"FrameLimit",60);hf_limit=limit==0||limit==30||limit==60||limit==120?limit:60;
 xbox_vsync=!!hf_get(L"VSync",xbox_vsync);d3d8_SetHostVsync(xbox_vsync);hf_display();hf_fullscreen(!!hf_get(L"Fullscreen",0));
 xbox_MouseSetSensitivity(hf_get(L"MouseSensitivity",xbox_MouseGetSensitivity()));
 uc_ragdoll_retention_set(hf_get(L"RagdollRetention",0)<=2?hf_get(L"RagdollRetention",0):0);
 uc_hybrid_scenery_lighting_show(!!hf_get(L"NativeSceneryLighting",1));
 uc_hybrid_character_lighting_show(!!hf_get(L"CharacterLighting",1));
 uc_hybrid_character_lighting_set_quality(hf_get(L"CharacterLightingQuality",0)==1);
 d3d8_SetHostPresentation(hf_present);
}
static void hf_shutdown(void){d3d8_SetHostPresentation(NULL);uc_native_control_frontend_destroy(hf_native);hf_native=NULL;}
extern int uc_hybrid_frontend_draw_movie(void (*)(ID3D11Device*,ID3D11DeviceContext*,ID3D11RenderTargetView*,const D3D11_VIEWPORT*));
int ucrr_native_frontend_movie_open(const char *source,int loop,float volume){
 if(!ucrr_native_frontend_enabled()||hf_failed||!hf_native||!source||!isfinite(volume)||volume<0||volume>1)return 0;
 const char *base=source;for(const char *p=source;*p;p++)if(*p=='/'||*p=='\\')base=p+1;
 static const char *names[]={"dolby","eidoslogo","rocklogo","intro_12_edit","gamelogo","splash","fe_bk1","intro_1"};
 const char *name=NULL;for(unsigned i=0;i<8;i++){size_t n=strlen(names[i]);if(!_strnicmp(base,names[i],n)&&!_stricmp(base+n,".xmv")){name=names[i];break;}}
 if(!name)return 0;
 wchar_t path[MAX_PATH],directory[MAX_PATH];DWORD n=GetEnvironmentVariableW(L"UCRR_HYBRID_MOVIES",directory,MAX_PATH);
 if(!n||n+strlen(name)+6>=MAX_PATH||swprintf(path,MAX_PATH,L"%ls/%hs.wmv",directory,name)<0||GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES)return 0;
 if(!uc_native_control_frontend_movie_open(hf_native,path,!!loop,volume,hf_error,sizeof(hf_error)))return 0;
 hf_movie=1;hf_movie_loop=!!loop;hf_movie_title=!strcmp(name,"gamelogo")||!strcmp(name,"splash");hf_skip_until=0;return 1;
}
int ucrr_native_frontend_movie_tick(void){
 if(hf_failed||!hf_movie||!hf_native)return 0;
 int state=uc_native_control_frontend_movie_tick(hf_native,hf_error,sizeof(hf_error));if(state<0)hf_fail();return state>0;
}
static void hf_movie_target(ID3D11Device *d,ID3D11DeviceContext *c,ID3D11RenderTargetView *v,const D3D11_VIEWPORT *p){
 (void)d;(void)c;RECT client;GetClientRect(xbox_output_window,&client);
 if(client.right<=0||client.bottom<=0)return;
 if(uc_native_control_frontend_draw(hf_native,v,p,client.right,client.bottom,2 /* Fill intro footage; retain the existing stretched title framing. */,hf_error,sizeof(hf_error))!=1)hf_fail();
}
void ucrr_native_frontend_movie_draw(void){if(hf_movie&&hf_native&&!hf_failed&&!uc_hybrid_frontend_draw_movie(hf_movie_target))hf_fail();}
void ucrr_native_frontend_movie_close(void){if(hf_native&&hf_movie)uc_native_control_frontend_movie_close(hf_native,hf_error,sizeof(hf_error));hf_movie=0;hf_skip_until=0;}
void ucrr_native_frontend_movie_volume(float volume){if(hf_native&&hf_movie)uc_native_control_frontend_movie_volume(hf_native,volume,hf_error,sizeof(hf_error));}
int ucrr_native_frontend_movie_take_skip(void){int pressed=hf_movie&&hf_skip_until&&GetTickCount64()<=hf_skip_until;hf_skip_until=0;return pressed;}
#else
static int hf_configured(void){return 0;}
int ucrr_native_frontend_enabled(void){return 0;}
void ucrr_native_frontend_publish(const UcHybridFrontendSnapshot *s){(void)s;}
int ucrr_native_frontend_take_action(UcHybridFrontendAction *a){(void)a;return 0;}
static int hf_window_input(HWND w,UINT m,WPARAM k,LPARAM f){(void)w;(void)m;(void)k;(void)f;return 0;}
static void hf_initialize(void){}
static void hf_shutdown(void){}
int ucrr_native_frontend_movie_open(const char *p,int loop,float volume){(void)p;(void)loop;(void)volume;return 0;}
int ucrr_native_frontend_movie_tick(void){return 0;}
void ucrr_native_frontend_movie_draw(void){}
void ucrr_native_frontend_movie_close(void){}
void ucrr_native_frontend_movie_volume(float v){(void)v;}
int ucrr_native_frontend_movie_take_skip(void){return 0;}
#endif
