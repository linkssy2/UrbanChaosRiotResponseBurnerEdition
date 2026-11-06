#pragma once
#include <cstdint>
struct X86Context;
void TrialRagdollDragInput(unsigned slot,bool allowed);
void TrialRagdollDragUpdate(const X86Context&,uint8_t*);
void TrialRagdollDragStep(X86Context&,uint8_t*);
