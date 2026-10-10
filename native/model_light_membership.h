#ifndef UC_MODEL_LIGHT_MEMBERSHIP_H
#define UC_MODEL_LIGHT_MEMBERSHIP_H
#include "point_light_geometry.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCModelMembershipLight {
    const UCPointLightState *state;
    const UCPointLightGeometry *geometry;
    const float *local_center; /* Four authoritative source+C0 floats, NOT color+60. */
    int32_t index;
} UCModelMembershipLight;
/* Exactly one stable descriptor address per actual light; enumerate duplicates
 * by repeating that address, never by cloning adapters for the same owner.
 * Unique membership ordering uses unsigned native pointer order. It does not
 * reproduce original heap addresses or certify cross-platform allocation order. */
typedef struct UCModelLightMembership UCModelLightMembership;
typedef struct UCModelLightMembershipView {
    uint32_t generation,static_budget;
    uint8_t flags,indexed,disabled;
    const float *world; /* Actual model world,16 floats; translation is score input. */
    const UCModelBounds *bounds;
    const UCModel *model; /* Current B5450 result, only needed for indexed selection. */
    const UCModelMembershipLight *const *lights; /* Source owner's enumeration order. */
    uint32_t light_count;
    void *context;
    /* Required only for moved, nonindexed, nonnegative, intersecting enabled
     * candidates with zero local-center XYZ sum. Implements the actual +94 service;
     * true excludes the candidate. No default successful/no-occlusion stub. */
    int (*world_query)(void *,const UCModelMembershipLight *,const float center[3]);
} UCModelLightMembershipView;
typedef struct UCModelLightScore {const UCModelMembershipLight *light;float score;} UCModelLightScore;
/* One native membership owner, no second world/light graph. One allocation,
 * fixed capacity1..65536. Initial generation/retained center must be established
 * by the caller: B60A0 alone does not prove their startup values. */
UCModelLightMembership *uc_model_light_membership_create(uint32_t capacity,uint32_t generation,const float center[3]);
void uc_model_light_membership_free(UCModelLightMembership *);
/* Actual B2BA0/B2A40 and B2BC0 finite domain; score includes W and native x86
 * RSQRTSS with one original refinement. Returns0 unsupported without output.
 * Zero squared norm is excluded; physical Xbox approximation is not claimed. */
int uc_model_light_intersection(const UCPointLightGeometry *,const UCModelBounds *,int *);
int uc_model_light_score(const UCPointLightGeometry *,const float position[4],float *);
/* BA040 and B9FD0, with complete native ordered vectors and unique identity set.
 * Source enumeration is explicitly supplied in order; registration is not here.
 * Stable/disjoint inputs, geometry, model, light identities, FP environment and
 * world_query through the call; callbacks complete, cannot mutate/reenter/free.
 * Retained light identities and their borrowed owners MUST remain alive across
 * frames until movement clears them or this owner is freed. Source lifetime and
 * invalidation controller is a caller prerequisite. No guessed radius cache.
 * Whole-call preflight/capacity and finite guards precede effects; callback FP
 * flags and allocator callback traces are excluded. Returns0 rejected,1 complete.
 * Same generation and flags bit4 gates do not read later caller fields. */
int uc_model_light_membership_update(UCModelLightMembership *,const UCModelLightMembershipView *);
int uc_model_light_membership_contains(const UCModelLightMembership *,const UCModelMembershipLight *);
const UCModelMembershipLight *const *uc_model_light_membership_members(const UCModelLightMembership *,uint32_t *count);
const UCModelLightScore *uc_model_light_membership_retained(const UCModelLightMembership *,uint32_t *count);
uint32_t uc_model_light_membership_generation(const UCModelLightMembership *);
const float *uc_model_light_membership_center(const UCModelLightMembership *);
#ifdef __cplusplus
}
#endif
#endif
