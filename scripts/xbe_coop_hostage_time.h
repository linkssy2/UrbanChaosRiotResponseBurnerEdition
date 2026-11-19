#ifndef UCRR_XBE_COOP_HOSTAGE_TIME_H
#define UCRR_XBE_COOP_HOSTAGE_TIME_H
#include <cstdint>
struct X86Context;
void TrialCoopHostageSlowmoAdmission(X86Context&,uint8_t*);
void TrialCoopHostageSlowmoQueryPlayer(X86Context&,const uint8_t*);
void TrialCoopHostageSlowmoLivingPlayer(X86Context&,const uint8_t*);
void TrialCoopHostageSlowmoFeedback(X86Context&,uint8_t*);
#endif
