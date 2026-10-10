#ifndef UC_HYBRID_DECAL_OWNER_GUEST_H
#define UC_HYBRID_DECAL_OWNER_GUEST_H
#include <stdint.h>
/* Source-staged only. Entry admission is effect-free; services retain live state. */
int uc_hybrid_decal_owner(uint32_t entry);
/* Narrow original-byte entry reconstruction for the otherwise unresolved
 * primary deleting entry9BB90. Integration is separate and still pending. */
void sub_0009BB90(void);
#endif
