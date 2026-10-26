#pragma once
#include <cstdint>
struct X86Context;
void TrialCoopShieldReset();

// Only the two byte-identified authored training shield triggers participate.
bool TrialCoopShieldOverlap(X86Context&,uint8_t*);
bool TrialCoopShieldCommand(X86Context&,uint8_t*);
void TrialCoopShieldPermission(X86Context&,const uint8_t*);
