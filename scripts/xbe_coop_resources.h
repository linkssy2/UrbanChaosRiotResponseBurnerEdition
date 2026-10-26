#pragma once
#include <cstdint>
struct X86Context;
void TrialCoopPreload(X86Context&,uint8_t*);
void TrialCoopResourceLoaded(const X86Context&,const uint8_t*);
struct TrialCoopArmsScope {
    uint8_t* base_=nullptr;uint32_t original_=0;
    TrialCoopArmsScope(const X86Context&,uint8_t*);
    ~TrialCoopArmsScope();
};
