#include "shot_lightflash.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int rounding(void){return fegetround()==FE_TONEAREST&&!(_mm_getcsr()&0xE040u);}
int uc_shot_lightflash_emit(uint32_t item,const uint32_t *entity_id,const UCShotLightFlashServices *h){
    if((item!=2&&item!=17)||!entity_id||!h||!h->preflight||!h->create||
       !rounding()||!h->preflight(h->context,item,entity_id))return 1;
    const UCShotLightFlashRequest request={
        {.18f,256.f,256.f,1.f,0.f,{2.f,1.5f,1.f},{.5f,.25f,0.f},.4f,0,0},
        {-20.f,150.f,75.f},{0.f,0.f,0.f},*entity_id,2};
    h->create(h->context,&request);return 0;
}
static void compose(const float *a,const float *b,float *out){
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)
        out[r*4+c]=(b[c]*a[r*4]+b[4+c]*a[r*4+1])+
            (b[8+c]*a[r*4+2]+b[12+c]*a[r*4+3]);
    out[12]=out[13]=out[14]=0.f;out[15]=1.f;
}
int uc_shot_lightflash_place_offset(const float parent[16],const float offset[3],UCShotLightFlashPlacement *out){
    if(!parent||!offset||!out||!rounding())return 1;
    for(unsigned i=0;i<16;i++)if(!isfinite(parent[i])||fabsf(parent[i])>1e6f)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(offset[i])||fabsf(offset[i])>1e6f)return 1;
    /* Exact2D3DC0(+0,+0,+0) result, including its two FCHS negative zeros. */
    static const float zero_rotation[16]={1.f,-0.f,0.f,0.f,0.f,1.f,-0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,1.f};
    float first[16],second[16];UCShotLightFlashPlacement value;
    compose(zero_rotation,zero_rotation,first);compose(first,zero_rotation,second);
    compose(second,parent,value.transform);
    for(unsigned c=0;c<3;c++)value.transform[12+c]=
        (parent[c]*offset[0]+parent[4+c]*offset[1])+(parent[8+c]*offset[2]+parent[12+c]);
    value.transform[15]=1.f;value.flags=0x400;value.source_index=UINT32_MAX;
    memcpy(out,&value,sizeof value);return 0;
}
int uc_shot_lightflash_place(const float parent[16],UCShotLightFlashPlacement *out){
    static const float offset[3]={-20,150,75};return uc_shot_lightflash_place_offset(parent,offset,out);
}
