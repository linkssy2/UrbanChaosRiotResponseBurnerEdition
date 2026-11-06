#pragma once
#include <array>
#include <cstdint>
struct X86Context;

// The retail pulse list owns timing and storage; only zoom-pulse ownership is added.
struct TrialCoopAimBlurRequestScope {
    uint8_t* base_;
    uint32_t manager_=0,sentinel_=0,previous_=0,owner_=0,id_=0;
    unsigned slot_=0;
    int exceptions_;
    TrialCoopAimBlurRequestScope(const X86Context&,uint8_t*);
    ~TrialCoopAimBlurRequestScope();
};
void TrialCoopAimBlurBeginUpdate(const X86Context&,const uint8_t*);
void TrialCoopAimBlurNodeBegin(const X86Context&,uint8_t*);
void TrialCoopAimBlurPublish(const X86Context&,const uint8_t*);
void TrialCoopAimBlurForgetNode(const X86Context&);
void TrialCoopAimBlurClear(const X86Context&);

struct TrialCoopAimBlurViewScope {
    uint8_t* base_;
    uint32_t descriptor_=0,table_=0;
    std::array<uint32_t,3> parameters_{};
    uint8_t enabled_=0;
    int previous_=-1,slot_=-1;
    TrialCoopAimBlurViewScope(const X86Context&,uint8_t*);
    void Apply(const X86Context&);
    ~TrialCoopAimBlurViewScope();
};
// Valid only within an ordinary, owned co-op scene blur pass.
int TrialCoopAimBlurActiveView();
