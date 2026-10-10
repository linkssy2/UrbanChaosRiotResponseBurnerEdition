#ifndef INDEX_NONFORWARD_BATCH_H
#define INDEX_NONFORWARD_BATCH_H
#include <stdint.h>
#include <stddef.h>
/* Same consecutive payload words as the existing scalar GET!=PUT loop.
 * A CALL/jump may place GET above the lower producer PUT. Before uint32 wrap,
 * ascending aligned addresses cannot meet that lower PUT. This proves only a
 * scalar-equivalent segment, NOT that arbitrary memory above PUT is published.
 * The caller must already own an active nonincrementing index packet and keep
 * the existing synchronous renderer admission; no guest lifetime is extended.
 * Deliberately leave the wrapping final dword to scalar handling. */
static uint32_t index_nonforward_words(uint32_t get,uint32_t put,size_t ram_size,
                                      uint32_t remaining,uint32_t budget)
{
    if(get<=put || (get&3u) || (put&3u) || ram_size<4 ||
       (size_t)get>ram_size-4)return 0;
    uint32_t count=remaining;
    size_t in_ram=(ram_size-get)/4u;
    uint32_t before_wrap=(UINT32_MAX-get)/4u;
    if(count>in_ram)count=(uint32_t)in_ram;
    if(count>before_wrap)count=before_wrap;
    if(count>budget)count=budget;
    return count;
}
#endif
