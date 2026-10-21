#undef NDEBUG
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include "core/input.h"

namespace {
std::recursive_mutex input_lock;
unsigned mouse_sensitivity=400;
wchar_t profile[MAX_PATH];
bool fail_replace=false;
struct XBOX_GAMEPAD {WORD wButtons=0;BYTE bAnalogButtons[8]={};SHORT sThumbLX=0,sThumbLY=0,sThumbRX=0,sThumbRY=0;};
const WORD controller_digital[]={16,32,64,128,1,2,4,8};
BOOL keyboard_key_down(int){return FALSE;}
int controller_path(wchar_t* out){wcscpy_s(out,MAX_PATH,profile);return 1;}
BOOL TestReplace(LPCWSTR from,LPCWSTR to,DWORD flags){return fail_replace?FALSE:MoveFileExW(from,to,flags);}
DWORD TestModuleName(HMODULE,LPWSTR out,DWORD size){wcscpy_s(out,size,profile);return DWORD(wcslen(profile));}
}
#include "third_party/xboxrecomp/src/input/host_keyboard_config.h"
#define MoveFileExW TestReplace
#define GetModuleFileNameW TestModuleName
#include "xbe_controls_backend.inc"
#undef MoveFileExW
#undef GetModuleFileNameW

static bool Same(const trial_controls::Draft& a,const trial_controls::Draft& b){
    return !std::memcmp(&a,&b,sizeof(a));
}
int main(){
    wchar_t folder[MAX_PATH],directory[MAX_PATH];assert(GetTempPathW(MAX_PATH,folder));
    assert(GetTempFileNameW(folder,L"uct",0,directory));assert(DeleteFileW(directory));
    assert(CreateDirectoryW(directory,nullptr));swprintf(profile,MAX_PATH,L"%s\\xboxrecomp.ini",directory);
    auto defaults=trial_controls::Defaults();std::string error;
    assert(GetFileAttributesW(profile)==INVALID_FILE_ATTRIBUTES);
    assert(trial_controls::Apply(defaults,error));
    assert(WritePrivateProfileStringW(L"Other",L"Unrelated",L"keep",profile));
    assert(WritePrivateProfileStringW(L"Graphics",L"Resolution",L"2560",profile));
    assert(trial_controls::Apply(defaults,error)&&error.empty());
    assert(Same(defaults,trial_controls::Snapshot()));
    auto changed=defaults;changed.keyboard[4][0]='G';changed.keyboard[24][0]=VK_XBUTTON1;
    changed.keyboard[1][1]=256;changed.controller[0][0]=4;changed.controller[3][0]=1;
    changed.deadzone[0]=12;changed.sensitivity=650;
    changed.spin_mode=1;changed.spin_binding[0][0]='J';changed.spin_binding[1][0]=10;
    assert(trial_controls::Apply(changed,error));
    assert(Same(changed,trial_controls::Snapshot()));
    assert(GetPrivateProfileIntW(L"KeyboardMouse",L"Binding4_0",0,profile)=='G');
    assert(GetPrivateProfileIntW(L"Controller",L"Binding0_0",0,profile)==4);
    assert(GetPrivateProfileIntW(L"Graphics",L"MouseSensitivity",0,profile)==650);
    assert(GetPrivateProfileIntW(L"Graphics",L"Resolution",0,profile)==2560);
    keyboard_defaults(&keyboard_config);controller_defaults(&controller_config);
    assert(keyboard_load(nullptr,nullptr,nullptr));assert(controller_load(nullptr,nullptr,nullptr));
    mouse_sensitivity=GetPrivateProfileIntW(L"Graphics",L"MouseSensitivity",400,profile);
    trial_controls::spin_loaded=false;
    assert(Same(changed,trial_controls::Snapshot()));
    wchar_t kept[32];GetPrivateProfileStringW(L"Other",L"Unrelated",L"",kept,32,profile);assert(!wcscmp(kept,L"keep"));
    fail_replace=true;assert(!trial_controls::Apply(defaults,error)&&!error.empty());
    assert(Same(changed,trial_controls::Snapshot()));
    assert(GetPrivateProfileIntW(L"KeyboardMouse",L"Binding4_0",0,profile)=='G');
    assert(GetPrivateProfileIntW(L"Controller",L"Binding0_0",0,profile)==4);
    fail_replace=false;
    {auto bad=changed;bad.spin_mode=2;assert(!trial_controls::Apply(bad,error));
     bad=changed;bad.spin_binding[0][0]=VK_ESCAPE;assert(!trial_controls::Apply(bad,error));}
    for(unsigned bad=0;bad<6;++bad){
        auto d=defaults;
        if(bad==0)d.keyboard[0][0]=VK_ESCAPE;
        if(bad==1)d.keyboard[24][0]=256;
        if(bad==2)d.controller[0][0]=25;
        if(bad==3)d.deadzone[1]=31;
        if(bad==4)d.sensitivity=0;
        if(bad==5)d.sensitivity=1601;
        assert(!trial_controls::Apply(d,error)&&Same(changed,trial_controls::Snapshot()));
    }
    for(unsigned value:{1u,2u,24u,25u,400u,1600u}){
        auto low=changed;low.sensitivity=value;assert(trial_controls::Apply(low,error));
        assert(trial_controls::Snapshot().sensitivity==value);
        assert(GetPrivateProfileIntW(L"Graphics",L"MouseSensitivity",0,profile)==value);
    }
    assert(trial_controls::Apply(defaults,error));
    // The original identity fast path must preserve even sub-threshold analog input.
    for(int axis:{-32768,-16000,-1,0,1,16000,32767})for(unsigned analog:{0u,1u,127u,255u}){
        input::Pad p;p.connected=true;p.buttons=255;p.lx=axis;p.ly=axis;p.rx=axis;p.ry=axis;
        std::fill(std::begin(p.analog),std::end(p.analog),uint8_t(analog));auto before=p;
        trial_controls::Remap(p);assert(!std::memcmp(&p,&before,sizeof(p)));
    }
    input::Pad p;p.connected=true;p.analog[3]=255;assert(trial_controls::ControllerSource(p)==4);
    assert(trial_controls::Apply(changed,error));trial_controls::Remap(p);
    assert(p.connected&&p.analog[0]==255&&p.analog[3]==0);
    p={};p.connected=true;p.lx=2000;trial_controls::Remap(p);assert(p.lx==0);
    p={};p.connected=true;p.lx=32767;trial_controls::Remap(p);assert(p.lx>=32766);
    p={};p.connected=true;p.ly=-20000;assert(trial_controls::ControllerSource(p)==20);
    p.connected=false;assert(trial_controls::ControllerSource(p)==0);
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,profile);DeleteFileW(profile);RemoveDirectoryW(directory);
    puts("Controls: atomic save/failure, validation, defaults, source capture, analog remap and identity parity passed.");
}
