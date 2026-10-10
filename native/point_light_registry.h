#ifndef UC_POINT_LIGHT_REGISTRY_H
#define UC_POINT_LIGHT_REGISTRY_H
#include "model_light_membership.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPointLightRegistry UCPointLightRegistry;
/* Stable heap owner: never copy/move it. Membership borrows these exact fields.
 * Device is the original constructor's borrowed provider+10 identity, not an
 * independently created renderer. Native types replace Xbox vtables. Self-borrowing descriptor/device/registry
 * identity fields are immutable; caller mutations of transform/center/attenuation
 * must set state.dirty and complete the existing geometry update before use. */
typedef struct UCPointLightInstance {
    UCPointLightState state;
    UCPointLightGeometry geometry;
    float world[16],local_center[4];
    UCModelMembershipLight membership;
    const void *device;
    UCPointLightRegistry *registry;
} UCPointLightInstance;
/* One actual ordered manager registry; native preallocation replaces list-node
 * allocations. Capacity1..65536 bounds entries and constructed live instances, no light graph clone. Free succeeds only after
 * all entries and all constructed instances are gone. */
UCPointLightRegistry *uc_point_light_registry_create(uint32_t capacity);
int uc_point_light_registry_free(UCPointLightRegistry *);
/* A5140 tail append and A4B10/A4A50 remove ALL equal identities, preserving other
 * order. Borrowed descriptors may occur multiple times. No light destruction.
 * Mutation invalidates snapshot storage. No mutation during enumeration/use. */
int uc_point_light_registry_add(UCPointLightRegistry *,const UCModelMembershipLight *);
uint32_t uc_point_light_registry_remove(UCPointLightRegistry *,const UCModelMembershipLight *);
const UCModelMembershipLight *const *uc_point_light_registry_view(const UCPointLightRegistry *,uint32_t *count);
/* Full B3110/B3000/B79D0 represented fields and actual registration, with existing
 * B2D60 geometry. Source B3110's third argument is unused. Finite position/radius,
 * nearest/gradual arithmetic; finite geometry/capacity preflight before commit.
 * NULL allocation or invalid native admission returns NULL without effects.
 * This deliberately excludes original allocation-failure exception behavior.
 * World manager is required, not the separate first-person directional manager. */
UCPointLightInstance *uc_point_light_registry_point(UCPointLightRegistry *,const float position[3],float attenuation,const void *device);
/* Source B30E0(flags1)->B3100/B7880 unregister then immediate free. Registry must
 * be the same live current graphics+2C manager. Borrowed descriptors in model
 * memberships/draws must already be quiescent or cleared; original receiver
 * invalidation is outside this owner. No reentry or concurrent use. */
int uc_point_light_registry_destroy(UCPointLightRegistry *,UCPointLightInstance *);
#ifdef __cplusplus
}
#endif
#endif
