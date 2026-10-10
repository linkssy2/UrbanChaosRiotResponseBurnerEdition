#ifndef UC_FEEDBACK_SAMPLE_H
#define UC_FEEDBACK_SAMPLE_H
#include "shot_feedback.h"
#include "vector_random.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCFeedbackPositionObserver {
    void *owner;
    int (*kind)(void *,uint32_t *);
    int (*sample)(void *,const float position[3],double *);
} UCFeedbackPositionObserver;
typedef struct UCFeedbackPlayerObserver {
    void *owner;
    int (*kind)(void *,uint32_t *);
    int (*player)(void *,uint32_t *);
    int (*sample)(void *,double *);
} UCFeedbackPlayerObserver;
typedef struct UCFeedbackObservers {
    const UCFeedbackPositionObserver *const *position;
    const UCFeedbackPlayerObserver *const *player;
    uint32_t position_count,player_count;
} UCFeedbackObservers;
/*674D0 spatial receiver; caller admits finite position/radius/amplitude and radius>0. */
double uc_feedback_spatial_weight(const UCSpatialFeedbackEffect *,const float position[3]);
/* Complete682F0 query over the SAME timed/spatial owner and borrowed views of
 * its actual+1C/+28 trees. Views must retain original ascending identity order,
 * stable membership and actual receiver identity for the whole call. A zero
 * count certifies a genuinely empty tree, not an unimplemented owner.
 * Required callbacks complete the original virtuals in order; they must not
 * mutate queues/views, query inputs or floating environment. No copied cache.
 * Lists must be live products of shot_feedback APIs. Player0..3; finite bounded
 * query coordinates/clock <=1e15, RN/no DAZ/FTZ; at most65536 observers per tree.
 * Return0 complete,1 invalid/preflight,2 receiver failure/unsupported result.
 * On2 prior receiver effects persist, output is unchanged: do not retry.
 * Full67EF0 construction and observer registration remain upstream owners. */
int uc_feedback_sample(const UCShotFeedback *,const UCFeedbackObservers *,uint32_t player,
    const float position[3],uint32_t kind,float clock,uint32_t mode,float *out);
/* Actual67550, sharing the same51E190 random owner with decals/glass. Positive
 * amplitude consumes one54160 four-lane draw, then2D3DC0 and the original matrix
 * product, restoring ALL four translation-row bits. <=0 copies without RNG.
 * Finite amplitude magnitude<=65536, matrix magnitude<=1e15; disjoint output.
 * Return1 before mutation on failure. The source has distinct alias behavior.
 * Host double-backed trig oracle, not a claim about physical x87 extended bits. */
int uc_feedback_perturb(UCVectorRandom *,float amplitude,const float world[16],float out[16]);
#ifdef __cplusplus
}
#endif
#endif
