#ifndef NV2A_PALETTE_H
#define NV2A_PALETTE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Expand already-unswizzled indices. Validate the entire row before writing. */
static int nv_palette_row(uint8_t *dst,size_t dst_bytes,const uint8_t *indices,size_t count,const uint8_t *palette,size_t palette_bytes)
{
    if(!dst||!indices||!palette||count>dst_bytes/4||palette_bytes%4||palette_bytes>1024)return 0;
    for(size_t i=0;i<count;++i)if(indices[i]>=palette_bytes/4)return 0;
    for(size_t i=0;i<count;++i)memcpy(dst+i*4,palette+(size_t)indices[i]*4,4);
    return 1;
}
#endif
