#ifndef UC_NATIVE_COLLISION_ASSET_QUERY_H
#define UC_NATIVE_COLLISION_ASSET_QUERY_H
#include "collision_asset.h"
#include "solid_block.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Original 2A2910 / 2A2810: render X/Y/Z centimetres <-> physics X/Z/Y
 * metres. Render Y and physics Z are up. These also support in-place use. */
void uc_collision_to_physics(const float render[3], float physics[3]);
void uc_collision_to_render(const float physics[3], float render[3]);

typedef struct UCCollisionRayHit {
    float fraction, position[3], normal[3];
    uint32_t triangle; /* Index into the asset's exact ordered triangle array. */
} UCCollisionRayHit;
/* Closest double-sided geometric segment hit, including endpoints. Coordinates
 * are physics metres; asset scaling is applied. Normal opposes the segment.
 * This queries the triangle surface, without mesh radius or welding. Equal
 * fractions retain source order. Returns 1 hit, 0 miss, -1 invalid arguments.
 * Empty/degenerate triangles miss. No MOPP, dynamic bodies or game filters. */
int uc_collision_raycast(const UCCollisionAsset *asset, const float from[3],
                        const float to[3], UCCollisionRayHit *hit);

typedef struct UCCollisionContact {
    float axis_point[3], surface_point[3], normal[3], penetration;
    uint32_t triangle;
} UCCollisionContact;
/* Geometric capsule-versus-triangle contacts in original triangle order.
 * Radius is the capsule radius; the authored mesh radius is added explicitly.
 * Writes at most capacity contacts and returns the full count (SIZE_MAX for
 * invalid arguments). At zero distance the winding normal is oriented toward
 * the capsule centre. Degenerate triangles are omitted. No contact welding,
 * friction, slope support, collision filtering, sweep or movement solver is
 * implied by this primitive. Caller must handle overflow before resolving. */
size_t uc_collision_capsule_contacts(const UCCollisionAsset *asset,
    const float end_a[3], const float end_b[3], float radius,
    UCCollisionContact *contacts, size_t capacity);

/* PC capsule query against an actual authored solid-block shape. Quaternion
 * is normalized only for this geometric query. A spine crossing the box uses
 * the shortest separating box-face translation, not a Havok manifold. Returns
 *1 contact,0 miss,-1 invalid; output unchanged on miss/error. triangle=UINT32_MAX
 * identifies a block, never an index into the scenery asset. */
int uc_collision_block_contact(const UCSolidBlockGeometry *block,
    const float end_a[3],const float end_b[3],float radius,UCCollisionContact *);

/* Original D2860/301140 authored player capsule dimensions, physics metres.
 * Body origin is the bottom of either capsule. These are shape parameters,
 * not a replacement for the original player controller/state machine. */
#define UC_PLAYER_CAPSULE_RADIUS 0.45f
#define UC_PLAYER_CAPSULE_BOTTOM 0.45f
#define UC_PLAYER_CAPSULE_STANDING_TOP 1.4f
#define UC_PLAYER_CAPSULE_CROUCH_TOP 0.8f

#ifdef __cplusplus
}
#endif
#endif
