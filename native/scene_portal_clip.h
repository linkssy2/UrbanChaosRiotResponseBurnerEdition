#ifndef UC_SCENE_PORTAL_CLIP_H
#define UC_SCENE_PORTAL_CLIP_H
#include "scene_portal_graph.h"
#include "scene_portal_planes.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCScenePortalClipBlock { uint32_t count; UCScenePortalPlane plane[64]; } UCScenePortalClipBlock;
typedef struct UCScenePortalClipArena {
    unsigned char *data;
    uint32_t capacity,used;
    UCScenePortalMemory memory;
} UCScenePortalClipArena;
/* Actual A88C0 facing and A85F0 near-plane tests: bit0 is strict facing>0,
 * bit1 is absolute double dot<100. The first vertex is required. */
int uc_scene_portal_classify(const UCScenePortalLink *,const float point[3],uint8_t *);
/* Whole AA240, including A8630 test-only planes and AA0A0 polygon clipping.
 * Original 60-byte plane projection is reused. Up to32 input planes, at most
 * two nonzero-kind planes (AA240 stack28/2C), and3..32 initial vertices;
 * clipping must fit64 vertices. Finite camera/vertices<=1e6 and plane
 * normals/distances<=1e9 in magnitude, including retained scratch holes;
 * RN/normal denormals. Borrowed graph/planes,
 * arena, output and allocator storage are disjoint. No mutation/reentry through
 * allocator callbacks. Arena data is four-byte aligned; used/capacity multiples
 * of4. If scratch does not fit, allocate exactly4+60*max(6,vertices+kept) bytes.
 * Allocate must return suitably aligned storage and preserve input lifetimes;
 * source allocation failure is excluded. Status2 is native failed allocation,
 * before arena/output changes. Status1 is invalid/capacity admission, also
 * before effects. Status0 publishes NULL for a clipped-away portal, otherwise
 * a block with source-defined fields only; unwritten fields retain storage bits.
 * Identical polygon edges are skipped WITHOUT compacting physical plane slots,
 * matching the source. Caller owns heap fallback release and scratch rewind;
 * this leaf does not perform the outer AA5E0 lifetime or graph traversal. */
int uc_scene_portal_clip(const UCScenePortalLink *,const float point[3],
                         uint32_t plane_count,const UCScenePortalPlane *,
                         UCScenePortalClipArena *,UCScenePortalClipBlock **);
#ifdef __cplusplus
}
#endif
#endif
