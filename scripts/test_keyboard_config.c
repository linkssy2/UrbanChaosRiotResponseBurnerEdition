#include <windows.h>
#include <xinput.h>
#include <stdio.h>
#include <string.h>
#include "xinput_xbox.h"
static wchar_t saved[2048];static int fail_save,blocked;static unsigned char keys[256];
static BOOL key_down(int v){return !blocked&&keys[v];}
static SHORT capture_key(int v){return keys[v]?(SHORT)0x8000:0;}
static DWORD module_path(HMODULE m,wchar_t*p,DWORD n){(void)m;(void)n;wcscpy(p,L"E:\\fixture\\game.exe");return (DWORD)wcslen(p);}
static UINT read_config(const wchar_t*s,const wchar_t*k,int fallback,const wchar_t*p){(void)p;if(wcscmp(s,L"KeyboardMouse"))return fallback;for(const wchar_t*q=saved;*q;q+=wcslen(q)+1)if(!wcsncmp(q,k,wcslen(k))&&q[wcslen(k)]=='=')return _wtoi(q+wcslen(k)+1);return fallback;}
static BOOL write_config(const wchar_t*s,const wchar_t*v,const wchar_t*p){if(fail_save||wcscmp(s,L"KeyboardMouse")||wcscmp(p,L"E:\\fixture\\xboxrecomp.ini"))return FALSE;size_t n=0;do{size_t k=wcslen(v+n)+1;memcpy(saved+n,v+n,k*sizeof(wchar_t));n+=k;}while(v[n]);saved[n]=0;return TRUE;}
#define GetModuleFileNameW module_path
#define GetPrivateProfileIntW read_config
#define WritePrivateProfileSectionW write_config
#define GetAsyncKeyState capture_key
#define keyboard_key_down key_down
#include "../third_party/xboxrecomp/src/input/host_controller_config.h"
#include "../third_party/xboxrecomp/src/input/host_keyboard_config.h"
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 CHECK(xbox_KeyboardBinding(4,0)=='T'&&xbox_KeyboardBinding(4,1)==0);
 CHECK(xbox_KeyboardBinding(0,0)=='R'&&xbox_KeyboardBinding(0,1)=='F');
 XBOX_GAMEPAD p={0};keys[VK_LBUTTON]=keys[VK_RBUTTON]=keys['W']=1;keyboard_apply_bindings(&p,0);CHECK(p.sThumbLY==32767&&!p.wButtons&&!p.bAnalogButtons[7]);
 keyboard_apply_bindings(&p,1);CHECK(p.bAnalogButtons[7]==255&&(p.wButtons&0x80));
 xbox_KeyboardConsumeMouseClick(1);memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,1);
 CHECK(!p.bAnalogButtons[7]&&p.sThumbLY==32767&&(p.wButtons&0x80));
 keys[VK_LBUTTON]=0;keyboard_apply_bindings(&p,1);keys[VK_LBUTTON]=1;
 memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,1);CHECK(p.bAnalogButtons[7]==255);
 xbox_KeyboardConsumeMouseClick(1);xbox_KeyboardConsumeMouseClick(0);memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,1);CHECK(p.bAnalogButtons[7]==255);
 memset(keys,0,sizeof(keys));memset(&p,0,sizeof(p));keys['A']=keys['D']=1;keyboard_apply_bindings(&p,1);CHECK(!p.sThumbLX);
 CHECK(xbox_KeyboardBind(7,0,'T')&&xbox_KeyboardBind(7,1,0));memset(keys,0,sizeof(keys));keys['T']=1;memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,0);CHECK(p.bAnalogButtons[7]==255);
 blocked=1;memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,1);CHECK(!p.bAnalogButtons[7]);blocked=0;
 InitOnceInitialize(&keyboard_once);CHECK(xbox_KeyboardBinding(7,0)=='T');
 fail_save=1;CHECK(!xbox_KeyboardBind(7,0,'U')&&xbox_KeyboardBinding(7,0)=='T');CHECK(!xbox_KeyboardReset());fail_save=0;
 CHECK(!xbox_KeyboardBind(25,0,'T')&&!xbox_KeyboardBind(0,2,'T')&&!xbox_KeyboardBind(0,0,258)&&!xbox_KeyboardBind(0,0,VK_ESCAPE));
 CHECK(xbox_KeyboardActionCount()==25&&xbox_KeyboardBinding(24,0)==VK_MBUTTON);
 CHECK(xbox_KeyboardBind(24,0,'O')&&!xbox_KeyboardBind(24,1,UCRR_KEYBOARD_WHEEL_UP));
 CHECK(xbox_KeyboardBind(2,1,UCRR_KEYBOARD_WHEEL_UP)&&xbox_KeyboardBind(3,1,UCRR_KEYBOARD_WHEEL_DOWN));
 char label[64];xbox_KeyboardActionLabel(7,label,sizeof(label));CHECK(!strcmp(label,"T"));
 CHECK(xbox_KeyboardBind(7,0,0)&&xbox_KeyboardBind(7,1,VK_XBUTTON1));xbox_KeyboardActionLabel(7,label,sizeof(label));CHECK(!strcmp(label,"Mouse 4"));
 char compact_label[3]={'!','!','!'};xbox_KeyboardActionLabel(7,compact_label,sizeof(compact_label));CHECK(compact_label[2]==0);
 InitOnceInitialize(&keyboard_once);CHECK(xbox_KeyboardBinding(24,0)=='O'&&xbox_KeyboardBinding(2,1)==256&&xbox_KeyboardBinding(3,1)==257);
 /* Simulate a legacy section containing only the 24 existing slot pairs. */
 for(wchar_t*q=saved;*q;q+=wcslen(q)+1)if(!wcsncmp(q,L"Binding24_",10)){*q=0;break;}
 InitOnceInitialize(&keyboard_once);CHECK(xbox_KeyboardBinding(24,0)==VK_MBUTTON&&xbox_KeyboardBinding(7,1)==VK_XBUTTON1&&xbox_KeyboardBinding(2,1)==256);
 CHECK(xbox_KeyboardBind(2,0,0));xbox_KeyboardActionLabel(2,label,sizeof(label));CHECK(!strcmp(label,"Wheel Up"));
 memset(keys,0,sizeof(keys));keyboard_wheel_record(60);keyboard_wheel_publish();CHECK(!keyboard_wheel_active);
 keyboard_wheel_record(180);keyboard_wheel_publish();
 for(int n=0;n<5;++n){memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,1);CHECK(p.bAnalogButtons[XBOX_BUTTON_X]==255);}
 keyboard_wheel_publish();memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,1);CHECK(!p.bAnalogButtons[XBOX_BUTTON_X]);
 keyboard_wheel_publish();memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,0);CHECK(!p.bAnalogButtons[XBOX_BUTTON_X]);
 keyboard_wheel_clear();memset(&p,0,sizeof(p));keyboard_apply_bindings(&p,1);CHECK(!p.bAnalogButtons[XBOX_BUTTON_X]);
 memset(keys,0,sizeof(keys));keys[VK_LBUTTON]=1;CHECK(xbox_KeyboardCapture()==VK_LBUTTON);CHECK(!strcmp(xbox_KeyboardSourceName(VK_LBUTTON),"Mouse Left"));
 CHECK(xbox_KeyboardReset()&&xbox_KeyboardBinding(7,0)==VK_LBUTTON);
 /* Flashlight is the existing physical Black slot, including both saved keys. */
 CHECK(xbox_KeyboardBind(4,0,'T')&&xbox_KeyboardBind(4,1,VK_MBUTTON));
 InitOnceInitialize(&keyboard_once);CHECK(xbox_KeyboardBinding(4,0)=='T'&&xbox_KeyboardBinding(4,1)==VK_MBUTTON);
 memset(keys,0,sizeof(keys));memset(&p,0,sizeof(p));keys['T']=1;keyboard_apply_bindings(&p,1);CHECK(p.bAnalogButtons[XBOX_BUTTON_BLACK]==255&&!p.bAnalogButtons[XBOX_BUTTON_WHITE]);
 memset(keys,0,sizeof(keys));memset(&p,0,sizeof(p));keys[VK_MBUTTON]=1;keyboard_apply_bindings(&p,0);CHECK(!p.bAnalogButtons[XBOX_BUTTON_BLACK]);
 keyboard_apply_bindings(&p,1);CHECK(p.bAnalogButtons[XBOX_BUTTON_BLACK]==255&&!p.bAnalogButtons[XBOX_BUTTON_WHITE]);
 int gameplay=0;CHECK(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&gameplay);
 fail_save=1;CHECK(!xbox_KeyboardBind(4,1,0));CHECK(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&gameplay);fail_save=0;
 CHECK(xbox_KeyboardBind(4,0,VK_MBUTTON)&&xbox_KeyboardBind(4,1,0));CHECK(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&gameplay);
 CHECK(xbox_KeyboardBind(4,0,0));CHECK(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&!gameplay);
 CHECK(xbox_KeyboardBinding(24,0)==VK_MBUTTON);InitOnceInitialize(&keyboard_once);CHECK(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&!gameplay);
 CHECK(xbox_KeyboardBind(5,1,VK_MBUTTON));CHECK(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&gameplay);
 puts("PASS keyboard/mouse defaults, remap, save/reload/failure, blocked input, mouse capture gate, axis opposition, bounds and reset");return 0;
}
