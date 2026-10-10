#pragma once
#include "renderer.h"
#include <filesystem>
#include <memory>
extern "C" {
#include "../scene_asset.h"
#include "../scene_models.h"
#include "../level_asset.h"
#include "../archive_reader/archive_reader.h"
#include "../spot_light.h"
}

struct UCModelMembershipLight;
struct NativeLightingPoint {
    float position[3]{},radius=0,color[4]{},intensity=0;
    bool shadows=false,specular=false;
    uint32_t source_index=0;
    const void *identity=nullptr;
    float local_center[4]{};
    const UCModelMembershipLight *membership=nullptr;
};
struct NativeLightingDirectional {
    // World-space source ray direction; receiver setup negates it toward light.
    float direction[3]{},color[4]{},intensity=0;
    uint32_t source_index=0;
};
struct NativeLightingFrame {
    const void *owner=nullptr;uint64_t generation=0;
    const NativeLightingPoint *points=nullptr;size_t point_count=0;
    const NativeLightingDirectional *directionals=nullptr;size_t directional_count=0;
    const UCNativeControlSpot *spots=nullptr;size_t spot_count=0;
    const uint32_t *spot_indices=nullptr;
    const float (*spot_planes)[4]=nullptr;const uint8_t *spot_specular=nullptr;
    // 977A0 sample after exposure division, before material C2 quantization.
    float eye[3]{},exposure=1,ambient[4]{};
    // Original A5180/A4AA0 manager+40 default; a live owner may replace it.
    float normal_weight=.5f;
};
struct NativeMaterial;
struct UCPointLightInstance;
struct NativeLightingShadow {
    ID3D11ShaderResourceView *cube=nullptr;
    float near_z=1,far_z=1,bias=0;
};
using NativeLightingShadowProvider=bool(*)(void *,uint32_t,const NativeLightingPoint &,NativeLightingShadow &);
struct NativeLightingCounts {
    uint32_t authored=0,points=0,unsupported_lights=0,configured=0,unmatched_entities=0;
    uint32_t materials=0,unsupported_materials=0,memberships=0,shadow_eligible=0;
    uint32_t excluded_specular_batches=0;
    uint32_t drawn_lights=0,drawn_batches=0,shadowed_lights=0,shadow_requests=0,shadow_unserved=0;
};
/* Source GEO lights, matched ENT configuration and indexed receivers. The
 * legacy draw() is an additive RGB preview; the full-native frame API owns
 * recovered ambient/light/base composition for explicitly qualified materials.
 * Optional cube shadows are explicitly a PC replacement for Xbox stencil.
 * Scene/model metadata must describe the SAME immutable decoded GEO. */
class NativeIntegrationLighting {
public:
    NativeIntegrationLighting(NativeRenderer &,const UCScene &,const UCSceneModels &,const std::vector<NativeMaterial> &,
        const void *geo,size_t geo_size,const UCLevel &,const UCArchive *,const std::filesystem::path &shaders);
    ~NativeIntegrationLighting();
    NativeIntegrationLighting(const NativeIntegrationLighting&)=delete;
    NativeIntegrationLighting& operator=(const NativeIntegrationLighting&)=delete;
    void draw(NativeRenderer &,const float view_projection[16],const float eye[3],float exposure,bool enabled,
        NativeLightingShadowProvider shadows=nullptr,void *shadow_owner=nullptr);
    const NativeLightingCounts &counts() const;
    uint32_t light_count() const;
    const NativeLightingPoint &light(uint32_t) const;
    // Exact ENT LightFittingOmniLight identity matched at construction. The
    // immutable authored base is distinct from the live controller multiplier.
    bool fitting_base(uint32_t entity_index,float &) const;
    bool set_fitting_intensity(uint32_t entity_index,float);
    /* Hybrid frame override. Only the supplied source indices are enabled;
     * UINT32_MAX entries are transient scenery lights. No guest-light fallback.
     * Transient shadows/specular remain unsupported and must be cleared by a
     * caller that explicitly counts/discloses those omissions. */
    void set_frame_lights(const NativeLightingPoint *,size_t);
    // Original spotlight matrices/volume, shared scenery receivers/resources.
    // Caller owns target/state restoration; no original lighting draw is run.
    uint32_t draw_spots(NativeRenderer &,const float view_projection[16],float exposure,
        const UCNativeControlSpot *,size_t);
    // Live effect lights share the scene's registry and existing diffuse pass.
    // Receiver selection uses base scenery geometry; moving actor receivers
    // and PC shadow caches for these transient lights remain outside this pass.
    UCPointLightInstance *create_dynamic(const float position[3],float radius);
    bool destroy_dynamic(UCPointLightInstance *);
    // Full-native shared state, borrowed until the next frame() call. Fresh
    // registry state includes live fitting/explosion/weapon effect lights.
    // The source ambient sample is supplied by the scene owner, not invented.
    NativeLightingFrame frame(const float eye[3],float exposure,const float ambient[4],
        const UCNativeControlSpot *flashlight=nullptr,bool enabled=true,const float flashlight_plane[4]=nullptr);
    void model_resources(ID3D11ShaderResourceView *&cube,ID3D11ShaderResourceView *&point,
        ID3D11ShaderResourceView *&spot,ID3D11SamplerState *&sampler);
    // Full-native qualified opaque domain. Original977A0 ambient first,
    // then source light contribution, then caller's base material with blend3.
    // Existing hybrid draw() remains the albedo-modulated adaptation.
    bool owns_material(uint32_t material_index)const;
    bool writes_alpha(uint32_t material_index)const;
    bool alpha_only(uint32_t material_index)const;
    // Original later alpha group, after opaque base composition; no ordinary base draw.
    void draw_alpha(NativeRenderer &,const NativeGpuMesh &,const float view_projection[16],const NativeLightingFrame &,
        NativeLightingShadowProvider shadows=nullptr,void *shadow_owner=nullptr);
    void draw_prepass(NativeRenderer &,const float view_projection[16],const NativeLightingFrame &);
    void draw_native(NativeRenderer &,const float view_projection[16],const NativeLightingFrame &,
        NativeLightingShadowProvider shadows=nullptr,void *shadow_owner=nullptr);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
