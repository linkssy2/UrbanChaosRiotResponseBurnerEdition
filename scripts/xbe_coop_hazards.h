#pragma once
#include <cstdint>
struct X86Context;

// Original 4ACD6: select this hazard loop's indexed player after the retail read.
void TrialCoopHazardPlayer(X86Context&,const uint8_t*);
