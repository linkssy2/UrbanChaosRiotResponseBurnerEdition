#ifndef NV2A_INLINE_GPU_H
#define NV2A_INLINE_GPU_H
/* Experimental owned INLINE_ARRAY forwarding. No guest/global publication here.
 * The first vertex retains CPU admission. As on the indexed GPU route, later
 * shader results are GPU-only: late invalid position behavior is not certified. */
typedef struct NvInlineGpuPrepared {
    unsigned count,upload_count,primitive;
    float final_attrs[16][4];
} NvInlineGpuPrepared;
static int nv_inline_gpu_prepare(NvInlineGpuPrepared *prepared,
    const uint32_t *data,unsigned words,const uint32_t formats[16],
    const float defaults[16][4],const uint32_t program[136][4],unsigned start,
    const float constants[192][4],unsigned mode,unsigned execution,float zmax,
    float (*vertices)[16][4],unsigned capacity)
{
    unsigned offsets[16],stride=nv2a_inline_layout(formats,offsets);
    NvInlinePlan plan;
    if(!data||!vertices||!prepared||!words||words>UINT32_MAX/4||!stride||
       (execution&3)!=2||!(zmax>0)||!isfinite(zmax)||
       (mode!=5&&mode!=6&&mode!=8)||!nv2a_inline_prepare(&plan,formats))return 0;
    unsigned bytes=words*4;
    if(bytes%stride)return 0;
    unsigned count=bytes/stride,upload_count=count;
    if(count<3||count>MAX_INLINE_VERTS)return 0;
    if(mode==8){if(count%4)return 0;upload_count=(count/4)*6;}
    if(upload_count>capacity)return 0;
    float attrs[16][4],output[16][4];unsigned written[16];
    memcpy(attrs,defaults,sizeof(attrs));
    for(unsigned v=0;v<count;++v){
        if(!nv2a_inline_fetch_prepared(attrs,&plan,(const uint8_t*)data+(size_t)v*stride,stride))return 0;
        if(!v){
            if(!nv_cpu_vertex_eval(3,program,start,constants,attrs,output,written)||written[0]!=15)return 0;
            for(unsigned k=0;k<4;++k)if(!isfinite(output[0][k]))return 0;
            if(output[0][3]==0)return 0;
        }
        if(mode==8){
            unsigned base=(v/4)*6;
            static const unsigned first[4]={0,1,2,5};
            memcpy(vertices[base+first[v%4]],attrs,sizeof(attrs));
            if(v%4==0)memcpy(vertices[base+3],attrs,sizeof(attrs));
            if(v%4==2)memcpy(vertices[base+4],attrs,sizeof(attrs));
        }else memcpy(vertices[v],attrs,sizeof(attrs));
    }
    prepared->count=count;prepared->upload_count=upload_count;
    prepared->primitive=mode==6?5:4; /* D3DPT_TRIANGLESTRIP / TRIANGLELIST */
    memcpy(prepared->final_attrs,attrs,sizeof(attrs));
    return 1;
}
#endif
