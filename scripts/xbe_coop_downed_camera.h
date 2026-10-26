#pragma once
#include <cstdint>
struct X86Context;
void TrialCoopDownedCameraReset();
void TrialCoopDownedCameraCapture(const X86Context&,uint8_t*);
void TrialCoopDownedCameraUpdate(const X86Context&,uint8_t*);
