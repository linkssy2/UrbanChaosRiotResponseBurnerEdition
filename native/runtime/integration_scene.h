#pragma once
#include "renderer.h"
#include "../level_asset.h"
#include "../collision_asset_query.h"
#include <filesystem>
#include <memory>

/* Runnable PC scene preview. Authored ExposureSettings selects the sole profile;
 * actual exposure/glow/LUT owners process main's existing RGBA8 scene before HUD.
 * Explicit black meter bootstrap, zero history blending, and one exposure update
 * per host frame. This does not claim original full-frame lighting/alpha ancestry.
 * Serial immediate-context use; callers draw main geometry between begin/finish.
 * Resizing recreates post resources/history. Metering uses the top-left complete
 * 40x30 sample grid (at most3840x2160); screen output is never cropped. */
class NativeIntegrationScene {
public:
    NativeIntegrationScene(const UCLevel &,const std::filesystem::path &shader_directory);
    ~NativeIntegrationScene();
    NativeIntegrationScene(const NativeIntegrationScene&)=delete;
    NativeIntegrationScene&operator=(const NativeIntegrationScene&)=delete;
    bool available()const;
    void begin(NativeRenderer &,float delta);
    /* Called after translucent geometry and before HUD. Repeated verification
     * renders use advance=false, preserving the same frame's exposure/history. */
    void finish(NativeRenderer &,bool advance=true);
    float inverse_exposure()const;
    float exposure()const; /* Shared material/fog input scale; not adapted gain. */
    float adapted_exposure()const;
    uint64_t frames()const;
    unsigned meter_samples()const;
    int request_blur(float start,float end,float amplitude);
    void publish_blur(); //556F6/48E80, once after completed world tick
    void draw_blur(NativeRenderer &,float projection_m22,float projection_m32);

    /* Explicit PC hitscan adaptation, not the source Bullet/entity/effect graph.
     * Render centimeters, normalized internally, fixed500m segment. true means
     * real static query completed, including a miss. No RNG/NPC/effects service. */
    bool shot(const UCCollisionAsset &,uint32_t item,const float origin_cm[3],
              const float direction[3]);
    uint64_t shots=0,hits=0;
    uint32_t last_item=0;
    bool last_hit=false;
    UCCollisionRayHit last_query{}; /* Original query units: physics meters. */
private:
    struct State;
    std::unique_ptr<State> state_;
};
