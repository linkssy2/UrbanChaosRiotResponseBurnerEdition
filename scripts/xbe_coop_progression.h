#pragma once
#include <cstdint>
struct X86Context;
void TrialCoopProgressionReset();
// Called only after the original PLAYER overlap, before its enter-state commit.
void TrialCoopProgressionEnter(X86Context&,uint8_t*);
