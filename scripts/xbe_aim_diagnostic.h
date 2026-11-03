#pragma once
#include <cstdint>
struct X86Context;
// Opt-in, bounded CPU evidence. These hooks never adjust the aim or guest state.
#ifdef UC_BURNER_RELEASE
// Release excludes recorder state, environment queries and file/clock work.
struct TrialAimTargetScope {
    TrialAimTargetScope(const X86Context&,const uint8_t*){}
    void Shake(const X86Context&,const uint8_t*){}
    void After(const uint8_t*){}
};
struct TrialAimFireScope {TrialAimFireScope(const X86Context&,const uint8_t*,bool=false){}};
struct TrialAimShotScope {TrialAimShotScope(const X86Context&,const uint8_t*){}};
struct TrialAimReticleScope {TrialAimReticleScope(const X86Context&,const uint8_t*){}};
struct TrialAimImpactScope {TrialAimImpactScope(const X86Context&,const uint8_t*){}};
inline void TrialAimDecalPublish(const X86Context&,const uint8_t*){}
inline void TrialAimBullet(const X86Context&,const uint8_t*){}
inline void TrialAimRender(const X86Context&,const uint8_t*){}
inline void TrialAimQuad(const X86Context&,const uint8_t*){}
#else
struct TrialAimTargetScope {
    const uint8_t* base_=nullptr;
    uint32_t owner_=0,id_=0,slot_=0,world_=0,state_=0,index_=0;
    uint64_t lifetime_=0,sequence_=0;
    TrialAimTargetScope(const X86Context&,const uint8_t*);
    void Shake(const X86Context&,const uint8_t*);
    void After(const uint8_t*);
};
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
struct TrialAimImpactScope {
    TrialAimImpactScope* previous_=nullptr;
    const uint8_t* base_=nullptr;
    uint32_t owner_=0,id_=0,shot_=0,bullet_=0,bullet_id_=0,world_=0,state_=0;
    uint64_t lifetime_=0;
    unsigned publications_=0;
    TrialAimImpactScope(const X86Context&,const uint8_t*);
    ~TrialAimImpactScope();
};
void TrialAimDecalPublish(const X86Context&,const uint8_t*);
void TrialAimBullet(const X86Context&,const uint8_t*);
void TrialAimRender(const X86Context&,const uint8_t*);
void TrialAimQuad(const X86Context&,const uint8_t*);
#endif
