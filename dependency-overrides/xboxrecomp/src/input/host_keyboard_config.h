#ifndef UCRR_HOST_KEYBOARD_CONFIG_H
#define UCRR_HOST_KEYBOARD_CONFIG_H
#include "../../include/host_controller.h"
typedef struct {unsigned binding[UCRR_KEYBOARD_ACTIONS][2];} UCRRKeyboardConfig;
static UCRRKeyboardConfig keyboard_config;
static INIT_ONCE keyboard_once=INIT_ONCE_STATIC_INIT;
static SRWLOCK keyboard_lock=SRWLOCK_INIT;
static int keyboard_valid(unsigned v){return v<=UCRR_KEYBOARD_WHEEL_DOWN&&v!=VK_ESCAPE&&v!=VK_LWIN&&v!=VK_RWIN;}
static int keyboard_valid_action(unsigned a,unsigned v){return keyboard_valid(v)&&!(a==UCRR_KEYBOARD_ORDERS&&v>=UCRR_KEYBOARD_WHEEL_UP);}
static LONG keyboard_mode=1;
int xbox_KeyboardMode(void){return InterlockedCompareExchange(&keyboard_mode,0,0)!=0;}
void xbox_KeyboardNoteInput(void){InterlockedExchange(&keyboard_mode,1);}
unsigned xbox_KeyboardActionCount(void){return UCRR_KEYBOARD_ACTIONS;}
/* Wheel intent is published once per original controller update. Every reader
 * sees the same pulse, followed by a neutral update before the next pulse. */
static SRWLOCK keyboard_wheel_lock=SRWLOCK_INIT;
static unsigned keyboard_wheel_queue[16],keyboard_wheel_count,keyboard_wheel_active;
static int keyboard_wheel_remainder;
static void keyboard_wheel_clear(void){AcquireSRWLockExclusive(&keyboard_wheel_lock);keyboard_wheel_count=keyboard_wheel_active=0;keyboard_wheel_remainder=0;ReleaseSRWLockExclusive(&keyboard_wheel_lock);}
static void keyboard_wheel_record(int delta){
 AcquireSRWLockExclusive(&keyboard_wheel_lock);
 if(delta>1920)delta=1920;if(delta< -1920)delta=-1920;
 keyboard_wheel_remainder+=delta;
 while(keyboard_wheel_remainder>=WHEEL_DELTA||keyboard_wheel_remainder<=-WHEEL_DELTA){
  unsigned source=keyboard_wheel_remainder>0?UCRR_KEYBOARD_WHEEL_UP:UCRR_KEYBOARD_WHEEL_DOWN;
  keyboard_wheel_remainder+=keyboard_wheel_remainder>0?-WHEEL_DELTA:WHEEL_DELTA;
  if(keyboard_wheel_count<16)keyboard_wheel_queue[keyboard_wheel_count++]=source;
 }
 ReleaseSRWLockExclusive(&keyboard_wheel_lock);
}
static void keyboard_wheel_publish(void){
 AcquireSRWLockExclusive(&keyboard_wheel_lock);
 if(keyboard_wheel_active)keyboard_wheel_active=0;
 else if(keyboard_wheel_count){keyboard_wheel_active=keyboard_wheel_queue[0];--keyboard_wheel_count;memmove(keyboard_wheel_queue,keyboard_wheel_queue+1,keyboard_wheel_count*sizeof(unsigned));}
 ReleaseSRWLockExclusive(&keyboard_wheel_lock);
}
static void keyboard_defaults(UCRRKeyboardConfig*c){
 static const unsigned first[24]={'R','X','C','V','T','E',VK_LCONTROL,VK_LBUTTON,VK_RETURN,VK_BACK,VK_SHIFT,VK_RBUTTON,VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,'A','D','W','S','J','L','I','K'};
 memset(c,0,sizeof(*c));for(unsigned i=0;i<24;++i)c->binding[i][0]=first[i];c->binding[0][1]='F';c->binding[7][1]=VK_RCONTROL;c->binding[UCRR_KEYBOARD_ORDERS][0]=VK_MBUTTON;
}
static BOOL CALLBACK keyboard_load(PINIT_ONCE once,PVOID parameter,PVOID*context){
 (void)once;(void)parameter;(void)context;keyboard_defaults(&keyboard_config);wchar_t path[MAX_PATH],key[32];
 if(controller_path(path))for(unsigned i=0;i<UCRR_KEYBOARD_ACTIONS;++i)for(unsigned j=0;j<2;++j){swprintf(key,32,L"Binding%u_%u",i,j);unsigned v=GetPrivateProfileIntW(L"KeyboardMouse",key,keyboard_config.binding[i][j],path);if(keyboard_valid_action(i,v))keyboard_config.binding[i][j]=v;}
 return TRUE;
}
static void keyboard_init(void){InitOnceExecuteOnce(&keyboard_once,keyboard_load,NULL,NULL);}
static int keyboard_save(const UCRRKeyboardConfig*c){
 wchar_t path[MAX_PATH],section[2048];size_t at=0;if(!controller_path(path))return 0;
 for(unsigned i=0;i<UCRR_KEYBOARD_ACTIONS;++i)for(unsigned j=0;j<2;++j)at+=swprintf(section+at,2048-at,L"Binding%u_%u=%u",i,j,c->binding[i][j])+1;
 section[at]=0;if(!WritePrivateProfileSectionW(L"KeyboardMouse",section,path))return 0;keyboard_config=*c;return 1;
}
unsigned xbox_KeyboardBinding(unsigned a,unsigned s){if(a>=UCRR_KEYBOARD_ACTIONS||s>=2)return 0;keyboard_init();AcquireSRWLockShared(&keyboard_lock);unsigned v=keyboard_config.binding[a][s];ReleaseSRWLockShared(&keyboard_lock);return v;}
/* Real keyboard events use sided modifiers; older profiles may use generic
 * Shift/Ctrl/Alt. A game binding claims the actual source before orders does. */
