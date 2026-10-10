#ifndef UCRR_XBOX_OVERLAY_H
#define UCRR_XBOX_OVERLAY_H
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib")
#include "../third_party/xboxrecomp/include/host_controller.h"
#if defined(UCRR_LIGHT_REMOVAL)
#include "light_removal.h"
static int xbox_lighting,xbox_pending_lighting;
static int xbox_lighting_path(wchar_t *path){
 DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);if(!n||n>=MAX_PATH)return 0;
 wchar_t *name=wcsrchr(path,L'\\');if(!name||name-path+16>=MAX_PATH)return 0;
 wcscpy(name+1,L"xboxrecomp.ini");return 1;
}
static void xbox_lighting_load(int native_frontend){
 xbox_lighting=xbox_pending_lighting=uc_light_removal_requested_lighting();
 if(uc_light_removal_live_toggle_enabled()){
  /* Native Graphics owns receiver filtering; the retired overlay must not suppress its light sources. */
  wchar_t path[MAX_PATH];int enabled=native_frontend||(xbox_lighting_path(path)&&GetPrivateProfileIntW(L"Graphics",L"DynamicLightingEnabled",0,path)==1);
  if(uc_light_removal_request_lighting(enabled))xbox_lighting=xbox_pending_lighting=enabled;
 }
}
static int xbox_lighting_save(int enabled){
 wchar_t path[MAX_PATH];if(!uc_light_removal_live_toggle_enabled()||!xbox_lighting_path(path))return 0;
 if(!WritePrivateProfileStringW(L"Graphics",L"DynamicLightingEnabled",enabled?L"1":L"0",path))return 0;
 if(!uc_light_removal_request_lighting(enabled)){
  WritePrivateProfileStringW(L"Graphics",L"DynamicLightingEnabled",xbox_lighting?L"1":L"0",path);return 0;
 }
 xbox_lighting=enabled;return 1;
}
#endif
#if defined(UCRR_FEATURE_REMOVAL) && defined(UCRR_DECAL_REMOVAL) && defined(UCRR_SHADOW_REMOVAL) && defined(UCRR_LIGHT_REMOVAL)
#define UCRR_GRAPHICS_EFFECT_UI 1
#include "graphics_features.h"
static unsigned xbox_effects=127,xbox_pending_effects=127;
static const char *xbox_effect_names[7]={"All particles (restart)","Snow / smoke particles (restart)","Character shadows (restart)","Dynamic decals (restart)","Dynamic shadows (restart)","Hidden particle prep (restart)","Sky models (restart)"};
#endif
extern int d3d8_GetFpsCounter(void);
extern int d3d8_SaveFpsCounter(int);
static int xbox_fps,xbox_pending_fps;
/* Native host controls. Never alter guest surfaces, projection or clocks. */
extern void d3d8_SetHostVsync(int enabled);
extern unsigned d3d8_GetHostRenderScale(void);
extern int d3d8_SaveHostRenderScale(unsigned scale);
extern unsigned d3d8_GetHostAspect(void);
extern int d3d8_SaveHostAspect(unsigned aspect);
extern unsigned d3d8_GetHostMsaa(void);
extern int d3d8_SaveHostMsaa(unsigned samples);
static unsigned xbox_msaa=1,xbox_pending_msaa=1;
static unsigned xbox_render_scale=1,xbox_pending_render_scale=1;
extern void d3d8_SetHostMessageFilter(int (*filter)(MSG*));
int ucrr_overlay_message(MSG*msg);
extern void xbox_InputSetBlocked(BOOL blocked);
extern void xbox_MouseSetSensitivity(unsigned percent);
extern unsigned xbox_MouseGetSensitivity(void);
static HWND xbox_overlay_window,xbox_output_window;
static int xbox_overlay_open,xbox_aspect,xbox_resolution,xbox_fullscreen,xbox_vsync=1;
static RECT xbox_windowed_rect;
static HFONT xbox_overlay_font,xbox_overlay_title;
/* A compositor-blended CHILD, not an owned popup or a copied game screenshot. */
static int xbox_ui_scale=1000,xbox_ui_width=760,xbox_ui_height=570;
static HFONT xbox_button_font,xbox_button_title,xbox_tab_font;
static const char *xbox_font_family="Segoe UI";
static int xbox_px(int n){return MulDiv(n,xbox_ui_scale,1000);}
static void xbox_place(int id,int x,int y,int w,int h){MoveWindow(GetDlgItem(xbox_overlay_window,id),xbox_px(x),xbox_px(y),xbox_px(w),xbox_px(h),TRUE);}
static int xbox_tab=1,xbox_pending_aspect,xbox_pending_resolution,xbox_pending_fullscreen,xbox_pending_vsync=1;
static int xbox_close_pending; /* requested close/tab destination, or zero */
static int xbox_graphics_scroll,xbox_graphics_help=1801;
static void xbox_graphics_layout(void);
static int xbox_control_page,xbox_control_device=1; /* first visible row / keyboard first */
static int xbox_control_rows=6,xbox_control_help;
static void xbox_bindings_layout(void);
static void xbox_bindings_scroll(int value);
static int xbox_bind_action=-1,xbox_bind_slot,xbox_bind_armed;
static unsigned xbox_last_ui_pad;
static int xbox_control_action_count(void){return xbox_control_device?(int)xbox_KeyboardActionCount():UCRR_CONTROLLER_ACTIONS;}
static unsigned xbox_binding(unsigned a,unsigned s){return xbox_control_device?xbox_KeyboardBinding(a,s):xbox_ControllerBinding(a,s);}
static int xbox_bind(unsigned a,unsigned s,unsigned v){return xbox_control_device?xbox_KeyboardBind(a,s,v):xbox_ControllerBind(a,s,v);}
/* Default Xbox actions, verified against the retail manual. Guest control
 * presets still resolve actions; these rows retain physical-slot identities. */
static const char *xbox_action_names[UCRR_KEYBOARD_ACTIONS]={"Reload / interact","Melee weapons","Firearms","Thrown weapons","Secondary / flashlight","Thermal breather","Raise shield","Fire / attack","Pause / start","Objectives","Crouch","Zoom","Order / menu up","Orders / menu down","Follow / menu left","Cover / menu right","Move left","Move right","Move forward","Move backward","Look left","Look right","Look up","Look down","Teammate orders"};
static const char *xbox_action_descriptions[UCRR_KEYBOARD_ACTIONS]={
 "Tap to reload, use the nearby object, or skip a cutscene. In menus: confirm the selection. Xbox A.",
 "Tap to select/cycle melee weapons or the stun gun; hold for quick use. In menus: go back or cancel. Xbox B.",
 "Tap to equip a firearm; tap again to cycle your firearms. On mission selection: view rewards. Xbox X.",
 "Tap to select/cycle thrown weapons; hold for a quick throw. On mission selection: change difficulty. Xbox Y.",
 "Tap for the equipped weapon's alternate action: flashlight, fire-mode change, grenade roll, or close attack. Xbox Black.",
 "Tap to put on or remove the thermal breather when available. Xbox White.",
 "Hold to protect yourself with the shield; release to lower it. Fire attacks with the shield while raised. Xbox left trigger.",
 "Fire or attack with the equipped weapon. Hold to use the stun gun or charge a throw; release to throw. Xbox right trigger.",
 "Tap to start the game or open its pause menu. Xbox Start.",
 "Tap to view mission objectives. Use menu up/down to scroll and Back to close. Xbox Back.",
 "Crouch using the game's selected toggle/hold setting. Xbox left-stick click.",
 "Tap to toggle zoom with supported weapons; also prepares zoom behind the shield. Xbox right-stick click.",
 "Issue the available context action to a teammate. In menus: move up. Xbox D-pad up.",
 "Display the available teammate orders. In menus: move down. Xbox D-pad down.",
 "Tell a teammate to follow, or request healing when offered. In menus: move left or decrease. Xbox D-pad left.",
 "Tell a teammate to take cover, or decline healing when offered. In menus: move right or increase. Xbox D-pad right.",
 "Hold to move left. Opposite movement keys cancel each other. Xbox left stick.",
 "Hold to move right. Opposite movement keys cancel each other. Xbox left stick.",
 "Hold to move forward. Opposite movement keys cancel each other. Xbox left stick.",
 "Hold to move backward. Opposite movement keys cancel each other. Xbox left stick.",
 "Hold to turn left. Mouse movement also turns the camera while captured. Xbox right stick.",
 "Hold to turn right. Mouse movement also turns the camera while captured. Xbox right stick.",
 "Hold to look up; the game's invert-look setting can change direction. Mouse aiming remains available. Xbox right stick.",
 "Hold to look down; the game's invert-look setting can change direction. Mouse aiming remains available. Xbox right stick.",
 "Hold and drag toward an available NPC order or help option, then release to select. The direction indicator previews your drag; the center is neutral. A tap opens the panel. Aim pauses while held."};
