#ifndef UC_HYBRID_DEBRIS_OWNER_GUEST_H
#define UC_HYBRID_DEBRIS_OWNER_GUEST_H
#include <stdint.h>
/* Complete debris manager/record ownership and rendering-data methods.
 * Source-staged; admission precedes every effect. Completing original service
 * callbacks retain their lifetime, alias and valid-memory preconditions. */
int uc_hybrid_debris_owner(uint32_t entry);
/* Also used by the byte-corrected generated481B0 fallback in this candidate. */
float uc_hybrid_debris_ordered_add(float a,float b);
float uc_hybrid_debris_ordered_mul(float a,float b);
#endif