static int keyboard_source_matches(unsigned binding,unsigned source){
 if(!binding||!source)return 0;if(binding==source)return 1;
 return (binding==VK_SHIFT&&(source==VK_LSHIFT||source==VK_RSHIFT))||
        (binding==VK_CONTROL&&(source==VK_LCONTROL||source==VK_RCONTROL))||
        (binding==VK_MENU&&(source==VK_LMENU||source==VK_RMENU));
}
static unsigned keyboard_orders_source_mask(unsigned source,int *gameplay){
 unsigned mask=0;*gameplay=0;keyboard_init();AcquireSRWLockShared(&keyboard_lock);
 for(unsigned slot=0;slot<2;++slot)if(keyboard_source_matches(keyboard_config.binding[UCRR_KEYBOARD_ORDERS][slot],source))mask|=1u<<slot;
 if(mask)for(unsigned action=0;action<UCRR_CONTROLLER_ACTIONS;++action)for(unsigned slot=0;slot<2;++slot)
  if(keyboard_source_matches(keyboard_config.binding[action][slot],source))*gameplay=1;
 ReleaseSRWLockShared(&keyboard_lock);return mask;
}
int xbox_KeyboardBind(unsigned a,unsigned s,unsigned v){if(a>=UCRR_KEYBOARD_ACTIONS||s>=2||!keyboard_valid_action(a,v))return 0;keyboard_init();AcquireSRWLockExclusive(&keyboard_lock);UCRRKeyboardConfig c=keyboard_config;c.binding[a][s]=v;int ok=keyboard_save(&c);ReleaseSRWLockExclusive(&keyboard_lock);return ok;}
int xbox_KeyboardReset(void){keyboard_init();AcquireSRWLockExclusive(&keyboard_lock);UCRRKeyboardConfig c;keyboard_defaults(&c);int ok=keyboard_save(&c);ReleaseSRWLockExclusive(&keyboard_lock);return ok;}
/* A menu click stays consumed through gameplay handoff until physical release. */
static LONG keyboard_consumed_click;
void xbox_KeyboardConsumeMouseClick(int held){InterlockedExchange(&keyboard_consumed_click,!!held);}
static int keyboard_is_mouse(unsigned v){return v==VK_LBUTTON||v==VK_RBUTTON||v==VK_MBUTTON||v==VK_XBUTTON1||v==VK_XBUTTON2;}
unsigned xbox_KeyboardCapture(void){for(unsigned i=1;i<256;++i){if(i==VK_SHIFT||i==VK_CONTROL||i==VK_MENU||!keyboard_valid(i))continue;if(GetAsyncKeyState(i)&0x8000)return i;}return 0;}
const char*xbox_KeyboardSourceName(unsigned v){
 static __declspec(thread) char name[64];if(!v)return "Unbound";
 switch(v){case VK_LBUTTON:return "Mouse Left";case VK_RBUTTON:return "Mouse Right";case VK_MBUTTON:return "Mouse Middle";case VK_XBUTTON1:return "Mouse 4";case VK_XBUTTON2:return "Mouse 5";case UCRR_KEYBOARD_WHEEL_UP:return "Wheel Up";case UCRR_KEYBOARD_WHEEL_DOWN:return "Wheel Down";}
 UINT scan=MapVirtualKeyA(v,MAPVK_VK_TO_VSC);if(v==VK_RCONTROL||v==VK_RMENU||(v>=VK_PRIOR&&v<=VK_DOWN)||v==VK_INSERT||v==VK_DELETE)scan|=0x100;
 if(!GetKeyNameTextA((LONG)(scan<<16),name,sizeof(name)))snprintf(name,sizeof(name),"Key %u",v);return name;
}
void xbox_KeyboardActionLabel(unsigned a,char *dest,unsigned size){
 if(!dest||!size)return;unsigned source=xbox_KeyboardBinding(a,0);if(!source)source=xbox_KeyboardBinding(a,1);
 snprintf(dest,size,"%s",xbox_KeyboardSourceName(source));
}
static void keyboard_apply_bindings(XBOX_GAMEPAD*pad,int mouse_active){
 keyboard_init();UCRRKeyboardConfig c;AcquireSRWLockShared(&keyboard_lock);c=keyboard_config;ReleaseSRWLockShared(&keyboard_lock);
 AcquireSRWLockShared(&keyboard_wheel_lock);unsigned wheel=keyboard_wheel_active;ReleaseSRWLockShared(&keyboard_wheel_lock);
 int consumed=InterlockedCompareExchange(&keyboard_consumed_click,0,0)!=0;
 if(consumed&&!(GetAsyncKeyState(VK_LBUTTON)&0x8000)){xbox_KeyboardConsumeMouseClick(0);consumed=0;}
 int pressed[24]={0};for(unsigned i=0;i<24;++i)for(unsigned s=0;s<2;++s){unsigned v=c.binding[i][s];if(v>=UCRR_KEYBOARD_WHEEL_UP){if(mouse_active&&v==wheel)pressed[i]=1;}else if(v&&!(consumed&&v==VK_LBUTTON)&&(!keyboard_is_mouse(v)||mouse_active)&&keyboard_key_down(v))pressed[i]=1;}
 for(unsigned i=0;i<24;++i)if(pressed[i]){xbox_KeyboardNoteInput();break;}
 for(unsigned i=0;i<8;++i)if(pressed[i])pad->bAnalogButtons[i]=255;
 for(unsigned i=0;i<8;++i)if(pressed[i+8])pad->wButtons|=controller_digital[i];
 SHORT*axis[]={&pad->sThumbLX,&pad->sThumbLY,&pad->sThumbRX,&pad->sThumbRY};
 for(unsigned i=0;i<4;++i){unsigned base=16+i*2;if(pressed[base]||pressed[base+1])*axis[i]=(SHORT)(32767*((i&1)?pressed[base]-pressed[base+1]:pressed[base+1]-pressed[base]));}
}
#endif
