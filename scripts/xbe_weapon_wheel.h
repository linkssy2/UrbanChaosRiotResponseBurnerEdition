#pragma once
#include <cstdint>
struct X86Context;

namespace trial_weapon_wheel {
constexpr unsigned MaxEntries=27;
enum class Kind:uint8_t {Guns,MeleeThrowables};
struct Entry {
    uint32_t slot=0,definition=0,category=0,loaded=0,reserve=0;
    bool usable=false;
};
struct Snapshot {
    Kind kind=Kind::Guns;
    bool available=false,open=false;
    unsigned count=0;
    int current=-1,selected=-1;
    float x=0,y=0;
    Entry entries[MaxEntries]{};
};
Snapshot GetSnapshot(unsigned player);
void Input(unsigned player,bool down,bool allowed,bool keyboard,float stick_x,float stick_y,Kind kind=Kind::Guns);
bool MouseDelta(float dx,float dy);
bool Blocked(unsigned player);
bool SlowTimeActive();
bool SlowTimeActive(uint32_t world);
void PublishInput(unsigned player);
void Cancel(unsigned player);
void GuestUpdate(X86Context&,uint8_t*);
bool GuestSelect(X86Context&,uint8_t*);
void GuestLook(const X86Context&,uint8_t*);
// Native definitions: 4E55D0 + id*F0, name +4, HUD material +B8.
// Pistol Mk4 deliberately shares the existing reconstructed Mk3 portrait.
inline const char* IconName(uint32_t id){
    constexpr const char* names[27]={"","taser","pistol","gangassault","gangshotgun",
        "machinepistol","magnum","pistol3","cleaver","chainsaw","grenadelauncher",
        "molotov","nailbomb","smokegrenade","riotgrenade","taser2","pistol2",
        "policeshotgun","policeshotgun2","policeshotgun3","policeassault",
        "policeassault2","policeassault3","pistol3","minigun","",""};
    return id<27?names[id]:"";
}
inline const char* Name(uint32_t id){
    constexpr const char* names[27]={"Shield","Taser","Pistol","Assault rifle","Shotgun",
        "Machine pistol","Magnum","Pistol Mk3","Cleaver","Chainsaw","Grenade launcher",
        "Molotov","Nail bomb","Smoke grenade","Riot grenade","Taser Mk2","Pistol Mk2",
        "Police shotgun","Police shotgun Mk2","Police shotgun Mk3","Police assault rifle",
        "Police assault rifle Mk2","Police assault rifle Mk3","Pistol Mk4","Minigun","Turret","Molotov"};
    return id<27?names[id]:"Weapon";
}
}
