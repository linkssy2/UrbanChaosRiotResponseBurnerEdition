#pragma once
#include <cstdint>
struct X86Context;
void TrialCoopKORewardsReset();
void TrialCoopKOForget(const X86Context&);
void TrialCoopKOCommitted(const X86Context&,const uint8_t*);
void TrialCoopKORecipient(X86Context&,const uint8_t*);
void TrialCoopKODirectPlayer(X86Context&,const uint8_t*);
void TrialCoopKODamage(const X86Context&,const uint8_t*);
void TrialCoopKOCycleFinished(const X86Context&,const uint8_t*);
class TrialCoopKOUpdateScope {
public:
    TrialCoopKOUpdateScope(const X86Context&,const uint8_t*);
    ~TrialCoopKOUpdateScope();
private:
    const uint8_t* base_=nullptr;
    uint32_t victim_=0;
};
class TrialCoopKOEventScope {
public:
    TrialCoopKOEventScope(const X86Context&,const uint8_t*);
    ~TrialCoopKOEventScope();
private:
    uint32_t world_=0,victim_=0,victim_id_=0,player_=0,player_id_=0;
};
