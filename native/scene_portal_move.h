#ifndef UC_SCENE_PORTAL_MOVE_H
#define UC_SCENE_PORTAL_MOVE_H
#include "scene_portal_start.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full AB2E0 including A8FE0/A8960/A8FA0 cached motion. SAME graph and start
 * membership; ambiguity borrows the engine's shared51B420 byte, not a second
 * per-camera flag. Graph, room identities, enabled groups and storage remain
 * stable throughout this synchronous call. No callbacks or allocation; original
 * temporary/persistent vectors use the existing caller-reserved room storage.
 * Fresh selection, strict100-unit movement gate, portal edge proximity and
 * source first-neighbor/order semantics are retained. The graph must be a valid
 * constructed UCScenePortalGraph; all room/neighbor pointers belong to it.
 * Guards are admission checks, not a general corrupted-graph validator.
 * A full-search miss keeps
 * prior rooms; only a truly empty prior list falls back to room0.
 * Finite coordinates/normals bounded1e6, nondegenerate portal edges, RN with
 * normal denormals; source graph links/neighbor identities are immutable.
 * Status0 complete,1 admission failure before any output writes. This alone
 * does not update visibility flags, construct the graph or render a frame. */
int uc_scene_portal_move(const UCScenePortalGraph *,const float matrix[16],
                         UCScenePortalStart *,uint8_t *ambiguity);
#ifdef __cplusplus
}
#endif
#endif