static int xbox_binding_sources_overlap(unsigned a,unsigned b){
 if(!a||!b)return 0;if(a==b)return 1;
 return (a==VK_SHIFT&&(b==VK_LSHIFT||b==VK_RSHIFT))||(b==VK_SHIFT&&(a==VK_LSHIFT||a==VK_RSHIFT))||
        (a==VK_CONTROL&&(b==VK_LCONTROL||b==VK_RCONTROL))||(b==VK_CONTROL&&(a==VK_LCONTROL||a==VK_RCONTROL))||
        (a==VK_MENU&&(b==VK_LMENU||b==VK_RMENU))||(b==VK_MENU&&(a==VK_LMENU||a==VK_RMENU));
}
static void xbox_binding_shared_notice(unsigned action,char *notice,unsigned size){
 if(!notice||!size)return;notice[0]=0;if(action>=UCRR_KEYBOARD_ACTIONS)return;
 char names[132]={0};int orders=0;
 for(unsigned slot=0;slot<2;++slot){unsigned source=xbox_KeyboardBinding(action,slot);int shared=0;
  if(!source||(slot&&source==xbox_KeyboardBinding(action,0)))continue;
  for(unsigned other=0;other<UCRR_KEYBOARD_ACTIONS;++other)if(other!=action)for(unsigned s=0;s<2;++s)
   if(xbox_binding_sources_overlap(source,xbox_KeyboardBinding(other,s))){shared=1;if(action==UCRR_KEYBOARD_ORDERS||other==UCRR_KEYBOARD_ORDERS)orders=1;}
  if(shared){size_t used=strlen(names);snprintf(names+used,sizeof(names)-used,"%s%s",used?" / ":"",xbox_KeyboardSourceName(source));}
 }
 if(names[0])snprintf(notice,size,"Shared input: %s. %s",names,orders?"Teammate orders yields to game actions on shared inputs. Other shared game actions still activate together.":"Both game actions activate together.");
}
static int xbox_dirty(void){return xbox_pending_fps!=xbox_fps||
#ifdef UCRR_GRAPHICS_EFFECT_UI
 xbox_pending_effects!=xbox_effects||
#endif
#if defined(UCRR_LIGHT_REMOVAL)
 xbox_pending_lighting!=xbox_lighting||
#endif
 xbox_pending_msaa!=xbox_msaa||xbox_pending_render_scale!=xbox_render_scale||xbox_pending_aspect!=xbox_aspect||xbox_pending_resolution!=xbox_resolution||xbox_pending_fullscreen!=xbox_fullscreen||xbox_pending_vsync!=xbox_vsync;}
