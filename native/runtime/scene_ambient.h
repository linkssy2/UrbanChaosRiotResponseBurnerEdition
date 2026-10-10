#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
extern "C" {
#include "../level_asset.h"
#include "../environment_ambient.h"
#include "../environment_ambient_region.h"
#include "../scene_ambient_query.h"
}
/* One native world ambient owner. Tick once per simulation step; sampling does
 * not change it. Original region order, smoothing and camera-position query.
 * Alternate gray mode is scene-manager2149B0 owner+4/+14, NOT world+10 flags. */
class NativeSceneAmbient {
    struct Region {UCEnvironmentAmbientRegion state{};float bounds[6]{};};
    struct Radial {UCPointLightState state{};float center[4]{};};
    UCEnvironmentAmbient owner_{};
    std::vector<Region> regions_;
    std::vector<Radial> radials_;
    std::vector<UCSceneAmbientLight> query_;
    uint32_t color_=0xffbfbfbfu,mode_=0;size_t selected_=0;
    float position_[3]{},delta_=0;
public:
    NativeSceneAmbient(const UCLevel &,const void *geo,size_t geo_size);
    void step(const float player_position[3],float delta,uint32_t scene_mode=0);
    void sample(const float camera_position[3],float exposure,float rgba[4])const;
    uint32_t color()const{return color_;}
};
