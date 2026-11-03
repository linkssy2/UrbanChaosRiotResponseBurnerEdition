// Standalone contract checks: no window, controller device or game process.
#undef NDEBUG
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "core/input.h"

namespace {
wchar_t profile[MAX_PATH];
bool path_available=true,focused=true,keys[256]{};
struct XBOX_GAMEPAD {WORD wButtons=0;BYTE bAnalogButtons[8]{};SHORT sThumbLX=0,sThumbLY=0,sThumbRX=0,sThumbRY=0;};
constexpr WORD controller_digital[]={16,32,64,128,1,2,4,8};
int controller_path(wchar_t* out){if(!path_available)return 0;wcscpy_s(out,MAX_PATH,profile);return 1;}
BOOL keyboard_key_down(int key){return focused&&key>0&&key<256&&keys[key];}
SHORT PhysicalKey(int key){return key>0&&key<256&&keys[key]?SHORT(0x8000):0;}
}
#define GetAsyncKeyState PhysicalKey
#include "xbe_keyboard_bindings.inc"
#include "xbe_controller_bindings.inc"
#undef GetAsyncKeyState

static void Reload(){
    keyboard_load(nullptr,nullptr,nullptr);controller_load(nullptr,nullptr,nullptr);
}
static void Write(const wchar_t* section,const wchar_t* key,const wchar_t* value){
    assert(WritePrivateProfileStringW(section,key,value,profile));
}
static XBOX_GAMEPAD Sample(bool capture=true){XBOX_GAMEPAD result;keyboard_apply_bindings(&result,capture);return result;}
static void Clear(){std::memset(keys,0,sizeof(keys));keyboard_wheel_clear();xbox_KeyboardConsumeMouseClick(0);focused=true;}

