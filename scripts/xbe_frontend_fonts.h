#pragma once
#include <filesystem>
#include "xbe_controller_style.h"
struct ImFont;
struct ImFontAtlas;

// Call once during startup, after any other fonts are added and before the
// first ImGui frame/GPU atlas upload. Original and HD variants share that upload;
// artwork is derived from the user's game files.
// Missing or unsupported assets return false before changing the atlas.
bool TrialLoadFrontendFonts(ImFontAtlas*,const std::filesystem::path& game,
                            ImFont*& body,ImFont*& title);
// Select a prebuilt variant with identical glyph metrics; never rebuild at draw time.
ImFont* TrialFrontendFontForUi(ImFont* original);
// Virtual-key codes, or 0x101..0x105 for original A/B/d-pad/Y/X artwork.
bool TrialMenuIconUV(unsigned code,float (&uv)[4]);
