#ifndef UC_ENTITY_FRESH_H
#define UC_ENTITY_FRESH_H
#include "entity_registration.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCEntityFreshAux UCEntityFreshAux;
typedef struct UCEntityFreshMemory {
    void *owner;
    void *(*allocate)(void *,size_t);
    void (*release)(void *,void *);
} UCEntityFreshMemory;

/* Actual218550/218A60 actor fields; embed once in the owning entity. Fields
 * absent here are not initialized or claimed by this bounded constructor. */
typedef struct UCEntityFreshActor {
    float distance88,distance8C;
    uint32_t word90;
    uint16_t group94;
    uint32_t word98;
    float health9C,maximumA0,valueA4;
    int32_t idB0,idB4; /*script priority B0; selected conditional-record index B4 */
    uint32_t script_indexB8; /*source-unwritten by218A60; preserve existing bytes */
    void *handleBC,*audioC0; /*C0 is the current typed script command */
    const uint8_t *wordC4; /*actual script cursor; constructor publishesNULL */
    void *scriptC8; /*script coroutine */
    uint8_t flagCC,flagCD,flagCE;
} UCEntityFreshActor;

/* Borrow actual storage, including the already owned actor/shell transform and
 * ID when composed. No copied identity map, duplicate matrix or flags owner. */
typedef struct UCEntityFreshView {
    UCEntityRegistrationTarget target;
    float *transform;                    /*16 floats, entity+10 */
    int32_t *source_index,*id;           /*50,54 */
    uint32_t *flags60;
    const void **definition70;
    void **model74,**helper78;
    UCEntityFreshAux **aux84;
    UCEntityFreshActor *actor;
} UCEntityFreshView;

typedef struct UCEntityFreshContext {
    const UCEntityRegistration *registration;
    uint32_t *group_counter;             /*the actual world+878 */
    const void *const *definition;       /*live active definition+18 */
    void *owner;
    /* Pure view of the given target from registration's SAME registry. */
    const UCEntityFreshView *(*resolve)(void *,UCEntityEventTarget *);
    UCEntityFreshMemory memory;
} UCEntityFreshContext;

/* Owned empty22A670/22A640 helper and sentinel, represented with native pointers.
 * No generic node insertion is admitted. Source-unwritten header/payload bytes
 * are not modeled as zero. Allocator really allocates/releases and may observe
 * fields; its callbacks must complete, preserve helper ownership and not reenter.
 * destroy preserves the enclosing entity's aux84 slot (source leaves it dangling).
 * clear is the actual empty2012C0 publications, not an omitted cleanup callback. */
UCEntityFreshAux *uc_entity_fresh_aux_create(const UCEntityFreshMemory *);
int uc_entity_fresh_aux_empty(const UCEntityFreshAux *);
int uc_entity_fresh_aux_clear(UCEntityFreshAux *);
int uc_entity_fresh_aux_destroy(UCEntityFreshAux *);

/* Whole20BDC0 ->218550 ->218A60, dynamic source-contextNULL/mode2 LightFlash
 * recipe only. Existing registration invokes its real record insertion service.
 * Borrowed owners/field slots remain stable and disjoint; aux84 starts NULL.
 * Registration/allocator effects may change live parent fields/parent ID and
 * active definition identity. They must leave a valid same-registry parent for
 * placement and retain it through each pure resolver call; allocation succeeds
 * in the original-qualified domain. Current parent is resolved AGAIN after both
 * helper allocations for group94 inheritance. Finite placement/RN requirements
 * are those of shot_lightflash_place. Required services never stand for noops.
 * Return0 completed,1 invalid before effects,2 committed native failure. Failure2
 * cannot be retried/rolled back; actual registration and any published aux remain
 * owned by the caller. Guest class/vtable words are not native representation.
 * No factory class registry, coroutine, effect-specific175D60 or main claim. */
int uc_entity_fresh_construct(const UCEntityFreshContext *,const UCEntityFreshView *);
/* Same exact mode2/218A60 construction using descriptor position with zero
 * Euler angles. Reads the borrowed position at the original placement boundary. */
