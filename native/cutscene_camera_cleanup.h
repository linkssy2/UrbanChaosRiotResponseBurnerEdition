#ifndef UC_CUTSCENE_CAMERA_CLEANUP_H
#define UC_CUTSCENE_CAMERA_CLEANUP_H
#include "cutscene_camera.h"
#include "entity_listener.h"
#include "entity_retirement.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCutsceneCameraCleanup {
    const UCEntityRetirement *retirement;
    void *owner;
    int (*preflight)(void *); /* pure, including real retirement capacity */
    int (*remove_camera)(void *); /* actual215000, SAME camera stack */
    void *(*current_provider)(void *);
    int (*player)(void *,void *,int32_t,int32_t *);
    /* Actual4F9F64 send to Mainplayer's CC240, which ignores its unwritten
     * one-byte payload. This endpoint must implement that receiver, not infer
     * a payload value for arbitrary subscribers. Sender captured after player. */
    int (*return_control)(void *,int32_t player,uint32_t sender);
    int (*mode)(void *,void *,int32_t,int32_t);
    void (*release)(void *,UCEntityEventTarget *);
} UCCutsceneCameraCleanup;
/* Null model/helper/audio/script/handle camera domain; same real empty aux84.
 * Listeners are retained during retirement. Full173340 calls stack remove,
 * live provider/control/mode,218B30 retirement, then stack remove again.
 * Full173F50/173F70 clears owned subscriptions, removes camera again, executes
 * the null218CE0/218180 suffix, and releases the outer object only if flags&1.
 * Subscription wrapper belongs to the enclosing native owner; its identity
 * remains valid through release. Registry must already reject the dying ID.
 * Services complete/nonreentrant and retain all graph storage until release;
 * provider callbacks may change the same camera fields but must preserve this
 * admitted profile and supply non-NULL providers on unconditional re-reads.
 *0 complete,1 pre-effect reject,2 committed failure; never retry after2.
 * No generic model/audio/coroutine destructor or arbitrary event payload claim. */
int uc_cutscene_camera_remove(const UCEntityFreshView *,UCCutsceneCamera *,const UCCutsceneCameraCleanup *);
int uc_cutscene_camera_destroy(const UCEntityFreshView *,UCEntitySubscriptions *,const UCCutsceneCameraCleanup *,uint32_t flags);
#ifdef __cplusplus
}
#endif
#endif
