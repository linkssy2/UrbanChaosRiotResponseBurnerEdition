#pragma once
#include <string>
#include "core/input.h"

namespace trial_controls {
struct Draft {
    // 0..30 replaces the native stick deadzone; 31 preserves the original curve.
    unsigned keyboard[25][2]{}, controller[24][2]{}, deadzone[2]{31,31}, sensitivity=400, invert_mouse=0;
    unsigned spin_mode=0,spin_binding[2][2]{}; // 0 retail gesture, 1 assigned button
    unsigned previous_firearm[2]{};
    unsigned health_pack[2][2]{};
    unsigned heal_partner[2]{}; // Keyboard/mouse only; controllers use hold D-pad Down.
    unsigned aim_mode=0,scope_zoom[2][2]{}; // 0 toggle, 1 hold
    unsigned zoom_sensitivity=100; // Percent of this player's existing zoomed look speed.
    unsigned walk_mode=0,walk_binding[2][2]{},hud_binding[2][2]{};
    unsigned movement_inertia=1;
    unsigned mixed_input=0; // P1 only; simultaneous controller/mouse gameplay is opt-in.
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
// Native stick descriptors are associated with the pad sampled by XInputGetState.
void BeginControllerSample(unsigned port);
void AllowControllerDeadzone(unsigned port,unsigned axis_mask);
void SuspendControllerDeadzones();
void RegisterControllerAxes(uint32_t axes);
bool NativeControllerResponse(uint32_t source,float minimum,float gain,float& value);
unsigned SpinMode(unsigned player=0);
unsigned AimMode(unsigned player=0);
unsigned ZoomSensitivity(unsigned player=0);
unsigned SecondProfileValue(unsigned setting,unsigned fallback);
bool SpinButtonDown(bool keyboard,const input::Pad&,unsigned player=0);
bool HealthPackDown(bool keyboard,const input::Pad&,unsigned player=0,bool mouse_active=false);
bool HealPartnerDown(bool mouse_active=false);
bool ScopeZoomDown(bool keyboard,const input::Pad&,unsigned player=0,bool mouse_active=false);
unsigned WalkMode(unsigned player=0);
unsigned MovementInertia(unsigned player=0);
unsigned MixedInput();
bool WalkDown(bool keyboard,const input::Pad&,unsigned player=0,bool mouse_active=false);
bool HudToggleDown(bool keyboard,const input::Pad&,unsigned player=0,bool mouse_active=false);
}
