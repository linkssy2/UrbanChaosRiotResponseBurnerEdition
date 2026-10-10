#ifndef UC_NATIVE_COLLISION_BOUNDARY_H
#define UC_NATIVE_COLLISION_BOUNDARY_H
#include "collision_asset.h"

typedef struct UCCollisionBoundary {
    const void *world;
    void *phantoms[6];
    uint32_t mode;
    uint16_t references, managed;
} UCCollisionBoundary;

typedef struct UCCollisionBoundaryHooks {
    void (*observer)(void *context, const void *world, UCCollisionBoundary *owner, int add);
    void *(*create)(void *context, const float lower[4], const float upper[4], uint32_t filter);
    void (*listener)(void *context, void *phantom, UCCollisionBoundary *owner, int add);
    void (*add)(void *context, const void *world, void *phantom);
    void (*remove_six)(void *context, const void *world, void *const phantoms[6]);
    void (*release)(void *context, void *phantom);
} UCCollisionBoundaryHooks;

/* 2FD660 / explicit 2FD570 detach, with the caller's external owner reference
 * held throughout. Zero-initialize owner before init. Its address and callbacks
 * remain stable; callbacks must not reenter or mutate owner/bounds. Hooks are
 * required, synchronous, and non-failing after caller resource preflight.
 * create supplies a fresh type2 AABB phantom with one owned reference; add and
 * remove_six perform real registration/world reference work; release drops one
 * owned reference. They are dependencies, not emulated successful operations.
 * observer manages its registry only: this module owns the source owner ref.
 * Only already published phantom slots may be inspected from callbacks.
 * Geometry order is +X,-X,+Y,-Y,+Z,-Z; zero thickness, filter0, W=positive zero.
 * Bounds XYZ must be finite/ordered; source W is ignored. Native errors precede
 * callbacks and preserve state. Boundaries/outputs must not alias owner.
 * No phantom pending opcode13, overlap response, world-destruction callback,
 * allocator/destructor, or broadphase implementation is supplied here. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_boundary_init(UCCollisionBoundary *owner,
    const void *world, const float lower[4], const float upper[4],
    uint32_t mode, unsigned managed, const UCCollisionBoundaryHooks *hooks, void *context);
UCCollisionStatus uc_collision_boundary_detach(UCCollisionBoundary *owner,
    const UCCollisionBoundaryHooks *hooks, void *context);
/* 2FD480's phantom-release loop only, after external world/observer detachment.
 * Preserves mode/reference fields, releases nonnull slots in order. */
UCCollisionStatus uc_collision_boundary_release_phantoms(UCCollisionBoundary *owner,
    const UCCollisionBoundaryHooks *hooks, void *context);
#ifdef __cplusplus
}
#endif
#endif
