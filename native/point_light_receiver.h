#ifndef UC_POINT_LIGHT_RECEIVER_H
#define UC_POINT_LIGHT_RECEIVER_H
#include "point_light_geometry.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow actual provider+90/+192 and light+120 fields. No second provider,
 * light or receiver owner is created. Services must complete, in order. */
typedef struct UCPointLightReceiverTarget {
    float *provider_world;
    uint8_t *provider_dirty;
    float *light_local_center;
    void *context;
    void (*upload)(void *,unsigned first,const float *,unsigned float_count);
    void (*draw)(void *,const void *receiver,const void *pass);
} UCPointLightReceiverTarget;
/* Full 2D4360 cofactor arithmetic, including RCPSS and one refinement. The
 * approximation is the host x86 RCPSS, not a physical Xbox precision claim.
 * Finite nonsingular input/result, RN, DAZ/FTZ off. Disjoint or exact alias.
 * Returns zero without publishing output on invalid input/result. */
int uc_point_light_inverse(const float world[16],float inverse[16]);
int uc_point_light_inverse_control(const float world[16],float inverse[16]);
/* B3370 for immutable borrowed matrix/bounds/geometry, the actual ADEB0
 * provider and completing upload/draw services. Returns 0 invalid, 1 culled,
 * 2 drawn. Provider world/dirty publish even when culled; light center changes
 * only after upload113 and before upload112/draw. All borrowed storage must
 * be disjoint and remain live/stable through services. Services cannot mutate
 * these inputs/provider/center, reenter, or change the arithmetic environment.
 * Pure arithmetic is preflighted before effects; callback-visible FP flags
 * are outside this contract. Nonfinite/singular inputs fail before effects.
 * Model getter ownership, candidate traversal, physical GPU upload/draw and
 * shader/material selection remain caller services, not successful noops. */
int uc_point_light_receiver(const UCPointLightGeometry *,const float world[16],
    const UCModelBounds *,const void *receiver,const void *pass,
    const UCPointLightReceiverTarget *);
#ifdef __cplusplus
}
#endif
#endif
