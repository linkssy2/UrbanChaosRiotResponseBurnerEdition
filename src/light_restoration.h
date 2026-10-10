#ifndef UCRR_LIGHT_RESTORATION_H
#define UCRR_LIGHT_RESTORATION_H
#include <stdint.h>
typedef struct UCLightRestorationCounts {
    uint64_t calls, eligible, hits, misses, forwarded, rejected, stores, evictions;
    uint64_t rejected_site, rejected_range, rejected_fp, uncacheable;
} UCLightRestorationCounts;
void uc_light_restoration_init(void);
int uc_light_restoration_enabled(void);
void uc_light_restoration_inverse(void);
void uc_light_restoration_capture_boundary(int start, uint64_t qpc);
/* Read-only diagnostic accessor; no runtime mode setter or cache reset. */
void uc_light_restoration_read_counts(UCLightRestorationCounts *out);
#endif
