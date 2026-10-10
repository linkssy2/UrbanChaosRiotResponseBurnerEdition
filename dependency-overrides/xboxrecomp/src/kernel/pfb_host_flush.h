#ifndef XBOX_PFB_HOST_FLUSH_H
#define XBOX_PFB_HOST_FLUSH_H
/* Host RAM is ordinary PAGE_READWRITE backing, not a device write-combining
 * buffer. Complete its ordering barrier, then acknowledge ONLY PFB_WBC.FLUSH.
 * This does not consume pushbuffer commands, DMA GET, or GPU semaphores. */
static int xbox_pfb_host_flush(void *aperture,size_t size,uint32_t va,int enabled)
{
    if(!enabled||va!=0xFD100410u||!aperture||size<0x100414u)return 0;
    MemoryBarrier();
    InterlockedAnd((volatile LONG*)((uint8_t*)aperture+0x100410u),~(LONG)0x10000);
    return 1;
}
#endif
