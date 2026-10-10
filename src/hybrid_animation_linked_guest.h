#ifndef UC_HYBRID_ANIMATION_LINKED_GUEST_H
#define UC_HYBRID_ANIMATION_LINKED_GUEST_H
#include <stdint.h>
enum { UC_ANIMATION_LINKED_CAP=256 };
typedef struct UCAnimationLinkedStage {
    float world[UC_ANIMATION_LINKED_CAP][16];
    uint32_t last,count;
} UCAnimationLinkedStage;
/* Cheap outer gate keeps disabled/tiny calls out of the large private frame.
 * Original B8E40 owns all recursion, integer control, and final SIMD state. */
int uc_hybrid_animation_linked_enabled(void);
int uc_hybrid_animation_linked(UCAnimationLinkedStage *);
#endif
