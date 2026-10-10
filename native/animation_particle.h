#ifndef UC_ANIMATION_PARTICLE_H
#define UC_ANIMATION_PARTICLE_H
#include "animation_controller.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAnimationParticleServices {
    void *context;
    const float *(*world)(void *,const void *entity);
    uint8_t (*named_matrix)(void *,const void *entity,const char *,float inout[16]);
    void *(*attached)(void *,int32_t entity,const char *bone,uint8_t hidden);
    void *(*fixed)(void *,int32_t entity,const float matrix[16]);
    void (*submit)(void *,void *source,const char *file);
} UCAnimationParticleServices;
/* Whole219B90 event ordering, bounded to authored Attach strings "0"/"1".
 * Unknown types remain unhandled. A null entity handles this type without
 * reading its parameters. Otherwise exactly three parameters are required.
 * id aliases the SAME entity's live +54 field. world/named_matrix complete
 * actual entity getters; attached creates219530/219740, fixed creates219450
 * then218F40 with the captured ID. submit resolves the CURRENT world manager
 * after source construction and completes its resource/lifetime submission.
 * Names/owner stay alive; callbacks may mutate ID, pose and current manager.
 * Source ownership transfers to submit. Successful allocations are required;
 * NULL creation is committed failure, never fabricated playback success.
 * Float inputs/results finite bounded, RN/gradual; disjoint output. Return
 *0 success,1 pre-effect invalid,2 committed service failure. No CRT parser,
 * allocation, resource lookup, entity lifetime or scheduler is supplied here. */
int uc_animation_particle(const UCAnimationEvent *,const void *entity,const int32_t *id,
    const UCAnimationParticleServices *,uint8_t *handled);
#ifdef __cplusplus
}
#endif
#endif
