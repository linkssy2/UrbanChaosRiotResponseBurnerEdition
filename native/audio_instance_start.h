#ifndef UC_NATIVE_AUDIO_INSTANCE_START_H
#define UC_NATIVE_AUDIO_INSTANCE_START_H
#include "audio_command_submit.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioInstanceStart {
 void *context;
 void *bank;UCAudioLink *bank_instances,*bank_timing; /* actual bank+58,+68 */
 int (*preflight)(void *,const UCAudioInstance *,uint32_t); /* pure0admits */
 const UCAudioTrackCommands *(*commands)(void *,const UCAudioInstanceTrack *); /* pure actual trkl spans */
 int (*preload)(void *,UCAudioInstance *,UCAudioInstanceTrack *,const UCAudioTimedCommand *); /* complete249CC0, original AL ignored */
 int32_t (*clock)(void *); /* actual23FCD0 current clock/provider result */
 int (*mix)(void *,UCAudioInstance *); /* complete248D10 */
 const UCAudioCommandSubmit *submit; /* SAME engine queue, time, RNG and tracks */
} UCAudioInstanceStart;
/* Actual24C800→24A420→24A260/24A1A0/249480 for ordinary AMPC, state2
 * start. Bank identity/type/interface are pure; bank_timing is the actual live
 * EMPTY +68 ring, making24B950 returnNULL. Flags2000/8000/10000 (special
 * transitions/timeline/engine queue) and synthetic NULL-definition tracks are
 * excluded. Actual bank publication retains first, appends links[3], then
 * performs source state admission; rejected state leaves that prefix intact.
 * Nonnull tracks use actual immutable trkl spans and1..255 commands1/2/3;
 * <=255 tracks. Unless flags4 skips preloading, each resume points inside its
 * command span. First time0 command calls required real249CC0 even if binding
 * will returnfalse; that Boolean is ignored by the source. No fake success.
 * Preload precedes clearing track flag1, timing publication and clock lookup.
 * Existing command owner then allocates/queues/executes on SAME graph/RNG.
 * The clock returns nonnegative signed32 source units; no replacement clock.
 * Completed scheduling finally invokes actual248D10 mix, even zero tracks.
 *
 * Preflight admits every completing effect and all nested command operations.
 * command getter is pure, stable, no private cursor. Other services may update
 * their permitted live voice/mix/RNG fields, not bank/track rings, metadata,
 * resume cursors or service storage; no reentry or early owner destruction.
 * Captured source lifetimes remain valid. Preload/mix can consume SAME RNG.
 * No later access to a freed command; queue owner has its usual contract.
 * Null instance completes started0; otherwise0 source-completed,1 rejection
 * before effects,2 unsupported dependency failure after committed prefix.
 * started is written only on0, not a claim of audible output. Failed starts
 * still require source24A500 cleanup from the caller, never rollback here. */
int uc_audio_instance_publish(UCAudioInstance *,uint32_t,const UCAudioInstanceStart *,uint8_t *started);
#ifdef __cplusplus
}
#endif
#endif
