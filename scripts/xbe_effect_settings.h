#pragma once
#include <cstdint>
struct X86Context;

// Receiver quality only: original base shading, casters and light Begin/End stay
// intact. A scope snapshots Apply once and unwinds with the generated function.
class TrialLightingScope {
public:
    TrialLightingScope(const X86Context&, const uint8_t*);
    ~TrialLightingScope();
    TrialLightingScope(const TrialLightingScope&) = delete;
    TrialLightingScope& operator=(const TrialLightingScope&) = delete;
private:
    friend bool TrialSkipLightingReceiver(const X86Context&, const uint8_t*);
    const TrialLightingScope* previous_;
    const X86Context* context_;
    bool world_ = false, westside_ = false, scenery_ = true, characters_ = true, simple_ = false;
};
bool TrialSkipLightingReceiver(const X86Context&, const uint8_t*);
