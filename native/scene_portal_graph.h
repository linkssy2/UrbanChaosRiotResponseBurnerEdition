#ifndef UC_SCENE_PORTAL_GRAPH_H
#define UC_SCENE_PORTAL_GRAPH_H
#include "scene_portal_data.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneRoomInput {
    float bounds[6];
    uint32_t count;
    const uint32_t *portal_indices;
    const float (*normals)[3];
} UCSceneRoomInput;
typedef struct UCScenePortalInput {
    UCScenePortalData data;
    uint32_t neighbor_count;
    const uint32_t *neighbors;
} UCScenePortalInput;
typedef struct UCScenePortalRoom UCScenePortalRoom;
typedef struct UCScenePortalLink {
    uint32_t group,neighbor_count;
    UCScenePortalRoom **neighbors;
    uint32_t vertex_count;
    float normal[3];
    float (*vertices)[3];
} UCScenePortalLink;
struct UCScenePortalRoom {
    uint32_t count;
    float minimum[3],maximum[3],center[3],half[3];
    UCScenePortalLink *links;
    uint8_t flags[2];
};
typedef struct UCScenePortalGraph {
    UCScenePortalRoom **rooms;
    uint32_t portal_count,room_count;
    uint8_t *seen,*enabled;
    UCScenePortalMemory memory;
    uint8_t active;
} UCScenePortalGraph;
/* Complete B69C0 graph construction and B6F30 graph release order, with its
 * unrelated two lists and scratch buffer empty. End therefore includes the
 * actual 17C5E0 NULL scratch release after releasing the graph. It does not own
 * the enclosing virtual object, full AA040/B70C0 loader, or view visibility.
 * Begin needs zero/inactive output. The graph owns all returned heap storage;
 * inputs are borrowed only during begin. Source temporary room bounds really
 * are modified in vertex/link order. Neighbor pointers preserve duplicates,
 * omit self and nonnegative out-of-range indices, and reject negative indices.
 * Direction uses the source float dot-product order; dot>0 reverses vertices.
 * Every count <=4096; total room links <=65536. Finite coordinates/normals and
 * bounds have magnitude <=1e15, RN/normal denormals. These bounds avoid source
 * overflow while admitting the actual GEO corpus. Input/output storage cannot
 * alias. All pointers/counts remain valid and immutable during allocation.
 * Allocator callbacks may affect external state but cannot read/mutate/reenter
 * this partially constructed graph. Positive and zero-size requests both need
 * distinct non-NULL storage. Release accepts NULL and can actually free memory.
 * Sizes are native host sizes, not the original 32-bit allocation byte counts.
 * During end, borrowed graph readers must be quiescent; links become dangling
 * in source order before the final native reset. Status0 complete,1 pre-effect
 * invalid,2 native allocation failure: partial owned allocations are released,
 * output stays zero, but room bounds may already have changed. This rollback
 * is a host safety policy, not a claim about original allocation failure. */
int uc_scene_portal_graph_begin(UCScenePortalGraph *,UCSceneRoomInput *,uint32_t,
                              const UCScenePortalInput *,uint32_t,const UCScenePortalMemory *);
int uc_scene_portal_graph_end(UCScenePortalGraph *);
#ifdef __cplusplus
}
#endif
#endif
