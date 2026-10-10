#pragma once
#include <stdint.h>
/* Exact 330440 plane-preparation phase, only under original 31EE30.
 * Decline preserves all guest state and resumes that local phase. */
int uc_hybrid_capsule_planes(uint32_t stack,uint32_t frame);
