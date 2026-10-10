#ifndef UC_HYBRID_LIGHTING_GUEST_H
#define UC_HYBRID_LIGHTING_GUEST_H
#include <stdint.h>
#include "../native/spot_light.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Owned values for ONE original 99240 invocation, not a whole Present. Camera
 * matrices are copied before receiver callbacks replace the model matrix.
 * Unknown/secondary cameras must not inherit another invocation's targets. */
typedef struct UcHybridLightingScene {
    uint64_t epoch;
    uint32_t caller,scheduler,manager,matrix_provider;
    float camera[16],view[16],projection[16],view_projection[16],exposure;
} UcHybridLightingScene;
/* Raw light input at 98E66, before Begin updates its derived fields. +1A0 and
 * +E0 of a point light can be stale here: calculate local * world instead.
 * source_index is the exact common +78 getter AF110; -1 means unindexed.
 * Point4573F0: local+C0, radius+D0, specular+D4.
 * Directional457338: local+C0 is a direction (W0), no radius.
 * Spot457270: local+C0, direction+D0, radius+E0, cone+E4, specular+E8.
 * Merely recording these variants does not implement their native shading. */
typedef struct UcHybridLightingRecord {
    uint32_t source_va,vtable;
    int32_t source_index;
    float world[16],local[4],color[4],intensity,radius,direction[4],cone;
    uint32_t shadow,specular;
    UCNativeControlSpot spot; /* Fresh AF5F0 output, never stale pre-Begin RAM. */
    uint32_t spot_valid;
} UcHybridLightingRecord;
/* Hooks preserve ALL guest registers, stack, x87/MMX/XMM state and MXCSR.
 * begin observes original ESP return address and ECX scheduler. collect follows
 * the original enabled predicate but precedes every old light Begin. Returning
 * 1 takes the ORIGINAL next-light continuation; no Begin/End pair is entered.
 * Replacement mode suppresses unsupported variants. The explicit
 * UCRR_HYBRID_CONTROL_FLASHLIGHT=1 exception returns 0 only for the original
 * spot subclass (457270), retaining its whole visitor and disabling native
 * spot preparation/callbacks, including when native spot flags also equal 1.
 * UCRR_HYBRID_CONTROL_MODELS=1 retains point/directional Begin/caster/End in
 * the original world and first-person scopes. The receiver hook below keeps
 * their ordinary models while native scenery continues to own map lighting.
 * This switch disables native model/caster replacement even if extended is on.
 * UCRR_HYBRID_CONTROL_SCENERY_EFFECTS additionally retains only the indexed
 * point-lit material batches documented below, in their original draw order.
 * apply is at9972F before later callbacks; ordered guest drain and retained-batch
 * flush are prerequisites for host drawing. end also handles early scene exit. */
uint64_t uc_hybrid_lighting_scene_begin(void);
int uc_hybrid_lighting_collect(uint32_t light_va,uint32_t iterator_va,
    uint32_t context_va,uint32_t shadow_permission);
/* Thread-safe UI setting, default ON; sampled at the next original scene.
 * OFF suppresses point/directional/spot receiving on world character/movable
 * models (457880, +B1=0). First-person, scenery and caster passes stay intact. */
int uc_hybrid_character_lighting_visible(void);
void uc_hybrid_character_lighting_show(int visible);
int uc_hybrid_character_lighting_quality(void);
void uc_hybrid_character_lighting_set_quality(int simple);
/* At 9911C, after original light membership; true uses original next receiver.
 * The character switch also covers CONTROL flashlight receivers; never casters. */
int uc_hybrid_lighting_skip_receiver(uint32_t light_va,uint32_t receiver_va,uint32_t context_va);
/* Optional source-ordered 1-01 scenery effects. Only six indexed material slots
 * omitted by the native diffuse pass are admitted; no transient/directional
 * scenery or native-covered batch is restored. At BB20B, true skips to BB232. */
int uc_hybrid_lighting_skip_scenery_batch(uint32_t light_va,uint32_t receiver_va,
    uint32_t node_va,uint32_t batch_va);
void uc_hybrid_lighting_scene_apply(uint64_t token);
void uc_hybrid_lighting_scene_end(uint64_t token);
void uc_hybrid_lighting_read_counts(uint64_t *out,unsigned count);
int uc_hybrid_extended_enabled(void);
/* Optional per-batch native model experiment. Original Begin/stencil/query and
 * material order remain authoritative; unsupported inputs retain CONTROL.
 * World ordinary models only; no first-person, flashlight or map replacement. */
int uc_hybrid_lighting_model_source(uint32_t light,uint32_t receiver,uint32_t modifier,
    UcHybridLightingScene *scene,UcHybridLightingRecord *record);
int uc_hybrid_lighting_model_replacement_enabled(void);
/* Independent reduced-build mode: native spotlight on world actors only.
 * Explicit CONTROL flashlight mode takes precedence and returns false. */
int uc_hybrid_flashlight_models_enabled(void);
#ifdef __cplusplus
}
#endif
#endif
