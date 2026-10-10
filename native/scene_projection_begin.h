#ifndef UC_SCENE_PROJECTION_BEGIN_H
#define UC_SCENE_PROJECTION_BEGIN_H
#include "scene_view.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Whole21E1E0 concrete478D20 represented initialization. Retail immutable
 * constants25/50000, depths0/1 and flags1/0 publish before required current
 * graphics+58 lookup; returned borrowed viewport and alternate0 publish after.
 * FOV, alternate FOV, cache and parameter bytes2/3 are untouched. Caller owns
 * their actual upstream initialization before use; no Camera is fabricated.
 * The completing callback may mutate projection fields at this frontier but
 * cannot free/reenter/replace its storage. A NULL returned viewport is real.
 *0 complete;1 missing input rejection before effects. No allocation/retains. */
int uc_scene_projection_begin(UCSceneProjection *,void *(*current_viewport)(void *),void *);
#ifdef __cplusplus
}
#endif
#endif
