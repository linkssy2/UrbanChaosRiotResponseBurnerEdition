#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include "app/app.h"
#include "core/input.h"
#include "xbe_trial_graphics_ui.h"
#include "xbe_intro_skip.h"
#include "xbe_audio_loop.h"
#include "xbe_conversation_skip.h"
#include "xbe_controls.h"
#include "xbe_controls_capture.h"

namespace {
std::recursive_mutex input_lock;
HWND game_window;
bool menu_mode=true, gameplay=false, captured=false, click_pending=false;
float click_x,click_y;
long dx=0,dy=0,order_x=0,order_y=0;
unsigned order_phase=0,order_buttons=0,order_selection=0;
unsigned mouse_sensitivity=400;
bool order_held=false;
bool escape_pause_held=false;
bool menu_right_down=false,menu_right_pending=false,menu_right_consumed=false;
bool graphics_release_pending[4]={};
trial_intro::Gate intro_gate;
constexpr UINT kCapture=WM_APP+71;
struct XBOX_GAMEPAD { WORD wButtons=0; BYTE bAnalogButtons[8]={}; SHORT sThumbLX=0,sThumbLY=0,sThumbRX=0,sThumbRY=0; };
const WORD controller_digital[]={16,32,64,128,1,2,4,8};
bool focused(){return game_window&&GetForegroundWindow()==game_window;}
BOOL keyboard_key_down(int key){return focused()&&!(key==VK_RBUTTON&&menu_right_consumed)&&(GetAsyncKeyState(key)&0x8000);}
int controller_path(wchar_t* path){auto p=app::ExeDir()/L"xboxrecomp.ini";if(p.native().size()>=MAX_PATH)return 0;wcscpy_s(path,MAX_PATH,p.c_str());return 1;}
}
#include "third_party/xboxrecomp/src/input/host_keyboard_config.h"
#include "xbe_controls_backend.inc"

namespace {
trial_controls::CaptureGate controls_capture;
unsigned RawKeyboardSource(){
    for(unsigned key=1;key<256;++key){
        if(key==VK_SHIFT||key==VK_CONTROL||key==VK_MENU)continue;
        if(GetAsyncKeyState(key)&0x8000)return key;
    }
    return 0;
}
bool ConversationDown(){
    unsigned source=xbox_KeyboardBinding(0,1);if(!source)source=xbox_KeyboardBinding(0,0);
    if(source>=UCRR_KEYBOARD_WHEEL_UP){
        AcquireSRWLockShared(&keyboard_wheel_lock);const bool down=source==keyboard_wheel_active;
        ReleaseSRWLockShared(&keyboard_wheel_lock);return down;
    }
    return source&&source<256&&(GetAsyncKeyState(source)&0x8000)!=0;
}
void ControlsCaptureMessage(UINT msg,WPARAM wp){
    const int wheel=GET_WHEEL_DELTA_WPARAM(wp);
    if(msg==WM_MOUSEWHEEL&&focused()&&wheel)controls_capture.Wheel(wheel>0?256:257);
    if(msg==WM_KILLFOCUS||(msg==WM_ACTIVATEAPP&&!wp))controls_capture.Cancel();
}
}
namespace trial_controls {
void BeginCapture(unsigned device){std::lock_guard guard(input_lock);controls_capture.Begin(device);}
void CancelCapture(){std::lock_guard guard(input_lock);controls_capture.Cancel();}
unsigned TakeCapture(){std::lock_guard guard(input_lock);return controls_capture.Take();}
bool CaptureWaiting(){std::lock_guard guard(input_lock);return controls_capture.active;}
}

void TrialMenuRightButton(bool down){
    std::lock_guard guard(input_lock);
    if(!down){menu_right_down=false;return;}
    if(menu_right_down)return;
    menu_right_down=true;
    if(focused()&&!intro_gate.owner&&(menu_mode||TrialGraphicsOpen())){
        menu_right_consumed=true;menu_right_pending=!TrialGraphicsOpen();
    }
}
bool TrialMenuRightBack(){
    std::lock_guard guard(input_lock);
    const bool back=menu_right_pending&&focused()&&menu_mode&&!TrialGraphicsOpen()&&!intro_gate.owner;
    menu_right_pending=false;
    if(!(GetAsyncKeyState(VK_RBUTTON)&0x8000)){menu_right_down=false;menu_right_consumed=false;}
    return back;
}
void TrialIntroMovie(uint32_t owner,uint32_t kind,bool opening,bool ready){
    std::lock_guard guard(input_lock);
    intro_gate.Context(owner,kind,opening,ready);
    if(owner)click_pending=false;
}
bool TrialIntroTake(uint32_t owner,unsigned port){
    std::lock_guard guard(input_lock);
    return focused()&&!TrialGraphicsOpen()&&intro_gate.Take(owner,port);
}
void TrialIntroClose(uint32_t owner){
    std::lock_guard guard(input_lock);
    intro_gate.Close(owner);click_pending=false;
}

