#ifndef NV2A_RESOLUTION_H
#define NV2A_RESOLUTION_H
#include <stdint.h>
/* Sample-count enhancement policy: preserve nonzero visibility, normalize area.
 * Does not claim bit-identical edge coverage to original Xbox rasterization. */
static uint64_t nv_native_samples(uint64_t samples,unsigned scale){
    unsigned area=scale*scale;
    return samples/area+(samples%area!=0);
}
#endif
