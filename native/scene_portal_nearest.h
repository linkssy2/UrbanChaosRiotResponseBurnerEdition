#ifndef UC_SCENE_PORTAL_NEAREST_H
#define UC_SCENE_PORTAL_NEAREST_H
#include "scene_portal_graph.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full A9C40 (+8 in457928), using the SAME owned B69C0 graph and group marks.
 * Select the group with the nearest vertex among the first encountered link
 * of each group, in room/link/vertex order; equality retains the first. This
 * is not nearest room, containing room, polygon distance or visibility.
 * Marks are cleared before/after the active traversal. Other fields, including
 * portal enabled flags, are unchanged. Inactive returns UINT32_MAX without
 * touching graph storage or reading point. Active empty also returns that ID.
 * Live immutable graph from graph_begin, group IDs strictly below portal_count,
 * finite |point/vertices|<=1e15, RN and normal denormals. No aliases with output.
 * Status1 rejects before marks/output changes;0 completes. */
int uc_scene_portal_nearest(UCScenePortalGraph *,const float point[3],uint32_t *group);
#ifdef __cplusplus
}
#endif
#endif
