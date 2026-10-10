#ifndef NV2A_RAW_UPLOAD_LAYOUT_H
#define NV2A_RAW_UPLOAD_LAYOUT_H
#include "nv2a_raw_packets.h"

/* GPU-only directory compaction. The canonical CPU packet arena remains the
 * recovery owner. A successful plan owns its small directory and borrows only
 * the immutable payload for the immediate upload call. No mapped memory reads. */
typedef struct NvRawUploadLayout {
    uint32_t directory[NV_RAW_PACKET_LIMIT][4];
    const uint8_t *payload;
    unsigned directory_bytes,payload_bytes,bytes;
} NvRawUploadLayout;

static int nv_raw_upload_compact(const void *source,unsigned source_bytes,
    unsigned packets,unsigned vertices,NvRawUploadLayout *out){
    if(!source||!out||!packets||packets>NV_RAW_PACKET_LIMIT||!vertices||vertices>16384||
       source_bytes<NV_RAW_DIRECTORY_BYTES||source_bytes>16384u*256u||(source_bytes&3))return 0;
    const uint8_t *data=source;
    unsigned cursor=NV_RAW_DIRECTORY_BYTES,seen=0;
    unsigned directory_bytes=packets*16u,gap=NV_RAW_DIRECTORY_BYTES-directory_bytes;
    /* Validate the full canonical contiguous directory before publishing any
     * result. Unsupported/malformed public inputs retain the original uploader. */
    for(unsigned i=0;i<packets;i++){
        uint32_t e[4];NvRawStream r;
        memcpy(e,data+i*16u,sizeof(e));
        if(e[0]!=seen||e[2]!=cursor||cursor>source_bytes||source_bytes-cursor<sizeof(r)||
           e[3]!=cursor+sizeof(r))return 0;
        memcpy(&r,data+cursor,sizeof(r));
        if(!e[1]||e[1]!=r.count||e[1]>16384u-seen||r.bytes>source_bytes-e[3]||(r.bytes&3))return 0;
        cursor=e[3]+r.bytes;seen+=e[1];
    }
    if(cursor!=source_bytes||seen!=vertices)return 0;
    for(unsigned i=0;i<packets;i++){
        uint32_t e[4];memcpy(e,data+i*16u,sizeof(e));
        e[2]-=gap;e[3]-=gap;memcpy(out->directory[i],e,sizeof(e));
    }
    out->payload=data+NV_RAW_DIRECTORY_BYTES;
    out->directory_bytes=directory_bytes;
    out->payload_bytes=source_bytes-NV_RAW_DIRECTORY_BYTES;
    out->bytes=source_bytes-gap;
    return 1;
}
#endif
