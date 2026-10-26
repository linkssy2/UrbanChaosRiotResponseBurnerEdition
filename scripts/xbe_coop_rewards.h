#pragma once
#include <cstdint>
struct X86Context;
void TrialCoopRewardsReset();
void TrialCoopRewardHudCreated(const X86Context&,const uint8_t*);
// Returns zero unless this actor still owns the captured live co-op HUD tree.
uint32_t TrialCoopRewardManager(const uint8_t*,uint32_t actor);
void TrialCoopHeadshotDamage(const X86Context&,const uint8_t*);
void TrialCoopHeadshotCycleFinished(const X86Context&,const uint8_t*);
void TrialCoopHeadshotRecipient(X86Context&,const uint8_t*);
// Shared authored objectives use the two existing HUD queues, including startup.
void TrialCoopObjectiveRecipient(X86Context&,uint8_t*);
void TrialCoopObjectiveFlush(const X86Context&,uint8_t*);
bool TrialCoopObjectiveSilent(const X86Context&,const uint8_t*);
void TrialCoopObjectiveRetire(const X86Context&,const uint8_t*);
