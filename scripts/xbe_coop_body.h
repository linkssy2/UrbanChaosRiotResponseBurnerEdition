#pragma once
#include <array>
#include <cstdint>
struct X86Context;

// Original ThirdPersonPlayer children, attached only to admitted local actors.
void TrialCoopBodiesEnsure(X86Context&,uint8_t*);
void TrialCoopBodiesReset();
void TrialCoopBodyResolve(X86Context&,uint8_t*);
void TrialCoopBodyWeapon(X86Context&,uint8_t*);
void TrialCoopBodyAction(X86Context&,uint8_t*);
uint32_t TrialCoopBodyForPlayer(const uint8_t*,uint32_t owner);
bool TrialCoopBodyRecreate(X86Context&,uint8_t*,uint32_t owner);
void TrialCoopBodiesRetire(X86Context&,uint8_t*);
bool TrialCoopBodyAnimationSoundSuppressed(const X86Context&,const uint8_t*);
void TrialCoopBodyDestroyed(const X86Context&,uint8_t*);
// 215222 has resolved the layer camera but has not entered its render scope.
bool TrialCoopSkipUnreadyView(const X86Context&,const uint8_t*);
struct TrialCoopBodyView {
    struct Saved {
        uint32_t object=0,id=0,flags=0,visibility=0;
        uint8_t visible=0;
    };
    uint8_t* base_=nullptr;
    std::array<Saved,10> saved_{};
    ~TrialCoopBodyView();
    void Select(const X86Context&,uint8_t*);
    void Clear();
};
