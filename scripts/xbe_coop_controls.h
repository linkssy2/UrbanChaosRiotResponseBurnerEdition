#pragma once
#include <cstdint>
// Replace only the admitted second player's control reads, never profile storage.
unsigned TrialCoopControlValue(const uint8_t* base,uint32_t actor,unsigned setting,unsigned original);
unsigned TrialCoopControlSlotValue(const uint8_t* base,unsigned slot,unsigned setting,unsigned original);
unsigned TrialCoopVibrationValue(const uint8_t* base,uint32_t manager,unsigned slot,unsigned original);
