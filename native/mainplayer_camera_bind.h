#ifndef UC_MAINPLAYER_CAMERA_BIND_H
#define UC_MAINPLAYER_CAMERA_BIND_H
#include "entity_events.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact Mainplayer17283A..1728A8 and alternate-playerD674B..D67B8
 * after Camera factory/20A260 returned its ID. Publish the
 * SAME player+308 slot, then source-tag0 and activation through the existing
 * synchronous entity graph. Each event reads current world registry/event IDs;
 * the second reloads player+54 and camera+308 after the first receiver returns.
 * Missing/stale IDs retain the original successful-unhandled behavior.
 * Borrowed fields are disjoint and live; current_registry is a pure view.
 * Receiver effects complete and retain that graph/these field lifetimes. No
 * factory, preceding player startup, scheduler or Camera deletion is implied.
 *0 complete,1 invalid before effects,2 committed failure: do not retry. */
int uc_mainplayer_camera_bind(int32_t returned_id,int32_t *camera_id,
    const int32_t *player_id,const uint32_t *source_event,const uint32_t *activate_event,
    void *owner,const UCEntityEventRegistry *(*current_registry)(void *));
#ifdef __cplusplus
}
#endif
#endif
