#ifndef UC_NATIVE_AUDIO_INSTANCE_CLEANUP_H
#define UC_NATIVE_AUDIO_INSTANCE_CLEANUP_H
#include "audio_command_queue.h"
#include "audio_voice_retirement.h"
#include "audio_buffer_factory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioInstanceCleanup {
 void *context,*bank;
 UCAudioLink *engine_auxiliary,*engine_timing,*bank_timing; /* actual40,58,bank68 */
 UCAudioCommandQueue *heap,*ready;
 const UCAudioBufferGroup *group;
 const UCAudioVoiceRetirement *voices;
 int (*preflight)(void *,const UCAudioInstance *); /* pure0admits */
 void (*release_command)(void *,UCAudioPendingCommand *);
 void (*release_bank)(void *,void *); /* completing actual bank virtual+8 */
 void (*release_track)(void *,UCAudioInstanceTrack *); /* actual allocator */
 void (*release_instance)(void *,UCAudioInstance *); /* actual allocator */
} UCAudioInstanceCleanup;
/* Ordinary24A500 with concrete command cancellation, actual track24B120,
 * instance248630/249260 and track24CFA0/24DFF0 deleting suffixes. One graph.
 * Actual current engine is stable; its auxiliary40/timing58 and AMPC bank68
 * rings are EMPTY (checked), so240810/240950/24B950 take their real quiet paths.
 * bank is NULL or the actual AMPC identity. sound is NULL or live >=16 bytes;
 * a detached policy link requires sound+0E==FFFF. No instance aux40/aux4C,
 * special track0200/aux40, or other retained track references are admitted.
 * All tracks belong to this instance, references1, <=255 unique live entries.
 * No callback may mutate owner scalars, rings, metadata, current engine or
 * future traversal nodes except the explicit release being completed. Bank
 * release can free the bank after the source's final bank read. Release hooks
 * really dispose their allocation and can immediately free; never success
 * stubs. Concrete voice retirement owns its device/backend effects; preflight
 * must admit that complete service plus all command cancellations BEFORE any
 * instance effect. Nested rejection is status2 after a committed prefix.
 * Group remains externally retained (references>=2), decremented in place.
 * Source buffer has flags8 clear, owner38NULL, references>=2; its ref2->1
 * release unlinks membership but leaves its actual external owner/device live.
 * General group/source-buffer final destruction is outside this domain.
 * Initial instance refs>=1+retaining memberships prevents early deletion;
 * flags1 preserves the source auto-delete-at1 rule. Remaining is distinct from
 * all owners, written on0 only; may be0 after true final instance destruction.
 * No natural-finish polling, start or full-event/playback claim. */
int uc_audio_instance_cleanup(UCAudioInstance *,const UCAudioInstanceCleanup *,uint32_t *remaining);
/* Same concrete24B120 for the natural-update per-track frontier. Parent is
 * live, track flags0200/aux40 absent, engine_timing truly empty; preflight
 * admits cancellation/voice retirement without altering the borrowed graph.
 * Does not release the track or its parent. Returns0/1/2 as above. */
int uc_audio_track_cleanup(UCAudioInstanceTrack *,const UCAudioInstanceCleanup *);
/* Actual deleting249260 suffix for an externally retained, already cleaned
 * instance when existing2487C0 reaches0. state8, refs0, group/bufferNULL,
 * every track's commands/voice already empty. This can be used as the real
 * destructor callback of uc_audio_instance_release; it never repeats24A500.
 * It uses the same checked empty-engine domain and completing release hooks.
 * Returns0 after actual deletion,1 pre-effect rejection,2 committed failure. */
int uc_audio_instance_delete(UCAudioInstance *,const UCAudioInstanceCleanup *);
#ifdef __cplusplus
}
#endif
#endif
