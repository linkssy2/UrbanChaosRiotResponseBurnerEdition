#ifndef D3D8_RENDER_SCALE_H
#define D3D8_RENDER_SCALE_H
/* Renderer-owner Apply; guest sizes, pitches, UVs and clocks stay native. */
static unsigned ucrr_host_scale,ucrr_host_msaa;
static int (*ucrr_resize_surfaces)(int (*persist)(void*),void*);
void d3d8_SetHostSurfaceResize(int (*callback)(int (*)(void*),void*)){ucrr_resize_surfaces=callback;}
typedef struct {const char *key;unsigned value;} UcRenderSetting;
static int ucrr_persist_render_setting(void *opaque);
static int ucrr_apply_surface_setting(unsigned *active,const char *key,unsigned value){
    if(!ucrr_resize_surfaces)return 0;
    d3d8_BeforeHostMutation();unsigned previous=*active;*active=value;
    UcRenderSetting setting={key,value};
    int ok=previous==value?ucrr_persist_render_setting(&setting):ucrr_resize_surfaces(ucrr_persist_render_setting,&setting);
    if(!ok)*active=previous;return ok;
}
static unsigned ucrr_valid_render_scale(unsigned value){return value>=1&&value<=8?value:1;}
static int ucrr_render_settings_path(char path[MAX_PATH]){
    DWORD n=GetModuleFileNameA(NULL,path,MAX_PATH);
    if(!n||n>=MAX_PATH)return 0;
    char *slash=strrchr(path,'\\');if(!slash)return 0;
    if((size_t)(slash-path)+sizeof("\\xboxrecomp.ini")>MAX_PATH)return 0;
    memcpy(slash+1,"xboxrecomp.ini",sizeof("xboxrecomp.ini"));return 1;
}
unsigned d3d8_GetHostRenderScale(void){
    unsigned active=ucrr_host_scale;
    if(!active){char path[MAX_PATH];active=1;
        if(ucrr_render_settings_path(path))active=ucrr_valid_render_scale(GetPrivateProfileIntA("Graphics","InternalScale",1,path));
        fprintf(stderr,"[RENDER-SCALE] active=%ux native guest coordinates; live Apply supported\n",active);
    }
    ucrr_host_scale=active;return active;
}
unsigned d3d8_GetHostAspect(void){
    static unsigned loaded,aspect;
    if(!loaded){char path[MAX_PATH];loaded=1;if(ucrr_render_settings_path(path)){
        unsigned v=GetPrivateProfileIntA("Graphics","Aspect",0,path);if(v<=3)aspect=v;
    }}return aspect;
}
int d3d8_SaveHostAspect(unsigned value){
    char path[MAX_PATH],text[2]={(char)('0'+value),0};
    if(value>3||!ucrr_render_settings_path(path))return 0;
    return WritePrivateProfileStringA("Graphics","Aspect",text,path)!=0;
}
static int ucrr_persist_render_setting(void *opaque){
    UcRenderSetting *setting=(UcRenderSetting*)opaque;char path[MAX_PATH],text[12];
    snprintf(text,sizeof text,"%u",setting->value);
    return ucrr_render_settings_path(path)&&WritePrivateProfileStringA("Graphics",setting->key,text,path);
}
int d3d8_SaveHostRenderScale(unsigned value){
    if(value<1||value>8)return 0;d3d8_GetHostRenderScale();
    return ucrr_apply_surface_setting(&ucrr_host_scale,"InternalScale",value);
}
unsigned d3d8_GetHostMsaa(void){
    unsigned active=ucrr_host_msaa;
    if(!active){char path[MAX_PATH];active=1;
        if(ucrr_render_settings_path(path)){unsigned v=GetPrivateProfileIntA("Graphics","MSAA",1,path);if(v==2||v==4)active=v;}
    }ucrr_host_msaa=active;return active;
}
int d3d8_SaveHostMsaa(unsigned value){
    if(value!=1&&value!=2&&value!=4)return 0;d3d8_GetHostMsaa();
    return ucrr_apply_surface_setting(&ucrr_host_msaa,"MSAA",value);
}
static LONG ucrr_host_anisotropy;
static int ucrr_valid_anisotropy(unsigned v){return v==1||v==2||v==4||v==8||v==16;}
unsigned d3d8_GetHostAnisotropy(void){
    LONG active=InterlockedCompareExchange(&ucrr_host_anisotropy,0,0);
    if(!active){char path[MAX_PATH];unsigned value=1;
        if(ucrr_render_settings_path(path))value=GetPrivateProfileIntA("Graphics","Anisotropy",1,path);
        if(!ucrr_valid_anisotropy(value))value=1;
        active=InterlockedCompareExchange(&ucrr_host_anisotropy,(LONG)value,0);
        if(!active)active=(LONG)value;
    }return (unsigned)active;
}
int d3d8_SaveHostAnisotropy(unsigned value){
    char path[MAX_PATH],text[3];if(!ucrr_valid_anisotropy(value)||!ucrr_render_settings_path(path))return 0;
    snprintf(text,sizeof text,"%u",value);
    if(!WritePrivateProfileStringA("Graphics","Anisotropy",text,path))return 0;
    /* This setter runs on the renderer owner, like other live graphics state. */
    d3d8_BeforeHostMutation();
    InterlockedExchange(&ucrr_host_anisotropy,(LONG)value);return 1;
}
#endif
