#pragma once
#include "imgui.h"
#include <filesystem>

namespace trial_coop_preview {
// Register before the shared font atlas build; fill after it, before GPU upload.
bool Initialize(ImFontAtlas* atlas,const std::filesystem::path& game);
bool Finalize();
bool Draw(unsigned character,ImVec2 minimum,ImVec2 maximum,double elapsed_seconds);
void Reset();
}
