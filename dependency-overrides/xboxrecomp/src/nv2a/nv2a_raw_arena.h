#ifndef NV2A_RAW_ARENA_H
#define NV2A_RAW_ARENA_H
#include "nv2a_raw_packets.h"
#define NV_RAW_ARENA_BYTES (16384u*256u)
typedef struct NvRawArena { uint8_t data[NV_RAW_ARENA_BYTES]; } NvRawArena;
/* Only complete private captures can transfer ownership. Validate the entire
 * directory before publishing pointers; no guest pointer or GPU mapping is
 * borrowed. The next capture writes the former, already consumed batch arena.
 * A noncanonical/malformed directory falls back to the checked join path. */
static int nv_raw_arena_take(NvRawPackets *dst,NvRawArena **owned,
    NvRawPackets *src,NvRawArena **staging){
    if(!dst||!owned||!src||!staging||!(*owned)||!(*staging)||
       *owned==*staging||dst==src||dst->packets||dst->vertices||
       dst->bytes!=NV_RAW_DIRECTORY_BYTES||!src->packets||
       src->packets>NV_RAW_PACKET_LIMIT||src->bytes>NV_RAW_ARENA_BYTES||
       src->bytes<NV_RAW_DIRECTORY_BYTES||src->vertices>16384)return 0;
    unsigned bytes=NV_RAW_DIRECTORY_BYTES,vertices=0;
    for(unsigned i=0;i<src->packets;i++){
        uint32_t e[4];NvRawStream r;
        memcpy(e,(*staging)->data+i*16,16);
        if(e[0]!=vertices||e[2]!=bytes||bytes>src->bytes||
           src->bytes-bytes<sizeof(r)||e[3]!=bytes+sizeof(r))return 0;
        memcpy(&r,(*staging)->data+bytes,sizeof(r));
        if(!e[1]||e[1]!=r.count||e[1]>16384-vertices||
           r.bytes>src->bytes-e[3]||(r.bytes&3))return 0;
        bytes=e[3]+r.bytes;vertices+=e[1];
    }
    if(bytes!=src->bytes||vertices!=src->vertices)return 0;
    NvRawArena *free_arena=*owned;*owned=*staging;*staging=free_arena;
    *dst=*src;nv_raw_packets_begin(src);return 1;
}
#endif
