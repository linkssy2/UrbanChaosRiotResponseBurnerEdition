#ifndef UC_NATIVE_AUDIO_COMMAND_QUEUE_H
#define UC_NATIVE_AUDIO_COMMAND_QUEUE_H
#include "audio_instance.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Same track.commands ring, one command object and two original queue indices.
 * Payload/scheduler execution belongs to the caller; addresses stay stable. */
typedef struct UCAudioPendingCommand {
    uint64_t time;
    uint32_t references;
    UCAudioLink membership;
    UCAudioInstanceTrack *track;
    uint16_t heap_index,ready_index;
} UCAudioPendingCommand;
typedef struct UCAudioCommandQueue {
    UCAudioPendingCommand **items;
    uint32_t capacity,count;
} UCAudioCommandQueue;
/* Actual244AB0/244B80 and244F80/245020. ready is0(heap) or1(ready).
 * Borrowed arrays have1..65535 allocated slots, count<=capacity, live unique
 * entries and exact index backlinks. Times are0..INT64_MAX, immutable while
 * queued. Both queues are the SAME engine arrays; a command belongs to at most
 * one. No hidden allocation. Full capacity completes added0 without mutation.
 * Removed tail slots retain/clear exactly as source, including stale slot0
 * when the last element is removed. Result storage is distinct from owners.
 * Return0 completed,1 rejected. Fast operations validate local frontiers;
 * caller retains the stated complete graph/lifetime domain. */
int uc_audio_commands_push(UCAudioCommandQueue *,UCAudioPendingCommand *,
                           unsigned ready,uint8_t *added);
int uc_audio_commands_remove(UCAudioCommandQueue *,UCAudioPendingCommand *,
                             unsigned ready,uint8_t *removed);
/* Actual23FF00 plus24D200/24B530: remove from heap, or ready on heap miss,
 * decrement the single reference, unlink from the actual track, then release.
 * Pure admission validates both full arrays and track ring (<=65535 nodes).
 * Each track command must have references1; other retained-command behavior
 * can make the original traversal use its sentinel as a command and is outside
 * this source-safe domain. release must really dispose this allocation without
 * mutating other live graph/queue fields or reentry; it sees indices0 and an
 * isolated membership, and may free immediately. No later command reads.
 * Returns0 completed,1 rejected before effects. */
int uc_audio_commands_cancel(UCAudioCommandQueue *heap,UCAudioCommandQueue *ready,
                             UCAudioInstanceTrack *,
                             void (*release)(void *,UCAudioPendingCommand *),void *);
#ifdef __cplusplus
}
#endif
#endif
