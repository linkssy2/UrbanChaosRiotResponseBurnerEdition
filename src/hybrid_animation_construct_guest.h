#ifndef UC_HYBRID_ANIMATION_CONSTRUCT_GUEST_H
#define UC_HYBRID_ANIMATION_CONSTRUCT_GUEST_H
/* Complete original descriptor/owner constructors. Zero means unchanged
 * fallback; success includes the original guest return. Allocation, array
 * initialization and matrix conversion callbacks remain synchronous. */
int uc_hybrid_animation_descriptor_init(void);
int uc_hybrid_animation_descriptor_copy(void);
int uc_hybrid_animation_descriptor_free(void);
int uc_hybrid_animation_construct(void);
#endif
