#pragma once
#include <cstdint>
struct X86Context;
// Admission only: the original pickup keeps its owner, grant and consume path.
bool TrialCoopPickupAllowed(const X86Context&, const uint8_t*);

// The retail controller owns one FIFO for large pickup/checkpoint messages.
struct TrialCoopPickupScope {
    uint32_t previous_;
    TrialCoopPickupScope(uint32_t actor,const uint8_t*);
    ~TrialCoopPickupScope();
};
void TrialCoopPickupNotice(const uint8_t*,uint32_t channel,uint32_t row);
void TrialCoopPickupNoticeExpired(const uint8_t*,uint32_t channel);
void TrialCoopPickupNoticesReset(uint32_t controller);
struct TrialCoopPickupTextScope {
    uint8_t* base_;uint32_t y_=0,saved_=0;bool skip_=false;
    TrialCoopPickupTextScope(const X86Context&,uint8_t*);
    ~TrialCoopPickupTextScope();
};
