#ifndef UC_NATIVE_AUDIO_SOURCE_RETIREMENT_H
#define UC_NATIVE_AUDIO_SOURCE_RETIREMENT_H
#include "audio_source_engine.h"
#include "audio_voice_retirement.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioSourceRetirement {
 void *context;
 UCAudioLink *instances; /* SAME current engine+28, not a caller quiet boolean */
 const UCAudioBufferGroup *group; /* one externally retained actual group */
 const UCAudioVoiceRetirement *buffers; /* SAME device budgets/backend owner */
 int (*preflight)(void *,const UCAudioSourceEngine *); /* pure0admits all effects */
 void (*release_source)(void *,UCAudioSourceEngine *); /* real final allocator */
} UCAudioSourceRetirement;
/* Whole245A80→247050→246510, with actual2463B0/240550 EMPTY/NONMATCHING
 * instance traversal,246230→246150 and real device/group release. The current
 * engine is stable and nonnull throughout, sharing instances and device budgets.
 * Source pending must be empty, and every source buffer QUIESCENT: no children,
 * instances or aux, flags8 clear, owner38/spatialNULL. Every reachable actual
 * engine instance is inspected; any matching buffer rejects BEFORE refs/unlink.
 * Matching instances require source249550 pause before24A900 and are unsupported,
 * never silently stopped. Rings bounded4096 with live reciprocal storage; actual
 * instance class/query is represented by its authoritative buffer field.
 * Sources refs>=1, buffers refs>=1. Source refs>1 only decrement. A buffer refs>1
 * survives detached with its real owner/group/device intact. group is shared by
 * all nonnull owner34 fields; it retains at least one external ref after all
 * possible deletions. No group final destructor or pending-source drain claim.
 * Completing callbacks preserve all borrowed scalar/ring state/lifetimes except
 * the explicit deletion. Preflight admits device polling and every release;
 * deletion callbacks may free immediately. No reentry or callback ring mutation.
 * 0 complete,1 no-effect reject,2 committed nested failure (no rollback/retry).
 * remaining is separate live storage, written only on0, valid after source free.
 * No audio request/voice creation or production player adapter is introduced. */
int uc_audio_source_release(UCAudioSourceEngine *,const UCAudioSourceRetirement *,uint32_t *remaining);
#ifdef __cplusplus
}
#endif
#endif
