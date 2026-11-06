#pragma once
#include <array>
#include <cstdint>
struct X86Context;

// Retail depth-of-field keeps its scratch textures and Gaussian filter. Only
// ordinary split-screen calls borrow one player's scene rectangle.
struct TrialCoopAimBlurRenderScope {
    X86Context& context;
    uint8_t* base;
    TrialCoopAimBlurRenderScope* previous;
    std::array<uint32_t,6> incoming{},viewport{};
    uint32_t device=0,target=0,depth=0;
    int view=-1,exceptions=0;
    bool copied=false;
    TrialCoopAimBlurRenderScope(X86Context&,uint8_t*);
    ~TrialCoopAimBlurRenderScope() noexcept(false);
};
void TrialCoopAimBlurDownsample(const X86Context&,uint8_t*);
void TrialCoopAimBlurVertical(const X86Context&,uint8_t*);
void TrialCoopAimBlurComposite(const X86Context&,uint8_t*);
// Captured by DrawVerticesUP on the title thread, never by the render worker.
unsigned TrialCoopAimBlurSourceView(uint32_t caller);
