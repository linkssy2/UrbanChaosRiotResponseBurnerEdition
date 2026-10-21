#pragma once
#include <cmath>
#include <cstdint>

namespace trial_view {
inline float Zoom(float original, unsigned degrees) {
    if(degrees==53 || degrees<40 || degrees>90 || !std::isfinite(original) || original<=0 || original>=150)
        return original;
    constexpr double half_radian=3.14159265358979323846/360;
    return float(std::atan(std::tan(original*half_radian)*
        std::tan(degrees*half_radian)/std::tan(53*half_radian))/half_radian);
}
// Undo anamorphic presentation, retaining the original half-pixel convention.
inline float HudX(float x,float scale){return (x+.5f)*scale-.5f;}
}

struct X86Context;
void TrialPlayerCameraCreated(const X86Context&,const uint8_t*);
float TrialCameraTarget(const X86Context&,const uint8_t*);
// BAC10's draw-local first-person projection, before the original publication.
void TrialWeaponProjection(const X86Context&,uint8_t*);
void TrialScopeBorders(X86Context&,uint8_t*);
struct TrialHudLayoutScope {
    TrialHudLayoutScope* previous_;float scale_=1;
    explicit TrialHudLayoutScope(const X86Context&,const uint8_t*);
    ~TrialHudLayoutScope();
};
void TrialHudLayout(const X86Context&,uint8_t*);
uint32_t TrialHudCenterBits();
// OPITEMS brackets already use the camera's projected viewport coordinates.
struct TrialProjectedHudScope {
    TrialHudLayoutScope* previous_;
    TrialProjectedHudScope(const X86Context&,const uint8_t*);
    ~TrialProjectedHudScope();
};
// Active only while the retail weapon-crosshair subtree is drawing.
struct TrialReticleScope {
    TrialReticleScope* previous_;float scale_=1;
    TrialReticleScope(const X86Context&,const uint8_t*);
    ~TrialReticleScope();
};
struct TrialHudQuadScope {
    uint8_t* base_;uint32_t address_[4]{},original_[4]{};
    TrialHudQuadScope(const X86Context&,uint8_t*);
    ~TrialHudQuadScope();
};
