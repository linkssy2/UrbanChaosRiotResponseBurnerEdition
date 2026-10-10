#ifndef UC_SHELL_LIFECYCLE_H
#define UC_SHELL_LIFECYCLE_H
#include "game_random.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCShellState {
    float transform[16];
    uint32_t entity_id,parent_id,weapon_type;
    void *model;
    float velocity[3];
    int32_t start_bone;
    float ground_y,parent_position[3];
    uint32_t family;
    uint8_t bounce;
    float age;
} UCShellState;
typedef struct UCShellProperties {
    /* Immutable projection of successful WeaponType/EjectVelocity/StartBoneID/
     * GroundY lookups. Bits1/2/4/8 mean present. WeaponType must be present2/17.
     * The property description/factory and C6670/C4790 producer are not owned. */
    uint32_t present,weapon_type;
    float velocity[3];
    int32_t start_bone;
    float ground_y;
} UCShellProperties;
typedef struct UCShellServices {
    void *context;
    /* Pure admission, before any effect: constructor when properties!=NULL.
     * Certify all completing services, finite bounded results, stable graph,
     * and a resolvable parent when first update has start_bone!=-1. */
    int (*preflight)(void *,const UCShellState *,const UCShellProperties *);
    int (*construct_base)(void *,UCShellState *); /*0 complete; nonzero stops with2 */
    /* Resolve each invocation; returned position is borrowed until next call.
     * Null is allowed except when the first update needs a bone position. */
    const float *(*parent_position)(void *,uint32_t parent_id);
    void (*bone_position)(void *,uint32_t parent_id,int32_t tag,float out[3]);
    void (*group)(void *,UCShellState *,int add);
    /* attach must publish the actual current wrapper in state.model (or0).
     * configure completes current wrapper->model getter, model+24(1), getter
     * again, model+54(.1,.15), then21CB90(wrapper,.9), in that source order.
     * This boundary is required, not an assumed successful empty effect. */
    void (*attach_model)(void *,UCShellState *,const char *asset);
    void (*configure_model)(void *,UCShellState *);
    void (*bounce_sound)(void *,UCShellState *); /*216020/216BD0 ShellBounce*/
    double (*delta)(void *); /*three distinct current1F9010 reads*/
    void (*remove)(void *,UCShellState *); /*complete218B30, after group removal*/
} UCShellServices;
/* Complete represented11DFB0/11DC50/11DBD0/11DBA0 ordering. No generic entity
 * factory, group registry, model/audio owner or deferred slot release is
 * fabricated. Native state is the sole shell state, never a second entity.
 * Stable disjoint state/properties/RNG; shared RNG is borrowed, never reseeded.
 * Hooks are nonreentrant; base may report its checked failure. Other hooks
 * complete. No mutation of this state except base
 * construction and attach.model, no changes to RNG/FP environment. Current
 * parent values/delta may vary between calls. Retained identities require
 * caller lifetime. Final remove may free state; no access follows it.
 * Preflight certifies all arithmetic finite and within float range, normal
 * nearest FP with FTZ/DAZ off. Constructor magnitude<=1e6, update live fields
 * <=1e9. Callback-visible FP status and original x87 extended precision are
 * not represented; math follows existing double-backed full-body reference.
 * Constructor requires only preflight/base/parent/group; update requires all hooks.
 * Return0 success,1 rejected before effects; constructor2 means base failure with
 * possible committed effects and no suffix/RNG/group execution. Update2 means removed, never
 * retry/use state unless its real owner establishes a valid new lifetime.
 * Only the checked base may fail after admission; there is no fabricated rollback. */
int uc_shell_construct(UCShellState *,const UCShellProperties *,UCGameRandom *,const UCShellServices *);
int uc_shell_update(UCShellState *,const UCShellServices *);
#ifdef __cplusplus
}
#endif
#endif
