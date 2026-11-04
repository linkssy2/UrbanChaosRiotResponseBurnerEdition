// The project's saved controls sit above the maintained runtime's input::Pad.
#include "xbe_controller_bindings.inc"
#include "xbe_controls.h"

namespace trial_controls {
static bool spin_loaded=false;
static unsigned spin_mode=0,spin_binding[2][2]{};
static bool firearm_loaded=false;
static unsigned previous_firearm[2]{};
static bool health_loaded[2]{};
static unsigned health_binding[2][2][2]{};
static bool partner_loaded=false;
static unsigned partner_binding[2]{};
static bool aim_loaded[2]{};
static unsigned aim_mode[2]{},scope_zoom[2][2][2]{},zoom_sensitivity[2]{100,100};
static bool actions_loaded[2]{};
static unsigned walk_mode[2]{},walk_binding[2][2][2]{},hud_binding[2][2][2]{};
static void LoadActions(unsigned player){
    if(actions_loaded[player])return;actions_loaded[player]=true;
    wchar_t path[MAX_PATH];if(!controller_path(path))return;
    const wchar_t* section=player?L"PlayerActions2":L"PlayerActions";
    walk_mode[player]=trial_input_profile::Read(path,section,L"WalkMode",0,1);
    for(unsigned device=player;device<2;++device)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];
        for(unsigned action=0;action<2;++action){
            swprintf(key,32,L"%s%u_%u",action?L"Hud":L"Walk",device,slot);
            const unsigned source=trial_input_profile::Read(path,section,key,0,device?24:UCRR_KEYBOARD_WHEEL_DOWN);
            (action?hud_binding:walk_binding)[player][device][slot]=(device?source<=24:keyboard_valid(source))?source:0;
        }
    }
}
static bool second_loaded=false;
static UCRRControllerConfig second_controller{};
static unsigned second_spin_mode=0,second_spin_binding[2]{};
static unsigned second_profile[7]{1,0,1,1,1,50,50};
static constexpr const wchar_t* profile_keys[]={L"Vibration",L"InvertLook",L"AutoCentre",L"CrouchMode",L"ThumbstickLayout",L"HorizontalSensitivity",L"VerticalSensitivity"};
static void LoadSecond(){
    if(second_loaded)return;second_loaded=true;controller_defaults(&second_controller);
    second_spin_mode=0;std::memset(second_spin_binding,0,sizeof(second_spin_binding));
    const Draft defaults;std::memcpy(second_profile,defaults.native_profile,sizeof(second_profile));
    wchar_t path[MAX_PATH],key[32];if(!controller_path(path))return;
    for(unsigned i=0;i<7;++i){
        const unsigned value=GetPrivateProfileIntW(L"ControllerOptionsPlayer2",profile_keys[i],second_profile[i],path);
        if(value<=(i<5?1u:100u))second_profile[i]=value;
    }
    for(unsigned action=0;action<24;++action)for(unsigned slot=0;slot<2;++slot){
        swprintf(key,32,L"Binding%u_%u",action,slot);
        const unsigned source=GetPrivateProfileIntW(L"ControllerPlayer2",key,second_controller.binding[action][slot],path);
        if(source<=24)second_controller.binding[action][slot]=source;
    }
    for(unsigned slot=0;slot<2;++slot){
        swprintf(key,32,L"Deadzone%u",slot);
        const unsigned value=GetPrivateProfileIntW(L"ControllerPlayer2",key,0,path);
        if(value<=30)second_controller.deadzone[slot]=value;
        swprintf(key,32,L"Binding1_%u",slot);
        const unsigned source=GetPrivateProfileIntW(L"PistolSpinPlayer2",key,0,path);
        if(source<=24)second_spin_binding[slot]=source;
    }
    second_spin_mode=GetPrivateProfileIntW(L"PistolSpinPlayer2",L"Mode",0,path)==1;
}
static void LoadFirearm(){
    if(firearm_loaded)return;firearm_loaded=true;keyboard_init();
    unsigned fallback=UCRR_KEYBOARD_WHEEL_DOWN;
    // Do not steal an existing wheel assignment when upgrading a saved profile.
    AcquireSRWLockShared(&keyboard_lock);
    for(const auto& row:keyboard_config.binding)for(unsigned source:row)if(source==fallback)fallback=0;
    ReleaseSRWLockShared(&keyboard_lock);
    previous_firearm[0]=fallback;
    wchar_t path[MAX_PATH];if(!controller_path(path))return;
    for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Previous%u",slot);
        const unsigned source=GetPrivateProfileIntW(L"Firearms",key,slot?0:fallback,path);
        previous_firearm[slot]=keyboard_valid(source)?source:0;
    }
}
static bool PreviousFirearmClaims(unsigned source){
    LoadFirearm();for(unsigned binding:previous_firearm)if(keyboard_source_matches(binding,source))return true;
    return false;
}
static bool PreviousFirearmDown(bool mouse_active){
    LoadFirearm();
    AcquireSRWLockShared(&keyboard_wheel_lock);const unsigned wheel=keyboard_wheel_active;
    ReleaseSRWLockShared(&keyboard_wheel_lock);
    const bool consumed=InterlockedCompareExchange(&keyboard_consumed_click,0,0)!=0;
    for(unsigned source:previous_firearm){
        if(source>=UCRR_KEYBOARD_WHEEL_UP){if(mouse_active&&source==wheel)return true;}
        else if(source&&!(consumed&&source==VK_LBUTTON)&&(!keyboard_is_mouse(source)||mouse_active)&&keyboard_key_down(source))return true;
    }
    return false;
}
static void LoadSpin(){
    if(spin_loaded)return;spin_loaded=true;wchar_t path[MAX_PATH];if(!controller_path(path))return;
    spin_mode=GetPrivateProfileIntW(L"PistolSpin",L"Mode",0,path)==1;
    for(unsigned device=0;device<2;++device)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Binding%u_%u",device,slot);
        unsigned value=GetPrivateProfileIntW(L"PistolSpin",key,0,path);
        spin_binding[device][slot]=(device?value<=24:keyboard_valid_action(24,value))?value:0;
    }
}
static void LoadHealth(unsigned player){
    if(health_loaded[player])return;
    health_loaded[player]=true;
    unsigned fallback='H';
    if(!player){
        keyboard_init();LoadSpin();LoadFirearm();
        AcquireSRWLockShared(&keyboard_lock);
        for(const auto& row:keyboard_config.binding)for(unsigned key:row)if(key==fallback)fallback=0;
        ReleaseSRWLockShared(&keyboard_lock);
        for(unsigned key:spin_binding[0])if(key==fallback)fallback=0;
        for(unsigned key:previous_firearm)if(key==fallback)fallback=0;
        health_binding[0][0][0]=fallback;
    }
    wchar_t path[MAX_PATH];if(!controller_path(path))return;
    for(unsigned device=player;device<2;++device)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Binding%u_%u",device,slot);
        const unsigned value=GetPrivateProfileIntW(player?L"HealthPacksPlayer2":L"HealthPacks",key,health_binding[player][device][slot],path);
        health_binding[player][device][slot]=(device?value<=24:keyboard_valid(value))?value:0;
    }
}
unsigned SpinMode(unsigned player){
    if(player>1)return 0;
    std::lock_guard guard(input_lock);
    if(player){LoadSecond();return second_spin_mode;}
    LoadSpin();return spin_mode;
}
static void LoadAim(unsigned player){
    if(aim_loaded[player])return;aim_loaded[player]=true;
    zoom_sensitivity[player]=100;
    aim_mode[player]=0;std::memset(scope_zoom[player],0,sizeof(scope_zoom[player]));
    wchar_t path[MAX_PATH];if(!controller_path(path))return;
    const wchar_t* section=player?L"AimPlayer2":L"Aim";
    aim_mode[player]=trial_input_profile::Read(path,section,L"Mode",0,1);
    const unsigned zoom=trial_input_profile::Read(path,section,L"ZoomSensitivity",100,200);
    zoom_sensitivity[player]=zoom?zoom:100;
    for(unsigned device=player;device<2;++device)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"ScopeZoom%u_%u",device,slot);
        const unsigned source=trial_input_profile::Read(path,section,key,0,device?24:UCRR_KEYBOARD_WHEEL_DOWN);
        scope_zoom[player][device][slot]=(device?source<=24:keyboard_valid(source))?source:0;
    }
}
unsigned AimMode(unsigned player){
    if(player>1)return 0;
    std::lock_guard guard(input_lock);LoadAim(player);return aim_mode[player];
}
unsigned ZoomSensitivity(unsigned player){
    if(player>1)return 100;
    std::lock_guard guard(input_lock);LoadAim(player);return zoom_sensitivity[player];
}
static void LoadPartner(){
    if(partner_loaded)return;partner_loaded=true;
    LoadHealth(0);LoadAim(0);unsigned fallback='U';
    AcquireSRWLockShared(&keyboard_lock);
    for(const auto& row:keyboard_config.binding)for(unsigned key:row)if(key==fallback)fallback=0;
    ReleaseSRWLockShared(&keyboard_lock);
    for(const auto* bindings:{spin_binding[0],previous_firearm,health_binding[0][0],scope_zoom[0][0]})
        for(unsigned slot=0;slot<2;++slot)if(bindings[slot]==fallback)fallback=0;
    partner_binding[0]=fallback;
    wchar_t path[MAX_PATH];if(!controller_path(path))return;
    for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Partner%u",slot);
        const unsigned value=trial_input_profile::Read(path,L"HealthPacks",key,slot?0:fallback,UCRR_KEYBOARD_WHEEL_DOWN);
        partner_binding[slot]=keyboard_valid(value)?value:0;
    }
}
unsigned SecondProfileValue(unsigned setting,unsigned fallback){
    if(setting>=7)return fallback;
    std::lock_guard guard(input_lock);LoadSecond();return second_profile[setting];
}
Draft Defaults(){
    Draft d;UCRRKeyboardConfig keys;UCRRControllerConfig pad;
    keyboard_defaults(&keys);controller_defaults(&pad);
    std::memcpy(d.keyboard,keys.binding,sizeof(d.keyboard));
    std::memcpy(d.controller,pad.binding,sizeof(d.controller));
    d.previous_firearm[0]=UCRR_KEYBOARD_WHEEL_DOWN;
    d.health_pack[0][0]='H';
    d.heal_partner[0]='U';
    return d;
}
Draft Snapshot(unsigned player){
    if(player>1)return {};
    std::lock_guard guard(input_lock);
    LoadActions(player);
    if(player){
        LoadSecond();LoadHealth(1);LoadAim(1);auto d=Defaults();
        d.walk_mode=walk_mode[player];std::memcpy(d.walk_binding,walk_binding[player],sizeof(d.walk_binding));std::memcpy(d.hud_binding,hud_binding[player],sizeof(d.hud_binding));
        d.zoom_sensitivity=zoom_sensitivity[1];d.aim_mode=aim_mode[1];std::memcpy(d.scope_zoom[1],scope_zoom[1][1],sizeof(d.scope_zoom[1]));
        std::memcpy(d.health_pack[1],health_binding[1][1],sizeof(d.health_pack[1]));
        std::memcpy(d.controller,second_controller.binding,sizeof(d.controller));
        std::memcpy(d.deadzone,second_controller.deadzone,sizeof(d.deadzone));
        std::memcpy(d.native_profile,second_profile,sizeof(d.native_profile));
        d.spin_mode=second_spin_mode;std::memcpy(d.spin_binding[1],second_spin_binding,sizeof(second_spin_binding));return d;
    }
    keyboard_init();controller_init();Draft d;LoadAim(0);
    d.walk_mode=walk_mode[player];std::memcpy(d.walk_binding,walk_binding[player],sizeof(d.walk_binding));std::memcpy(d.hud_binding,hud_binding[player],sizeof(d.hud_binding));
    d.zoom_sensitivity=zoom_sensitivity[0];d.aim_mode=aim_mode[0];std::memcpy(d.scope_zoom,scope_zoom[0],sizeof(d.scope_zoom));
    LoadSpin();LoadFirearm();LoadHealth(0);d.spin_mode=spin_mode;std::memcpy(d.spin_binding,spin_binding,sizeof(spin_binding));
    std::memcpy(d.health_pack,health_binding[0],sizeof(d.health_pack));
    LoadPartner();std::memcpy(d.heal_partner,partner_binding,sizeof(d.heal_partner));
    std::memcpy(d.previous_firearm,previous_firearm,sizeof(previous_firearm));
    AcquireSRWLockShared(&keyboard_lock);AcquireSRWLockShared(&controller_lock);
    std::memcpy(d.keyboard,keyboard_config.binding,sizeof(d.keyboard));
    std::memcpy(d.controller,controller_config.binding,sizeof(d.controller));
    std::memcpy(d.deadzone,controller_config.deadzone,sizeof(d.deadzone));d.sensitivity=mouse_sensitivity;d.invert_mouse=mouse_invert;
    ReleaseSRWLockShared(&controller_lock);ReleaseSRWLockShared(&keyboard_lock);return d;
}
static bool Save(const Draft& d,unsigned player){
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
    if(!player){
        for(unsigned a=0;a<25;++a)for(unsigned s=0;s<2;++s)
            at+=swprintf(section+at,2048-at,L"Binding%u_%u=%u",a,s,d.keyboard[a][s])+1;
        section[at]=0;ok=ok&&WritePrivateProfileSectionW(L"KeyboardMouse",section,temp);
    }
    at=0;
    for(unsigned a=0;a<24;++a)for(unsigned s=0;s<2;++s)
        at+=swprintf(section+at,2048-at,L"Binding%u_%u=%u",a,s,d.controller[a][s])+1;
    for(unsigned s=0;s<2;++s)at+=swprintf(section+at,2048-at,L"Deadzone%u=%u",s,d.deadzone[s])+1;
    section[at]=0;ok=ok&&WritePrivateProfileSectionW(player?L"ControllerPlayer2":L"Controller",section,temp);
    if(player){
        at=0;
        for(unsigned i=0;i<7;++i)at+=swprintf(section+at,2048-at,L"%s=%u",profile_keys[i],d.native_profile[i])+1;
        section[at]=0;ok=ok&&WritePrivateProfileSectionW(L"ControllerOptionsPlayer2",section,temp);
    }
    if(!player){
        swprintf(value,32,L"%u",d.sensitivity);
        ok=ok&&WritePrivateProfileStringW(L"Graphics",L"MouseSensitivity",value,temp);
        swprintf(value,32,L"%u",d.invert_mouse);
        ok=ok&&WritePrivateProfileStringW(L"Graphics",L"InvertMouseLook",value,temp);
    }
    const wchar_t* spin_section=player?L"PistolSpinPlayer2":L"PistolSpin";
    swprintf(value,32,L"%u",d.spin_mode);ok=ok&&WritePrivateProfileStringW(spin_section,L"Mode",value,temp);
    for(unsigned device=player;device<2;++device)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Binding%u_%u",device,slot);swprintf(value,32,L"%u",d.spin_binding[device][slot]);
        ok=ok&&WritePrivateProfileStringW(spin_section,key,value,temp);
    }
    if(!player)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Previous%u",slot);swprintf(value,32,L"%u",d.previous_firearm[slot]);
        ok=ok&&WritePrivateProfileStringW(L"Firearms",key,value,temp);
    }
    for(unsigned device=player;device<2;++device)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Binding%u_%u",device,slot);swprintf(value,32,L"%u",d.health_pack[device][slot]);
        ok=ok&&WritePrivateProfileStringW(player?L"HealthPacksPlayer2":L"HealthPacks",key,value,temp);
    }
    if(!player)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"Partner%u",slot);swprintf(value,32,L"%u",d.heal_partner[slot]);
        ok=ok&&WritePrivateProfileStringW(L"HealthPacks",key,value,temp);
    }
    const wchar_t* aim_section=player?L"AimPlayer2":L"Aim";
    swprintf(value,32,L"%u",d.aim_mode);ok=ok&&WritePrivateProfileStringW(aim_section,L"Mode",value,temp);
    swprintf(value,32,L"%u",d.zoom_sensitivity);ok=ok&&WritePrivateProfileStringW(aim_section,L"ZoomSensitivity",value,temp);
    for(unsigned device=player;device<2;++device)for(unsigned slot=0;slot<2;++slot){
        wchar_t key[32];swprintf(key,32,L"ScopeZoom%u_%u",device,slot);swprintf(value,32,L"%u",d.scope_zoom[device][slot]);
        ok=ok&&WritePrivateProfileStringW(aim_section,key,value,temp);
    }
    const wchar_t* actions_section=player?L"PlayerActions2":L"PlayerActions";
    swprintf(value,32,L"%u",d.walk_mode);ok=ok&&WritePrivateProfileStringW(actions_section,L"WalkMode",value,temp);
    for(unsigned device=player;device<2;++device)for(unsigned slot=0;slot<2;++slot)for(unsigned action=0;action<2;++action){
        wchar_t key[32];swprintf(key,32,L"%s%u_%u",action?L"Hud":L"Walk",device,slot);
        swprintf(value,32,L"%u",(action?d.hud_binding:d.walk_binding)[device][slot]);
        ok=ok&&WritePrivateProfileStringW(actions_section,key,value,temp);
    }
    if(ok)WritePrivateProfileStringW(nullptr,nullptr,nullptr,temp);
    ok=ok&&MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok)DeleteFileW(temp);return ok;
}
bool Apply(const Draft& d,std::string& error,unsigned player){
    error.clear();
    if(player>1){error="Invalid local player.";return false;}
    if(player)for(unsigned i=0;i<7;++i)if(d.native_profile[i]>(i<5?1u:100u)){error="Invalid controller option.";return false;}
    if(d.spin_mode>1){error="Invalid pistol spin mode.";return false;}
    if(d.aim_mode>1){error="Invalid aiming mode.";return false;}
    if(d.walk_mode>1){error="Invalid walking mode.";return false;}
    for(unsigned action=0;action<2;++action)for(unsigned slot=0;slot<2;++slot){
        const auto& binding=action?d.hud_binding:d.walk_binding;
        if((!player&&!keyboard_valid(binding[0][slot]))||binding[1][slot]>24){error="Invalid walk or HUD binding.";return false;}
    }
    if(d.zoom_sensitivity<1||d.zoom_sensitivity>200){error="Invalid zoom sensitivity.";return false;}
    if(!player&&d.invert_mouse>1){error="Invalid mouse look inversion.";return false;}
    for(unsigned slot=0;slot<2;++slot)if((!player&&!keyboard_valid(d.scope_zoom[0][slot]))||d.scope_zoom[1][slot]>24){error="Invalid scope zoom binding.";return false;}
    for(unsigned slot=0;slot<2;++slot)if((!player&&!keyboard_valid(d.health_pack[0][slot]))||d.health_pack[1][slot]>24){error="Invalid health pack binding.";return false;}
    for(unsigned slot=0;slot<2;++slot)if((!player&&!keyboard_valid_action(24,d.spin_binding[0][slot]))||d.spin_binding[1][slot]>24){error="Invalid pistol spin binding.";return false;}
    if(!player){
        for(unsigned source:d.heal_partner)if(!keyboard_valid(source)){error="Invalid partner healing binding.";return false;}
        for(unsigned source:d.previous_firearm)if(!keyboard_valid(source)){error="That key cannot be assigned to this action.";return false;}
        for(unsigned a=0;a<25;++a)for(unsigned s=0;s<2;++s)
            if(!keyboard_valid_action(a,d.keyboard[a][s])){error="That key cannot be assigned to this action.";return false;}
    }
    for(auto& row:d.controller)for(auto source:row)if(source>24){error="Invalid controller button.";return false;}
    if(d.deadzone[0]>30||d.deadzone[1]>30||(!player&&(d.sensitivity<1||d.sensitivity>1600))){error="Invalid input sensitivity.";return false;}
    std::lock_guard guard(input_lock);
    if(!player){keyboard_init();controller_init();}
    if(!Save(d,player)){error="Could not save controls. Your previous bindings are unchanged.";return false;}
    actions_loaded[player]=true;walk_mode[player]=d.walk_mode;
    for(unsigned device=player;device<2;++device){
        std::memcpy(walk_binding[player][device],d.walk_binding[device],sizeof(d.walk_binding[device]));
        std::memcpy(hud_binding[player][device],d.hud_binding[device],sizeof(d.hud_binding[device]));
    }
    health_loaded[player]=true;std::memcpy(health_binding[player],d.health_pack,sizeof(d.health_pack));
    if(!player){partner_loaded=true;std::memcpy(partner_binding,d.heal_partner,sizeof(d.heal_partner));}
    aim_loaded[player]=true;aim_mode[player]=d.aim_mode;zoom_sensitivity[player]=d.zoom_sensitivity;
    for(unsigned device=player;device<2;++device)std::memcpy(scope_zoom[player][device],d.scope_zoom[device],sizeof(d.scope_zoom[device]));
    if(player){
        second_loaded=true;std::memcpy(second_controller.binding,d.controller,sizeof(d.controller));
        std::memcpy(second_controller.deadzone,d.deadzone,sizeof(d.deadzone));
        std::memcpy(second_profile,d.native_profile,sizeof(d.native_profile));
        second_spin_mode=d.spin_mode;std::memcpy(second_spin_binding,d.spin_binding[1],sizeof(second_spin_binding));return true;
    }
    spin_loaded=true;spin_mode=d.spin_mode;std::memcpy(spin_binding,d.spin_binding,sizeof(spin_binding));
    firearm_loaded=true;std::memcpy(previous_firearm,d.previous_firearm,sizeof(previous_firearm));
    AcquireSRWLockExclusive(&keyboard_lock);AcquireSRWLockExclusive(&controller_lock);
    std::memcpy(keyboard_config.binding,d.keyboard,sizeof(d.keyboard));
    std::memcpy(controller_config.binding,d.controller,sizeof(d.controller));
    std::memcpy(controller_config.deadzone,d.deadzone,sizeof(d.deadzone));mouse_sensitivity=d.sensitivity;mouse_invert=d.invert_mouse;
    controller_publish();ReleaseSRWLockExclusive(&controller_lock);ReleaseSRWLockExclusive(&keyboard_lock);
    return true;
}
static bool BindingDown(const unsigned (&bindings)[2],bool keyboard,const input::Pad& p,bool mouse_active){
    int strengths[24]{};
    if(!keyboard&&p.connected)controller_strengths(&p,strengths);
    for(unsigned source:bindings)if(source){
        if(!keyboard){if(strengths[source-1]>16384)return true;}
        else if(source>=UCRR_KEYBOARD_WHEEL_UP){
            AcquireSRWLockShared(&keyboard_wheel_lock);const bool down=keyboard_wheel_active==source;
            ReleaseSRWLockShared(&keyboard_wheel_lock);if(mouse_active&&down)return true;
        }else if((!keyboard_is_mouse(source)||mouse_active)&&keyboard_key_down(source))return true;
    }
    return false;
}
bool HealthPackDown(bool keyboard,const input::Pad& p,unsigned player,bool mouse_active){
    if(player>1||(player&&keyboard))return false;
    std::lock_guard guard(input_lock);LoadHealth(player);
    return BindingDown(health_binding[player][keyboard?0:1],keyboard,p,mouse_active);
}
bool HealPartnerDown(bool mouse_active){
    std::lock_guard guard(input_lock);LoadPartner();
    return BindingDown(partner_binding,true,input::Pad{},mouse_active);
}
bool ScopeZoomDown(bool keyboard,const input::Pad& p,unsigned player,bool mouse_active){
    if(player>1||(player&&keyboard))return false;
    std::lock_guard guard(input_lock);LoadAim(player);
    return BindingDown(scope_zoom[player][keyboard?0:1],keyboard,p,mouse_active);
}
unsigned WalkMode(unsigned player){
    if(player>1)return 0;
    std::lock_guard guard(input_lock);LoadActions(player);return walk_mode[player];
}
bool WalkDown(bool keyboard,const input::Pad& p,unsigned player,bool mouse_active){
    if(player>1||(player&&keyboard))return false;
    std::lock_guard guard(input_lock);LoadActions(player);
    return BindingDown(walk_binding[player][keyboard?0:1],keyboard,p,mouse_active);
}
bool HudToggleDown(bool keyboard,const input::Pad& p,unsigned player,bool mouse_active){
    if(player>1||(player&&keyboard))return false;
    std::lock_guard guard(input_lock);LoadActions(player);
    return BindingDown(hud_binding[player][keyboard?0:1],keyboard,p,mouse_active);
}
bool SpinButtonDown(bool keyboard,const input::Pad& p,unsigned player){
    if(player>1||(player&&keyboard))return false;
    std::lock_guard guard(input_lock);
    if(player)LoadSecond();else LoadSpin();int strengths[24]{};
    if(!keyboard&&p.connected)controller_strengths(&p,strengths);
    const unsigned* bindings=player?second_spin_binding:spin_binding[keyboard?0:1];
    for(unsigned slot=0;slot<2;++slot)if(const unsigned source=bindings[slot]){
        if(keyboard?keyboard_key_down(source)!=0:strengths[source-1]>16384)return true;
    }
    return false;
}
unsigned ControllerSource(const input::Pad& p){
    if(!p.connected)return 0;
    int strengths[24];controller_strengths(&p,strengths);
    for(unsigned i=0;i<24;++i)if(strengths[i]>(i>=16?16000:16384))return i+1;
    return 0;
}
void Remap(input::Pad& p,unsigned player){
    if(!p.connected||player>1)return;
    std::lock_guard guard(input_lock);
    if(player){LoadSecond();controller_apply_profile(&p,&second_controller);}
    else {controller_init();controller_apply(&p);}
}
}
