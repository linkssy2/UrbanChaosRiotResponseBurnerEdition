#ifndef UC_NATIVE_AUDIO_INSTANCE_UPDATE_H
#define UC_NATIVE_AUDIO_INSTANCE_UPDATE_H
#include "audio_instance_cleanup.h"
#include "audio_group.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioInstanceUpdate {
 void *context;
 const uint32_t *engine_flags; /* actual current engine+B4 */
 const UCAudioGroup *(*group)(void *,const void *); /* pure same group projection */
 int (*preflight)(void *,const UCAudioInstance *); /* pure0admits whole operation */
 const UCAudioPlaybackServices *playback;
 const UCAudioInstanceCleanup *cleanup;
} UCAudioInstanceUpdate;
/* Natural24A6F0 state2 or immediate-stop state7 with actual245810/2519C0 status and the SAME concrete
 * per-track/final cleanup owners. States0/1/6/8 take their real quiet returns;
 * other state transitions are excluded. Source buffer+18 ring must be EMPTY,
 * so full246360 performs its actual empty traversal. Ordinary track flags2200
 * and voice flags40000000 are excluded. <=255 live reciprocal tracks.
 * This steady-mix domain requires actual shared group+5C dirty fields zero
 * along the <=64 acyclic parent chain, and engine+B4 bit2 clear. No omitted
 * dirty-mix callback is claimed complete. Status is tested NONZERO, not bit1.
 * State7 polls every track regardless of flag4; state2 requires that flag.
 * Absence of voice/device on a polled track
 * follows the source finished branch. Finished tracks clean immediately, then
 * all-finished (including zero tracks) invokes real24A500, which may delete p.
 * Preflight admits all status, cancellation, release and bank effects before
 * any callback. They preserve current engine/group/state/metadata/topology and
 * captured lifetimes except explicit cleanup deletion. No reentry. Unexpected
 * nested error returns2 with committed prefix; no rollback or retry implied.
 * finished is distinct storage, written on0;1 means cleanup ran, not necessarily
 * deletion if caller retains the instance. No scheduler advancement/start or
 * arbitrary dynamic-mix/spatial/timed-track support is claimed. */
int uc_audio_instance_update(UCAudioInstance *,const UCAudioInstanceUpdate *,uint8_t *finished);
#ifdef __cplusplus
}
#endif
#endif
