#pragma once
#include <cstdint>
struct X86Context;

namespace trial_cheats {
// These are the ten entries in the retail Xbox code table, in its original order.
// They belong to the active profile and therefore affect both co-op players.
inline constexpr unsigned Count=10;
inline constexpr const char* Names[Count]={
    "Mini-gun", "All levels and emergencies", "Disco mode", "Terror mode",
    "Enhanced stun gun", "Pistol Mk. 4", "Squeaky voices", "Burning bullets",
    "Headless mode", "Assault Rifle Mk. 3 / infinite shells"
};
inline constexpr const char* Codes[Count]={
    "MINIFUN", "KEYTOTHECITY", "DANCINGFEET", "BURNERSREVENGE", "FRYINGTIME",
    "ZEROTOLERANCE", "WHATWASTHAT", "BURNINGBULLET", "KEEPYOURHEAD", "ULTIMATEPOWER"
};
struct State {
    uint64_t identity=0;uint32_t enabled=0;bool valid=false,pending=false;
    uint64_t pause_epoch=0;
};
State Snapshot();
// Queues one toggle against the last copied pause/profile state. The UI never
// reads or writes guest memory; stale clicks are discarded by the guest thread.
bool Toggle(unsigned index,const State& expected);
}

// Called only from the validated native pause-menu publication. Zero identity
// revokes availability. This uses the original profile flags and sound action;
// it does not grant inventory or introduce a separate persistence mechanism.
void TrialCheatsUpdate(X86Context&,uint8_t* base,uint64_t pause_identity);
void TrialCheatsInvalidate();
