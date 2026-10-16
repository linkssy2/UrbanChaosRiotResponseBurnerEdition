#include <cmath>
#include <cfloat>
#include <cstdio>
#include "imgui.h"
#include "xbe_trial_graphics_ui.h"

bool TrialShowFps = true;

void TrialHudUi(double fps) {
    TrialGraphicsUi();
    if (!TrialShowFps) return;
    char text[32] = "--.- FPS";
    if (std::isfinite(fps) && fps > 0) std::snprintf(text, sizeof(text), "%.1f FPS", fps);
    // Use the renderer's measured presentation rate; never infer it from the cap.
    ImVec2 at = ImGui::GetMainViewport()->Pos;
    at.x += 8; at.y += 8;
    const float fontSize = ImGui::GetIO().DisplaySize.y >= 1000 ? 28.0f : 20.0f;
    auto* font = ImGui::GetFont();
    const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0, text);
    auto* draw = ImGui::GetForegroundDrawList();
    draw->AddRectFilled(ImVec2(at.x - 3, at.y - 2), ImVec2(at.x + size.x + 3, at.y + size.y + 2), IM_COL32(0, 0, 0, 210));
    draw->AddText(font, fontSize, at, IM_COL32(190, 213, 149, 255), text);
}
