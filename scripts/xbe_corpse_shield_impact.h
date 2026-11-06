#pragma once
#include <cstdint>
struct X86Context;
uint32_t TrialCorpseShieldSound(const uint8_t*,uint32_t owner,uint32_t original);
uint32_t TrialCorpseShieldDecalCount(const uint8_t*,uint32_t batch,uint32_t original);
bool TrialCorpseShieldImpact(X86Context&,uint8_t*);
