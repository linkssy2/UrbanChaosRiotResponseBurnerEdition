#ifndef UC_SCENE_PORTAL_START_H
#define UC_SCENE_PORTAL_START_H
#include "scene_portal_graph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCScenePortalStart {
    float position[3];
    UCScenePortalRoom **rooms;
    uint32_t count,capacity;
    uint8_t initialized;
} UCScenePortalStart;
/* Actual AB2E0 FIRST-camera domain: B70C0's prior room list is empty. Publish
 * camera matrix translation then select all inclusively containing padded room
 * bounds, preserving graph index order; if none, use room0. Source temporary
 * and persistent vectors become caller-reserved native pointer storage. Their
 * allocation callbacks/capacity growth are not reproduced or exposed here.
 * Output supplies disjoint pointer storage with capacity>=graph.room_count;
 * count/initialized initially0. Position initially source0. Storage and borrowed
 * room identities stay live through the later visibility traversal. No owned
 * graph fields change. A subsequent camera move is explicitly unsupported:
 * AB2E0's cached movement/A8FE0 path is a separate owner, not this routine.
 * Active actual graph,1..4096 rooms, finite translation and room min/max values.
 * Matrix is source row-major with translation at12..14; other entries are not
 * read. Status1 rejects before effects;0 completes. No allocator or draw hook.
 * This does not perform AB1E0 recursive visibility or A9310 sector queries. */
int uc_scene_portal_start(const UCScenePortalGraph *,const float matrix[16],UCScenePortalStart *);
#ifdef __cplusplus
}
#endif
#endif
