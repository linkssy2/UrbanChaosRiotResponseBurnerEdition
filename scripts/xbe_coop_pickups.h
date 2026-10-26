#pragma once
#include <cstdint>
struct X86Context;
// Admission only: the original pickup keeps its owner, grant and consume path.
bool TrialCoopPickupAllowed(const X86Context&, const uint8_t*);
