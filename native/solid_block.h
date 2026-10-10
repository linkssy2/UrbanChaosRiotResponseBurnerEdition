#ifndef UC_SOLID_BLOCK_H
#define UC_SOLID_BLOCK_H
#include <stdint.h>
#include "collision_triangle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual111200 box/rigid-body descriptor geometry. Physics coordinates are
 * metres with source render Y/Z conversion. half is the core half extent;
 * radius is separate. Original302330 rays use the sharp expanded half+radius
 * box, while3021E0 bounds add tolerance+radius. No asset/model-derived box. */
typedef struct UCSolidBlockGeometry {float half[3],center[3],rotation[4],radius;} UCSolidBlockGeometry;
/* Finite affine matrix (basis W=0,translation W=1), magnitude<=1e6, nonzero
 * first/second basis and nonparallel axes. RN/gradual-underflow; exact host
 * RSQRTSS refinement and source double-backed x87 quaternion conversion.
 * Output disjoint, unchanged on rejection.0 complete,1 unsupported/invalid.
 * radius is the live505AC0 input to302130, bounded[0,1e4]; source initial .05.
 * Body allocator/material, world membership and solver excluded. */
int uc_solid_block_geometry(const float transform[16],float radius,UCSolidBlockGeometry *);
/*302330 local sharp box ray: half+radius, starts strictly inside miss, source
 * axis/sign tie order and inclusive fraction limit. No world transform/filter.
 * Returns1 hit,0 miss,-1 invalid, preserving output on miss/error. RN domain. */
int uc_solid_block_cast_local(const float half[3],float radius,const float from[3],
    const float to[3],float limit,UCCollisionTriangleHit *);
/* PC world-space adapter, physics metres. Same normalized near-unit rotation
 * as capsule contacts; original local leaf above. No world filters implied. */
int uc_solid_block_cast(const UCSolidBlockGeometry *,const float from[3],
    const float to[3],float limit,UCCollisionTriangleHit *);
/* Embed once beside the SAME FreshView. D0 is the actual native body identity;
 * D4 is source state. Enclosing allocation and generic actor lifetime belong
 * to the caller; no second entity flags/transform/ID owner. */
typedef struct UCSolidBlock {void *body;int32_t state;} UCSolidBlock;
typedef struct UCSolidBlockServices {
 void *owner;
 /* Completing111200: real shape/body creation, flag10000000 publication and
  * initial world ADD. body must be published at its actual source frontier. */
 int (*prepare)(void *,UCSolidBlock *);
 int (*add)(void *,void *body);    /*2ED570, activation1*/
 int (*remove)(void *,void *body); /*2ED710*/
 int (*actor_update)(void *);     /*actual218C40*/
} UCSolidBlockServices;
/* Actual111180 suffix after required218A60. Caller already allocated this
 * state beside its actor. No fake allocation/factory success is supplied. */
int uc_solid_block_construct(UCSolidBlock *,void *owner,int (*base)(void *));
/* Whole111610 control:0 prepare->1;1 with flag10000000 clear REMOVE->2;
 *2 with flag set ADD->1; every completed path then218C40. Callbacks may change
 * current fields but must retain this state, body and flag pointer lifetime.
 * Bindings stable/no reentry.0 completed,1 rejected beforeeffects,2 committed
 * completing-service failure; state assignment follows successful service.
 * This controller does not pretend missing Havok/mission services complete. */
int uc_solid_block_update(UCSolidBlock *,const uint32_t *flags,const UCSolidBlockServices *);
#ifdef __cplusplus
}
#endif
#endif
