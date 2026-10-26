#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace trial_coop_preview {
inline constexpr unsigned MaxImages=32, MaxImageDimension=1024;
struct Vertex {
    float position[3]{},normal[3]{},uv[2]{};
    uint32_t color=0xffffffffu; // Original ARGB, independent of ImGui's packing.
};
struct Image {unsigned width=0,height=0;std::vector<uint8_t> rgba;};
struct Triangle {uint32_t indices[3]{};unsigned image=0;};
struct Model {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::vector<Image> images;
    float center[3]{},radius=0;
};
// Owned original Stand-pose geometry. Wrapped source textures are tiled so all
// returned UVs are within [0,1]. Failure preserves out; names never become paths.
bool Load(const std::filesystem::path& game,unsigned character,Model& out,std::string& error);
}
