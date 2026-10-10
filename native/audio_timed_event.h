#ifndef UC_NATIVE_AUDIO_TIMED_EVENT_H
#define UC_NATIVE_AUDIO_TIMED_EVENT_H
#include "audio_event.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioTimedCommand {
    UCAudioCommand command;
    uint32_t random_delay; /* Actual opcode2 command+18; scheduler units. */
} UCAudioTimedCommand;
/* Bounded original opcodes1/2/3. Direct flags0/20/A0 (upper reference wordFFFF),
 * weighted flags4/24, selector-mode3 flags64, shuffle flagsA4/84 (upper
 * reference word0); actual
 * selection/resolution remains required at creation. Opcode2 is28bytes;
 * opcode3 accepts its whole repeat16 field, including0/15/FFFF. Opcodes1/2
 * retain repeat1, with actual MolotovFire opcode1 repeat0 also admitted. The
 * real voice consumer must preserve repeat0's whole/authored-region loop;
 * scheduler state.repeat is only assigned by opcode3. No policy/subbank,
 * special timing or loop-control command is admitted. */
UCAudioStatus uc_audio_timed_command_read(const void *,size_t,UCAudioTimedCommand *);
typedef void (*UCAudioTimedEnqueue)(void *,UCAudioTrackState *,const UCAudioTimedCommand *,uint64_t);
/* Actual24E010 ordinary scheduling. The SAME session audio RNG is consumed
 * once iff an emitted opcode2 has nonzero random_delay, before cursor advance
 * and enqueue. Delay is uint16 random % uint32 delay; horizon compares original
 * command time before random delay, so emitted time can exceed the horizon.
 * Otherwise the existing audio_event scheduler's admitted semantics/lifetimes
 * apply, including all-command prevalidation, reset from resume each call,
 * exclusive horizon and no guessed milliseconds/implicit resume advancement.
 * Completing enqueue owns the actual queue and can consume the SAME RNG or
 * update permitted live flags/parameters, preserving command/cursor storage.
 * No hidden queue or fabricated playback. Output command pointer is temporary
 * for synchronous enqueue; retain its value or original immutable bank offset. */
UCAudioStatus uc_audio_timed_schedule(UCAudioTrackState *,const UCAudioTrackCommands *,
    uint64_t now,uint32_t horizon,UCAudioRandom *,UCAudioTimedEnqueue,void *);
/* Actual241610 maps opcodes1 AND2 to241692;3 to2416DC. Validated control
 * projection reuses the existing event executor without changing frozen code.
 * External preflight/create receive &command->command unchanged (opcode2,
 * 28-byte size, selectors and repeat preserved). Real voice creation must run
 * the matching selection/resource/arbitration/position effects. Delay is NOT
 * consumed again on dispatch. Existing event completing-service contracts
 * apply; initial cached voice identity and current flags remain authoritative. */
UCAudioStatus uc_audio_timed_execute(UCAudioTrackState *,const UCAudioTimedCommand *,
    UCAudioRandom *,const UCAudioEventServices *);
#ifdef __cplusplus
}
#endif
#endif
