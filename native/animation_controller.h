#ifndef UC_NATIVE_ANIMATION_CONTROLLER_H
#define UC_NATIVE_ANIMATION_CONTROLLER_H
#include "animation_asset.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Scalar timing from 1F2250/1F1FC0. No clip/cache ownership or event dispatch.
 * Flags retain original bits; source_control is independent of those flags.
 * Caller owns clip selection, controller ordering and the persistent pose cache. */
typedef struct UCAnimationController {
    float phase, previous, weight, rate, phase_scale, blend_in, blend_out;
    int32_t repeats;
    uint32_t flags, source_control;
} UCAnimationController;
/*1F22B0, after active positive-weight admission. A held controller suppresses
 * root motion while fading in; otherwise movement scales with its real rate. */
static inline float uc_animation_controller_root_factor(const UCAnimationController *c){
    return (c->flags&0x20)&&c->weight<1?0:c->rate;
}
typedef struct UCAnimationInterval { float begin, end; uint32_t repetitions; } UCAnimationInterval;
typedef struct UCAnimationAdvance {
    UCAnimationInterval intervals[3];
    uint32_t count, original_result; /* Original update result 0/1/2, not dispatched. */
} UCAnimationAdvance;

typedef struct UCAnimationControllerTag { uint16_t id, state; float phase; } UCAnimationControllerTag;
typedef struct UCAnimationEventParameter { const char *name,*value; uint32_t words[2]; } UCAnimationEventParameter;
typedef struct UCAnimationEvent {
    const char *name,*type;float phase;uint32_t code,parameter_count;
    const UCAnimationEventParameter *parameters;
} UCAnimationEvent;
typedef struct UCAnimationAnnotations {
    uint8_t *storage;UCAnimationEvent *events;UCAnimationEventParameter *parameters;
    UCAnimationControllerTag *tags;uint32_t event_count,parameter_count,tag_count;
} UCAnimationAnnotations;
typedef struct UCAnimationEventRef { const UCAnimationAnnotations *owner;uint32_t index; } UCAnimationEventRef;
typedef struct UCAnimationEventQueue { uint32_t count;UCAnimationEventRef events[16]; } UCAnimationEventQueue;

typedef struct UCAnimationControllerEntry {
    UCAnimationController *controller;
    const UCAnimationClip *clip;
    const UCAnimationAnnotations *annotations;
} UCAnimationControllerEntry;
typedef struct UCAnimationControllerListUpdate {
    uint32_t survivors[32], survivor_count;
    uint32_t removed[32], removed_count;
    uint32_t advanced[32], advanced_count;
    UCAnimationAdvance advances[32]; /* Indexed by the input entry, not advanced[]. */
    UCAnimationEventQueue events;
} UCAnimationControllerListUpdate;

/* 1F1670 insertion point: descending signed source_control, before equals.
 * Does not mutate entries/current/new. After successful resource preparation,
 * caller clears old-current flags&=~9u and inserts the fresh controller here.
 * Entries are distinct, already ordered; count<=31 leaves space for insertion. */
UCAnimationStatus uc_animation_controller_list_insert_index(
    const UCAnimationControllerEntry *,uint32_t count,const UCAnimationController *,uint32_t *index);
/* 1F1D60 list advance/arbitration, before event handlers and pose sampling.
 * Uses the SKELETON TEMPLATE bone_flags (template+1C), not model bone enable
 * bytes. Newest/equal-priority first. Result2 removes unprotected later entries
 * whose enabled tracks it covers BEFORE they advance; result1 removes itself.
 * All advanced scalars and removed flags publish together on success; failure
 * preserves controllers and output. Arrays contain original input indices in
 * traversal/removal order. Caller retains annotation owners through dispatch,
 * releases removed samplers, and samples surviving ACTIVE controllers in order.
 * <=32 entries/4096 bones are native bounds. Tracks must fit the template and
 * their final decoded track must be the largest, matching original mask sizing.
 * No allocation/pool callback, handler dispatch or pose sampling occurs here. */
UCAnimationStatus uc_animation_controller_list_advance(
    const UCAnimationControllerEntry *,uint32_t count,const UCAnimationTemplate *,
    float delta,UCAnimationControllerListUpdate *);

/* Finite forward-time domain: phase in [0,1), rate/phase_scale/delta >=0;
 * finite signed blend durations and signed repeat counts are retained.
 * Advance rejects overflow or a phase outside original CVTTSS2SI's positive
 * range. Repeated full-cycle intervals remain compact even for large deltas.
 * Original signed-zero fade-out may yield infinite weight (including during
 * hold); this state is preserved and can be advanced. NaN weight is rejected.
 * Failure preserves every output. Flags are explicit: 1F2250 does not set them. */
UCAnimationStatus uc_animation_controller_init(const UCAnimationClip *,uint32_t flags,UCAnimationController *);
UCAnimationStatus uc_animation_controller_advance(UCAnimationController *,float delta,UCAnimationAdvance *);
/* Append references in original controller/interval/authored order, capped at16.
 * Owners and their records must remain immutable/alive until queue consumption.
 * Repeated cycles intentionally duplicate references. No handler is dispatched. */
UCAnimationStatus uc_animation_collect_events(const UCAnimationAnnotations *owner,
    const UCAnimationAdvance *,UCAnimationEventQueue *);
/* Exact authored-order scan: stop at the first tag later than phase. */
UCAnimationStatus uc_animation_tag_state(const UCAnimationControllerTag *,uint32_t count,
    uint16_t id,float phase,int *state);
/* Owns numeric records and exact name bytes independently of the clip. Source
 * order is retained; parameter words and event codes are not dispatched.
 * Output must be empty/freed first; failure preserves it. */
UCAnimationStatus uc_animation_annotations_load(const UCAnimationClip *,UCAnimationAnnotations *);
void uc_animation_annotations_free(UCAnimationAnnotations *);
#ifdef __cplusplus
}
#endif
#endif
