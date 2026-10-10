#ifndef NV2A_TEXTURE_MIP_TOTALS_H
#define NV2A_TEXTURE_MIP_TOTALS_H
#include <stddef.h>
#include <stdint.h>
/* Experimental metadata-only counterpart to nv_texture_mips. Exponents come
 * directly from the NV097 format register. This does not certify RAM contents,
 * construct upload offsets, or cover cube/volume/embedded-border layouts. */
typedef struct NvTextureMipTotals {size_t size,host_bytes;} NvTextureMipTotals;
static uint64_t nv_mip_area_sum(unsigned u,unsigned v,unsigned count){
    unsigned low_exp=u<v?u:v,high_exp=u>v?u:v;
    unsigned square=count<low_exp+1?count:low_exp+1;
    uint64_t first=UINT64_C(1)<<(u+v);
    uint64_t sum=(4*first-(first>>(2*(square-1))))/3;
    unsigned remaining=count-square,tail=remaining<high_exp-low_exp?remaining:high_exp-low_exp;
    if(tail){
        uint64_t t=UINT64_C(1)<<(high_exp-low_exp-1);
        sum+=2*t-(t>>(tail-1));
    }
    return sum+remaining-tail;
}
static int nv_texture_mip_totals(NvTextureMipTotals*out,unsigned u,unsigned v,
    unsigned count,unsigned bpp,unsigned block){
    if(!out||u>12||v>12||!count||count>(u>v?u:v)+1)return 0;
    if(block?(block!=8&&block!=16):(bpp!=1&&bpp!=2&&bpp!=4))return 0;
    NvTextureMipTotals totals;
    totals.host_bytes=(size_t)(4*nv_mip_area_sum(u,v,count));
    totals.size=(size_t)(block?block*nv_mip_area_sum(u>2?u-2:0,v>2?v-2:0,count):
        bpp*nv_mip_area_sum(u,v,count));
    *out=totals;return 1;
}
#endif
