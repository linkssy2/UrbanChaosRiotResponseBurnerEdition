#pragma once
#include <stddef.h>
#include <stdint.h>
#include <windows.h>
#include <d3d11.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNativeControlFrontend UCNativeControlFrontend;
enum { UC_NATIVE_FRONTEND_HORIZONTAL=1, UC_NATIVE_FRONTEND_OVER_GAME=2, UC_NATIVE_FRONTEND_LEVEL_BROWSER=4, UC_NATIVE_FRONTEND_REWARDS_BROWSER=8 };
enum { UC_NATIVE_FRONTEND_NONE=0, UC_NATIVE_FRONTEND_HOVER=1,
       UC_NATIVE_FRONTEND_ACTIVATE=2, UC_NATIVE_FRONTEND_BACK=3,
       UC_NATIVE_FRONTEND_DIFFICULTY=5, UC_NATIVE_FRONTEND_REWARDS=6 };
typedef struct UCNativeFrontendRow {
    uint32_t id,enabled,adjustable,keycap;
    const char *label,*value,*help;
    uint32_t artwork_id,completed; /* level browser only; source art0..11, completed0..4 (none, source difficulty+1) */
} UCNativeFrontendRow;
typedef struct UCNativeFrontendDetailText {
    uint32_t flags,color,font;
    float x,y;
    const char *prefix,*suffix;
} UCNativeFrontendDetailText;
typedef struct UCNativeFrontendDetailIcon {
    uint32_t kind,color; /* original resolved art identity, not a texture pointer */
    float x,y;
} UCNativeFrontendDetailIcon;
typedef struct UCNativeFrontendLevelDetails {
    uint32_t text_count,icon_count;
    const UCNativeFrontendDetailText *texts;
    const UCNativeFrontendDetailIcon *icons;
    uint32_t actions,difficulty,medals; /* bit0: original Difficulty action; bit1: original Rewards */
} UCNativeFrontendLevelDetails;
typedef struct UCNativeFrontendMenu {
    uint64_t identity;
    uint32_t flags,selected,count; /* selected=UINT32_MAX retains selection within identity */
    const char *title,*status,*footer;
    const UCNativeFrontendRow *rows; /* copied synchronously; maximum20 */
    const UCNativeFrontendLevelDetails *level; /* borrowed original presentation, copied */
} UCNativeFrontendMenu;
typedef struct UCNativeFrontendInput {
    uint32_t key; /* Windows virtual key; zero means mouse */
    float x,y;
    uint32_t client_width,client_height,click;
} UCNativeFrontendInput;
/* Activation direction:0=confirm/body click,-1/+1=explicit adjustment arrow. */
typedef struct UCNativeFrontendEvent {uint32_t kind,id,index;int32_t direction;} UCNativeFrontendEvent;

/* All state calls use the creating/render thread. The host queues window input,
 * owns guest menu transitions/settings/saves, and drains its rendering first.
 * No window, swapchain, Present, mission or profile is owned here. */
UCNativeControlFrontend *uc_native_control_frontend_create(ID3D11Device *,ID3D11DeviceContext *,
    const wchar_t *game_directory,char *error,size_t capacity);
void uc_native_control_frontend_destroy(UCNativeControlFrontend *);
int uc_native_control_frontend_menu(UCNativeControlFrontend *,const UCNativeFrontendMenu *,char *,size_t);
int uc_native_control_frontend_input(UCNativeControlFrontend *,const UCNativeFrontendInput *,UCNativeFrontendEvent *,char *,size_t);
/* Draw into an already-owned target. Restores the full parent context on every
 * path. 1=complete,0=rejected/no draw,-1=render error (pixels may be partial).
 * Movies accept 1/2/4 samples; menu snapshots require the resolved Present target.
 * draw_menu:0=uncropped movie on black,1=menu,2=stretched movie,
 * 3=menu transition (retain the last original information panel)
 * stretched to the complete output like CONTROL. Both retain the full texture;
 * ordinary intro movies keep aspect-preserving fit. */
int uc_native_control_frontend_draw(UCNativeControlFrontend *,ID3D11RenderTargetView *,
    const D3D11_VIEWPORT *,uint32_t client_width,uint32_t client_height,uint32_t draw_menu,char *,size_t);
/* Original movie suppression and whether to cover the menu with this movie are
 * host policies. Audio belongs to NativeMovie and stops on close/destruction. */
int uc_native_control_frontend_movie_open(UCNativeControlFrontend *,const wchar_t *,uint32_t loop,float volume,char *,size_t);
int uc_native_control_frontend_movie_pause(UCNativeControlFrontend *,uint32_t paused,char *,size_t);
int uc_native_control_frontend_movie_volume(UCNativeControlFrontend *,float volume,char *,size_t);
int uc_native_control_frontend_movie_tick(UCNativeControlFrontend *,char *,size_t); /* -1 error,0 done,1 playing */
int uc_native_control_frontend_movie_close(UCNativeControlFrontend *,char *,size_t);
int uc_native_control_frontend_movie_finished(UCNativeControlFrontend *); /* -1 wrong owner,0 playing,1 done */
/* Independent per-window-thread cursor cache. Borrowed handle: do not destroy. */
HCURSOR uc_native_control_frontend_cursor(HWND);
/* Call outside draw/context scope. Pumps this thread's window messages. This
 * changes presentation pacing only; it does not change CONTROL's game clock. */
int uc_native_control_frontend_pace(UCNativeControlFrontend *,uint32_t limit,char *,size_t);
#ifdef __cplusplus
}
#endif