int main(){
    wchar_t temp[MAX_PATH],directory[MAX_PATH];assert(GetTempPathW(MAX_PATH,temp));
    assert(GetTempFileNameW(temp,L"ucb",0,directory));assert(DeleteFileW(directory));assert(CreateDirectoryW(directory,nullptr));
    swprintf(profile,MAX_PATH,L"%s\\xboxrecomp.ini",directory);
    keyboard_init();controller_init();
    assert(GetFileAttributesW(profile)==INVALID_FILE_ATTRIBUTES);
    assert(xbox_KeyboardBinding(0,0)=='R'&&xbox_KeyboardBinding(0,1)=='F');
    assert(xbox_KeyboardBinding(24,0)==VK_MBUTTON&&xbox_KeyboardBinding(25,0)==0&&xbox_KeyboardBinding(0,2)==0);
    assert(controller_identity(controller_config));
    path_available=false;Reload();assert(keyboard_config.binding[7][0]==VK_LBUTTON&&controller_identity(controller_config));path_available=true;

    // Old 24-row profiles preserve their slots and receive the later Orders default.
    Write(L"KeyboardMouse",L"Binding0_0",L"71");Write(L"KeyboardMouse",L"Binding0_1",L"0");
    Write(L"KeyboardMouse",L"Binding2_1",L"256");Write(L"Controller",L"Binding0_0",L"4");
    Write(L"Controller",L"Deadzone0",L"12");Reload();
    assert(xbox_KeyboardBinding(0,0)=='G'&&!xbox_KeyboardBinding(0,1));
    assert(keyboard_config.binding[2][1]==256&&keyboard_config.binding[24][0]==VK_MBUTTON);
    assert(xbox_ControllerBinding(0,0)==4&&controller_config.deadzone[0]==12);
    for(const wchar_t* malformed:{L"-1",L"no",L"12wrong",L"4294967296",L"999999999999999999999999999999999999",L"258"}){
        Write(L"KeyboardMouse",L"Binding0_0",malformed);Write(L"Controller",L"Binding0_0",malformed);
        Reload();assert(keyboard_config.binding[0][0]=='R'&&controller_config.binding[0][0]==1);
    }
    Write(L"KeyboardMouse",L"Binding0_0",L"27");Write(L"KeyboardMouse",L"Binding24_0",L"256");
    Write(L"Controller",L"Deadzone0",L"31");Reload();
    assert(keyboard_config.binding[0][0]=='R'&&keyboard_config.binding[24][0]==VK_MBUTTON&&!controller_config.deadzone[0]);
    Write(L"KeyboardMouse",L"Binding0_0",L" 71 ");Reload();assert(keyboard_config.binding[0][0]=='G');
    assert(!keyboard_valid(VK_ESCAPE)&&!keyboard_valid(VK_LWIN)&&!keyboard_valid(VK_RWIN)&&keyboard_valid(0));
    assert(!keyboard_valid_action(24,257)&&keyboard_valid_action(0,257));
    keyboard_defaults(&keyboard_config);controller_defaults(&controller_config);controller_publish();

    Clear();keys['W']=keys[VK_LBUTTON]=keys[VK_RBUTTON]=true;
    auto pad=Sample(false);assert(pad.sThumbLY==32767&&!pad.bAnalogButtons[7]&&!pad.wButtons);
    pad=Sample();assert(pad.bAnalogButtons[7]==255&&(pad.wButtons&128));
    xbox_KeyboardConsumeMouseClick(1);pad=Sample();assert(!pad.bAnalogButtons[7]&&(pad.wButtons&128));
    keys[VK_LBUTTON]=false;Sample();keys[VK_LBUTTON]=true;assert(Sample().bAnalogButtons[7]==255);
    focused=false;pad=Sample();assert(!pad.bAnalogButtons[7]&&!pad.wButtons&&!pad.sThumbLY);
    Clear();keys['A']=keys['D']=true;assert(!Sample().sThumbLX);
    keys['D']=false;assert(Sample().sThumbLX==-32767);
    int gameplay=0;assert(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&!gameplay);
    keyboard_config.binding[4][1]=VK_MBUTTON;assert(keyboard_orders_source_mask(VK_MBUTTON,&gameplay)==1&&gameplay);
    keyboard_config.binding[24][0]=VK_SHIFT;keyboard_config.binding[10][0]=0;
    assert(keyboard_orders_source_mask(VK_LSHIFT,&gameplay)==1&&!gameplay);
    assert(keyboard_orders_source_mask(VK_RSHIFT,&gameplay)==1&&!gameplay);
    assert(!keyboard_source_matches(0,0)&&!keyboard_source_matches(VK_LSHIFT,VK_SHIFT));
    keyboard_defaults(&keyboard_config);

    Clear();keyboard_config.binding[2][1]=256;keyboard_config.binding[3][1]=257;
    keyboard_wheel_record(60);keyboard_wheel_publish();assert(!keyboard_wheel_active);
    keyboard_wheel_record(180);keyboard_wheel_record(-120);keyboard_wheel_publish();
    for(unsigned read=0;read<5;++read)assert(Sample().bAnalogButtons[2]==255);
    assert(!Sample(false).bAnalogButtons[2]);
    keyboard_wheel_publish();assert(!Sample().bAnalogButtons[2]);
    keyboard_wheel_publish();assert(Sample().bAnalogButtons[2]==255);
    keyboard_wheel_publish();keyboard_wheel_publish();assert(Sample().bAnalogButtons[3]==255);
    keyboard_wheel_clear();keyboard_wheel_record(2147483647);assert(keyboard_wheel_count==16);
    keyboard_wheel_record(-2147483647);assert(keyboard_wheel_count==16);
    keyboard_wheel_clear();assert(!keyboard_wheel_active&&!keyboard_wheel_count&&!keyboard_wheel_remainder);
    char short_label[3]={'!','!','!'};xbox_KeyboardActionLabel(24,short_label,sizeof(short_label));assert(!short_label[2]);
    assert(!std::strcmp(xbox_KeyboardSourceName(256),"Wheel Up"));
    assert(!std::strcmp(xbox_KeyboardSourceName(VK_XBUTTON1),"Mouse 4"));

    // The identity mapping must not quantize native controller reports.
    for(int axis:{-32768,-16000,-1,0,1,16000,32767})for(unsigned pressure:{0u,1u,127u,255u}){
        input::Pad p;p.connected=true;p.buttons=255;p.lx=p.ly=p.rx=p.ry=int16_t(axis);
        std::fill(std::begin(p.analog),std::end(p.analog),uint8_t(pressure));const auto before=p;
        controller_apply_profile(&p,&controller_config);assert(!std::memcmp(&p,&before,sizeof(p)));
    }
    for(unsigned source=1;source<=24;++source){
        input::Pad raw;raw.connected=true;
        if(source<=8)raw.analog[source-1]=255;
        else if(source<=16)raw.buttons=controller_digital[source-9];
        else {int16_t* axes[]={&raw.lx,&raw.ly,&raw.rx,&raw.ry};const unsigned dir=source-17;*axes[dir/2]=(bool(dir%2)!=bool((dir/2)%2))?32767:-32768;}
        assert(controller_strength(raw,source)>=32767);
        for(unsigned other=1;other<=24;++other)if(other!=source)assert(!controller_strength(raw,other));
        UCRRControllerConfig only;only.binding[0][0]=source;
        controller_apply_profile(&raw,&only);assert(raw.connected&&raw.analog[0]==255&&!raw.buttons&&!raw.lx&&!raw.ly&&!raw.rx&&!raw.ry);
    }
    auto mapping=controller_config;mapping.binding[0][0]=17;mapping.binding[0][1]=18;
    mapping.deadzone[0]=20;
    input::Pad raw;raw.connected=true;raw.lx=-6553;controller_apply_profile(&raw,&mapping);assert(!raw.analog[0]);
    raw={};raw.connected=true;raw.lx=-32768;controller_apply_profile(&raw,&mapping);assert(raw.analog[0]==255&&raw.lx==-32768);
    raw={};raw.connected=true;raw.lx=20000;controller_apply_profile(&raw,&mapping);assert(raw.analog[0]==131&&raw.lx==16808);
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,profile);assert(DeleteFileW(profile));assert(RemoveDirectoryW(directory));
    std::puts("Input bindings: profile compatibility/validation, focus, wheel pulses, modifiers, consumed clicks, identity and remapping passed.");
}
