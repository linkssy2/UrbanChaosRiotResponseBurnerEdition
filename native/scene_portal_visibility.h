#ifndef UC_SCENE_PORTAL_VISIBILITY_H
#define UC_SCENE_PORTAL_VISIBILITY_H
#include "scene_portal_clip.h"
#include "scene_portal_start.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCScenePortalVisibility {
    UCScenePortalRoom **rooms;
    uint32_t count,capacity;
    UCScenePortalClipArena scratch;
    void *cursor;
    uint8_t failed;
} UCScenePortalVisibility;
/* AB1E0 -> AB160/AA5E0, using actual plane and clipping owners. Borrowed graph,
 * first-camera list/position and three original matrix arguments must remain
 * stable. The visible list uses caller-reserved capacity>=room_count, replacing
 * only the source STL allocation mechanics; identity order/flags are retained.
 * Graph<=256 rooms/groups and<=4096 links, <=32 vertices/link. Scratch retains
 * its bytes between views, as the source does. Heap clipping fallback uses the
 * required allocator and actual release; a freed external cursor is retained
 * as an opaque value, never dereferenced, until another source rewind/reset.
 * Admitted original heap addresses are above scratch end; native pointer order
 * does not decide subsequent fallback. All allocator callbacks are completing,
 * immutable with respect to this graph/owner and may not reenter. No allocation
 * failure equivalence claim. Budget bounds native traversal only; every entered
 * room consumes one unit. Status0 complete,1 pre-effect invalid,2 incomplete
 * after effects (budget/clipping/allocation failure): outstanding clipping
 * blocks are released and recursion flags unwound, but published state remains
 * partial and failed=1 prevents reuse. Destruction may release caller storage
 * after readers quiesce. The routine does not own cached camera motion,
 * AB7F0's freeze gate, full AA040 loading, source frame scheduling or rendering. */
int uc_scene_portal_visibility(UCScenePortalGraph *,const UCScenePortalStart *,
                              UCScenePortalPlanes *,const float argument0[16],
                              const float argument1[16],const float argument2[16],
                              UCScenePortalVisibility *,uint32_t budget);
#ifdef __cplusplus
}
#endif
#endif
