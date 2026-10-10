#ifndef UC_PLAYER_CUTSCENE_H
#define UC_PLAYER_CUTSCENE_H
#include "player_equipment_transition.h"
#include "entity_events.h"
#ifdef __cplusplus
extern "C" {
#endif
/* C2670/CC240 borrow the SAME player's equipment/action fields. The input
 * pointer (+1B0) and disabled byte (+848) are live across completing calls.
 * Services return0 on completion, not just on queuing a request. No default
 * success for missing input, flashlight, animation, sound or flag owners.
 * Sender is the event sender ID, NOT its one-byte payload. */
typedef struct UCPlayerCutscene {
    UCPlayerEquipmentTransition *equipment;
    void **input;
    uint8_t *disabled;
} UCPlayerCutscene;
typedef struct UCPlayerCutsceneServices {
    void *owner;
    int (*preflight)(void *,uint8_t returning);
    int (*action)(void *,uint32_t,int32_t,int32_t); /* C1E60 */
    int (*transition)(void *,uint32_t,uint32_t); /* BFE00, including old-state cleanup */
    int (*input)(void *,void *,uint8_t returning); /* virtual10/14 */
    int (*inhibit)(void *,uint8_t enabled,uint8_t death_camera); /*423E0/42140*/
    int (*stop_idle)(void *); /* live41F80 item+44,216060("Idle") */
    int (*sender_class)(void *,int32_t,const char **); /*1F8FC0; NULL if missing*/
    int (*stop_zoom)(void *); /*BDDD0*/
    int (*commit)(void *); /*C8930*/
    int (*flags_ready)(void *,uint8_t *); /*2107C0 on live+840/+844*/
} UCPlayerCutsceneServices;
/*0 completed,1 invalid/admission failure,2 committed service failure. Do not
 * retry2. These are event receivers, not a Mainplayer factory or input device.
 * Same graph stays alive and nonreentrant; services may mutate its live fields. */
int uc_player_cutscene_take(UCPlayerCutscene *,const UCPlayerCutsceneServices *,int32_t sender);
int uc_player_cutscene_return(UCPlayerCutscene *,const UCPlayerCutsceneServices *);
/* Borrowed same-player event binding for D3910's take113/return166 and
 * D4E11's pitch-reset34 rows. angles and pitch_limit are the live player fields.
 * Install listener in the existing entity's listener chain; this is not an
 * entity constructor. Storage must not move after initialization and must
 * outlive registration. Payload is unused; take consumes the actual sender. */
typedef struct UCPlayerCutsceneEvents {
    UCPlayerCutscene *player;
    const UCPlayerCutsceneServices *services;
    float *angles;
    const float *pitch_limit;
    UCEntityEventSubscription rows[3];
    UCEntityEventListener listener;
} UCPlayerCutsceneEvents;
int uc_player_cutscene_events_init(UCPlayerCutsceneEvents *,UCPlayerCutscene *,
    const UCPlayerCutsceneServices *,float angles[3],const float *pitch_limit);
/* Live presentation bytes at player+4C4 offsets0..3,8. The object at+4 is
 * supplied by its real owner, not manufactured here. Callbacks may mutate
 * bytes; update re-reads after querying the object as original41ED0 does. */
typedef struct UCPlayerPresentationGate {
    uint8_t override_on,enabled,compatible,inhibited,restore;
} UCPlayerPresentationGate;
typedef struct UCPlayerPresentationGateServices {
    void *owner;
    int (*query)(void *,uint8_t *);
    int (*set)(void *,uint8_t);
    int (*suspend)(void *); /*41C30 on the same player*/
    int (*resume)(void *); /*42070 on the same player*/
} UCPlayerPresentationGateServices;
/* Whole41ED0 and42140/423E0 orchestration.0 complete,1 invalid,2 committed
 * failure. Ordinary inhibition invokes suspend/resume only when live+8 is set;
 * death-camera inhibition performs only42140's object update. */
int uc_player_presentation_update(UCPlayerPresentationGate *,const UCPlayerPresentationGateServices *);
int uc_player_presentation_inhibit(UCPlayerPresentationGate *,const UCPlayerPresentationGateServices *,uint8_t,uint8_t death_camera);
#ifdef __cplusplus
}
#endif
#endif
