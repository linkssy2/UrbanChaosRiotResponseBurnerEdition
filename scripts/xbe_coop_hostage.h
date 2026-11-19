#ifndef UCRR_XBE_COOP_HOSTAGE_H
#define UCRR_XBE_COOP_HOSTAGE_H
#include <cstdint>
struct X86Context;
void TrialCoopHostageTarget(X86Context&,const uint8_t*,bool dragging=false);
void TrialCoopHostageFacing(X86Context&,const uint8_t*);
void TrialCoopHostageIdleFacing(X86Context&,const uint8_t*);
#endif
