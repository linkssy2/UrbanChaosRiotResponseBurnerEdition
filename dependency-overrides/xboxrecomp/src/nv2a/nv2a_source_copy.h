#ifndef NV2A_SOURCE_COPY_H
#define NV2A_SOURCE_COPY_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* Private CPU-owned bytes, not a guest-address immutability assertion.
 * One renderer owner. The palette is a separate, size-keyed part of identity. */
typedef struct NvSourceCopy { uint8_t *data; size_t source_size,palette_size; } NvSourceCopy;
static void nv_source_copy_release(NvSourceCopy *c,size_t *resident){
    if(c->data){*resident-=c->source_size+c->palette_size;free(c->data);}
    memset(c,0,sizeof(*c));
}
static int nv_source_copy_equal(const NvSourceCopy *c,const uint8_t *source,size_t size,
    const uint8_t *palette,size_t palette_size){
    return c->data && c->source_size==size && c->palette_size==palette_size &&
        !memcmp(c->data,source,size) && (!palette_size || !memcmp(c->data+size,palette,palette_size));
}
static int nv_source_copy_store(NvSourceCopy *c,size_t *resident,size_t limit,
    const uint8_t *source,size_t size,const uint8_t *palette,size_t palette_size){
    if(!size || !source || (palette_size&&!palette) || palette_size>SIZE_MAX-size)return 0;
    size_t total=size+palette_size,old=c->data?c->source_size+c->palette_size:0;
    if(*resident<old || total>limit || *resident-old>limit-total)return 0;
    if(!c->data || total!=old){
        uint8_t *next=malloc(total);if(!next)return 0;
        free(c->data);c->data=next;*resident=*resident-old+total;
    }
    memcpy(c->data,source,size);if(palette_size)memcpy(c->data+size,palette,palette_size);
    c->source_size=size;c->palette_size=palette_size;return 1;
}
#endif
