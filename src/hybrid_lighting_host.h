#ifndef UC_HYBRID_LIGHTING_HOST_H
#define UC_HYBRID_LIGHTING_HOST_H
#include <stdint.h>
#include <stdio.h>
#include "hybrid_lighting_guest.h"
#include "../native/spot_light.h"
int uc_hybrid_lighting_host_apply(const UcHybridLightingScene *, const UcHybridLightingRecord *, uint32_t);
int uc_hybrid_lighting_host_spots(const UcHybridLightingScene *, const UCNativeControlSpot *, uint32_t);
void uc_hybrid_lighting_host_reject(const char *reason);
void uc_hybrid_lighting_host_capture(FILE *);
#endif