static void xbox_controls_layout(void);
static const int xbox_sizes[5][2]={{640,480},{960,720},{1280,720},{1920,1080},{2560,1440}};
static RECT xbox_fit_rect(int w,int h,int mode){
 RECT r={0,0,w,h};if(w<=0||h<=0||mode==3)return r;
 int aw=(mode==1||mode==2)?16:4,ah=(mode==1||mode==2)?9:3;
 if((int64_t)w*ah>(int64_t)h*aw){int fit=h*aw/ah;r.left=(w-fit)/2;r.right=r.left+fit;}
 else{int fit=w*ah/aw;r.top=(h-fit)/2;r.bottom=r.top+fit;}return r;
}
static void xbox_overlay_layout(HWND parent){
 RECT c;GetClientRect(parent,&c);RECT r=xbox_fit_rect(c.right,c.bottom,xbox_aspect);
 if(xbox_output_window)MoveWindow(xbox_output_window,r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);
 /* Match the N64 frontend's inset modal: leave game visible on all sides. */
 if(xbox_overlay_window){int mx=MulDiv(c.bottom,64,1080),my=mx;
  SetWindowPos(xbox_overlay_window,HWND_TOP,mx,my,c.right-2*mx,c.bottom-2*my,SWP_NOACTIVATE);xbox_controls_layout();}
 InvalidateRect(parent,NULL,TRUE);
}
static void xbox_overlay_labels(void){
 SendMessage(GetDlgItem(xbox_overlay_window,1800),CB_SETCURSEL,xbox_pending_render_scale-1,0);
 SendMessageA(GetDlgItem(xbox_overlay_window,1700),TBM_SETPOS,TRUE,xbox_MouseGetSensitivity());
 EnableWindow(GetDlgItem(xbox_overlay_window,1500),xbox_dirty());InvalidateRect(xbox_overlay_window,NULL,TRUE);
 for(int id=1100;id<1105;++id)EnableWindow(GetDlgItem(xbox_overlay_window,id),!xbox_pending_fullscreen);
 RedrawWindow(xbox_overlay_window,NULL,NULL,RDW_INVALIDATE|RDW_ALLCHILDREN);
 for(int row=0;row<8;++row)for(int slot=0;slot<2;++slot){int a=xbox_control_page+row;if(a>=xbox_control_action_count())continue;
  SetWindowTextA(GetDlgItem(xbox_overlay_window,2000+row*4+slot*2),xbox_bind_action==a&&xbox_bind_slot==slot?"Press input...":xbox_control_device?xbox_KeyboardSourceName(xbox_binding(a,slot)):xbox_ControllerSourceName(xbox_binding(a,slot)));
 }
 for(int stick=0;stick<2;++stick)SendMessage(GetDlgItem(xbox_overlay_window,1910+stick),TBM_SETPOS,TRUE,xbox_ControllerDeadzone(stick));
}
static void xbox_overlay_show(HWND parent,int show){
 if(show){xbox_control_device=xbox_KeyboardMode();xbox_control_page=0;xbox_control_help=0;xbox_controls_layout();}
 if(show)ucrr_host_release_mouse();
 if(show){xbox_pending_fps=xbox_fps;xbox_pending_render_scale=xbox_render_scale;xbox_pending_msaa=xbox_msaa;}
#if defined(UCRR_LIGHT_REMOVAL)
 if(show)xbox_pending_lighting=xbox_lighting;
#ifdef UCRR_GRAPHICS_EFFECT_UI
 if(show)xbox_pending_effects=xbox_effects;
#endif
#endif
 if(show){xbox_pending_aspect=xbox_aspect;xbox_pending_resolution=xbox_resolution;xbox_pending_fullscreen=xbox_fullscreen;xbox_pending_vsync=xbox_vsync;xbox_overlay_labels();}
 xbox_overlay_open=show;xbox_InputSetBlocked(show);ShowWindow(xbox_overlay_window,show?SW_SHOW:SW_HIDE);
 xbox_bind_action=-1;xbox_last_ui_pad=0;
 if(show)SetTimer(xbox_overlay_window,2,50,NULL);else KillTimer(xbox_overlay_window,2);
 if(show){xbox_overlay_layout(parent);SetForegroundWindow(parent);SetFocus(GetDlgItem(xbox_overlay_window,101));}
 else{SetForegroundWindow(parent);SetFocus(parent);
  /* Focus alone does not resume raw mouse look after overlay release.
   * The capture gate preserves menu cursors, focus and input blocking. */
  if(xbox_KeyboardMode())pc_mouse_capture();
  /* Retry after child-window hide/focus messages have settled. */
  if(xbox_KeyboardMode())PostMessageA(parent,WM_APP+71,0,0);
 }
}
static void xbox_overlay_action(HWND parent,int id){
 if(xbox_bind_action>=0){if(id==101){xbox_bind_action=-1;xbox_overlay_labels();}return;}
 if(xbox_close_pending){
  if(id==101||id==1602){xbox_close_pending=0;xbox_controls_layout();xbox_overlay_labels();return;}
  if(id==1600||id==1601){int destination=xbox_close_pending;xbox_close_pending=0;
   if(id==1600)xbox_overlay_action(parent,1500);
   else{xbox_pending_fps=xbox_fps;xbox_pending_msaa=xbox_msaa;xbox_pending_render_scale=xbox_render_scale;xbox_pending_aspect=xbox_aspect;xbox_pending_resolution=xbox_resolution;xbox_pending_vsync=xbox_vsync;xbox_pending_fullscreen=xbox_fullscreen;
#if defined(UCRR_LIGHT_REMOVAL)
    xbox_pending_lighting=xbox_lighting;
#ifdef UCRR_GRAPHICS_EFFECT_UI
    xbox_pending_effects=xbox_effects;
#endif
#endif
   }
   if(xbox_dirty())return; /* Save failure: retain unapplied choices. */
   xbox_overlay_action(parent,destination);xbox_controls_layout();xbox_overlay_labels();return;}
  return;
 }
 if((id==101||(id>=201&&id<=204&&id-201!=xbox_tab))&&xbox_dirty()){
  xbox_close_pending=id;xbox_controls_layout();xbox_overlay_labels();SetFocus(GetDlgItem(xbox_overlay_window,1602));return;
 }
 if(id==101){xbox_overlay_show(parent,0);return;}
 if(id==100){if(MessageBoxA(parent,"Quit Urban Chaos? Unsaved game progress will be lost.","Quit game",MB_YESNO|MB_ICONQUESTION)==IDYES)PostMessage(parent,WM_CLOSE,0,0);return;}
 if(id>=201&&id<=204){xbox_tab=id-201;xbox_controls_layout();xbox_overlay_labels();return;}
 if(id>=1100&&id<=1104){xbox_pending_resolution=id-1100;xbox_overlay_labels();return;}
 if(id>=1200&&id<=1203){xbox_pending_aspect=id-1200;xbox_overlay_labels();return;}
 if(id>=1300&&id<=1301){xbox_pending_fullscreen=id-1300;xbox_overlay_labels();return;}
 if(id>=1400&&id<=1401){xbox_pending_vsync=id-1400;xbox_overlay_labels();return;}
#ifdef UCRR_GRAPHICS_EFFECT_UI
 if(id>=1871&&id<=1877){if(uc_graphics_features_enabled())xbox_pending_effects^=1u<<(id-1871);xbox_overlay_labels();return;}
#endif
 if(id==1880){xbox_pending_fps=!xbox_pending_fps;xbox_overlay_labels();return;}
 if(id==1850){xbox_pending_vsync=!xbox_pending_vsync;xbox_overlay_labels();return;}
#if defined(UCRR_LIGHT_REMOVAL)
 if(id==1870){if(uc_light_removal_live_toggle_enabled())xbox_pending_lighting=!xbox_pending_lighting;xbox_overlay_labels();return;}
#endif
 if(id>=1860&&id<=1862){xbox_pending_msaa=1u<<(id-1860);xbox_overlay_labels();return;}
 if(id>=1801&&id<=1803){xbox_pending_render_scale=id-1800;xbox_overlay_labels();return;}
 if(id==1900||id==1901){xbox_control_device=id-1900;xbox_control_page=0;xbox_control_help=0;xbox_controls_layout();xbox_overlay_labels();return;}
 if(id==1902||id==1903){xbox_bindings_scroll(xbox_control_page+(id==1902?-1:1)*xbox_control_rows);return;}
 if(id==1904){if(MessageBoxA(parent,xbox_control_device?"Reset keyboard and mouse bindings?":"Reset all controller bindings and deadzones?","Controls",MB_YESNO|MB_ICONQUESTION)==IDYES&&!(xbox_control_device?xbox_KeyboardReset():xbox_ControllerReset()))MessageBoxA(parent,"Could not save controls.","Controls",MB_OK|MB_ICONERROR);xbox_overlay_labels();return;}
 if(id>=2000&&id<2032){int n=id-2000,a=xbox_control_page+n/4,s=(n%4)/2;if(a>=xbox_control_action_count())return;
  if(n&1){if(!xbox_bind(a,s,0))MessageBoxA(parent,"Could not save controls.","Controls",MB_OK|MB_ICONERROR);}
  else{xbox_bind_action=a;xbox_bind_slot=s;xbox_bind_armed=0;}
  xbox_overlay_labels();return;
 }
 if(id==1500){
#ifdef UCRR_GRAPHICS_EFFECT_UI
  if(xbox_pending_effects!=xbox_effects){if(!uc_graphics_features_save(xbox_pending_effects)){MessageBoxA(parent,"Could not save effect settings. Changes remain unapplied.","Graphics settings",MB_OK|MB_ICONERROR);return;}xbox_effects=xbox_pending_effects;}
#endif
  if(xbox_pending_fps!=xbox_fps){if(!d3d8_SaveFpsCounter(xbox_pending_fps)){MessageBoxA(parent,"Could not save the FPS counter setting.","Graphics settings",MB_OK|MB_ICONERROR);return;}xbox_fps=xbox_pending_fps;}
#if defined(UCRR_LIGHT_REMOVAL)
  if(xbox_pending_lighting!=xbox_lighting&&!xbox_lighting_save(xbox_pending_lighting)){MessageBoxA(parent,"Could not save or apply dynamic lighting. Your choice remains unapplied.","Graphics settings",MB_OK|MB_ICONERROR);return;}
#endif
  if(xbox_pending_msaa!=xbox_msaa){if(!d3d8_SaveHostMsaa(xbox_pending_msaa)){MessageBoxA(parent,"Could not save MSAA settings.","Graphics settings",MB_OK|MB_ICONERROR);return;}xbox_msaa=xbox_pending_msaa;}
  if(xbox_pending_aspect!=xbox_aspect&&!d3d8_SaveHostAspect(xbox_pending_aspect)){MessageBoxA(parent,"Could not save aspect ratio. Changes have not been applied.","Graphics settings",MB_OK|MB_ICONERROR);return;}
  if(xbox_pending_render_scale!=xbox_render_scale){
   if(!d3d8_SaveHostRenderScale(xbox_pending_render_scale)){MessageBoxA(parent,"Could not save xboxrecomp.ini beside the executable. Changes have not been applied.","Graphics settings",MB_OK|MB_ICONERROR);return;}
   xbox_render_scale=xbox_pending_render_scale;
  }
  if(xbox_pending_fullscreen!=xbox_fullscreen)xbox_overlay_action(parent,105);
  xbox_aspect=xbox_pending_aspect;xbox_vsync=xbox_pending_vsync;d3d8_SetHostVsync(xbox_vsync);
  if(!xbox_fullscreen&&xbox_resolution!=xbox_pending_resolution){xbox_resolution=xbox_pending_resolution;RECT r={0,0,xbox_sizes[xbox_resolution][0],xbox_sizes[xbox_resolution][1]};AdjustWindowRect(&r,(DWORD)GetWindowLongPtrA(parent,GWL_STYLE),FALSE);SetWindowPos(parent,NULL,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER);}
  xbox_pending_resolution=xbox_resolution;xbox_overlay_layout(parent);xbox_overlay_labels();return;
 }
 if(id==102)xbox_aspect=(xbox_aspect+1)%4;
 if(id==104){xbox_vsync=!xbox_vsync;d3d8_SetHostVsync(xbox_vsync);}
 if(id==105){
  xbox_fullscreen=!xbox_fullscreen;
  if(xbox_fullscreen){MONITORINFO m={sizeof(m)};GetWindowRect(parent,&xbox_windowed_rect);GetMonitorInfoA(MonitorFromWindow(parent,MONITOR_DEFAULTTONEAREST),&m);SetWindowLongPtrA(parent,GWL_STYLE,WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN);SetWindowPos(parent,NULL,m.rcMonitor.left,m.rcMonitor.top,m.rcMonitor.right-m.rcMonitor.left,m.rcMonitor.bottom-m.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOZORDER);}
  else{SetWindowLongPtrA(parent,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPCHILDREN);SetWindowPos(parent,NULL,xbox_windowed_rect.left,xbox_windowed_rect.top,xbox_windowed_rect.right-xbox_windowed_rect.left,xbox_windowed_rect.bottom-xbox_windowed_rect.top,SWP_FRAMECHANGED|SWP_NOZORDER);}
 }
 if(id==103&&!xbox_fullscreen){xbox_resolution=(xbox_resolution+1)%5;RECT r={0,0,xbox_sizes[xbox_resolution][0],xbox_sizes[xbox_resolution][1]};AdjustWindowRect(&r,(DWORD)GetWindowLongPtrA(parent,GWL_STYLE),FALSE);SetWindowPos(parent,NULL,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER);}
 EnableWindow(GetDlgItem(xbox_overlay_window,103),!xbox_fullscreen);
 xbox_overlay_layout(parent);xbox_overlay_labels();
}
static int xbox_choice_selected(int id){
 if(id>=1860&&id<=1862)return (1u<<(id-1860))==xbox_pending_msaa;
 if(id>=1801&&id<=1803)return id-1800==xbox_pending_render_scale;
 if(id==1900||id==1901)return id-1900==xbox_control_device;
 if(id>=201&&id<=204)return id-201==xbox_tab;
 if(id>=1100&&id<=1104)return id-1100==xbox_pending_resolution;
 if(id>=1200&&id<=1203)return id-1200==xbox_pending_aspect;
 if(id>=1300&&id<=1301)return id-1300==xbox_pending_fullscreen;
 if(id>=1400&&id<=1401)return id-1400==xbox_pending_vsync;return 0;
}
static void xbox_controls_layout(void){
 RECT r={0};if(!xbox_overlay_window||!GetClientRect(xbox_overlay_window,&r))return;int width=r.right,height=r.bottom;
 if(width<=0||height<=0)return;
 int scale=width*1000/760;if(height*1000/714<scale)scale=height*1000/714;if(scale<1)scale=1;
 if(scale!=xbox_ui_scale||!xbox_button_font){
  xbox_ui_scale=scale;if(xbox_button_font)DeleteObject(xbox_button_font);if(xbox_button_title)DeleteObject(xbox_button_title);if(xbox_tab_font)DeleteObject(xbox_tab_font);
  xbox_button_font=CreateFontA(-xbox_px(15),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,xbox_font_family);
  xbox_button_title=CreateFontA(-xbox_px(21),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,xbox_font_family);
  xbox_tab_font=CreateFontA(-xbox_px(27),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,xbox_font_family);
 }
 width=xbox_ui_width=MulDiv(width,1000,scale);height=xbox_ui_height=MulDiv(height,1000,scale);
 HRGN rounded=CreateRoundRectRgn(0,0,r.right+1,r.bottom+1,xbox_px(12),xbox_px(12));if(!SetWindowRgn(xbox_overlay_window,rounded,TRUE))DeleteObject(rounded);
 {const int x[]={6,156,300,442},w[]={150,144,142,110};for(int i=0;i<4;++i)xbox_place(201+i,x[i],0,w[i],52);}
 xbox_place(101,width-50,6,40,40);
 xbox_place(100,width-98,6,40,40);
 xbox_place(1500,width-140,height-65,110,40);
 for(int group=11;group<=14;++group){int count=group==11?5:group==12?4:2;int unit=group==11?(width-64)/5:group==12?(width-64)/4:215;
  for(int i=0;i<count;++i){HWND b=GetDlgItem(xbox_overlay_window,group*100+i);xbox_place(group*100+i,30+i*unit,115+(group-11)*85,unit-8,34);ShowWindow(b,xbox_tab==1&&!xbox_close_pending?SW_SHOW:SW_HIDE);}}
 ShowWindow(GetDlgItem(xbox_overlay_window,1500),xbox_tab==1&&!xbox_close_pending?SW_SHOW:SW_HIDE);
 for(int i=0;i<3;++i){xbox_place(1600+i,30+i*190,245,170,42);ShowWindow(GetDlgItem(xbox_overlay_window,1600+i),xbox_close_pending?SW_SHOW:SW_HIDE);}
 for(int i=0;i<4;++i)EnableWindow(GetDlgItem(xbox_overlay_window,201+i),!xbox_close_pending);
 xbox_place(1700,30,415,width-60,34);ShowWindow(GetDlgItem(xbox_overlay_window,1700),xbox_tab==2&&xbox_control_device==1&&!xbox_close_pending?SW_SHOW:SW_HIDE);
 for(int i=0;i<2;++i){xbox_place(1900+i,30+i*230,78,220,36);ShowWindow(GetDlgItem(xbox_overlay_window,1900+i),xbox_tab==2&&!xbox_close_pending?SW_SHOW:SW_HIDE);}
 int visible=xbox_tab==2&&!xbox_control_device&&!xbox_close_pending;
 xbox_bindings_layout();
 xbox_graphics_layout();
}
static void xbox_text(HDC dc,int x,int y,const char*t){TextOutA(dc,xbox_px(x),xbox_px(y),t,(int)strlen(t));}
/* Scroll only the settings column. Header, help and Apply remain stationary. */
static int xbox_graphics_content_height(void){
#ifdef UCRR_GRAPHICS_EFFECT_UI
 return 1244;
#elif defined(UCRR_LIGHT_REMOVAL)
 return 740;
#else
 return 668;
#endif
}
static int xbox_graphics_max_scroll(void){int n=xbox_graphics_content_height()-(xbox_ui_height-88);return n>0?n:0;}
static void xbox_graphics_place(int id,int x,int y,int width){
 y-=xbox_graphics_scroll;xbox_place(id,x,y,width,30);
 ShowWindow(GetDlgItem(xbox_overlay_window,id),xbox_tab==1&&!xbox_close_pending&&y>=74&&y+30<=xbox_ui_height-88?SW_SHOW:SW_HIDE);
}
static void xbox_graphics_layout(void){
 int split=xbox_ui_width/2,unit=(split-55)/3;
 if(xbox_graphics_scroll>xbox_graphics_max_scroll())xbox_graphics_scroll=xbox_graphics_max_scroll();
 {int y=104-xbox_graphics_scroll; xbox_place(1800,24,y,split-64,285);
  SendMessage(GetDlgItem(xbox_overlay_window,1800),CB_SETITEMHEIGHT,(WPARAM)-1,xbox_px(28));
  SendMessage(GetDlgItem(xbox_overlay_window,1800),CB_SETITEMHEIGHT,0,xbox_px(28));
  ShowWindow(GetDlgItem(xbox_overlay_window,1800),xbox_tab==1&&!xbox_close_pending&&y>=58&&y+32<=xbox_ui_height-88?SW_SHOW:SW_HIDE);}
 for(int i=0;i<5;++i)xbox_graphics_place(1100+i,24+(i%3)*unit,176+(i/3)*28,unit-6);
 int widths[]={99,75,70,70},x=24;
 for(int i=0;i<4;++i){xbox_graphics_place(1200+i,x,272,widths[i]);x+=widths[i];}
 for(int i=0;i<2;++i)xbox_graphics_place(1300+i,24+i*120,344,114);
 for(int i=0;i<2;++i)ShowWindow(GetDlgItem(xbox_overlay_window,1400+i),SW_HIDE);
 xbox_graphics_place(1850,24,416,64);
 for(int i=0;i<3;++i)xbox_graphics_place(1860+i,24+i*60,488,54);
#if defined(UCRR_LIGHT_REMOVAL)
 xbox_graphics_place(1870,24,560,120);EnableWindow(GetDlgItem(xbox_overlay_window,1870),uc_light_removal_live_toggle_enabled());
#endif
 #ifdef UCRR_GRAPHICS_EFFECT_UI
 for(int i=0;i<7;i++){xbox_graphics_place(1871+i,24,632+i*72,120);EnableWindow(GetDlgItem(xbox_overlay_window,1871+i),uc_graphics_features_enabled());}
 #endif
 xbox_graphics_place(1880,24,xbox_graphics_content_height()-36,120);
 xbox_place(1851,split-12,54,10,xbox_ui_height-132);
 ShowWindow(GetDlgItem(xbox_overlay_window,1851),xbox_tab==1&&!xbox_close_pending&&xbox_graphics_max_scroll()>0?SW_SHOW:SW_HIDE);
 InvalidateRect(GetDlgItem(xbox_overlay_window,1851),NULL,FALSE);
 SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS,0,xbox_graphics_content_height()-1,(UINT)(xbox_ui_height-88),xbox_graphics_scroll,0};SetScrollInfo(GetDlgItem(xbox_overlay_window,1851),SB_CTL,&si,TRUE);
}
static void xbox_graphics_set_scroll(int value){
 xbox_graphics_scroll=value<0?0:value>xbox_graphics_max_scroll()?xbox_graphics_max_scroll():value;
 xbox_graphics_layout();InvalidateRect(xbox_overlay_window,NULL,FALSE);
}
static LRESULT CALLBACK xbox_scroll_proc(HWND w,UINT m,WPARAM a,LPARAM b){
 RECT r;GetClientRect(w,&r);int maximum=xbox_graphics_max_scroll();
 int thumb=maximum?r.bottom*(xbox_ui_height-88)/xbox_graphics_content_height():r.bottom;if(thumb<12)thumb=12;
 int travel=r.bottom-thumb;
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);HBRUSH bg=CreateSolidBrush(RGB(12,14,12));FillRect(dc,&r,bg);DeleteObject(bg);RECT t=r;t.top=maximum?MulDiv(xbox_graphics_scroll,travel,maximum):0;t.bottom=t.top+thumb;HBRUSH brush=CreateSolidBrush(RGB(40,57,32));FillRect(dc,&t,brush);DeleteObject(brush);EndPaint(w,&ps);return 0;}
 if(m==WM_LBUTTONDOWN||m==WM_MOUSEMOVE&&(a&MK_LBUTTON)){if(m==WM_LBUTTONDOWN)SetCapture(w);if(travel>0)xbox_graphics_set_scroll(MulDiv((short)HIWORD(b)-thumb/2,maximum,travel));return 0;}
 if(m==WM_LBUTTONUP){ReleaseCapture();return 0;}
 if(m==WM_MOUSEWHEEL)return SendMessage(GetParent(w),m,a,b);
 return DefWindowProcA(w,m,a,b);
}
static LRESULT CALLBACK xbox_graphics_control_proc(HWND w,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR data){
 (void)data;
 if(id==1800&&m==WM_PAINT){
  PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);HBRUSH bg=CreateSolidBrush(RGB(16,17,16));FillRect(dc,&r,bg);DeleteObject(bg);
  HPEN pen=CreatePen(PS_SOLID,1,GetFocus()==w?RGB(129,222,65):RGB(60,65,58));HGDIOBJ oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,0,0,r.right,r.bottom,xbox_px(6),xbox_px(6));
  SelectObject(dc,xbox_button_font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(242,242,242));char text[100]={0};LRESULT n=SendMessage(w,CB_GETCURSEL,0,0);if(n>=0)SendMessageA(w,CB_GETLBTEXT,n,(LPARAM)text);RECT label=r;label.left+=xbox_px(10);label.right-=xbox_px(30);DrawTextA(dc,text,-1,&label,DT_SINGLELINE|DT_VCENTER);
  int x=r.right-xbox_px(16),y=r.bottom/2;MoveToEx(dc,x-xbox_px(4),y-2,NULL);LineTo(dc,x,y+xbox_px(3));LineTo(dc,x+xbox_px(4),y-2);SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);EndPaint(w,&ps);return 0;
 }
 if(m==WM_MOUSEMOVE||m==WM_SETFOCUS){
  if(xbox_graphics_help!=(int)id){xbox_graphics_help=(int)id;InvalidateRect(xbox_overlay_window,NULL,FALSE);}
 }
 if(m==WM_MOUSEWHEEL){SendMessage(xbox_overlay_window,m,a,b);return 0;}
 if(m==WM_NCDESTROY)RemoveWindowSubclass(w,xbox_graphics_control_proc,id);
 return DefSubclassProc(w,m,a,b);
}
static void xbox_graphics_paint(HDC dc){
 int split=xbox_ui_width/2,saved=SaveDC(dc);
 IntersectClipRect(dc,xbox_px(18),xbox_px(74),xbox_px(split-18),xbox_px(xbox_ui_height-88));
 const char*titles[]={"Resolution","Output Resolution","Aspect Ratio","Window Mode","VSync","MS Anti-Aliasing",
#if defined(UCRR_LIGHT_REMOVAL)
 "Dynamic lighting",
#endif
 #ifdef UCRR_GRAPHICS_EFFECT_UI
 "All particles (restart)","Snow / smoke particles (restart)","Character shadows (restart)","Dynamic decals (restart)","Dynamic shadows (restart)","Hidden particle prep (restart)","Sky models (restart)",
 #endif
 "HUD Placement","FPS counter"};
 int extra=0;
#if defined(UCRR_LIGHT_REMOVAL)
 extra=72;
#ifdef UCRR_GRAPHICS_EFFECT_UI
 extra+=504;
#endif
#endif
 for(int i=0;i<(int)(sizeof(titles)/sizeof(titles[0]));++i){int y=76+i*72+(i>=2?24:0);SelectObject(dc,xbox_button_title);SetTextColor(dc,RGB(223,231,220));xbox_text(dc,24,y-xbox_graphics_scroll,titles[i]);}
 SelectObject(dc,xbox_button_font);SetTextColor(dc,RGB(105,115,101));
 xbox_text(dc,24,560+extra-xbox_graphics_scroll,"ORIGINAL     Expanded HUD unavailable");

 RestoreDC(dc,saved);
 const char*help="Choose a setting on the left. Changes are applied together with Apply. Scroll for additional graphics settings.";
 int id=xbox_graphics_help;
 if(id>=1800&&id<=1803)help="Internal resolution scale: 1X through 8X. 3X renders a 640 x 480 target at 1920 x 1440; 8X at 5120 x 3840.\n\nApply, then restart the game. 8X uses 64 times the native pixel count, before MSAA. Higher scales need substantially more GPU memory and may reduce performance.\n\nThe scaled image and screen-effect copies retain their resolution through presentation, then fit the window. Original textures, videos and authored blur effects are unchanged.";
 else if(id>=1100&&id<=1104)help="Sets the output window size, up to 2560 x 1440. This does not change the internal rendering resolution.\n\nAvailable in Windowed mode. Applies for the current session.";
 else if(id>=1200&&id<=1203)help="Original preserves 4:3. 16:9 enables the game's original widescreen camera mode. Stretch fills 16:9 without expanding a 4:3 camera. Fill uses the whole window.\n\nApply, then restart when changing true widescreen. 21:9 camera and independent HUD placement are not implemented.";
 else if(id>=1300&&id<=1301)help="Sets whether the game displays Windowed or Fullscreen.\n\nFullscreen fills the current monitor; the selected aspect ratio controls the game picture within it. Applies for the current session.";
#ifdef UCRR_GRAPHICS_EFFECT_UI
 else if(id>=1871&&id<=1877){
 static const char*descriptions[]={"Controls all tested particle rendering, including snow, smoke, fire and sparks. Off overrides the separate snow / smoke switch.","Controls the rotated-particle class, including snow and some smoke. Other particle types remain. Requires All particles to be On.","Controls the simple blob shadows beneath characters. Separate from dynamic lighting and stencil shadows.","Controls marks rendered by the dynamic decal manager. Marks baked into scenery may remain.","Controls the selected dynamic stencil-shadow path. With dynamic lighting off, this may remove no additional work.","On keeps normal preparation of hidden particles. Off skips visibility checks and sorting for eligible hidden particles. Requires All particles to be Off; spawning and motion continue. Experimental: no repeatable FPS gain measured.","Experimental sky-model visibility cut. Not yet verified to remove visible work: Westside did not use the targeted setup path. No FPS gain established. Original updates and game logic remain."};
 static char effect_help[600];snprintf(effect_help,sizeof(effect_help),"%s\n\nChoose Apply to save, then close and relaunch the game. The saved choice takes effect after restart. The current session keeps its startup effect settings.",descriptions[id-1871]);help=effect_help;
 }
#endif
 else if(id==1880)help="Shows completed presentations per second in the top-left corner during play. Updates four times a second using about one second of recent frames. No log file is created.\n\nChoose Apply. The setting is saved. This measures presentation rate; videos can repeat frames and the counter does not prove simulation speed.";
 else if(id==1850||id==1400||id==1401)help="Synchronizes presentation to the display refresh when enabled. Disabling VSync may reduce waiting but can introduce tearing.\n\nThis does not change game simulation speed or guarantee 60 FPS. Applies for the current session.";
 else if(id>=1860&&id<=1862)help="Multisample anti-aliasing smooths geometry edges. NONE is fastest; 2X and 4X increase color/depth memory and GPU bandwidth.\n\nApply, then restart. Unsupported hardware falls back to NONE.\n\nExperimental: depth/stencil effects use sample-zero reconstruction. Gameplay shadows and effects still require visual verification.";
#if defined(UCRR_LIGHT_REMOVAL)
 else if(id==1870)help=uc_light_removal_live_toggle_enabled()?"Turns the selected dynamic lighting effects on or off. Off may improve performance and change the scene's brightness and appearance.\n\nChoose Apply to change lighting during play. No restart is needed. Your choice is saved for the next launch. This test package starts with lighting off.\n\nParticles and character body shadows keep their separate settings.":"Live lighting controls are unavailable in this launch. Use the test package launcher to enable this setting.";
#endif
 SelectObject(dc,xbox_button_font);SetTextColor(dc,RGB(180,190,175));RECT box={xbox_px(split+24),xbox_px(88),xbox_px(xbox_ui_width-28),xbox_px(xbox_ui_height-110)};
 DrawTextA(dc,help,-1,&box,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
 HPEN pen=CreatePen(PS_SOLID,1,RGB(35,53,30));HGDIOBJ old=SelectObject(dc,pen);MoveToEx(dc,xbox_px(split),xbox_px(64),NULL);LineTo(dc,xbox_px(split),xbox_px(xbox_ui_height-78));SelectObject(dc,old);DeleteObject(pen);
}
#include "xbox_controls_layout.h"
static LRESULT CALLBACK xbox_overlay_proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 HWND parent=GetParent(w);
 if(m==WM_DRAWITEM&&((DRAWITEMSTRUCT*)lp)->CtlID>=2000&&((DRAWITEMSTRUCT*)lp)->CtlID<2032){
  DRAWITEMSTRUCT*d=(DRAWITEMSTRUCT*)lp;int saved=SaveDC(d->hDC),clear=(d->CtlID-2000)&1;RECT r=d->rcItem;
  SetDCBrushColor(d->hDC,(d->itemState&(ODS_FOCUS|ODS_SELECTED))?RGB(35,52,28):RGB(24,30,22));FillRect(d->hDC,&r,GetStockObject(DC_BRUSH));
  SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,clear?RGB(187,154,146):RGB(223,231,220));SelectObject(d->hDC,xbox_button_font);
  char text[100];GetWindowTextA(d->hwndItem,text,sizeof(text));InflateRect(&r,-xbox_px(5),0);DrawTextA(d->hDC,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
  HPEN pen=CreatePen(PS_SOLID,1,d->itemState&ODS_FOCUS?RGB(129,222,65):RGB(57,68,49));SelectObject(d->hDC,pen);SelectObject(d->hDC,GetStockObject(NULL_BRUSH));RoundRect(d->hDC,0,0,d->rcItem.right-1,d->rcItem.bottom-1,xbox_px(6),xbox_px(6));RestoreDC(d->hDC,saved);DeleteObject(pen);return TRUE;
 }
 if(m==WM_MEASUREITEM&&((MEASUREITEMSTRUCT*)lp)->CtlID==1800){((MEASUREITEMSTRUCT*)lp)->itemHeight=xbox_px(28);return TRUE;}
 if(m==WM_DRAWITEM&&((DRAWITEMSTRUCT*)lp)->CtlID==1800){
  DRAWITEMSTRUCT*d=(DRAWITEMSTRUCT*)lp;int saved=SaveDC(d->hDC);RECT r=d->rcItem;
  HBRUSH bg=CreateSolidBrush((d->itemState&ODS_SELECTED)?RGB(28,48,24):RGB(16,17,16));FillRect(d->hDC,&r,bg);DeleteObject(bg);
  SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,RGB(242,242,242));SelectObject(d->hDC,xbox_button_font);
  char label[100]={0};if(d->itemID!=(UINT)-1)SendMessageA(d->hwndItem,CB_GETLBTEXT,d->itemID,(LPARAM)label);
  r.left+=xbox_px(10);DrawTextA(d->hDC,label,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
  if(d->itemState&ODS_FOCUS){HPEN pen=CreatePen(PS_SOLID,1,RGB(129,222,65));SelectObject(d->hDC,pen);SelectObject(d->hDC,GetStockObject(NULL_BRUSH));Rectangle(d->hDC,1,1,d->rcItem.right-1,d->rcItem.bottom-1);RestoreDC(d->hDC,saved);DeleteObject(pen);}else RestoreDC(d->hDC,saved);return TRUE;
 }
 if(m==WM_DRAWITEM&&(((DRAWITEMSTRUCT*)lp)->CtlID==1850||((DRAWITEMSTRUCT*)lp)->CtlID==1880
#if defined(UCRR_LIGHT_REMOVAL)
  ||((DRAWITEMSTRUCT*)lp)->CtlID==1870
#ifdef UCRR_GRAPHICS_EFFECT_UI
  ||(((DRAWITEMSTRUCT*)lp)->CtlID>=1871&&((DRAWITEMSTRUCT*)lp)->CtlID<=1877)
#endif
#endif
 )){
  DRAWITEMSTRUCT*d=(DRAWITEMSTRUCT*)lp;int saved=SaveDC(d->hDC);RECT r=d->rcItem;
  int on=d->CtlID==1880?xbox_pending_fps:xbox_pending_vsync;
#if defined(UCRR_LIGHT_REMOVAL)
  if(d->CtlID==1870)on=xbox_pending_lighting;
#ifdef UCRR_GRAPHICS_EFFECT_UI
  if(d->CtlID>=1871&&d->CtlID<=1877)on=!!(xbox_pending_effects&(1u<<(d->CtlID-1871)));
#endif
#endif
  HBRUSH background=CreateSolidBrush(RGB(10,13,10));FillRect(d->hDC,&r,background);DeleteObject(background);
  HBRUSH track=CreateSolidBrush(on?RGB(57,103,29):RGB(45,50,43));HPEN edge=CreatePen(PS_SOLID,1,RGB(103,122,92));SelectObject(d->hDC,track);SelectObject(d->hDC,edge);
  int diameter=xbox_px(24),top=(r.bottom-diameter)/2,right=xbox_px(52);RoundRect(d->hDC,0,top,right,top+diameter,diameter,diameter);
  HBRUSH thumb=CreateSolidBrush(on?RGB(129,222,65):RGB(180,188,175));SelectObject(d->hDC,thumb);int x=on?right-diameter+3:3;Ellipse(d->hDC,x,top+3,x+diameter-6,top+diameter-3);
#if defined(UCRR_LIGHT_REMOVAL)
  if(d->CtlID>=1870&&d->CtlID<=1877){RECT label=r;label.left=xbox_px(62);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,(d->itemState&ODS_DISABLED)?RGB(105,115,101):RGB(223,231,220));SelectObject(d->hDC,xbox_button_font);DrawTextA(d->hDC,on?"On":"Off",-1,&label,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);}
#endif
  if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&r);RestoreDC(d->hDC,saved);DeleteObject(track);DeleteObject(thumb);DeleteObject(edge);return TRUE;
 }
 if(m==WM_DRAWITEM&&((DRAWITEMSTRUCT*)lp)->CtlID==101){
  DRAWITEMSTRUCT*d=(DRAWITEMSTRUCT*)lp;int saved=SaveDC(d->hDC);RECT r=d->rcItem;
  HBRUSH b=CreateSolidBrush((d->itemState&ODS_SELECTED)?RGB(31,44,27):RGB(10,13,10));FillRect(d->hDC,&r,b);DeleteObject(b);
  int cx=(r.left+r.right)/2,cy=(r.top+r.bottom)/2,arm=xbox_px(6);
  HPEN pen=CreatePen(PS_SOLID,xbox_px(2)>0?xbox_px(2):1,RGB(223,231,220));SelectObject(d->hDC,pen);
  MoveToEx(d->hDC,cx-arm,cy-arm,NULL);LineTo(d->hDC,cx+arm,cy+arm);MoveToEx(d->hDC,cx-arm,cy+arm,NULL);LineTo(d->hDC,cx+arm,cy-arm);
  if(d->itemState&ODS_FOCUS){InflateRect(&r,-xbox_px(4),-xbox_px(4));DrawFocusRect(d->hDC,&r);}
  RestoreDC(d->hDC,saved);DeleteObject(pen);return TRUE;
 }
 switch(m){
 case WM_MOUSEMOVE:if(xbox_tab==2&&!xbox_close_pending){int x=MulDiv((short)LOWORD(lp),1000,xbox_ui_scale),y=MulDiv((short)HIWORD(lp),1000,xbox_ui_scale);int row=(y-142)/80;if(x>=18&&x<xbox_bindings_split()-18&&y>=142&&row<xbox_control_rows){int action=xbox_control_page+row;if(xbox_control_help!=action){xbox_control_help=action;InvalidateRect(w,NULL,FALSE);}}}break;
 case WM_CTLCOLORLISTBOX:SetTextColor((HDC)wp,RGB(242,242,242));SetBkColor((HDC)wp,RGB(16,17,16));SetDCBrushColor((HDC)wp,RGB(16,17,16));return (LRESULT)GetStockObject(DC_BRUSH);
 case WM_MOUSEWHEEL:if(xbox_tab==2&&!xbox_close_pending){
  if(xbox_control_device&&xbox_bind_action>=0){
   if(xbox_bind_armed&&xbox_bind_action!=UCRR_KEYBOARD_ORDERS&&GET_WHEEL_DELTA_WPARAM(wp)){
    unsigned source=GET_WHEEL_DELTA_WPARAM(wp)>0?UCRR_KEYBOARD_WHEEL_UP:UCRR_KEYBOARD_WHEEL_DOWN;
    if(!xbox_bind(xbox_bind_action,xbox_bind_slot,source))MessageBoxA(parent,"Could not save controls.","Controls",MB_OK|MB_ICONERROR);
    xbox_bind_action=-1;xbox_overlay_labels();
   }
   return 0;
  }
  xbox_bindings_scroll(xbox_control_page-GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*2);return 0;
 }if(xbox_tab==1&&!xbox_close_pending){xbox_graphics_set_scroll(xbox_graphics_scroll-GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*72);return 0;}break;
 case WM_VSCROLL:if((HWND)lp==GetDlgItem(w,1851)){
  SCROLLINFO si={sizeof(si),SIF_ALL};GetScrollInfo((HWND)lp,SB_CTL,&si);int p=xbox_graphics_scroll;
  switch(LOWORD(wp)){case SB_LINEUP:p-=36;break;case SB_LINEDOWN:p+=36;break;case SB_PAGEUP:p-=200;break;case SB_PAGEDOWN:p+=200;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:p=si.nTrackPos;break;case SB_TOP:p=0;break;case SB_BOTTOM:p=xbox_graphics_max_scroll();break;}
  xbox_graphics_set_scroll(p);return 0;
 }break;
 case WM_TIMER:if(wp==2&&xbox_overlay_open){
  if(IsWindowVisible(parent)&&GetForegroundWindow()!=parent){if(xbox_bind_action>=0){xbox_bind_action=-1;xbox_overlay_labels();}xbox_last_ui_pad=0;return 0;}
  unsigned input=xbox_ControllerCapture();
  if(xbox_bind_action>=0){
   if(xbox_control_device)input=xbox_KeyboardCapture();
   if(!input)xbox_bind_armed=1;
   else if(xbox_bind_armed){if(!xbox_bind(xbox_bind_action,xbox_bind_slot,input))MessageBoxA(parent,"Could not save controls.","Controls",MB_OK|MB_ICONERROR);xbox_bind_action=-1;xbox_overlay_labels();}
  }else if(input&&input!=xbox_last_ui_pad){
   if(input==13)xbox_bindings_navigate(1);
   if(input==14)xbox_bindings_navigate(0);
   if(input==15)SetFocus(GetNextDlgTabItem(w,GetFocus(),TRUE));
   if(input==16)SetFocus(GetNextDlgTabItem(w,GetFocus(),FALSE));
   if(input==1){HWND focus=GetFocus();char cls[32]={0};GetClassNameA(focus,cls,sizeof(cls));if(IsChild(w,focus)&&!strcmp(cls,"Button"))SendMessage(focus,BM_CLICK,0,0);}
   if(input==2)xbox_overlay_action(parent,101);
  }
  xbox_last_ui_pad=input;return 0;
 }break;
 case WM_HSCROLL:
  for(int i=0;i<2;++i)if((HWND)lp==GetDlgItem(w,1910+i)&&xbox_tab==2&&!xbox_control_device&&!xbox_close_pending){
   if(LOWORD(wp)==TB_ENDTRACK){if(!xbox_ControllerSetDeadzone(i,(unsigned)SendMessage((HWND)lp,TBM_GETPOS,0,0)))MessageBoxA(parent,"Could not save controller settings.","Controller settings",MB_OK|MB_ICONERROR);xbox_overlay_labels();}return 0;
  }
  if((HWND)lp==GetDlgItem(w,1700)&&xbox_tab==2&&!xbox_close_pending){
   xbox_MouseSetSensitivity((unsigned)SendMessageA((HWND)lp,TBM_GETPOS,0,0));xbox_overlay_labels();
  }return 0;
 case WM_CTLCOLORSTATIC:
  if((HWND)lp==GetDlgItem(w,1700)){SetDCBrushColor((HDC)wp,RGB(10,13,10));return (LRESULT)GetStockObject(DC_BRUSH);}break;
 case WM_NOTIFY:{
  NMCUSTOMDRAW*d=(NMCUSTOMDRAW*)lp;
  if(d->hdr.idFrom==1700&&d->hdr.code==NM_CUSTOMDRAW){
   if(d->dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW;
   if(d->dwDrawStage==CDDS_ITEMPREPAINT&&(d->dwItemSpec==TBCD_CHANNEL||d->dwItemSpec==TBCD_THUMB)){
    SetDCBrushColor(d->hdc,d->dwItemSpec==TBCD_THUMB?RGB(129,222,65):RGB(50,70,40));FillRect(d->hdc,&d->rc,GetStockObject(DC_BRUSH));
    if(d->dwItemSpec==TBCD_THUMB&&GetFocus()==d->hdr.hwndFrom)DrawFocusRect(d->hdc,&d->rc);
    return CDRF_SKIPDEFAULT;
   }
  }break;
 }
 case WM_CLOSE:xbox_overlay_action(parent,101);return 0;
 case WM_KEYDOWN:
  if(wp==VK_ESCAPE&&!(lp&(1LL<<30))){xbox_overlay_action(parent,101);return 0;}break;
 case WM_COMMAND:
  if(LOWORD(wp)==1800&&HIWORD(wp)==CBN_SELCHANGE){LRESULT choice=SendMessage(GetDlgItem(w,1800),CB_GETCURSEL,0,0);if(choice>=0&&choice<8){xbox_pending_render_scale=(unsigned)choice+1;xbox_overlay_labels();}return 0;}
  if(HIWORD(wp)==BN_CLICKED)xbox_overlay_action(parent,LOWORD(wp));return 0;
 case WM_ERASEBKGND:return 1;
 case WM_SIZE:xbox_controls_layout();return 0;
 case WM_PAINT:{PAINTSTRUCT ps;HDC target=BeginPaint(w,&ps);RECT pixels;GetClientRect(w,&pixels);
  /* Paint one clean pixel-coordinate buffer. Never leave a scaled mapping on a
     DC that Windows may reuse for a child control or the next resize. */
  HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,pixels.right>0?pixels.right:1,pixels.bottom>0?pixels.bottom:1);HGDIOBJ oldBitmap=SelectObject(dc,bitmap);
  RECT r={0,0,xbox_ui_width,xbox_ui_height};HBRUSH b=CreateSolidBrush(RGB(10,13,10));FillRect(dc,&pixels,b);DeleteObject(b);SetBkMode(dc,TRANSPARENT);SelectObject(dc,xbox_button_title);SetTextColor(dc,RGB(223,231,220));
  if(xbox_close_pending){xbox_text(dc,30,135,"Graphics options have unapplied changes.");SelectObject(dc,xbox_button_font);xbox_text(dc,30,187,"Would you like to apply or discard the changes?");}
  else if(xbox_tab==1)xbox_graphics_paint(dc);
  else if(xbox_tab==0){xbox_text(dc,30,96,"Urban Chaos: Riot Response");SelectObject(dc,xbox_button_font);xbox_text(dc,30,150,"Xbox-to-Windows recompilation");xbox_text(dc,30,185,"Use the original game menu for gameplay and difficulty settings.");}
  else if(xbox_tab==2)xbox_bindings_paint(dc);
  else{xbox_text(dc,30,96,"Audio");SelectObject(dc,xbox_button_font);xbox_text(dc,30,153,"Original game mix through the Windows audio backend.");xbox_text(dc,30,194,"Gameplay dialogue and cutouts are under investigation.");xbox_text(dc,30,235,"Use Windows volume controls; per-game mix settings are not yet exposed.");}
  SelectObject(dc,xbox_button_font);SetTextColor(dc,RGB(139,151,135));xbox_text(dc,30,r.bottom-31,"Esc: Close    |    Game continues while settings are open.");
  HPEN pen=CreatePen(PS_SOLID,1,RGB(35,43,32));HGDIOBJ oldPen=SelectObject(dc,pen);MoveToEx(dc,0,xbox_px(64),NULL);LineTo(dc,pixels.right,xbox_px(64));MoveToEx(dc,0,xbox_px(r.bottom-78),NULL);LineTo(dc,pixels.right,xbox_px(r.bottom-78));SelectObject(dc,oldPen);DeleteObject(pen);
  BitBlt(target,0,0,pixels.right,pixels.bottom,dc,0,0,SRCCOPY);SelectObject(dc,oldBitmap);DeleteObject(bitmap);DeleteDC(dc);EndPaint(w,&ps);return 0;}
 case WM_DRAWITEM:{
  DRAWITEMSTRUCT*d=(DRAWITEMSTRUCT*)lp;int saved=SaveDC(d->hDC),selected=xbox_choice_selected((int)d->CtlID),tab=d->CtlID>=201&&d->CtlID<=204,apply=d->CtlID==1500;
  HBRUSH b=CreateSolidBrush((d->itemState&ODS_SELECTED)?RGB(28,42,24):RGB(8,8,9));FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);SetBkMode(d->hDC,TRANSPARENT);
  SetTextColor(d->hDC,(d->itemState&ODS_DISABLED)?RGB(85,85,85):selected?(tab?RGB(129,222,65):RGB(242,242,242)):RGB(153,153,153));SelectObject(d->hDC,tab?xbox_tab_font:apply?xbox_button_title:xbox_button_font);
  char t[128];GetWindowTextA(d->hwndItem,t,sizeof(t));RECT r=d->rcItem;r.left+=xbox_px(tab?14:4);
  DrawTextA(d->hDC,t,-1,&r,(apply?DT_CENTER:DT_LEFT)|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
  if(selected){SIZE size;GetTextExtentPoint32A(d->hDC,t,(int)strlen(t),&size);b=CreateSolidBrush(tab?RGB(129,222,65):RGB(180,180,180));RECT line=d->rcItem;line.top=line.bottom-xbox_px(tab?2:1);if(!tab){line.left=r.left;line.right=line.left+size.cx;}FillRect(d->hDC,&line,b);DeleteObject(b);}
  if(apply||d->itemState&ODS_FOCUS){HPEN pen=CreatePen(PS_SOLID,1,(d->itemState&ODS_DISABLED)?RGB(35,43,30):RGB(129,222,65));SelectObject(d->hDC,pen);SelectObject(d->hDC,GetStockObject(NULL_BRUSH));RoundRect(d->hDC,1,1,d->rcItem.right-1,d->rcItem.bottom-1,xbox_px(8),xbox_px(8));RestoreDC(d->hDC,saved);DeleteObject(pen);}else RestoreDC(d->hDC,saved);return TRUE;
 }
 }
 return DefWindowProcA(w,m,wp,lp);
}
static void xbox_overlay_initialize(HWND parent,HINSTANCE instance){
 if(ucrr_native_frontend_enabled()){
  xbox_output_window=CreateWindowExA(0,"STATIC","Xbox game output",WS_CHILD|WS_VISIBLE,0,0,640,480,parent,NULL,instance,NULL);
  xbox_fps=xbox_pending_fps=d3d8_GetFpsCounter();
  xbox_msaa=xbox_pending_msaa=d3d8_GetHostMsaa();
  xbox_render_scale=xbox_pending_render_scale=d3d8_GetHostRenderScale();
  xbox_aspect=xbox_pending_aspect=d3d8_GetHostAspect();
#if defined(UCRR_LIGHT_REMOVAL)
  xbox_lighting_load(1);
#endif
#ifdef UCRR_GRAPHICS_EFFECT_UI
  xbox_effects=xbox_pending_effects=uc_graphics_features_load();
#endif
  xbox_vsync=getenv("UCRR_ASYNC_PRESENT")==NULL;xbox_overlay_layout(parent);return;
 }
 {HRSRC resource=FindResourceA(instance,MAKEINTRESOURCEA(301),RT_RCDATA);if(resource){HGLOBAL data=LoadResource(instance,resource);DWORD count=0;if(data&&AddFontMemResourceEx(LockResource(data),SizeofResource(instance,resource),NULL,&count)&&count)xbox_font_family="LatoLatin";}}
 d3d8_SetHostMessageFilter(ucrr_overlay_message);
 xbox_output_window=CreateWindowExA(0,"STATIC","Xbox game output",WS_CHILD|WS_VISIBLE,0,0,640,480,parent,NULL,instance,NULL);
 xbox_overlay_font=CreateFontA(-16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
 xbox_overlay_title=CreateFontA(-23,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
 WNDCLASSA c={0};c.lpfnWndProc=xbox_overlay_proc;c.hInstance=instance;c.hCursor=LoadCursor(NULL,IDC_ARROW);c.lpszClassName="XboxRecompOverlay";RegisterClassA(&c);
 xbox_overlay_window=CreateWindowExA(WS_EX_LAYERED|WS_EX_CONTROLPARENT,c.lpszClassName,"Xbox Recomp Settings",WS_CHILD|WS_CLIPCHILDREN,0,0,640,480,parent,NULL,instance,NULL);
 if(!xbox_overlay_window||!SetLayeredWindowAttributes(xbox_overlay_window,0,242,LWA_ALPHA)){fprintf(stderr,"[OVERLAY] Child composition failed: %lu\n",GetLastError());}
 const char*labels[]={"X","Gameplay","Graphics","Controls","Audio","640x480","960x720","1280x720","1920x1080","2560x1440","ORIGINAL","16:9","STRETCH","FILL","WINDOWED","FULLSCREEN","OFF","ON","Apply","Apply","Discard","Cancel"};
 const int ids[]={101,201,202,203,204,1100,1101,1102,1103,1104,1200,1201,1202,1203,1300,1301,1400,1401,1500,1600,1601,1602};
 for(int i=0;i<22;++i){HWND b=CreateWindowExA(0,"BUTTON",labels[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,100,34,xbox_overlay_window,(HMENU)(INT_PTR)ids[i],instance,NULL);SendMessageA(b,WM_SETFONT,(WPARAM)xbox_overlay_font,FALSE);}
 CreateWindowExA(0,"BUTTON","Quit",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,40,40,xbox_overlay_window,(HMENU)(INT_PTR)100,instance,NULL);
 HWND scale_combo=CreateWindowExA(0,"COMBOBOX","Internal resolution",WS_CHILD|WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS,0,0,260,285,xbox_overlay_window,(HMENU)(INT_PTR)1800,instance,NULL);
 for(unsigned i=1;i<=8;++i){char option[96];snprintf(option,sizeof(option),"%uX%s  (%u x %u)",i,i==1?" - Original":"",640*i,480*i);SendMessageA(scale_combo,CB_ADDSTRING,0,(LPARAM)option);}
 SendMessage(scale_combo,CB_SETCURSEL,0,0);SetWindowSubclass(scale_combo,xbox_graphics_control_proc,1800,0);
 xbox_fps=xbox_pending_fps=d3d8_GetFpsCounter();
 CreateWindowExA(0,"BUTTON","FPS counter",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,120,30,xbox_overlay_window,(HMENU)(INT_PTR)1880,instance,NULL);
 CreateWindowExA(0,"BUTTON","VSync",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,64,30,xbox_overlay_window,(HMENU)(INT_PTR)1850,instance,NULL);
 WNDCLASSA scroll_class={0};scroll_class.lpfnWndProc=xbox_scroll_proc;scroll_class.hInstance=instance;scroll_class.hCursor=LoadCursor(NULL,IDC_ARROW);scroll_class.lpszClassName="XboxRecompScroll";RegisterClassA(&scroll_class);
 CreateWindowExA(0,scroll_class.lpszClassName,"Graphics scroll",WS_CHILD,0,0,10,100,xbox_overlay_window,(HMENU)(INT_PTR)1851,instance,NULL);
 for(int i=0;i<3;++i)CreateWindowExA(0,"BUTTON",i==0?"NONE":i==1?"2X":"4X",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,72,30,xbox_overlay_window,(HMENU)(INT_PTR)(1860+i),instance,NULL);
#if defined(UCRR_LIGHT_REMOVAL)
 CreateWindowExA(0,"BUTTON","Dynamic lighting",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,120,30,xbox_overlay_window,(HMENU)(INT_PTR)1870,instance,NULL);
 xbox_lighting_load(0);
#ifdef UCRR_GRAPHICS_EFFECT_UI
 xbox_effects=xbox_pending_effects=uc_graphics_features_load();
 for(int i=0;i<7;i++)CreateWindowExA(0,"BUTTON",xbox_effect_names[i],WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,120,30,xbox_overlay_window,(HMENU)(INT_PTR)(1871+i),instance,NULL);
#endif
#endif
 for(int id=1100;id<=1880;++id){HWND control=GetDlgItem(xbox_overlay_window,id);if(control&&((id!=1500&&id<1600)||id>=1801))SetWindowSubclass(control,xbox_graphics_control_proc,id,0);}
 xbox_msaa=xbox_pending_msaa=d3d8_GetHostMsaa();
 xbox_render_scale=xbox_pending_render_scale=d3d8_GetHostRenderScale();
 xbox_aspect=xbox_pending_aspect=d3d8_GetHostAspect();
 INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_BAR_CLASSES};InitCommonControlsEx(&controls);
 HWND slider=CreateWindowExA(0,TRACKBAR_CLASSA,"Mouse sensitivity",WS_CHILD|WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,0,0,100,34,xbox_overlay_window,(HMENU)(INT_PTR)1700,instance,NULL);
 SendMessageA(slider,TBM_SETRANGE,TRUE,MAKELONG(25,1600));SendMessageA(slider,TBM_SETLINESIZE,0,5);SendMessageA(slider,TBM_SETPAGESIZE,0,100);
 const char*control_labels[]={"Controller","Keyboard & Mouse","Previous","Next","Reset defaults"};
 for(int i=0;i<5;++i)CreateWindowExA(0,"BUTTON",control_labels[i],WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,100,34,xbox_overlay_window,(HMENU)(INT_PTR)(1900+i),instance,NULL);
 for(int i=0;i<32;++i){HWND button=CreateWindowExA(0,"BUTTON",(i&1)?"X":"Unbound",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,100,34,xbox_overlay_window,(HMENU)(INT_PTR)(2000+i),instance,NULL);SetWindowSubclass(button,xbox_binding_control_proc,2000+i,0);}
 WNDCLASSA bindings_scroll={0};bindings_scroll.lpfnWndProc=xbox_bindings_scroll_proc;bindings_scroll.hInstance=instance;bindings_scroll.hCursor=LoadCursor(NULL,IDC_ARROW);bindings_scroll.lpszClassName="XboxBindingsScroll";RegisterClassA(&bindings_scroll);
 CreateWindowExA(0,bindings_scroll.lpszClassName,"Controller inputs scroll",WS_CHILD,0,0,10,100,xbox_overlay_window,(HMENU)(INT_PTR)1920,instance,NULL);
 for(int i=0;i<2;++i){HWND s=CreateWindowExA(0,TRACKBAR_CLASSA,i?"Right stick deadzone":"Left stick deadzone",WS_CHILD|WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,0,0,100,34,xbox_overlay_window,(HMENU)(INT_PTR)(1910+i),instance,NULL);SendMessage(s,TBM_SETRANGE,TRUE,MAKELONG(0,30));}
 xbox_vsync=getenv("UCRR_ASYNC_PRESENT")==NULL;xbox_overlay_labels();xbox_overlay_layout(parent);
}
/* Called by every host message pump, before translating button-focused keys. */
int ucrr_overlay_message(MSG*msg){
 if(!xbox_overlay_open)return 0;
 if(msg->message==WM_KEYDOWN&&msg->wParam==VK_ESCAPE){if(!(msg->lParam&(1LL<<30)))xbox_overlay_action(GetParent(xbox_overlay_window),101);return 1;}
 return IsDialogMessageA(xbox_overlay_window,msg);
}
#endif
