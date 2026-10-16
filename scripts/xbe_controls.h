#pragma once
#include <string>
#include "core/input.h"

namespace trial_controls {
struct Draft {
    unsigned keyboard[25][2]{}, controller[24][2]{}, deadzone[2]{}, sensitivity=400;
};
Draft Snapshot();
Draft Defaults();
bool Apply(const Draft&, std::string& error);
unsigned ControllerSource(const input::Pad&);
void Remap(input::Pad&);
}