int uc_entity_fresh_construct_lightflash(const UCEntityFreshContext *,const UCEntityFreshView *,const float position[3]);
/* Same base/actor construction, actual empty159C0 descriptor mode0. The default
 * 20A850 branch publishes identity basis, flags400 and source_index-1, retaining
 * current translation (initially zero; registration may have changed it). No
 * placement parent is required; the later live group inheritance is unchanged.
 * All other ownership, callback and failure contracts above apply. */
int uc_entity_fresh_construct_unplaced(const UCEntityFreshContext *,const UCEntityFreshView *);
/*218550 health base with actual dynamic descriptor mode5 (20B83F). Copies
 * all16 descriptor floats bit-for-bit after registration, before aux and group
 * inheritance. No scripted actor tail. Matrix stays alive and disjoint through
 * callbacks; caller admits its downstream numeric domain before construction. */
int uc_entity_health_construct_matrix(const UCEntityFreshContext *,const UCEntityFreshView *,const float matrix[16]);
/* Same mode5 placement, including the218A60 scripted actor tail. */
int uc_entity_fresh_construct_matrix(const UCEntityFreshContext *,const UCEntityFreshView *,const float matrix[16]);
/* Actual mode3 absolute position/degree Euler descriptor. Borrowed six floats
 * are read after registration, before the original auxiliary allocations. */
int uc_entity_fresh_construct_pose(const UCEntityFreshContext *,const UCEntityFreshView *,const float position_angles[6]);
/* Same dynamic base/actor construction with actual descriptor mode1. After
 * registration, resolve the CURRENT parent from the same registry and copy all
 *16 transform floats bit-for-bit (20AD8B), then publish flags400/source_index-1.
 * This branch requires a valid parent through that copy; no virtual getter,
 * position adjustment or fabricated identity fallback occurs. The later group
 * inheritance resolves the parent again after both auxiliary allocations.
 * Missing/invalid parent after registration is committed failure2. All existing
 * callback/storage contracts apply. This does not construct a Camera suffix. */
int uc_entity_fresh_construct_parent(const UCEntityFreshContext *,const UCEntityFreshView *);
/* Only20BDC0, dynamic mode1: the GunFlare base call does not run218550/218A60.
 * source_context borrows the actual active descriptor+10 slot, required NULL
 * initially and through registration. It is re-read after source-timed ID and
 * listener publication; an unexpected nonNULL then returns committed failure2.
 * The active definition slot and mode1 parent remain under the same completing
 * callback contracts above. This entry never reads/writes group_counter, memory,
 * aux84 or actor; all may be NULL and existing contents stay untouched. Mode1
 * copies bits and needs no floating-point arithmetic-mode restriction. Other
 * base fields/services remain required. Registration and placement alone are
 * qualified here, not187680 GunFlare fields/listeners/model initialization. */
int uc_entity_base_construct_parent(const UCEntityFreshContext *,const UCEntityFreshView *,
    const void *const *source_context);
/* Same base-only20BDC0, dynamic empty159C0 descriptor mode0. Preserves the
 * registered translation and does not allocate or initialize actor fields. */
int uc_entity_base_construct_unplaced(const UCEntityFreshContext *,const UCEntityFreshView *);
/* Base-only20BDC0, mode3 absolute position with positive-zero Euler angles.
 * Borrowed position is read after registration; finite RN coordinates<=1e6.
 * No parent placement or actor suffix. Same completing callback contracts. */
int uc_entity_base_construct_position(const UCEntityFreshContext *,const UCEntityFreshView *,const float position[3]);
/* Actual C4790 mode4 Shell recipe: positive-zero position/Euler, tag4/5.
 * tag_world is the required parent virtual+10 effect on the SAME registry target.
 * It returns0 after writing16 finite floats of magnitude<=1e6; it may observe
 * constructor fields but must preserve borrowed graph/fields and arithmetic mode.
 * Matrix composition preserves original pairwise operations and signed zeros.
 * Placement precedes aux allocations and the later parent/group lookup. A failed
 * getter/admission after registration returns2 without rollback. This constructs
 * only the shared base; actual Shell suffix/model/group/lifetime remain separate. */
typedef int (*UCEntityFreshTagWorld)(void *,const UCEntityEventTarget *,uint32_t,float[16]);
int uc_entity_fresh_construct_shell(const UCEntityFreshContext *,const UCEntityFreshView *,
    uint32_t tag,void *owner,UCEntityFreshTagWorld);
#ifdef __cplusplus
}
#endif
#endif
