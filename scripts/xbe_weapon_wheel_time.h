#pragma once
#include <cstdint>
struct X86Context;

// Borrow only the world frame's gameplay argument. Wall time, unscaled delta,
// input/UI clocks and the original slow-motion/audio scale remain untouched.
class TrialWeaponWheelTime {
    uint8_t* argument_=nullptr;
    uint32_t original_=0;
    bool owner_=false;
public:
    TrialWeaponWheelTime(const X86Context&,uint8_t*);
    ~TrialWeaponWheelTime();
    TrialWeaponWheelTime(const TrialWeaponWheelTime&)=delete;
    TrialWeaponWheelTime& operator=(const TrialWeaponWheelTime&)=delete;
};