static void ReconcileCapture(){
    bool want=focused()&&gameplay&&!menu_mode&&!TrialGraphicsOpen();
    if(captured!=want){captured=want;dx=dy=0;}
    if(want){RECT r;GetClientRect(game_window,&r);MapWindowPoints(game_window,nullptr,(POINT*)&r,2);ClipCursor(&r);SetCursor(nullptr);}
    else{ClipCursor(nullptr);SetCursor(LoadCursor(nullptr,IDC_ARROW));}
}
void TrialGraphicsInputChanged(){
    std::lock_guard guard(input_lock);
    for(auto& pending:graphics_release_pending)pending=true;
    click_pending=false;dx=dy=0;
    order_phase=order_buttons=0;order_held=false;keyboard_wheel_clear();
    xbox_KeyboardConsumeMouseClick(1);
    PostMessage(game_window,kCapture,0,0);
}
void TrialInputWindow(void* window){
    std::lock_guard guard(input_lock);game_window=(HWND)window;
    wchar_t path[MAX_PATH];
    if(controller_path(path))mouse_sensitivity=std::clamp(GetPrivateProfileIntW(L"Graphics",L"MouseSensitivity",400,path),25u,1600u);
    RAWINPUTDEVICE device{1,2,0,game_window};
    if(!RegisterRawInputDevices(&device,1,sizeof(device)))throw std::runtime_error("Raw mouse registration failed");
}
int TrialMouseActive(){std::lock_guard guard(input_lock);return captured&&focused()&&xbox_KeyboardMode();}
void TrialGameplay(){std::lock_guard guard(input_lock);if(!gameplay){gameplay=true;menu_mode=false;PostMessage(game_window,kCapture,0,0);}}
void TrialMouseTake(long* x,long* y,unsigned* sensitivity){
    std::lock_guard guard(input_lock);*x=dx;*y=dy;dx=dy=0;
    *sensitivity=mouse_sensitivity;
}
void ucrr_host_menu_mode(int enabled){
    std::lock_guard guard(input_lock);
    if(menu_mode!=!!enabled){menu_mode=!!enabled;click_pending=false;dx=dy=0;PostMessage(game_window,kCapture,0,0);}
}
uint64_t ucrr_host_menu_milliseconds(){return GetTickCount64();}
int ucrr_host_menu_pointer(float* x,float* y,int* click){
    std::lock_guard guard(input_lock);*click=0;
    if(!menu_mode||!focused()||TrialGraphicsOpen()||graphics_release_pending[0]){click_pending=false;return 0;}
    RECT r;POINT p;
    if(!GetClientRect(game_window,&r)||!GetCursorPos(&p)||!ScreenToClient(game_window,&p)||r.right<=0||r.bottom<=0)return 0;
    *x=p.x*640.f/r.right;*y=p.y*480.f/r.bottom;
    if(click_pending){*x=click_x;*y=click_y;*click=1;click_pending=false;}
    return *x>=0&&*x<640&&*y>=0&&*y<480;
}
static unsigned OrderDirection(){return std::max(std::abs(order_x),std::abs(order_y))<24?0:std::abs(order_x)>std::abs(order_y)?(order_x<0?4:8):(order_y<0?1:2);}
void xbox_KeyboardSourceEvent(unsigned source,int down){
    if(down)xbox_KeyboardNoteInput();
    int claimed=0;unsigned mask=keyboard_orders_source_mask(source,&claimed);
    if(!mask||claimed||!captured)return;
    if(down&&!order_phase){order_held=true;order_phase=1;order_x=order_y=0;}
    else if(!down&&order_held){order_held=false;order_selection=OrderDirection();}
    dx=dy=0;
}
void xbox_KeyboardWheel(int delta){if(captured)keyboard_wheel_record(delta);}
void TrialInputPublish(){
    std::lock_guard guard(input_lock);
    if(captured&&focused())keyboard_wheel_publish();else keyboard_wheel_clear();
    TrialConversationPublish(ConversationDown(),focused()&&!TrialGraphicsOpen()&&!menu_mode);
    if(!captured||!focused()){keyboard_wheel_clear();order_phase=order_buttons=0;order_held=false;return;}
    order_buttons=0;
    switch(order_phase){case 1:order_buttons=2;order_phase=2;break;case 2:order_phase=3;break;
    case 3:if(!order_held){order_buttons=order_selection;order_phase=4;}break;case 4:order_phase=0;break;}
}
bool TrialInputMessage(void*,uint32_t msg,uintptr_t wp,intptr_t lp,intptr_t* result){
    std::lock_guard guard(input_lock);
    ControlsCaptureMessage(msg,wp);
    if(TrialAudioTraceHotkey(msg,uint32_t(wp),uint64_t(lp),focused(),gameplay&&!menu_mode&&!TrialGraphicsOpen())){
        auto path=(app::ExeDir()/("audio-trace-"+std::to_string(GetTickCount64())+".csv")).string();
        TrialAudioTraceStart(path.c_str());*result=0;return true;
    }
    if(msg==WM_RBUTTONDOWN||msg==WM_RBUTTONUP)TrialMenuRightButton(msg==WM_RBUTTONDOWN);
    if(TrialGraphicsMessage(msg,wp,lp)){click_pending=false;dx=dy=0;*result=0;return true;}
    if(msg==WM_KILLFOCUS||(msg==WM_ACTIVATEAPP&&!wp)){
        intro_gate.FocusLost();
        menu_right_pending=menu_right_down=false;menu_right_consumed=(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0;
        captured=false;dx=dy=0;order_phase=order_buttons=0;order_held=false;click_pending=false;keyboard_wheel_clear();ClipCursor(nullptr);
    }
    if(msg==kCapture||msg==WM_SETFOCUS||msg==WM_SIZE)ReconcileCapture();
    if(msg==WM_SETCURSOR&&LOWORD(lp)==HTCLIENT){ReconcileCapture();*result=TRUE;return true;}
    // Record short taps as well as polled holds. Repeated WM_KEYDOWN cannot
    // produce another edge, and consumed intro clicks never reach a menu row.
    if((msg==WM_KEYDOWN||msg==WM_KEYUP)&&wp==VK_RETURN)
        intro_gate.Key(1,msg==WM_KEYDOWN,focused());
    if(msg==WM_LBUTTONDOWN||msg==WM_LBUTTONUP)
        intro_gate.Key(2,msg==WM_LBUTTONDOWN,focused());
    if(intro_gate.owner&&(msg==WM_LBUTTONDOWN||msg==WM_LBUTTONUP)){
        click_pending=false;xbox_KeyboardConsumeMouseClick(1);*result=0;return true;
    }
    if(msg==WM_KEYDOWN||msg==WM_KEYUP){
        unsigned key=unsigned(wp);
        if(key==VK_CONTROL)key=(lp&(1LL<<24))?VK_RCONTROL:VK_LCONTROL;
        if(key==VK_SHIFT)key=MapVirtualKey(unsigned(lp>>16)&255,MAPVK_VSC_TO_VK_EX);
        xbox_KeyboardSourceEvent(key,msg==WM_KEYDOWN);
    }
    if(msg==WM_LBUTTONDOWN&&menu_mode){
        float x,y;int ignored;
        if(ucrr_host_menu_pointer(&x,&y,&ignored)){click_x=x;click_y=y;click_pending=true;xbox_KeyboardConsumeMouseClick(1);}
    }
    if(msg==WM_INPUT){
        RAWINPUT raw;UINT n=sizeof(raw);
        if(GetRawInputData((HRAWINPUT)lp,RID_INPUT,&raw,&n,sizeof(RAWINPUTHEADER))!=(UINT)-1&&raw.header.dwType==RIM_TYPEMOUSE&&captured&&focused()){
            const unsigned downs[]={RI_MOUSE_LEFT_BUTTON_DOWN,RI_MOUSE_RIGHT_BUTTON_DOWN,RI_MOUSE_MIDDLE_BUTTON_DOWN,RI_MOUSE_BUTTON_4_DOWN,RI_MOUSE_BUTTON_5_DOWN};
            const unsigned ups[]={RI_MOUSE_LEFT_BUTTON_UP,RI_MOUSE_RIGHT_BUTTON_UP,RI_MOUSE_MIDDLE_BUTTON_UP,RI_MOUSE_BUTTON_4_UP,RI_MOUSE_BUTTON_5_UP};
            const unsigned sources[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
            for(unsigned i=0;i<5;i++)if(raw.data.mouse.usButtonFlags&downs[i])xbox_KeyboardSourceEvent(sources[i],1);
            if(!(raw.data.mouse.usFlags&MOUSE_MOVE_ABSOLUTE)){
                if(raw.data.mouse.lLastX||raw.data.mouse.lLastY)xbox_KeyboardNoteInput();
                auto add=[](long a,long b){return long(std::clamp(int64_t(a)+b,int64_t(-1000000),int64_t(1000000)));};
                if(order_held){order_x=add(order_x,raw.data.mouse.lLastX);order_y=add(order_y,raw.data.mouse.lLastY);}
                else{dx=add(dx,raw.data.mouse.lLastX);dy=add(dy,raw.data.mouse.lLastY);}
            }
            if(raw.data.mouse.usButtonFlags&RI_MOUSE_WHEEL)xbox_KeyboardWheel((SHORT)raw.data.mouse.usButtonData);
            for(unsigned i=0;i<5;i++)if(raw.data.mouse.usButtonFlags&ups[i])xbox_KeyboardSourceEvent(sources[i],0);
        }
    }
    return false;
}
void TrialInputPad(int port,input::Pad& pad){
    std::lock_guard guard(input_lock);
    if(port<0||port>=4)return;
    if(port==0&&controls_capture.active)
        controls_capture.Sample(RawKeyboardSource(),trial_controls::ControllerSource(pad),focused());
    const bool menu_right_back=port==0&&TrialMenuRightBack();
    if(port==0)pad.connected=true;
    intro_gate.Pad(port,pad.buttons,pad.analog,focused());
    if(port==0){
        unsigned intro_keys=((GetAsyncKeyState(VK_RETURN)&0x8000)?1u:0u)|
                            ((GetAsyncKeyState(VK_LBUTTON)&0x8000)?2u:0u);
        // Preserve the configured A/Start keys as well as the PC skip inputs.
        for(unsigned action:{0u,8u})for(unsigned slot=0;slot<2;++slot){
            unsigned key=xbox_KeyboardBinding(action,slot);
            if(key&&key<256&&keyboard_key_down(key))intro_keys|=4;
        }
        intro_gate.Keyboard(intro_keys,focused());
    }
    if(port==0&&focused())TrialFrontendPad(pad.buttons,pad.analog[0],pad.analog[1],pad.lx,pad.ly);
    if(graphics_release_pending[port]&&!TrialGraphicsOpen()){
        bool held=pad.buttons!=0;for(auto value:pad.analog)held|=value!=0;
        held|=std::abs(int(pad.lx))>7849||std::abs(int(pad.ly))>7849||
              std::abs(int(pad.rx))>8689||std::abs(int(pad.ry))>8689;
        if(port==0)for(int key=1;key<256&&!held;++key)held=(GetAsyncKeyState(key)&0x8000)!=0;
        if(!held)graphics_release_pending[port]=false;
    }
    if(TrialGraphicsOpen()||graphics_release_pending[port]){bool connected=pad.connected;pad={};pad.connected=connected;return;}
    if(intro_gate.OwnsPad(port)){bool connected=pad.connected;pad={};pad.connected=connected;return;}
    // Keep menu navigation on the physical controller; remaps apply to the
    // original gameplay slots before keyboard/mouse contributions are merged.
    if(gameplay&&!menu_mode)trial_controls::Remap(pad);
    if(port!=0||!focused())return;
    if(pad.buttons||std::abs(int(pad.rx))>8689||std::abs(int(pad.ry))>8689)InterlockedExchange(&keyboard_mode,0);
    XBOX_GAMEPAD keys;keyboard_apply_bindings(&keys,captured);keys.wButtons|=order_buttons;
    // CONTROL capped the title-facing stick magnitude in ucrr_host_stick_axis.
    // Keep keyboard movement within that circle before merging physical input;
    // full strength on both digital axes otherwise requests sqrt(2) movement.
    if(gameplay&&!menu_mode&&keys.sThumbLX&&keys.sThumbLY){
        keys.sThumbLX=SHORT(keys.sThumbLX*0.7071067811865475f);
        keys.sThumbLY=SHORT(keys.sThumbLY*0.7071067811865475f);
    }
    const bool escape=keyboard_key_down(VK_ESCAPE);
    if(!escape)escape_pause_held=false;
    if(gameplay&&!menu_mode){
        // Enter stays menu-only; a rebound Start key may pause as well as the
        // fixed Escape shortcut, without changing physical controller Start.
        keys.wButtons&=~16;
        for(unsigned slot=0;slot<2;++slot){
            const unsigned source=xbox_KeyboardBinding(8,slot);
            if(source&&source!=VK_RETURN&&source<256&&keyboard_key_down(source))keys.wButtons|=16;
            if(source>=256&&source==keyboard_wheel_active)keys.wButtons|=16;
        }
        if(escape){keys.wButtons|=16;escape_pause_held=true;}
    }else if(escape&&!escape_pause_held)keys.bAnalogButtons[1]=255;
    if(menu_right_back)keys.bAnalogButtons[1]=255;
    pad.buttons|=keys.wButtons;
    for(unsigned i=0;i<8;i++)pad.analog[i]=std::max(pad.analog[i],keys.bAnalogButtons[i]);
    if(keys.sThumbLX)pad.lx=keys.sThumbLX;if(keys.sThumbLY)pad.ly=keys.sThumbLY;
    if(keys.sThumbRX)pad.rx=keys.sThumbRX;if(keys.sThumbRY)pad.ry=keys.sThumbRY;
}
