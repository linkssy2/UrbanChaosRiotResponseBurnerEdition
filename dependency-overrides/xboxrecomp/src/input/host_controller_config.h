#ifndef UCRR_HOST_CONTROLLER_CONFIG_H
#define UCRR_HOST_CONTROLLER_CONFIG_H
#include <stdio.h>
#include <wchar.h>
#include "../../include/host_controller.h"
typedef struct UCRRControllerConfig {
    unsigned binding[24][2],deadzone[2];
} UCRRControllerConfig;
static UCRRControllerConfig controller_config;
static INIT_ONCE controller_once=INIT_ONCE_STATIC_INIT;
static SRWLOCK controller_lock=SRWLOCK_INIT;
static LONG controller_custom;
static const WORD controller_digital[8]={0x10,0x20,0x40,0x80,1,2,4,8};
static void controller_defaults(UCRRControllerConfig*c){
    memset(c,0,sizeof(*c));for(unsigned i=0;i<24;++i)c->binding[i][0]=i+1;
}
static int controller_path(wchar_t *path){
    DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);if(!n||n>=MAX_PATH)return 0;
    wchar_t*s=wcsrchr(path,L'\\');if(!s||(size_t)(s-path)+16>=MAX_PATH)return 0;
    wcscpy(s+1,L"xboxrecomp.ini");return 1;
}
static void controller_publish(void){
    UCRRControllerConfig defaults;controller_defaults(&defaults);
    InterlockedExchange(&controller_custom,memcmp(&defaults,&controller_config,sizeof(defaults))!=0);
}
static BOOL CALLBACK controller_load(PINIT_ONCE once,PVOID parameter,PVOID *context){
    (void)once;(void)parameter;(void)context;controller_defaults(&controller_config);
    wchar_t path[MAX_PATH],key[32];
    if(controller_path(path)){
        for(unsigned i=0;i<24;++i)for(unsigned j=0;j<2;++j){
            swprintf(key,32,L"Binding%u_%u",i,j);
            unsigned v=GetPrivateProfileIntW(L"Controller",key,controller_config.binding[i][j],path);
            if(v<=24)controller_config.binding[i][j]=v;
        }
        for(unsigned i=0;i<2;++i){swprintf(key,32,L"Deadzone%u",i);
            unsigned v=GetPrivateProfileIntW(L"Controller",key,0,path);
            if(v<=30)controller_config.deadzone[i]=v;
        }
    }
    controller_publish();return TRUE;
}
static void controller_init(void){InitOnceExecuteOnce(&controller_once,controller_load,NULL,NULL);}
/* Replace one complete profile section, then publish only a successful save.
 * Graphics and other INI sections are never overwritten. */
