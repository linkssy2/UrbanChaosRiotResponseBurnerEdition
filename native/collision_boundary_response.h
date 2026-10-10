#ifndef UC_NATIVE_COLLISION_BOUNDARY_RESPONSE_H
#define UC_NATIVE_COLLISION_BOUNDARY_RESPONSE_H
#include "collision_boundary.h"
typedef struct UCCollisionBoundaryResponseHooks {
    void (*motion)(void *context,const void *body,uint32_t kind,uint32_t argument1,uint32_t argument2);
    void (*remove)(void *context,const void *world,const void *body);
} UCCollisionBoundaryResponseHooks;
/* Complete2FD630 ->2FD4F0 routing for the installed boundary listener.
 * added=0 is the installed return-only removal handler. A class1 collidable
 * with nonnull resolved owner requests motion(7,1,0) in modes0/1, world removal
 * in mode2, and nothing for other modes. Other classes/null owners do nothing.
 * No acceptance-status field is written. Type is the original unsigned byte.
 * Boundary/body/context remain alive and stable throughout. Required callbacks
 * are synchronous, non-failing actual services after caller preflight. They
 * own world/motion/pending effects; this helper does not implement motion
 * conversion, enqueue opcode4, world removal, references or overlap storage.
 * Boundary world is needed only for an active mode2 removal. Entry errors
 * invoke no callbacks. No rollback of callback effects is implied. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_boundary_response(const UCCollisionBoundary *,int added,
    uint32_t collidable_type,const void *rigid_owner,
    const UCCollisionBoundaryResponseHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
