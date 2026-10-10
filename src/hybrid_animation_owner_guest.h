#ifndef UC_HYBRID_ANIMATION_OWNER_GUEST_H
#define UC_HYBRID_ANIMATION_OWNER_GUEST_H
/* Complete1F1D60 list owner. Returns zero without effects when disabled or
 * initial owner/stack/configuration is unsupported. Success returns through
 * the complete original guest ABI; every downstream callback is retained. */
int uc_hybrid_animation_owner(void);
#endif
