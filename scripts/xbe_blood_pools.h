#pragma once
#include <cstdint>
struct X86Context;
void TrialGrowDecalRing(const X86Context&, uint8_t*);
uint64_t TrialDecalSubmissions();
void TrialDecalSubmitted();

struct TrialBloodPoolScope { TrialBloodPoolScope(); ~TrialBloodPoolScope(); };
void TrialBloodSupport(const X86Context&,uint8_t*);
void TrialClearBloodSupport();
bool TrialBloodVertex(const X86Context&,uint8_t*);
