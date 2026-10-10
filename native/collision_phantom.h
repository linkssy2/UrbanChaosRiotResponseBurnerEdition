#ifndef UC_NATIVE_COLLISION_PHANTOM_H
#define UC_NATIVE_COLLISION_PHANTOM_H
#include "collision_sap.h"
typedef struct UCCollisionPhantom UCCollisionPhantom;
typedef struct UCCollisionPhantomCollidable {
    const void *identity;
    UCCollisionPhantom *owner;
    const void *shape;
    uint32_t type, shape_key, filter;
} UCCollisionPhantomCollidable;
typedef struct UCCollisionPhantomMember {
    const void *world;
    UCCollisionSAPHandle broadphase;
} UCCollisionPhantomMember;
typedef struct UCCollisionPhantomView {
    float lower[4], upper[4];
    UCCollisionPhantomCollidable collidable;
    const void *const *listeners;
    uint32_t listener_count, listener_capacity;
} UCCollisionPhantomView;

/* Fresh300750/2F5CA0 type2 null-shape AABB owner. One managed reference;
 * copies all float4 bits, finite ordered XYZ required, W ignored by admission.
 * Allocates owner and0..65536 listener slots once. *out must beNULL; failures
 * preserve it. Storage must not alias input/output arguments. Collidable and
 * owner identities are distinct, stable and valid until final release.
 * Source prepared transform staysNULL: this exact class's virtual getter
 *2C2B60 returnsNULL; no identity matrix or new transform owner is implied.
 * Property, lifecycle-observer and accepted-overlap arrays are initially empty
 * and have no mutator in this slice. Their effects are NOT silently implemented.
 *
 * Listener identities are BORROWED (includingNULL), never retained/released.
 * Append retains duplicates. Remove shifts after the FIRST matching slot;
 * missing entries reject unchanged instead of original's out-of-range access.
 * Views are readonly; count/storage may change at add/remove. No concurrent use.
 * The member is writable ONLY by a real world/SAP owner: publication itself
 * performs no registration, notification, ref change or overlap operation.
 * Its native DETACHED sentinel is additional lifetime state (source starts0).
 *
 * Retain admits1..65534 refs; overflow rejects before source16-bit wrap.
 * Release must have a live reference; after the final release pointer/view/
 * collidable become invalid. Final release requires worldNULL and detached
 * broadphase. Borrowed overlap listeners may remain: original frees their
 * storage without invoking them. This empty-other-arrays destructor scope
 * cannot stand in for a phantom with lifecycle observers or accepted overlaps. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_phantom_create(const float lower[4], const float upper[4],
    uint32_t filter, uint32_t listener_capacity, UCCollisionPhantom **out);
UCCollisionStatus uc_collision_phantom_retain(UCCollisionPhantom *phantom);
UCCollisionStatus uc_collision_phantom_release(UCCollisionPhantom *phantom);
UCCollisionStatus uc_collision_phantom_add_listener(UCCollisionPhantom *, const void *listener);
UCCollisionStatus uc_collision_phantom_remove_listener(UCCollisionPhantom *, const void *listener);
const UCCollisionPhantomView *uc_collision_phantom_view(const UCCollisionPhantom *);
UCCollisionPhantomMember *uc_collision_phantom_member(UCCollisionPhantom *);
uint16_t uc_collision_phantom_references(const UCCollisionPhantom *);
#ifdef __cplusplus
}
#endif
#endif
