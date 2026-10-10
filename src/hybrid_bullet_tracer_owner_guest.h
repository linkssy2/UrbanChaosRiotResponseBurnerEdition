#ifndef UC_HYBRID_BULLET_TRACER_OWNER_GUEST_H
#define UC_HYBRID_BULLET_TRACER_OWNER_GUEST_H
#include <stdint.h>
/* Complete collection owners. Retained services must complete with valid
 * original lifetimes; admission never causes replay after an effect. */
int uc_hybrid_bullet_tracer_owner(uint32_t entry);
#endif
