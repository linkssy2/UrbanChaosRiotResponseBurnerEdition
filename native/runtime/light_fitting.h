#pragma once
#include "mission_world.h"
#include "model.h"
#include "integration_lighting.h"
extern "C" {
#include "../light_fitting.h"
#include "../rigid_prop.h"
#include "../solid_block.h"
#include "../game_random.h"
}
/* Initial steady-state LightFitting preview, through SAME authored registry/slots.
 * Concrete218A10 health base, properties, shared constructor RNG, source-selected model,
 * group0 imported body boxes and existing linked point-light output. The PC static body and
 * selected-model owner replace the original physics/wrapper manager here.
 * No primary B0 coroutine, script C8, sleeping records, audio request, flicker,
 * damage/broken transition or complete177230/178470 class equivalence claim.
 * The real sound-bank payload is retained, not treated as a played sound.
 * Borrows lighting until close; admits immutable mode0/state0 and mode2/state1,
 * health100. The off branch selects group2 for Emergency, group0 for Fire;
 * both intensities are zero. Fire has no eligible ordinary-pass batch,
 * so its real model/pose is retained without a substituted draw. It does not advance the sleeping coroutine or wake.
 */
class NativeLightFitting {
    NativeAnimatedModel model_;
    BytesOwner sound_bank_;
    std::vector<UCSolidBlockGeometry> boxes_;
    std::vector<UCModelCollisionShape> shapes_;
    NativeIntegrationLighting *lighting_=nullptr;
    UCLightFittingParameters parameters_{};
    UCLightFittingValues values_{};
    float initial_delay_=0,base_intensity_=0;
    bool ready_=false;
public:
    // Only fields through A4 are constructed/read; the ScriptController tail
    // and holder are unused. No second transform, ID or health storage.
    NativeMissionEntity entity;
    int construct(NativeRenderer &,NativeMissionWorld &,uint32_t,const UCArchive *,
        const uint8_t[32],const std::filesystem::path &,NativeIntegrationLighting &,UCGameRandom &);
    int update();
    int late()const{return ready_?0:1;}
    void draw(NativeRenderer &,const float[16],float inverse_exposure=1,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    void collect_shadows(NativeRenderer &,NativeModelLighting &,const NativeLightingFrame &,class NativeModelShadowCasters &,float fraction=1);
    int close_detached();
    const std::vector<UCSolidBlockGeometry>&boxes()const{return boxes_;}
    const std::vector<UCModelCollisionShape>&collision_shapes()const{return shapes_;}
    const UCLightFittingParameters&parameters()const{return parameters_;}
    const UCLightFittingValues&values()const{return values_;}
    float initial_delay()const{return initial_delay_;}
    size_t sound_bank_size()const{return sound_bank_.value.size;}
    bool ready()const{return ready_;}
};
