#pragma once
#include <string>
#include "core/input.h"

namespace trial_controls {
struct Draft {
    unsigned keyboard[25][2]{}, controller[24][2]{}, deadzone[2]{}, sensitivity=400;
    unsigned spin_mode=0,spin_binding[2][2]{}; // 0 retail gesture, 1 assigned button
    unsigned previous_firearm[2]{};
};
Draft Snapshot();
Draft Defaults();
bool Apply(const Draft&, std::string& error);
unsigned ControllerSource(const input::Pad&);
void Remap(input::Pad&);
unsigned SpinMode();
bool SpinButtonDown(bool keyboard,const input::Pad&);
}
