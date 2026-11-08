// Urban Chaos host keyboard adapter. Physical events and focus remain owned by
// xbe_trial_menu_input; this file maps its snapshot into the guest control slots.
#include "xbe_host_input.h"
#include <array>

namespace trial_input_profile {
unsigned Read(const wchar_t* path,const wchar_t* section,const wchar_t* key,unsigned fallback,unsigned maximum){
    wchar_t text[32];
    const DWORD length=GetPrivateProfileStringW(section,key,L"",text,32,path);
    if(!length||length>=31)return fallback;
    unsigned value=0;bool digit=false;
    const wchar_t* cursor=text;
    while(*cursor==L' '||*cursor==L'\t')++cursor;
    for(;*cursor>=L'0'&&*cursor<=L'9';++cursor){
        const unsigned next=unsigned(*cursor-L'0');
        if(next>maximum||value>(maximum-next)/10)return fallback;
        value=value*10+next;digit=true;
    }
    while(*cursor==L' '||*cursor==L'\t')++cursor;
    return digit&&!*cursor&&value<=maximum?value:fallback;
}
}

struct UCRRKeyboardConfig {unsigned binding[UCRR_KEYBOARD_ACTIONS][2]{};};
static UCRRKeyboardConfig keyboard_config;
static SRWLOCK keyboard_lock=SRWLOCK_INIT;
static INIT_ONCE keyboard_once=INIT_ONCE_STATIC_INIT;
static LONG keyboard_mode=1,keyboard_consumed_click=0;

static bool keyboard_valid(unsigned source){
    return source<=UCRR_KEYBOARD_WHEEL_DOWN&&source!=VK_ESCAPE&&source!=VK_LWIN&&source!=VK_RWIN;
}
static bool keyboard_valid_action(unsigned action,unsigned source){
    return keyboard_valid(source)&&(action!=UCRR_KEYBOARD_ORDERS||source<UCRR_KEYBOARD_WHEEL_UP);
}
static bool keyboard_is_mouse(unsigned source){
    switch(source){case VK_LBUTTON:case VK_RBUTTON:case VK_MBUTTON:case VK_XBUTTON1:case VK_XBUTTON2:return true;default:return false;}
}
static void keyboard_defaults(UCRRKeyboardConfig* config){
    *config={};
    constexpr unsigned defaults[UCRR_KEYBOARD_ACTIONS]={
        'R','X','C','V','T','E',VK_LCONTROL,VK_LBUTTON,
        VK_RETURN,VK_BACK,VK_SHIFT,VK_RBUTTON,VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,
        'A','D','W','S','J','L','I','K',VK_MBUTTON};
    for(unsigned action=0;action<UCRR_KEYBOARD_ACTIONS;++action)config->binding[action][0]=defaults[action];
    config->binding[0][1]='F';config->binding[7][1]=VK_RCONTROL;
}
static BOOL CALLBACK keyboard_load(PINIT_ONCE,PVOID,PVOID*){
    UCRRKeyboardConfig loaded;keyboard_defaults(&loaded);
    wchar_t path[MAX_PATH];
    if(controller_path(path))for(unsigned action=0;action<UCRR_KEYBOARD_ACTIONS;++action)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Binding%u_%u",action,slot);
        const unsigned value=trial_input_profile::Read(path,L"KeyboardMouse",key,loaded.binding[action][slot],UCRR_KEYBOARD_WHEEL_DOWN);
        if(keyboard_valid_action(action,value))loaded.binding[action][slot]=value;
    }
    keyboard_config=loaded;return TRUE;
}
static void keyboard_init(){InitOnceExecuteOnce(&keyboard_once,keyboard_load,nullptr,nullptr);}
int xbox_KeyboardMode(){return InterlockedCompareExchange(&keyboard_mode,0,0)!=0;}
void xbox_KeyboardNoteInput(){InterlockedExchange(&keyboard_mode,1);}
void xbox_KeyboardConsumeMouseClick(int held){InterlockedExchange(&keyboard_consumed_click,held!=0);}
unsigned xbox_KeyboardBinding(unsigned action,unsigned slot){
    if(action>=UCRR_KEYBOARD_ACTIONS||slot>=2)return 0;
    keyboard_init();AcquireSRWLockShared(&keyboard_lock);
    const unsigned source=keyboard_config.binding[action][slot];
    ReleaseSRWLockShared(&keyboard_lock);return source;
}

static bool keyboard_source_matches(unsigned binding,unsigned source){
    if(!binding||!source)return false;
    if(binding==source)return true;
    switch(binding){
    case VK_SHIFT:return source==VK_LSHIFT||source==VK_RSHIFT;
    case VK_CONTROL:return source==VK_LCONTROL||source==VK_RCONTROL;
    case VK_MENU:return source==VK_LMENU||source==VK_RMENU;
    default:return false;
    }
}
static unsigned keyboard_orders_source_mask(unsigned source,int* gameplay){
    keyboard_init();AcquireSRWLockShared(&keyboard_lock);
    unsigned slots=0;*gameplay=0;
    for(unsigned slot=0;slot<2;++slot)if(keyboard_source_matches(keyboard_config.binding[24][slot],source))slots|=1u<<slot;
    if(slots)for(unsigned action=0;action<24;++action)
        for(unsigned binding:keyboard_config.binding[action])if(keyboard_source_matches(binding,source))*gameplay=1;
    ReleaseSRWLockShared(&keyboard_lock);return slots;
}

