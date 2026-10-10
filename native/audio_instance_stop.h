#ifndef UC_NATIVE_AUDIO_INSTANCE_STOP_H
#define UC_NATIVE_AUDIO_INSTANCE_STOP_H
#include "audio_instance.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioInstanceStopServices {
    void *context;
    int (*preflight)(void *,const UCAudioInstance *,uint32_t flags); /* pure */
    int (*cancel_track)(void *,UCAudioInstanceTrack *); /* real23FF00 */
    int (*stop_voice)(void *,void *voice,uint32_t flags); /* real245450 */
    int (*update_instance)(void *,UCAudioInstance *,uint32_t horizon); /*24A6F0*/
} UCAudioInstanceStopServices;
/* Actual24A900 immediate branch (request bit10000 clear),248DD0 and24B310.
 * Existing instance state8 completes accepted0 without consulting its graph.
 * Otherwise the SAME instance graph has <=4096 live tracks with correct parent
 * links. Services must complete the named original effects; no queue or device
 * success is invented here. preflight guarantees stable instance/current track
 * through each cancellation/stop, and a finite <=4096 visited sequence.
 * Effects may change current flags/voice and edit other track memberships;
 * the next link is reloaded after stop. cancel precedes the current voice read
 * and track flags edit. Captured voice stays live through stop_voice.
 * update_instance runs last only for flagsbit2, with horizonFFFFFFFF, and may
 * destroy this instance and tracks; no owner field is read afterward.
 * Return0 completed,1 pure rejection,2 downstream failure/budget after a
 * committed prefix. Effects return0 on completion. accepted is distinct live
 * storage written only on completed0. This is not delayed-stop scheduling,
 * concrete queue cancellation, backend stop or full24A6F0 cleanup ownership. */
int uc_audio_instance_stop(UCAudioInstance *,uint32_t flags,
                          const UCAudioInstanceStopServices *,uint8_t *accepted);
#ifdef __cplusplus
}
#endif
#endif
