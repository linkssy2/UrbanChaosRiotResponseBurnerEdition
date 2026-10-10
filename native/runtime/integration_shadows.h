#pragma once
#include "renderer.h"
#include "control_shadow_caster.h"
#include <memory>
struct UCScene;
struct NativeMaterial;
struct NativeLightingPoint;
struct NativeLightingShadow;

/* Explicit PC preview replacement for the original depth-fail stencil volumes.
 * Retained static opaque triangles and explicitly source-admitted posed world
 * casters cast. No alpha-test silhouette or original stencil parity claim.
 * The source light's shadows flag still controls admission in prepare().
 * Borrows the immutable mesh; the scene/material metadata is copied at create.
 * begin_frame admits the configured number of lights (default eight); request
 * relevant lights first. 512px depth cubes persist for unchanged light poses.
 * Serial immediate-context use. prepare restores OM targets/viewports; the
 * caller must rebind its shaders, IA, rasterizer and depth state afterward.
 * No frame-loop GPU readback, waiting, query or resource allocation after each
 * cache slot has first been used. Capacity is explicit, from1 to128 maps. */
class NativeIntegrationShadows {
    struct State;
    std::unique_ptr<State> state_;
public:
    NativeIntegrationShadows(NativeRenderer &,const UCScene &,const NativeGpuMesh &,
                             const std::vector<NativeMaterial> &,uint32_t capacity=8,bool batch_ranges=false);
    ~NativeIntegrationShadows();
    NativeIntegrationShadows(const NativeIntegrationShadows&)=delete;
    NativeIntegrationShadows&operator=(const NativeIntegrationShadows&)=delete;
    /* Replaces all dynamic caster/light pairs, copying posed geometry. Invalid
     * input throws before replacing prior collection. Zero clears all dynamic
     * casters; unchanged geometry/membership preserves cached light maps. */
    void set_dynamic_casters(const NativeShadowCaster *,uint32_t count);
    void begin_frame();
    static bool prepare(void *,uint32_t,const NativeLightingPoint &,NativeLightingShadow &);
    uint32_t caster_batches()const;
    uint32_t excluded_batches()const;
    uint32_t capacity()const;
    uint64_t maps_rendered()const;
    uint64_t cache_hits()const;
    uint64_t indices_submitted()const;
    uint64_t draw_calls()const;
};
