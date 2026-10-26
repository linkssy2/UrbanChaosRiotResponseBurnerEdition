#pragma once
#include <cstdint>

void TrialCoopSpinReset();
void TrialCoopSpinActorReset(const uint8_t*,uint32_t actor);
void TrialCoopSpinWrite(uint8_t*,uint32_t actor,uint8_t value);
uint8_t TrialCoopSpinHud(const uint8_t*,uint32_t reticle);
uint8_t TrialCoopSpinReward(const uint8_t*,uint32_t controller,uint32_t killer);
void TrialCoopSpinConsume(uint8_t*,uint32_t controller,uint32_t killer);
