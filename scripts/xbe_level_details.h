#pragma once
#include <cstdint>
struct X86Context;
uint32_t TrialLevelDetailTarget(const X86Context&,uint8_t*,uint32_t target);
void TrialLevelDetailBlur(X86Context&,uint8_t*);
