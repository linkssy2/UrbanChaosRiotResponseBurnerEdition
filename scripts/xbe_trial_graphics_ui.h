#pragma once
#include <cstdint>
#include <filesystem>

extern bool TrialShowFps;
extern bool TrialSmoothFog;
void TrialGraphicsUi();
void TrialGraphicsFonts(const std::filesystem::path& game);
bool TrialGraphicsOpen();
bool TrialGraphicsMessage(uint32_t message, uintptr_t key, intptr_t position);
void TrialGraphicsMenuContext(uint32_t manager, uint32_t screen, uint32_t owner, uint32_t kind);
void TrialGraphicsInputChanged();
void TrialFrontendPad(uint32_t buttons,uint8_t confirm,uint8_t back,int16_t x,int16_t y);
void TrialCoopMenuPad(bool connected,uint32_t buttons,uint8_t confirm,uint8_t back,int16_t x,int16_t y);
bool TrialCoopMenuOpen();
