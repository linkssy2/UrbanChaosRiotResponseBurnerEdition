#ifndef UC_NATIVE_SHOT_FEEDBACK_H
#define UC_NATIVE_SHOT_FEEDBACK_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum UCFeedbackStatus {
    UC_FEEDBACK_OK, UC_FEEDBACK_ARGUMENT, UC_FEEDBACK_UNSUPPORTED,
    UC_FEEDBACK_MEMORY
} UCFeedbackStatus;
typedef struct UCFeedbackEffect {
    uintptr_t player; /* Original unsigned player value, not an owned object. */
    float amplitude, duration;
    uint32_t kind;
    float start;
} UCFeedbackEffect;
typedef struct UCFeedbackNode {
    struct UCFeedbackNode *next, *previous;
    UCFeedbackEffect effect;
} UCFeedbackNode;
typedef struct UCSpatialFeedbackEffect {
    float position[3], radius, amplitude, duration;
    uint32_t kind;
    float start;
} UCSpatialFeedbackEffect;
typedef struct UCSpatialFeedbackNode {
    struct UCSpatialFeedbackNode *next, *previous;
    UCSpatialFeedbackEffect effect;
} UCSpatialFeedbackNode;
typedef struct UCFeedbackMemory {
    void *context;
    void *(*allocate)(void *, size_t);
    void (*release)(void *, void *);
} UCFeedbackMemory;
typedef struct UCShotFeedback {
    UCFeedbackNode sentinel;
    uint32_t count;
    UCSpatialFeedbackNode spatial_sentinel;
    uint32_t spatial_count;
    float time_offset, output[4];
    uint8_t suppress_once, enabled[4]; /* Actual manager+54 query gates. */
    UCFeedbackMemory memory;
} UCShotFeedback;
/* Actual timed-effect (manager+4) and spatial-effect (manager+10) lists.
 * Initialize fresh, disjoint storage once; never copy or move the owner.
 * Nodes are owned; player identities and allocator context must remain valid
 * until clear. The owner is single-threaded/non-reentrant. Allocators must not
 * inspect or mutate the owner and release must complete. OOM leaves it intact.
 * Init is native storage setup, not the original complete67EF0 constructor. */
UCFeedbackStatus uc_shot_feedback_init(UCShotFeedback *, UCFeedbackMemory);
/* Actual688C0/68950: append in order, retain duplicates, ignore either signed
 * zero input, replace negative duration with source .0001. mode2 adds the
 * existing owner time offset to the borrowed world clock. No device gate is
 * applied at enqueue. Finite nearest-even arithmetic, no DAZ/FTZ; count must
 * stay below original0x0CCCCCCC. No arbitrary guest float/allocator exception
 * equivalence is claimed. All argument/float failures precede effects. */
UCFeedbackStatus uc_feedback_enqueue(UCShotFeedback *, uintptr_t player,
    float amplitude, float duration, uint32_t kind, float clock, uint32_t mode);
/* Complete43A90 in the fixed original primary item2/17 domain: both divisors
 * are zero, modes are4, amplitudes .5/1 and durations .1/.5; kind2. The original
 * equipment+30 value is not read on this branch. Other/alternate definitions
 * are not accepted. This performs real queue allocation and publication. */
UCFeedbackStatus uc_shot_feedback_enqueue(UCShotFeedback *, uint32_t item,
    uint8_t alternate, uintptr_t player, float clock, uint32_t mode);
/* Actual68A30/68AD0 spatial enqueue: radius, amplitude or duration zero skips;
 * negative radius/duration become .0001. Position is copied, duplicates stay,
 * and mode2 uses the same manager offset as timed effects. Inputs must remain
 * finite/immutable/disjoint during the call; native capacity0x07FFFFFF and
 * allocation guards reject before publication. No distance/sample calculation
 * or physical output is implied by enqueue. */
UCFeedbackStatus uc_spatial_feedback_enqueue(UCShotFeedback *,const float position[3],
    float radius,float amplitude,float duration,uint32_t kind,float clock,uint32_t mode);
/* Bounded68080: already-registered four output objects,
 * and all player/device output paths suppressed. Pure admission is REQUIRED
 * to certify actual upstream input/profile/world/player predicates, stable
 * through the call; it may not mutate owner, clock or floating environment.
 * Returning1 certifies this domain, not a completed hardware effect. A missing
 * controller alone is NOT an enqueue gate. No keyboard/controller provenance
 * is fabricated here; that upstream owner remains an integration requirement.
 * Actual effects: zero four outputs; mode2 accumulates world delta into offset
 * (other modes reset offset); erase strictly expired spatial nodes FIRST, then
 * timed nodes, each in encounter order;
 * reset suppress_once. Equality retains a node. No hardware API is called. */
UCFeedbackStatus uc_shot_feedback_advance(UCShotFeedback *, float clock,
    float delta, uint32_t mode, int (*admit_suppressed)(void *), void *context);
/* Actual68060/2012C0: clear spatial list first, then timed list; publish each
 * list's empty links/count before releasing its nodes in encounter order.
 * Leaves offset/output/suppress untouched.
 * Call before owner/context destruction; the owner may be reused afterward. */
UCFeedbackStatus uc_shot_feedback_clear(UCShotFeedback *);
#ifdef __cplusplus
}
#endif
#endif
