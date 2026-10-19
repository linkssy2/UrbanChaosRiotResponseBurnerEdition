#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

// Identify imported UI and prepare its approved HD art once before rendering.
// Images and returned pointers remain immutable for the process lifetime.
namespace uc_hd_ui {
namespace detail {
struct Source { const char* file; bool hud_only; };
// Equipment icons are loaded separately from the mission/frontend archives.
// Named UI entries only: equipment meshes and world textures stay untouched.
inline constexpr Source kSources[] = {
    {"PersistantFilesXBox.lst",false},{"99-01.ent",false},{"frontendload.lst",false},
    {"40-01.ent",true},{"9-01.ent",true},
    {"mobilechaingun.lst",true},{"pistol.lst",true},{"pistolmk2.lst",true},
    {"pistolmk3.lst",true},{"pistolmk4.lst",true},
    {"policeassaultrifle.lst",true},{"policeassaultriflemk2.lst",true},
    {"policeassaultriflemk3.lst",true},{"policeshotgun.lst",true},
    {"policeshotgunmk2.lst",true},{"policeshotgunmk3.lst",true},
    {"taser.lst",true},{"tasermk2.lst",true}
};
}
struct Image {
    uint32_t width = 0, height = 0;
    std::vector<uint8_t> bgra;
};
struct Statistics {
    size_t images = 0, source_bytes = 0, pixel_bytes = 0;
    size_t fonts = 0, contoured_fonts = 0, panels_2x = 0, rejected = 0;
    size_t restored_images = 0;
};
void Initialize(const std::filesystem::path& game);
const Image* Match(uint32_t width, uint32_t height, uint32_t format,
                   const uint8_t* payload, size_t bytes);
const Image* Font(const char* name);
const Image* Buttons();
bool Ready();
Statistics Stats();
}
