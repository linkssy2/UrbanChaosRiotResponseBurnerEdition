#pragma once
#include <cstdint>
struct X86Context;

struct TrialCoopInteractionStatus {bool skip=false,waiting=false;};
TrialCoopInteractionStatus TrialCoopInteractionSnapshot(unsigned slot);
void TrialCoopInteractionReset();
// 0 = ordinary single-player path, 1/2 = the admitted conversation owner.
unsigned TrialCoopInteractionPlayer(const uint8_t*,uint32_t behavior);
void TrialCoopInteractionLookup(X86Context&,const uint8_t*);
void TrialCoopInteractionInput(X86Context&,uint8_t*);
void TrialCoopInteractionClose(X86Context&,uint8_t*,uint32_t behavior);
bool TrialCoopInteractionWeaponsBlocked(const uint8_t*,uint32_t actor);
bool TrialCoopInteractionWeaponInputBlocked(uint32_t actor);

// Restrict only the native weapon update; movement/look runs outside this scope.
class TrialCoopInteractionWeaponScope {
public:
    TrialCoopInteractionWeaponScope(X86Context&,uint8_t*);
    ~TrialCoopInteractionWeaponScope();
private:
    uint8_t* base_;uint32_t actor_=0,id_=0,world_=0;
    uint8_t lock_=0,fire_=0;bool blocked_=false;
    TrialCoopInteractionWeaponScope* previous_;
    friend bool TrialCoopInteractionWeaponInputBlocked(uint32_t);
};

class TrialCoopInteractionScope {
public:
    TrialCoopInteractionScope(X86Context&,uint8_t*);
    ~TrialCoopInteractionScope();
    bool Update();
    bool Begin();
    bool Commit();
    // The retail conversation's Fades calls are global; their co-op replacement
    // is a host-owned prompt in only the initiating player's half.
    bool Fades(bool prompt);
private:
    X86Context& context_;uint8_t* base_;
    TrialCoopInteractionScope* previous_;
    uint32_t behavior_=0,previous_lookup_=0;bool qualified_=false;
};