static int controller_save(const UCRRControllerConfig*c){
    wchar_t path[MAX_PATH],section[2048];size_t at=0;
    if(!controller_path(path))return 0;
    for(unsigned i=0;i<24;++i)for(unsigned j=0;j<2;++j)
        at+=swprintf(section+at,2048-at,L"Binding%u_%u=%u",i,j,c->binding[i][j])+1;
    for(unsigned i=0;i<2;++i)at+=swprintf(section+at,2048-at,L"Deadzone%u=%u",i,c->deadzone[i])+1;
    section[at]=0;
    if(!WritePrivateProfileSectionW(L"Controller",section,path))return 0;
    controller_config=*c;controller_publish();return 1;
}
unsigned xbox_ControllerBinding(unsigned a,unsigned s){
    if(a>=24||s>=2)return 0;controller_init();AcquireSRWLockShared(&controller_lock);
    unsigned v=controller_config.binding[a][s];ReleaseSRWLockShared(&controller_lock);return v;
}
int xbox_ControllerBind(unsigned a,unsigned s,unsigned v){
    if(a>=24||s>=2||v>24)return 0;controller_init();AcquireSRWLockExclusive(&controller_lock);
    UCRRControllerConfig c=controller_config;c.binding[a][s]=v;int ok=controller_save(&c);
    ReleaseSRWLockExclusive(&controller_lock);return ok;
}
int xbox_ControllerReset(void){
    controller_init();AcquireSRWLockExclusive(&controller_lock);UCRRControllerConfig c;controller_defaults(&c);
    int ok=controller_save(&c);ReleaseSRWLockExclusive(&controller_lock);return ok;
}
unsigned xbox_ControllerDeadzone(unsigned s){
    if(s>1)return 0;controller_init();AcquireSRWLockShared(&controller_lock);
    unsigned v=controller_config.deadzone[s];ReleaseSRWLockShared(&controller_lock);return v;
}
int xbox_ControllerSetDeadzone(unsigned s,unsigned v){
    if(s>1||v>30)return 0;controller_init();AcquireSRWLockExclusive(&controller_lock);
    UCRRControllerConfig c=controller_config;c.deadzone[s]=v;int ok=controller_save(&c);
    ReleaseSRWLockExclusive(&controller_lock);return ok;
}
const char *xbox_ControllerSourceName(unsigned s){
    static const char*names[]={"Unbound","A","B","X","Y","LB","RB","LT","RT","Start","Back","LS click","RS click","D-pad Up","D-pad Down","D-pad Left","D-pad Right","LS Left","LS Right","LS Up","LS Down","RS Left","RS Right","RS Up","RS Down"};
    return s<=24?names[s]:"Invalid";
}
static void controller_strengths(const XBOX_GAMEPAD*p,int v[24]){
    for(unsigned i=0;i<8;++i)v[i]=(p->bAnalogButtons[i]*32768+127)/255;
    for(unsigned i=0;i<8;++i)v[8+i]=(p->wButtons&controller_digital[i])?32768:0;
    const int axis[]={p->sThumbLX,p->sThumbLY,p->sThumbRX,p->sThumbRY};
    for(unsigned i=0;i<4;++i){unsigned base=16+i*2;int x=axis[i];
        if(i&1){v[base]=x>0?x:0;v[base+1]=x<0?-x:0;}
        else{v[base]=x<0?-x:0;v[base+1]=x>0?x:0;}
    }
}
static void controller_apply(XBOX_GAMEPAD*p){
    /* Default profile is bit-identical and avoids all gathering/math. */
    if(!controller_custom)return;
    UCRRControllerConfig c;AcquireSRWLockShared(&controller_lock);c=controller_config;ReleaseSRWLockShared(&controller_lock);
    int raw[24],v[24];controller_strengths(p,raw);
    for(unsigned i=16;i<24;++i){int dz=(int)c.deadzone[(i-16)/4]*32768/100;
        if(dz)raw[i]=raw[i]<=dz?0:(raw[i]-dz)*32768/(32768-dz);}
    for(unsigned i=0;i<24;++i){unsigned a=c.binding[i][0],b=c.binding[i][1];int x=a?raw[a-1]:0,y=b?raw[b-1]:0;v[i]=x>y?x:y;}
    memset(p,0,sizeof(*p));for(unsigned i=0;i<8;++i)p->bAnalogButtons[i]=(BYTE)((v[i]*255+16384)/32768);
    for(unsigned i=0;i<8;++i)if(v[8+i]>4096)p->wButtons|=controller_digital[i];
    SHORT*axis[]={&p->sThumbLX,&p->sThumbLY,&p->sThumbRX,&p->sThumbRY};
    for(unsigned i=0;i<4;++i){unsigned base=16+i*2;int x=(i&1)?v[base]-v[base+1]:v[base+1]-v[base];
        *axis[i]=(SHORT)(x>32767?32767:x< -32768?-32768:x);}
}
static LONG controller_consumed_confirm[4];
void xbox_ControllerConsumeConfirm(unsigned port){if(port<4)InterlockedExchange(&controller_consumed_confirm[port],1);}
static void controller_confirm_filter(unsigned port,XINPUT_STATE *state,int filter){
    if(!InterlockedCompareExchange(&controller_consumed_confirm[port],0,0))return;
    if(!state||!(state->Gamepad.wButtons&XINPUT_GAMEPAD_A))InterlockedExchange(&controller_consumed_confirm[port],0);
    else if(filter)state->Gamepad.wButtons&=(WORD)~XINPUT_GAMEPAD_A;
}
unsigned xbox_ControllerCapturePort(unsigned *captured_port){
    if(captured_port)*captured_port=UINT32_MAX;
    for(DWORD port=0;port<4;++port){XINPUT_STATE s;DWORD result=XInputGetState(port,&s);
        if(result!=ERROR_SUCCESS){if(result==ERROR_DEVICE_NOT_CONNECTED)controller_confirm_filter(port,NULL,0);continue;}
        controller_confirm_filter(port,&s,0);if(captured_port)*captured_port=port;
        static const WORD bits[]={XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y,XINPUT_GAMEPAD_LEFT_SHOULDER,XINPUT_GAMEPAD_RIGHT_SHOULDER};
        for(unsigned i=0;i<6;++i)if(s.Gamepad.wButtons&bits[i])return i+1;
        if(s.Gamepad.bLeftTrigger>127)return 7;if(s.Gamepad.bRightTrigger>127)return 8;
        for(unsigned i=0;i<8;++i)if(s.Gamepad.wButtons&controller_digital[i])return i+9;
        const int axis[]={s.Gamepad.sThumbLX,s.Gamepad.sThumbLY,s.Gamepad.sThumbRX,s.Gamepad.sThumbRY};
        for(unsigned i=0;i<4;++i){if(axis[i]< -16000)return 17+i*2+(i&1);if(axis[i]>16000)return 17+i*2+!(i&1);}
    }if(captured_port)*captured_port=UINT32_MAX;return 0;
}
unsigned xbox_ControllerCapture(void){return xbox_ControllerCapturePort(NULL);}
#endif
