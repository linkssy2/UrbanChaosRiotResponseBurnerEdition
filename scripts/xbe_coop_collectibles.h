#pragma once
#include <cstdint>
struct X86Context;

// One original MaskPickup update owns the selection and its single reward.
class TrialCoopEvidenceScope {
public:
    TrialCoopEvidenceScope(const X86Context&,const uint8_t*);
    ~TrialCoopEvidenceScope();
    void Select(X86Context&,const uint8_t*);
private:
    TrialCoopEvidenceScope* previous_;
    uint32_t pickup_=0,pickup_id_=0,world_=0,actor_=0,actor_id_=0;
    unsigned slot_=0;
    friend void TrialCoopEvidencePopup(X86Context&,const uint8_t*);
};
void TrialCoopEvidencePopup(X86Context&,const uint8_t*);