static SRWLOCK keyboard_wheel_lock=SRWLOCK_INIT;
static std::array<unsigned,16> keyboard_wheel_queue{};
static unsigned keyboard_wheel_head=0,keyboard_wheel_count=0,keyboard_wheel_active=0;
static int keyboard_wheel_remainder=0;
static void keyboard_wheel_clear(){
    AcquireSRWLockExclusive(&keyboard_wheel_lock);
    keyboard_wheel_head=keyboard_wheel_count=keyboard_wheel_active=0;keyboard_wheel_remainder=0;
    ReleaseSRWLockExclusive(&keyboard_wheel_lock);
}
static void keyboard_wheel_record(int delta){
    AcquireSRWLockExclusive(&keyboard_wheel_lock);
    keyboard_wheel_remainder+=std::clamp(delta,-1920,1920);
    const int steps=keyboard_wheel_remainder/WHEEL_DELTA;
    keyboard_wheel_remainder%=WHEEL_DELTA;
    for(int i=0;i<std::abs(steps);++i)if(keyboard_wheel_count<keyboard_wheel_queue.size()){
        keyboard_wheel_queue[(keyboard_wheel_head+keyboard_wheel_count++)%keyboard_wheel_queue.size()]=
            steps>0?UCRR_KEYBOARD_WHEEL_UP:UCRR_KEYBOARD_WHEEL_DOWN;
    }
    ReleaseSRWLockExclusive(&keyboard_wheel_lock);
}
static void keyboard_wheel_publish(){
    AcquireSRWLockExclusive(&keyboard_wheel_lock);
    // A neutral guest update separates queued notches; repeated reads share one pulse.
    if(keyboard_wheel_active)keyboard_wheel_active=0;
    else if(keyboard_wheel_count){
        keyboard_wheel_active=keyboard_wheel_queue[keyboard_wheel_head];
        keyboard_wheel_head=(keyboard_wheel_head+1)%keyboard_wheel_queue.size();--keyboard_wheel_count;
    }
    ReleaseSRWLockExclusive(&keyboard_wheel_lock);
}

const char* xbox_KeyboardSourceName(unsigned source){
    switch(source){
    case 0:return "Unbound";
    case VK_LBUTTON:return "Mouse Left";case VK_RBUTTON:return "Mouse Right";case VK_MBUTTON:return "Mouse Middle";
    case VK_XBUTTON1:return "Mouse 4";case VK_XBUTTON2:return "Mouse 5";
    case UCRR_KEYBOARD_WHEEL_UP:return "Wheel Up";case UCRR_KEYBOARD_WHEEL_DOWN:return "Wheel Down";
    }
    thread_local char label[64];
    unsigned scan=MapVirtualKeyA(source,MAPVK_VK_TO_VSC);
    if(source==VK_RCONTROL||source==VK_RMENU||(source>=VK_PRIOR&&source<=VK_DOWN)||source==VK_INSERT||source==VK_DELETE)scan|=0x100;
    if(!GetKeyNameTextA(LONG(scan<<16),label,sizeof(label)))std::snprintf(label,sizeof(label),"Key %u",source);
    return label;
}
void xbox_KeyboardActionLabel(unsigned action,char* destination,unsigned capacity){
    if(!destination||!capacity)return;
    unsigned source=xbox_KeyboardBinding(action,0);
    if(!source)source=xbox_KeyboardBinding(action,1);
    std::snprintf(destination,capacity,"%s",xbox_KeyboardSourceName(source));
}

static unsigned keyboard_apply_bindings(XBOX_GAMEPAD* pad,int mouse_active){
    keyboard_init();AcquireSRWLockShared(&keyboard_lock);
    const auto profile=keyboard_config;
    ReleaseSRWLockShared(&keyboard_lock);
    AcquireSRWLockShared(&keyboard_wheel_lock);const unsigned wheel=keyboard_wheel_active;
    ReleaseSRWLockShared(&keyboard_wheel_lock);
    bool consumed=InterlockedCompareExchange(&keyboard_consumed_click,0,0)!=0;
    if(consumed&&!(GetAsyncKeyState(VK_LBUTTON)&0x8000)){xbox_KeyboardConsumeMouseClick(0);consumed=false;}
    bool active[24]{};
    for(unsigned action=0;action<24;++action)for(unsigned source:profile.binding[action]){
        if(!source||(source==VK_LBUTTON&&consumed))continue;
        if(source>=UCRR_KEYBOARD_WHEEL_UP)active[action]|=mouse_active&&source==wheel;
        else if(mouse_active||!keyboard_is_mouse(source))active[action]|=keyboard_key_down(source)!=0;
    }
    for(unsigned action=0;action<24;++action)if(active[action]){
        xbox_KeyboardNoteInput();
        if(action<8)pad->bAnalogButtons[action]=255;
        else if(action<16)pad->wButtons|=controller_digital[action-8];
    }
    unsigned axis_mask=0;
    SHORT* axes[]={&pad->sThumbLX,&pad->sThumbLY,&pad->sThumbRX,&pad->sThumbRY};
    for(unsigned axis=0;axis<4;++axis){
        const unsigned pair=16+axis*2;
        if(active[pair]||active[pair+1]){
            axis_mask|=1u<<axis;
            const int difference=int(active[pair+1])-int(active[pair]);
            *axes[axis]=SHORT(32767*(axis%2?-difference:difference));
        }
    }
    return axis_mask;
}

