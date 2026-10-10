#ifndef UC_HYBRID_EFFECT_QUOTA_GUEST_H
#define UC_HYBRID_EFFECT_QUOTA_GUEST_H
/* Complete persistent impact quota: construct=0, append=1, query=2, tick=3.
 * Returns zero only before effects. Original allocation remains shared. */
int uc_hybrid_effect_quota(unsigned operation);
#endif
