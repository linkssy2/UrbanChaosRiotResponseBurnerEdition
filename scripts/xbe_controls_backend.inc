// Reuse the original remapping arithmetic and profile format for controller parity.
#include <xinput.h>
#define controller_path controls_legacy_path
#define controller_digital controls_legacy_digital
#include "third_party/xboxrecomp/src/input/host_controller_config.h"
#undef controller_digital
#undef controller_path
#include "xbe_controls.h"

namespace trial_controls {
Draft Defaults(){
    Draft d;UCRRKeyboardConfig keys;UCRRControllerConfig pad;
    keyboard_defaults(&keys);controller_defaults(&pad);
    std::memcpy(d.keyboard,keys.binding,sizeof(d.keyboard));
    std::memcpy(d.controller,pad.binding,sizeof(d.controller));
    return d;
}
Draft Snapshot(){
    std::lock_guard guard(input_lock);keyboard_init();controller_init();Draft d;
    AcquireSRWLockShared(&keyboard_lock);AcquireSRWLockShared(&controller_lock);
    std::memcpy(d.keyboard,keyboard_config.binding,sizeof(d.keyboard));
    std::memcpy(d.controller,controller_config.binding,sizeof(d.controller));
    std::memcpy(d.deadzone,controller_config.deadzone,sizeof(d.deadzone));d.sensitivity=mouse_sensitivity;
    ReleaseSRWLockShared(&controller_lock);ReleaseSRWLockShared(&keyboard_lock);return d;
}
static bool Save(const Draft& d){
    wchar_t path[MAX_PATH],dir[MAX_PATH],temp[MAX_PATH];
    if(!controller_path(path))return false;
    wcscpy_s(dir,path);auto slash=wcsrchr(dir,L'\\');if(!slash)slash=wcsrchr(dir,L'/');
    if(!slash)return false;slash[1]=0;
    if(!GetTempFileNameW(dir,L"ucb",0,temp))return false;
    // All settings are replaced together; a failed write leaves the live profile intact.
    // The all-null cache-flush form returns zero even when it succeeds.
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);
    bool ok=true;
    const auto attributes=GetFileAttributesW(path);
    if(attributes!=INVALID_FILE_ATTRIBUTES)ok=ok&&CopyFileW(path,temp,FALSE);
    else if(GetLastError()!=ERROR_FILE_NOT_FOUND)ok=false;
    wchar_t section[2048],value[32];size_t at=0;
    for(unsigned a=0;a<25;++a)for(unsigned s=0;s<2;++s)
        at+=swprintf(section+at,2048-at,L"Binding%u_%u=%u",a,s,d.keyboard[a][s])+1;
    section[at]=0;ok=ok&&WritePrivateProfileSectionW(L"KeyboardMouse",section,temp);
    at=0;
    for(unsigned a=0;a<24;++a)for(unsigned s=0;s<2;++s)
        at+=swprintf(section+at,2048-at,L"Binding%u_%u=%u",a,s,d.controller[a][s])+1;
    for(unsigned s=0;s<2;++s)at+=swprintf(section+at,2048-at,L"Deadzone%u=%u",s,d.deadzone[s])+1;
    section[at]=0;ok=ok&&WritePrivateProfileSectionW(L"Controller",section,temp);
    swprintf(value,32,L"%u",d.sensitivity);
    ok=ok&&WritePrivateProfileStringW(L"Graphics",L"MouseSensitivity",value,temp);
    if(ok)WritePrivateProfileStringW(nullptr,nullptr,nullptr,temp);
    ok=ok&&MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok)DeleteFileW(temp);return ok;
}
bool Apply(const Draft& d,std::string& error){
    error.clear();
    for(unsigned a=0;a<25;++a)for(unsigned s=0;s<2;++s)
        if(!keyboard_valid_action(a,d.keyboard[a][s])){error="That key cannot be assigned to this action.";return false;}
    for(auto& row:d.controller)for(auto source:row)if(source>24){error="Invalid controller button.";return false;}
    if(d.deadzone[0]>30||d.deadzone[1]>30||d.sensitivity<25||d.sensitivity>1600){error="Invalid input sensitivity.";return false;}
    std::lock_guard guard(input_lock);keyboard_init();controller_init();
    if(!Save(d)){error="Could not save controls. Your previous bindings are unchanged.";return false;}
    AcquireSRWLockExclusive(&keyboard_lock);AcquireSRWLockExclusive(&controller_lock);
    std::memcpy(keyboard_config.binding,d.keyboard,sizeof(d.keyboard));
    std::memcpy(controller_config.binding,d.controller,sizeof(d.controller));
    std::memcpy(controller_config.deadzone,d.deadzone,sizeof(d.deadzone));mouse_sensitivity=d.sensitivity;
    controller_publish();ReleaseSRWLockExclusive(&controller_lock);ReleaseSRWLockExclusive(&keyboard_lock);
    return true;
}
static XBOX_GAMEPAD OriginalPad(const input::Pad& p){
    XBOX_GAMEPAD pad;pad.wButtons=p.buttons;
    std::memcpy(pad.bAnalogButtons,p.analog,sizeof(p.analog));
    pad.sThumbLX=p.lx;pad.sThumbLY=p.ly;pad.sThumbRX=p.rx;pad.sThumbRY=p.ry;return pad;
}
unsigned ControllerSource(const input::Pad& p){
    if(!p.connected)return 0;
    auto pad=OriginalPad(p);int strengths[24];controller_strengths(&pad,strengths);
    for(unsigned i=0;i<24;++i)if(strengths[i]>(i>=16?16000:16384))return i+1;
    return 0;
}
void Remap(input::Pad& p){
    if(!p.connected)return;
    std::lock_guard guard(input_lock);controller_init();auto pad=OriginalPad(p);controller_apply(&pad);
    p.buttons=pad.wButtons;std::memcpy(p.analog,pad.bAnalogButtons,sizeof(p.analog));
    p.lx=pad.sThumbLX;p.ly=pad.sThumbLY;p.rx=pad.sThumbRX;p.ry=pad.sThumbRY;
}
}
