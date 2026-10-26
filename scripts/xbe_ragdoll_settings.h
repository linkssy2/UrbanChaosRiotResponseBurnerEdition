#pragma once
#include <cstdint>
struct X86Context;
bool TrialRagdollPool(X86Context&,uint8_t*,uint32_t entry);
// Read the original representative physics body's current world position.
// Called on the guest simulation thread; supports retained overflow slots.
bool TrialRagdollWorldPosition(const uint8_t*,uint32_t handle,float (&position)[3]);
bool TrialRetainedObjectPool(X86Context&,uint8_t*,uint32_t entry);
bool TrialRetainedObjectPoolFull(uint32_t owner,bool full);
float TrialRagdollSeconds();
bool TrialRagdollKeepUnseen(float seconds);
uint32_t TrialRagdollRetireCount(uint32_t original,uint32_t dead);
