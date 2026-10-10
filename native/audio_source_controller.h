#ifndef UC_NATIVE_AUDIO_SOURCE_CONTROLLER_H
#define UC_NATIVE_AUDIO_SOURCE_CONTROLLER_H
#include "audio_position_pool.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One source owner. position is the SAME identity inserted in position_pool,
 * not a second actor position. Caller owns this stable allocation; active/busy
 * are native lifetime guards, not original fields. Impact notifier is NULL,
 * hence the original completion list is always empty in this domain. */
typedef struct UCAudioSourceController {
    UCAudioPositionSource position;
    void *engine;
    uint8_t flag24,flag25,active,busy;
} UCAudioSourceController;
typedef struct UCAudioSourceBank {
    const void *identity;
    uint32_t type;
} UCAudioSourceBank;
typedef struct UCAudioSourceServices {
    void *context;
    int (*preflight)(void *,const UCAudioSourceController *,const UCAudioSourceBank *);
    void *(*current_audio)(void *); /* actual world+6C, may be NULL */
    void *(*factory)(void *,void *audio); /* audio virtual+4, may be NULL */
    void *(*create)(void *,void *factory); /* factory+0C, actual type4FCB8C */
    int (*position)(void *,void *engine,const float *point); /* engine+20 */
    int (*enable)(void *,void *engine); /* engine+2C only on new creation */
    uint32_t (*event)(void *,const UCAudioSourceBank *,const char *name);
    int (*request)(void *,void *engine,const UCAudioSourceBank *,uint32_t index);
    int (*close)(void *,void *engine); /* engine+0C, completing real teardown */
} UCAudioSourceServices;
/* Fresh/inactive caller storage only. Bitwise point copy, including signed0.
 * Input point storage is stable and disjoint from the controller.
 * Native successful caller allocation replaces original controller/list
 * allocations; no original allocator-failure or address-order claim. */
int uc_audio_source_begin(UCAudioSourceController *,const float point[3]);
/* Shared whole2165A0 prefix. Caller has admitted completing services and holds
 * this active source's busy guard. Does not enable flag24 or issue a request;
 * those belong to the differing216D30 and216400 callers. */
int uc_audio_source_ensure(UCAudioSourceController *,const UCAudioSourceServices *);
/* Whole216D30 NULL-notifier route plus2165A0. Preflight is pure. Other services
 * must complete real engine work; they may mutate source flags, attachment,
 * position and engine handle at source callback frontiers, retaining all
 * owner/request/service lifetimes. No reentry or notifier-list mutation.
 * Bank type/interface/event getters are admitted pure stable projections;
 * type104 resolves name, other types pass indexFFFFFFFF as source does.
 * A valid factory must produce a nonnull real source; unexpected NULL is
 * committed failure, not the original's null-dereference success. request
 * means actual246BB0 work completed, including policy rejection when present,
 * not necessarily a voice started. accepted is216D30's low byte, not audio.
 * Return0 complete,1 pre-effect rejection,2 committed dependency failure.
 * Failures do not roll back flags/created handles or publish accepted. */
int uc_audio_source_play(UCAudioSourceController *,const UCAudioSourceBank *,
    const char *name,const UCAudioSourceServices *,uint8_t *accepted);
/* Empty-notifier2169B0. Must first detach pool membership. Calls actual current
 * engine close if present, then marks the caller-owned source inactive. The
 * engine field remains source-stale; caller may free the enclosing allocation
 * after completion. Populated notifier/other controller domains unclaimed. */
int uc_audio_source_end(UCAudioSourceController *,const UCAudioSourceServices *);
#ifdef __cplusplus
}
#endif
#endif
