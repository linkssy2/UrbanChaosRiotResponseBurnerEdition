#pragma once
#include <string>
#include "core/input.h"

namespace trial_controls {
struct Draft {
    unsigned keyboard[25][2]{}, controller[24][2]{}, deadzone[2]{}, sensitivity=400;
    unsigned spin_mode=0,spin_binding[2][2]{}; // 0 retail gesture, 1 assigned button
    unsigned previous_firearm[2]{};
    unsigned health_pack[2][2]{};
    // Retail 63B70 defaults: vibration, invert, auto-centre, crouch, layout, X/Y.
    // P1 remains in the original saved profile; these values belong to P2 only.
    unsigned native_profile[7]{1,0,1,1,1,50,50};
};
// Player 2 owns its controller/deadzone, pistol spin and health-pack bindings.
Draft Snapshot(unsigned player=0);
Draft Defaults();
bool Apply(const Draft&, std::string& error,unsigned player=0);
unsigned ControllerSource(const input::Pad&);
void Remap(input::Pad&,unsigned player=0);
unsigned SpinMode(unsigned player=0);
unsigned SecondProfileValue(unsigned setting,unsigned fallback);
bool SpinButtonDown(bool keyboard,const input::Pad&,unsigned player=0);
bool HealthPackDown(bool keyboard,const input::Pad&,unsigned player=0,bool mouse_active=false);
}
