#ifndef UC_MAINPLAYER_CAMERA_PROPERTIES_H
#define UC_MAINPLAYER_CAMERA_PROPERTIES_H
#include "entity_properties.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Player1726F4..172819 (also D6605..D672A), then20A260's1F8E30(mode1).
 * Same seven typed properties and source-index-1. FOV is the live4F8A9C value
 * copied by the caller; all payload bits, including signed zero, are retained.
 * Uses the existing property owner. Source-unwritten scratch/table fields keep
 * caller provenance and are not promised equal to an original stack allocation.
 * Table/scratch are distinct, initialized, exclusive owners. No class factory,
 * active descriptor publication, Camera constructor or event sends occur here.
 *0 complete,1 invalid before effects. Camera admission happens at construction. */
int uc_mainplayer_camera_properties(UCEntityProperties *,UCEntityProperty *,float fov);
#ifdef __cplusplus
}
#endif
#endif
