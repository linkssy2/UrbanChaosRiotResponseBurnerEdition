#ifndef UC_SCENE_PORTAL_LOAD_H
#define UC_SCENE_PORTAL_LOAD_H
#include "scene_portal_graph.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Decode the B70C0 tail starting at UCScene.geometry_end, then reuse the SAME
 * normal/group/graph owners. Active only with >1 rooms and >0 portals. Exact
 * successful-stream graph fields and consumed bytes are preserved, including
 * ignored room records, normal subtraction, directed grouping and link order.
 * This is graph loading, not the complete enclosing object's constructor:
 * planes,10000-byte visibility scratch and start/visible vectors remain owned
 * by its caller. Temporary decode allocations are native malloc/free and are
 * not exposed as original allocation callbacks. Caller graph allocator cannot
 * inspect/reenter the temporary decode state. Input stays immutable throughout.
 * Zero output graph required. Finite coordinates/normals<=1e6 and raw room
 * bounds<=1e15 (authored empty bounds use +/-1e12); <=256 rooms/portals,
 * <=4096 total room links, <=256 neighbors, <=16MiB complete tail. Negative
 * indices and degenerate portal normals are rejected. Out-of-range positive
 * neighbors retain existing graph semantics. Source truncated reads/OOM are
 * excluded; native failure frees its allocations and leaves outputs unchanged.
 *0 complete,1 invalid/unsupported tail,2 allocation failure. Active0 leaves
 * graph zero and needs no graph_end. No visibility/render/FPS claim. */
int uc_scene_portal_load(UCScenePortalGraph *,const void *tail,size_t size,
                        const UCScenePortalMemory *,size_t *consumed,uint8_t *active);
#ifdef __cplusplus
}
#endif
#endif
