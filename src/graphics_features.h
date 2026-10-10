#ifndef UCRR_GRAPHICS_FEATURES_H
#define UCRR_GRAPHICS_FEATURES_H
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
/* Startup-only visibility choices; keep per-frame experiment masks immutable.
 * bits: all particles, rotated particles, body blobs, decals, stencil shadows, hidden-particle preparation, sky models. */
static int uc_graphics_features_enabled(void){
 const char*s=getenv("UCRR_GRAPHICS_FEATURE_TOGGLES");return s&&!strcmp(s,"1");
}
static int uc_graphics_features_path(wchar_t *path){
 DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);if(!n||n>=MAX_PATH)return 0;
 wchar_t*p=wcsrchr(path,L'\\');if(!p||p-path+16>=MAX_PATH)return 0;
 wcscpy(p+1,L"xboxrecomp.ini");return 1;
}
static unsigned uc_graphics_features_load(void){
 wchar_t path[MAX_PATH];if(!uc_graphics_features_path(path))return 127;
 unsigned value=GetPrivateProfileIntW(L"Graphics",L"EffectVisibility",31,path);
 unsigned legacy=value<=31?value:31;
 value=GetPrivateProfileIntW(L"Graphics",L"EffectVisibilityV2",legacy|32u,path);
 unsigned v2=value<=63?value:(legacy|32u);
 value=GetPrivateProfileIntW(L"Graphics",L"EffectVisibilityV3",v2|64u,path);
 return value<=127?value:(v2|64u);
}
static int uc_graphics_features_save(unsigned value){
 wchar_t path[MAX_PATH],text[16];if(value>127||!uc_graphics_features_enabled()||!uc_graphics_features_path(path))return 0;
 swprintf(text,16,L"%u",value);
 return WritePrivateProfileStringW(L"Graphics",L"EffectVisibilityV3",text,path)!=0;
}
#endif
