#pragma once
#include <filesystem>
struct ImFont;
struct ImFontAtlas;

// Call once during startup, after any other fonts are added and before the
// first ImGui frame/GPU atlas upload. Artwork stays in the user's game files.
// Missing or unsupported assets return false before changing the atlas.
bool TrialLoadFrontendFonts(ImFontAtlas*,const std::filesystem::path& game,
                            ImFont*& body,ImFont*& title);
// Virtual-key codes, or 0x101..0x105 for original A/B/d-pad/Y/X artwork.
bool TrialMenuIconUV(unsigned code,float (&uv)[4]);
