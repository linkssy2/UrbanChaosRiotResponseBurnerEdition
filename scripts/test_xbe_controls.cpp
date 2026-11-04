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
unsigned mouse_invert=0;
wchar_t profile[MAX_PATH];
bool fail_replace=false;
struct XBOX_GAMEPAD {WORD wButtons=0;BYTE bAnalogButtons[8]={};SHORT sThumbLX=0,sThumbLY=0,sThumbRX=0,sThumbRY=0;};
const WORD controller_digital[]={16,32,64,128,1,2,4,8};
bool keys[256]{};
BOOL keyboard_key_down(int key){return key>0&&key<256&&keys[key];}
int controller_path(wchar_t* out){wcscpy_s(out,MAX_PATH,profile);return 1;}
BOOL TestReplace(LPCWSTR from,LPCWSTR to,DWORD flags){return fail_replace?FALSE:MoveFileExW(from,to,flags);}
DWORD TestModuleName(HMODULE,LPWSTR out,DWORD size){wcscpy_s(out,size,profile);return DWORD(wcslen(profile));}
}
#include "xbe_keyboard_bindings.inc"
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
    assert(defaults.previous_firearm[0]==257&&defaults.previous_firearm[1]==0);
    assert(defaults.health_pack[0][0]=='H'&&!defaults.health_pack[1][0]);
    assert(defaults.heal_partner[0]=='U'&&!defaults.heal_partner[1]);
    assert(!defaults.aim_mode&&!defaults.scope_zoom[0][0]&&!defaults.scope_zoom[1][0]);
    assert(!defaults.invert_mouse&&defaults.zoom_sensitivity==100);
    assert(trial_controls::ZoomSensitivity()==100&&trial_controls::ZoomSensitivity(1)==100);
    assert(GetFileAttributesW(profile)==INVALID_FILE_ATTRIBUTES);
    assert(trial_controls::Apply(defaults,error));
    assert(WritePrivateProfileStringW(L"Other",L"Unrelated",L"keep",profile));
    assert(WritePrivateProfileStringW(L"Graphics",L"Resolution",L"2560",profile));
    assert(trial_controls::Apply(defaults,error)&&error.empty());
    assert(Same(defaults,trial_controls::Snapshot()));
    auto changed=defaults;changed.keyboard[4][0]='G';changed.keyboard[24][0]=VK_XBUTTON1;
    changed.keyboard[1][1]=256;changed.controller[0][0]=4;changed.controller[3][0]=1;
    changed.deadzone[0]=12;changed.sensitivity=650;changed.invert_mouse=1;
    changed.spin_mode=1;changed.spin_binding[0][0]='J';changed.spin_binding[1][0]=10;
    changed.previous_firearm[0]=VK_XBUTTON2;changed.previous_firearm[1]='B';
    changed.health_pack[0][0]='K';changed.health_pack[1][0]=10;
    changed.heal_partner[0]='L';changed.walk_mode=1;changed.walk_binding[0][0]='Z';changed.hud_binding[0][0]='I';
    changed.zoom_sensitivity=45;changed.aim_mode=1;changed.scope_zoom[0][0]='T';changed.scope_zoom[0][1]=256;changed.scope_zoom[1][0]=11;
    assert(trial_controls::Apply(changed,error));
    assert(Same(changed,trial_controls::Snapshot()));
    assert(GetPrivateProfileIntW(L"KeyboardMouse",L"Binding4_0",0,profile)=='G');
    assert(GetPrivateProfileIntW(L"Controller",L"Binding0_0",0,profile)==4);
    assert(GetPrivateProfileIntW(L"Graphics",L"MouseSensitivity",0,profile)==650);
    assert(GetPrivateProfileIntW(L"Graphics",L"InvertMouseLook",0,profile)==1);
    assert(GetPrivateProfileIntW(L"Graphics",L"Resolution",0,profile)==2560);
    assert(GetPrivateProfileIntW(L"Firearms",L"Previous0",0,profile)==VK_XBUTTON2);
    assert(GetPrivateProfileIntW(L"Firearms",L"Previous1",0,profile)=='B');
    keyboard_defaults(&keyboard_config);controller_defaults(&controller_config);
    assert(keyboard_load(nullptr,nullptr,nullptr));assert(controller_load(nullptr,nullptr,nullptr));
    mouse_sensitivity=GetPrivateProfileIntW(L"Graphics",L"MouseSensitivity",400,profile);
    mouse_invert=trial_input_profile::Read(profile,L"Graphics",L"InvertMouseLook",0,1);
    trial_controls::spin_loaded=false;
    trial_controls::firearm_loaded=false;
    trial_controls::health_loaded[0]=false;
    trial_controls::aim_loaded[0]=false;
    trial_controls::partner_loaded=false;trial_controls::actions_loaded[0]=false;
    assert(Same(changed,trial_controls::Snapshot()));
    keys['Z']=true;assert(trial_controls::WalkMode()==1&&trial_controls::WalkDown(true,input::Pad{})&&!trial_controls::WalkDown(true,input::Pad{},1));keys['Z']=false;
    keys['I']=true;assert(trial_controls::HudToggleDown(true,input::Pad{})&&!trial_controls::HudToggleDown(true,input::Pad{},1));keys['I']=false;
    assert(trial_controls::AimMode()==1&&trial_controls::ZoomSensitivity()==45);
    assert(GetPrivateProfileIntW(L"Aim",L"ZoomSensitivity",0,profile)==45);
    assert(GetPrivateProfileIntW(L"Aim",L"Mode",0,profile)==1);
    assert(GetPrivateProfileIntW(L"Aim",L"ScopeZoom0_1",0,profile)==256);
    assert(GetPrivateProfileIntW(L"HealthPacks",L"Partner0",0,profile)=='L');
    wchar_t kept[32];GetPrivateProfileStringW(L"Other",L"Unrelated",L"",kept,32,profile);assert(!wcscmp(kept,L"keep"));
    fail_replace=true;assert(!trial_controls::Apply(defaults,error)&&!error.empty());
    assert(Same(changed,trial_controls::Snapshot()));
    assert(GetPrivateProfileIntW(L"KeyboardMouse",L"Binding4_0",0,profile)=='G');
    assert(GetPrivateProfileIntW(L"Controller",L"Binding0_0",0,profile)==4);
    fail_replace=false;
    {auto bad=changed;bad.spin_mode=2;assert(!trial_controls::Apply(bad,error));
     for(unsigned zoom:{0u,201u}){bad=changed;bad.zoom_sensitivity=zoom;assert(!trial_controls::Apply(bad,error));}
     bad=changed;bad.aim_mode=2;assert(!trial_controls::Apply(bad,error));
     bad=changed;bad.invert_mouse=2;assert(!trial_controls::Apply(bad,error));
     for(unsigned source:{unsigned(VK_ESCAPE),unsigned(VK_LWIN),258u}){
         bad=changed;bad.scope_zoom[0][0]=source;assert(!trial_controls::Apply(bad,error));}
     bad=changed;bad.spin_binding[0][0]=VK_ESCAPE;assert(!trial_controls::Apply(bad,error));
     for(unsigned source:{unsigned(VK_ESCAPE),unsigned(VK_LWIN),258u}){
         bad=changed;bad.heal_partner[0]=source;assert(!trial_controls::Apply(bad,error));}
     for(unsigned source:{unsigned(VK_ESCAPE),unsigned(VK_LWIN),258u}){
         bad=changed;bad.previous_firearm[0]=source;assert(!trial_controls::Apply(bad,error));}}
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
    // Legacy Firearms bindings stay Next; a wheel claimed elsewhere is not stolen.
    assert(WritePrivateProfileStringW(L"Firearms",nullptr,nullptr,profile));
    keyboard_config.binding[2][0]='N';keyboard_config.binding[2][1]=257;
    trial_controls::firearm_loaded=false;auto legacy=trial_controls::Snapshot();
    assert(legacy.keyboard[2][0]=='N'&&legacy.keyboard[2][1]==257&&!legacy.previous_firearm[0]);
    keyboard_config.binding[2][1]=0;trial_controls::firearm_loaded=false;
    assert(trial_controls::Snapshot().previous_firearm[0]==257);
    auto unbound=trial_controls::Snapshot();unbound.previous_firearm[0]=0;
    assert(trial_controls::Apply(unbound,error));trial_controls::firearm_loaded=false;
    assert(!trial_controls::Snapshot().previous_firearm[0]);
    // Player 2 gets its own controller profile without inheriting Player 1's edits.
    const auto first=trial_controls::Snapshot();assert(Same(defaults,trial_controls::Snapshot(1)));
    auto second=defaults;second.controller[0][0]=2;second.controller[1][0]=1;
    second.deadzone[0]=20;second.spin_mode=1;second.spin_binding[1][0]=9;
    second.health_pack[1][0]=12;
    second.scope_zoom[1][0]=10;second.zoom_sensitivity=130;
    const unsigned native_options[]={0,1,0,0,0,15,85};
    std::memcpy(second.native_profile,native_options,sizeof(native_options));
    auto submitted=second;submitted.keyboard[0][0]=VK_ESCAPE;submitted.sensitivity=0;submitted.invert_mouse=99;
    submitted.previous_firearm[0]=258;submitted.spin_binding[0][0]=VK_ESCAPE;
    submitted.scope_zoom[0][0]=VK_ESCAPE; // P2 cannot modify P1's keyboard settings.
    submitted.heal_partner[0]=VK_ESCAPE;
    assert(trial_controls::Apply(submitted,error,1));
    assert(Same(second,trial_controls::Snapshot(1))&&Same(first,trial_controls::Snapshot()));
    assert(GetPrivateProfileIntW(L"ControllerPlayer2",L"Binding0_0",0,profile)==2);
    assert(GetPrivateProfileIntW(L"PistolSpinPlayer2",L"Binding1_0",0,profile)==9);
    for(unsigned i=0;i<7;++i){
        assert(GetPrivateProfileIntW(L"ControllerOptionsPlayer2",trial_controls::profile_keys[i],999,profile)==native_options[i]);
        assert(trial_controls::SecondProfileValue(i,999)==native_options[i]);
    }
    assert(trial_controls::SecondProfileValue(7,123)==123);
    assert(GetPrivateProfileIntW(L"Controller",L"Binding0_0",0,profile)==first.controller[0][0]);
    assert(GetPrivateProfileIntW(L"KeyboardMouse",L"Binding0_0",0,profile)==first.keyboard[0][0]);
    assert(GetPrivateProfileIntW(L"Graphics",L"MouseSensitivity",0,profile)==first.sensitivity);
    assert(GetPrivateProfileIntW(L"Graphics",L"InvertMouseLook",0,profile)==first.invert_mouse);
    assert(GetPrivateProfileIntW(L"PistolSpin",L"Binding0_0",0,profile)==first.spin_binding[0][0]);
    assert(GetPrivateProfileIntW(L"HealthPacks",L"Partner0",0,profile)==first.heal_partner[0]);
    trial_controls::second_loaded=false;assert(Same(second,trial_controls::Snapshot(1)));
    trial_controls::aim_loaded[1]=false;assert(Same(second,trial_controls::Snapshot(1)));
    assert(trial_controls::AimMode(1)==0&&trial_controls::AimMode()==1);
    assert(GetPrivateProfileIntW(L"AimPlayer2",L"ScopeZoom1_0",0,profile)==10);
    assert(GetPrivateProfileIntW(L"AimPlayer2",L"ZoomSensitivity",0,profile)==130);
    assert(trial_controls::ZoomSensitivity(1)==130&&trial_controls::ZoomSensitivity()==first.zoom_sensitivity);
    p={};p.connected=true;p.analog[1]=255;auto first_pad=p;trial_controls::Remap(first_pad);
    trial_controls::Remap(p,1);assert(p.analog[0]==255&&!p.analog[1]);
    assert(!first_pad.analog[0]&&first_pad.analog[1]==255);
    p={};p.connected=true;p.lx=5000;trial_controls::Remap(p,1);assert(!p.lx);
    p={};p.connected=true;p.buttons=16;
    assert(trial_controls::SpinMode(1)==1&&trial_controls::SpinButtonDown(false,p,1));
    assert(!trial_controls::SpinButtonDown(false,p)&&!trial_controls::SpinButtonDown(true,p,1));
    p.buttons=128;assert(trial_controls::HealthPackDown(false,p,1));
    assert(!trial_controls::HealthPackDown(false,p)&&!trial_controls::HealthPackDown(true,p,1));
    keys['H']=true;assert(!trial_controls::HealthPackDown(true,p));keys['H']=false;
    keys['K']=true;assert(trial_controls::HealthPackDown(true,p));keys['K']=false;
    keys['L']=true;assert(trial_controls::HealPartnerDown()&&!trial_controls::HealthPackDown(true,p));keys['L']=false;
    assert(!trial_controls::HealPartnerDown());
    keys['T']=true;assert(trial_controls::ScopeZoomDown(true,p));assert(!trial_controls::ScopeZoomDown(true,p,1));keys['T']=false;
    keyboard_wheel_active=256;assert(!trial_controls::ScopeZoomDown(true,p));assert(trial_controls::ScopeZoomDown(true,p,0,true));keyboard_wheel_active=0;
    p.buttons=32;assert(trial_controls::ScopeZoomDown(false,p,1)&&!trial_controls::ScopeZoomDown(false,p));
    p.buttons=64;assert(trial_controls::ScopeZoomDown(false,p)&&!trial_controls::ScopeZoomDown(false,p,1));
    p.connected=false;assert(!trial_controls::ScopeZoomDown(false,p));p.connected=true;
    trial_controls::health_loaded[1]=false;assert(Same(second,trial_controls::Snapshot(1)));
    assert(GetPrivateProfileIntW(L"HealthPacksPlayer2",L"Binding1_0",0,profile)==12);
    {auto bad=second;bad.health_pack[1][0]=25;assert(!trial_controls::Apply(bad,error,1));}
    {auto bad=second;bad.scope_zoom[1][0]=25;assert(!trial_controls::Apply(bad,error,1));}
    fail_replace=true;assert(!trial_controls::Apply(defaults,error,1)&&!error.empty());fail_replace=false;
    assert(Same(second,trial_controls::Snapshot(1))&&Same(first,trial_controls::Snapshot()));
    trial_controls::second_loaded=false;assert(Same(second,trial_controls::Snapshot(1)));
    assert(!trial_controls::Apply(defaults,error,2)&&!error.empty());
    assert(Same(second,trial_controls::Snapshot(1))&&Same(first,trial_controls::Snapshot()));
    auto invalid_pad=p;trial_controls::Remap(invalid_pad,2);assert(!std::memcmp(&p,&invalid_pad,sizeof(p)));
    assert(!trial_controls::SpinMode(2)&&!trial_controls::SpinButtonDown(false,p,2));
    assert(!trial_controls::AimMode(2)&&!trial_controls::ScopeZoomDown(false,p,2));
    const trial_controls::Draft empty{};assert(Same(empty,trial_controls::Snapshot(2)));
    for(unsigned bad=0;bad<7;++bad){
        auto d=second;
        if(bad==0)d.controller[0][0]=25;
        if(bad==1)d.deadzone[0]=31;
        if(bad==2)d.spin_mode=2;
        if(bad==3)d.spin_binding[1][0]=25;
        if(bad==4)d.walk_mode=2;
        if(bad==5)d.walk_binding[1][0]=25;
        if(bad==6)d.hud_binding[1][0]=25;
        assert(!trial_controls::Apply(d,error,1)&&Same(second,trial_controls::Snapshot(1)));
    }
    for(unsigned i=0;i<7;++i){
        auto bad=second;bad.native_profile[i]=i<5?2:101;
        assert(!trial_controls::Apply(bad,error,1)&&Same(second,trial_controls::Snapshot(1)));
    }
    // Saving Player 1 cannot overwrite Player 2, including after a process reload.
    assert(trial_controls::Apply(defaults,error));trial_controls::second_loaded=false;
    assert(Same(second,trial_controls::Snapshot(1)));
    assert(WritePrivateProfileStringW(L"ControllerPlayer2",L"Binding0_0",L"25",profile));
    assert(WritePrivateProfileStringW(L"ControllerPlayer2",L"Deadzone0",L"31",profile));
    assert(WritePrivateProfileStringW(L"PistolSpinPlayer2",L"Mode",L"2",profile));
    assert(WritePrivateProfileStringW(L"PistolSpinPlayer2",L"Binding1_0",L"25",profile));
    for(unsigned i=0;i<7;++i)assert(WritePrivateProfileStringW(L"ControllerOptionsPlayer2",trial_controls::profile_keys[i],L"-1",profile));
    trial_controls::second_loaded=false;auto safe=trial_controls::Snapshot(1);
    assert(safe.controller[0][0]==1&&!safe.deadzone[0]&&!safe.spin_mode&&!safe.spin_binding[1][0]);
    assert(!std::memcmp(safe.native_profile,defaults.native_profile,sizeof(safe.native_profile)));
    assert(trial_controls::Apply(defaults,error,1));
    for(const wchar_t* invalid:{L"0",L"201",L"-1",L"25junk",L"999999999999999999999999999999999999"}){
        assert(WritePrivateProfileStringW(L"AimPlayer2",L"ZoomSensitivity",invalid,profile));
        trial_controls::aim_loaded[1]=false;assert(trial_controls::Snapshot(1).zoom_sensitivity==100);
    }
    for(const wchar_t* malformed:{L"2",L"1junk",L"-1",L"999999999999999999999999999999999999"}){
        assert(WritePrivateProfileStringW(L"Graphics",L"InvertMouseLook",malformed,profile));
        assert(!trial_input_profile::Read(profile,L"Graphics",L"InvertMouseLook",0,1));
        assert(WritePrivateProfileStringW(L"AimPlayer2",L"Mode",malformed,profile));
        assert(WritePrivateProfileStringW(L"AimPlayer2",L"ScopeZoom1_0",malformed,profile));
        trial_controls::aim_loaded[1]=false;const auto parsed=trial_controls::Snapshot(1);
        assert(!parsed.aim_mode&&parsed.scope_zoom[1][0]==(!wcscmp(malformed,L"2")?2u:0u));
    }
    assert(trial_controls::Apply(defaults,error,1));
    assert(WritePrivateProfileStringW(L"HealthPacks",nullptr,nullptr,profile));
    keyboard_config.binding[4][0]='H';trial_controls::health_loaded[0]=false;
    assert(!trial_controls::Snapshot().health_pack[0][0]); // Do not claim a user's existing H binding.
    keyboard_config.binding[5][0]='U';trial_controls::partner_loaded=false;
    assert(!trial_controls::Snapshot().heal_partner[0]); // Preserve an existing U assignment too.
    for(int axis:{-32768,-1,0,1,32767})for(unsigned analog:{0u,1u,127u,255u}){
        p={};p.connected=true;p.buttons=255;p.lx=axis;p.ly=axis;p.rx=axis;p.ry=axis;
        std::fill(std::begin(p.analog),std::end(p.analog),uint8_t(analog));const auto before=p;
        trial_controls::Remap(p,1);assert(!std::memcmp(&p,&before,sizeof(p)));
    }
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,profile);DeleteFileW(profile);RemoveDirectoryW(directory);
    puts("Controls: atomic save/failure, validation, defaults, capture, remapping, identity parity and player isolation passed.");
}
