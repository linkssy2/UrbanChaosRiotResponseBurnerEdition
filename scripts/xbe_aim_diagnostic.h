#pragma once
#include <cstdint>
struct X86Context;
// Opt-in, bounded CPU evidence. These hooks never adjust the aim or guest state.
#ifdef UC_BURNER_RELEASE
// Release excludes recorder state, environment queries and file/clock work.
struct TrialAimFireScope {TrialAimFireScope(const X86Context&,const uint8_t*,bool=false){}};
struct TrialAimShotScope {TrialAimShotScope(const X86Context&,const uint8_t*){}};
struct TrialAimReticleScope {TrialAimReticleScope(const X86Context&,const uint8_t*){}};
inline void TrialAimImpact(const X86Context&,const uint8_t*){}
inline void TrialAimBullet(const X86Context&,const uint8_t*){}
inline void TrialAimRender(const X86Context&,const uint8_t*){}
inline void TrialAimQuad(const X86Context&,const uint8_t*){}
#else
struct TrialAimFireScope {
    TrialAimFireScope* previous_=nullptr;
    uint32_t serial_=0,owner_=0,ordinary_=0,accepted_=0;
    TrialAimFireScope(const X86Context&,const uint8_t*,bool resumed=false);
    ~TrialAimFireScope();
};
struct TrialAimShotScope {
    uint32_t previous_;
    TrialAimShotScope(const X86Context&,const uint8_t*);
    ~TrialAimShotScope();
};
struct TrialAimReticleScope {
    uint32_t previous_;
    TrialAimReticleScope(const X86Context&,const uint8_t*);
    ~TrialAimReticleScope();
};
void TrialAimImpact(const X86Context&,const uint8_t*);
void TrialAimBullet(const X86Context&,const uint8_t*);
void TrialAimRender(const X86Context&,const uint8_t*);
void TrialAimQuad(const X86Context&,const uint8_t*);
#endif
