#pragma once
#include <cstdint>
struct X86Context;
bool TrialRagdollPool(X86Context&,uint8_t*,uint32_t entry);
bool TrialRetainedObjectPool(X86Context&,uint8_t*,uint32_t entry);
float TrialRagdollSeconds();
bool TrialRagdollKeepUnseen(float seconds);
uint32_t TrialRagdollRetireCount(uint32_t original,uint32_t dead);
