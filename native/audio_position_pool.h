#ifndef UC_NATIVE_AUDIO_POSITION_POOL_H
#define UC_NATIVE_AUDIO_POSITION_POOL_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* The two actual2162A0 position sources. attached_position borrows the current
 * actor+40 position; NULL selects this source's own+8 position. Neither is a
 * second actor/engine owner. Native pointer order replaces original pointer
 * keys, so source-address allocation differences can change which nearby
 * source is first; closest-distance selection would not match the source. */
typedef struct UCAudioPositionSource {
    const float *attached_position;
    float position[3];
} UCAudioPositionSource;
typedef struct UCAudioPositionPool {
    UCAudioPositionSource **sources;
    uint32_t count,capacity;
    uint8_t active;
} UCAudioPositionPool;
typedef struct UCAudioPositionServices {
    void *context;
    /* Pure admission: no source, pool, request, external or FP-mode effects. */
    int (*preflight)(void *,const UCAudioPositionPool *,const void *bank);
    /* Actual216B70 construction, including engine/controller state, must
     * complete. NULL is the original allocation-miss outcome. A returned
     * source and its position storage remain live through pool removal. */
    UCAudioPositionSource *(*create)(void *,const float point[3]);
    /* Actual216D30 request. Return its low-byte result through accepted, NOT
     * audible completion. Nonzero status reports committed service failure. */
    int (*play)(void *,UCAudioPositionSource *,const void *bank,const char *name,
                uint8_t *accepted);
} UCAudioPositionServices;
/* Caller-owned zeroed pool and preallocated pointer storage, capacity1..4096.
 * No allocation occurs in this owner. Storage is disjoint from all sources,
 * requests and services; initialization does not clear unused pointer slots.
 * Removal/ending release membership only, not sources or backend voices.
 * They are native storage operations, not a claim of original audio teardown. */
int uc_audio_position_pool_begin(UCAudioPositionPool *,UCAudioPositionSource **,uint32_t);
int uc_audio_position_pool_remove(UCAudioPositionPool *,UCAudioPositionSource *);
int uc_audio_position_pool_end(UCAudioPositionPool *);
/* Full20D900 selection plus20D8D0 or20DA70, with concrete20D360 names.
 * Impact domain: NULL completion-notifier and NULL material prefix. Named
 * radius10; material radius1 and one Default fallback on the SAME source.
 * Create/play services complete synchronously and may change source positions,
 * attachments and external audio state, but not pool membership, request
 * storage or service identities; they cannot reenter this pool. The position
 * getter itself is the pure2162A0 operation. Finite |coordinates|<=1e15,
 * round-to-nearest with DAZ/FTZ disabled; callbacks cannot inspect FP status.
 * Preflight guarantees real creation/play dependencies; no mute/no-op success
 * substitutes for missing engine work. Outputs are borrowed selected source
 * and final low-byte acceptance; sourceNULL/accepted0 is an actual miss.
 * Status0 source-completed,1 pre-effect rejection,2 committed service failure.
 * Capacity exhaustion rejects before creation; existing matches remain valid
 * at full capacity. Invalid callback results are not rolled back or retried. */
int uc_audio_position_named(UCAudioPositionPool *,const void *bank,const char *name,
    const float point[3],const UCAudioPositionServices *,UCAudioPositionSource **,uint8_t *accepted);
int uc_audio_position_material(UCAudioPositionPool *,const void *bank,uint32_t material,
    const float point[3],const UCAudioPositionServices *,UCAudioPositionSource **,uint8_t *accepted);
#ifdef __cplusplus
}
#endif
#endif
