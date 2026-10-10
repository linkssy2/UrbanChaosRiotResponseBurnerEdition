#ifndef UC_SCENE_VIEW_PUBLISH_H
#define UC_SCENE_VIEW_PUBLISH_H
#include "scene_view.h"
#include "scene_matrix_provider.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact9E77A..9E880 publication block only: capture current graphics+50 once,
 * publish identity material, camera world, inverse, projection, alternate, then
 * flush. View/provider storage is stable/disjoint; all required services really
 * complete their corresponding owners. Getter may observe/mutate provider
 * before the five setters; matrix flush has its existing completing contract.
 * Caller must admit finite supported view matrices and service completion.
 *0 complete,1 initial rejection,2 committed failure (discard partial frame;
 * no retry). No render-target/state/grid/collection/palette/frame scheduling. */
int uc_scene_view_publish(const UCSceneView *,
 UCSceneMatrixProvider *(*current_provider)(void *),void *,const UCSceneMatrixServices *);
#ifdef __cplusplus
}
#endif
#endif
