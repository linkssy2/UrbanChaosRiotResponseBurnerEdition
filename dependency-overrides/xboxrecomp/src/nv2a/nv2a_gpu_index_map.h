#ifndef NV2A_GPU_INDEX_MAP_H
#define NV2A_GPU_INDEX_MAP_H
#include <stdint.h>
#include <string.h>
/* Candidate draw-local reuse map. A hit is allowed only when the caller also
 * proved a decoded-attribute cache hit. Never infer source immutability from
 * an index alone. Direct-map collisions append a fresh record. */
#define NV_GPU_INDEX_CAPACITY 16384u
#define NV_GPU_INDEX_SLOTS 1024u
typedef struct NvGPUIndexMap {
    uint32_t generation,count;
    struct {uint32_t generation,source_epoch,key,index;} slots[NV_GPU_INDEX_SLOTS];
} NvGPUIndexMap;
static void nv_gpu_index_begin(NvGPUIndexMap *m){
    if(++m->generation==0){memset(m->slots,0,sizeof(m->slots));m->generation=1;}
    m->count=0;
}
static int nv_gpu_index_resolve(NvGPUIndexMap *m,uint32_t key,uint32_t source_epoch,
                                int decoded_hit,uint32_t *index,int *fresh){
    unsigned slot=key&(NV_GPU_INDEX_SLOTS-1u);
    if(!m->generation)return 0;
    if(decoded_hit&&m->slots[slot].generation==m->generation&&
       m->slots[slot].source_epoch==source_epoch&&m->slots[slot].key==key){
        *index=m->slots[slot].index;*fresh=0;return 1;
    }
    if(m->count>=NV_GPU_INDEX_CAPACITY)return 0;
    *index=m->count++;*fresh=1;
    m->slots[slot].generation=m->generation;m->slots[slot].source_epoch=source_epoch;
    m->slots[slot].key=key;m->slots[slot].index=*index;return 1;
}
/* Expanded input slot v is at or above the compact prefix. Copy only a new
 * unique row; previously produced indices keep referencing owned prefix rows. */
static int nv_gpu_index_stage(NvGPUIndexMap *m,uint32_t key,uint32_t source_epoch,
                              int decoded_hit,const float attrs[16][4],
                              float compact[NV_GPU_INDEX_CAPACITY][16][4],
                              uint32_t indices[NV_GPU_INDEX_CAPACITY],unsigned v){
    uint32_t index;int fresh;
    if(v>=NV_GPU_INDEX_CAPACITY||!attrs||!compact||!indices)return 0;
    if(!nv_gpu_index_resolve(m,key,source_epoch,decoded_hit,&index,&fresh))return 0;
    if(fresh&&attrs!=compact[index])memcpy(compact[index],attrs,sizeof(compact[index]));
    indices[v]=index;return 1;
}
#endif
