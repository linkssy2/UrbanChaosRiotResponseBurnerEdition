#ifndef UC_NATIVE_AUDIO_COMMAND_SUBMIT_H
#define UC_NATIVE_AUDIO_COMMAND_SUBMIT_H
#include "audio_command_queue.h"
#include "audio_timed_event.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioQueuedCommand {
    UCAudioPendingCommand pending; /* first; exact same cancellation owner */
    const uint8_t *source; /* immutable original bank command, borrowed */
    UCAudioTimedCommand decoded;
} UCAudioQueuedCommand;
typedef struct UCAudioCommandSubmit {
    void *context;
    uint64_t *current_time; /* same engine+8, abstract source units */
    UCAudioCommandQueue *heap; /* same engine+10 */
    UCAudioRandom *random; /* same session AMPC stream */
    int (*preflight)(void *,const UCAudioInstanceTrack *,const UCAudioTrackCommands *);
    UCAudioQueuedCommand *(*allocate)(void *);
    void (*release)(void *,UCAudioQueuedCommand *);
    const UCAudioEventServices *(*events)(void *,UCAudioInstanceTrack *);
} UCAudioCommandSubmit;
/* Actual24E010 ordinary opcode1/2/3 constructor/publication,23FEA0 and
 * 241940/244FE0 ordering. Reuses qualified timed parsing/RNG/event execution
 * and SAME queue/track graph. Definition exists, parent timing query isNULL.
 * Pure preflight admits all completing allocations and actual event services,
 * live reciprocal rings, unique heap backlinks, every existing queued command
 * represented by this type with refs1, and borrowed immutable command lifetime.
 * Allocation/release preserve other owners/RNG; events may mutate live event
 * fields and consume the same RNG as their qualified contract permits, but
 * cannot edit queues/rings/cadence/cursors or reenter. Events getter is pure.
 * allocate returns fresh real storage; release really disposes it. No copied
 * voice/source graph, hidden engine clock or invented playback effect.
 * now<=INT64_MAX-UINT32_MAX-65535; current time/queued times nonnegative signed64.
 * Delayed command time may exceed horizon. Allocation precedes delay RNG and
 * cursor advance. A full heap drops/unlinks/frees only that new command after
 * cursor advancement, exactly as source. Immediate dispatch first drains the
 * old heap through a captured current-time value. Events complete before
 * concrete reference decrement/unlink/free. No access follows free.
 * Return0 completed,1 pre-effect rejection,2 unexpected allocation/event
 * failure after committed prefix. No rollback; remaining allocations stay
 * linked for actual cancellation. This is not copied-command24E3A0 or a
 * wall-clock update owner, and does not supply missing voice/backend services. */
int uc_audio_command_schedule(UCAudioInstanceTrack *,const UCAudioTrackCommands *,
                              uint64_t now,uint32_t horizon,const UCAudioCommandSubmit *);
/* Explicit same heap drain; pure preflight called with track/commandsNULL and
 * must admit its existing commands/services. Does not publish engine time. */
int uc_audio_command_dispatch_due(uint64_t limit,const UCAudioCommandSubmit *);
#ifdef __cplusplus
}
#endif
#endif
