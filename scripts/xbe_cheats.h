#pragma once
#include <cstdint>
struct X86Context;
extern bool TrialCorpseShield;

namespace trial_cheats {
// Retail entries keep their original table indices; the PC option has no guest flag.
inline constexpr unsigned RetailCount=10,CorpseShield=RetailCount,Count=RetailCount+1;
inline constexpr const char* Names[Count]={
    "Mini-gun", "All levels and emergencies", "Disco mode", "Terror mode",
    "Enhanced stun gun", "Pistol Mk. 4", "Squeaky voices", "Burning bullets",
    "Headless mode", "Assault Rifle Mk. 3 / infinite shells", "Burner body shield"
};
inline constexpr const char* Codes[RetailCount]={
    "MINIFUN", "KEYTOTHECITY", "DANCINGFEET", "BURNERSREVENGE", "FRYINGTIME",
    "ZEROTOLERANCE", "WHATWASTHAT", "BURNINGBULLET", "KEEPYOURHEAD", "ULTIMATEPOWER"
};
struct State {
    uint64_t identity=0;uint32_t enabled=0;bool valid=false,pending=false;
    uint64_t pause_epoch=0;
};
State Snapshot();
void Initialize();
bool CorpseShieldEnabled();
// Queues one toggle against the last copied pause/profile state. The UI never
// reads or writes guest memory. The PC-only option saves on the UI thread.
bool Toggle(unsigned index,const State& expected);
}

// Called only from the validated native pause-menu publication. Zero identity
// revokes availability. Retail cheats use original profile flags and sound
// actions; the PC-only option never grants inventory or alters those flags.
void TrialCheatsUpdate(X86Context&,uint8_t* base,uint64_t pause_identity);
void TrialCheatsInvalidate();
