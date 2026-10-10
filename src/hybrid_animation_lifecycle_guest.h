#ifndef UC_HYBRID_ANIMATION_LIFECYCLE_GUEST_H
#define UC_HYBRID_ANIMATION_LIFECYCLE_GUEST_H
/* Complete original list insertion, clear and owner destruction. Zero declines
 * unchanged; nonzero has completed the original ABI and every live callback. */
int uc_hybrid_animation_insert(void);
int uc_hybrid_animation_clear(void);
int uc_hybrid_animation_destroy(void);
#endif
