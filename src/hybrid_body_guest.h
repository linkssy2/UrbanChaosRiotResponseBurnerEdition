#pragma once
#include <stdint.h>
/* Staged pure boundaries; default off, no production build connection.
 * A decline preserves CPU/RAM and resumes only the original local block. */
int uc_hybrid_body_filter(uint32_t stack);
int uc_hybrid_body_local(uint32_t state,uint32_t body,uint32_t ray,uint32_t stack);
