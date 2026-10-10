#ifndef NV2A_VERTEX_FETCH_H
#define NV2A_VERTEX_FETCH_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#if defined(_MSC_VER)
#define NV_FETCH_INLINE static __forceinline
#else
#define NV_FETCH_INLINE static inline
#endif
NV_FETCH_INLINE unsigned nv2a_vertex_attribute_bytes(uint32_t format)
{
 unsigned n=(format>>4)&15,type=format&15;
 if(!n || n>4)return 0;
 switch(type){case 0:case 4:return n;case 1:case 5:return n*2;case 2:return n*4;case 6:return n==1?4:0;default:return 0;}
}
/* Element decoding only. Caller resolves DMA ownership and full byte bounds. */
NV_FETCH_INLINE int nv2a_vertex_fetch(float out[4],uint32_t format,const uint8_t *data)
{
 unsigned n=(format>>4)&15,type=format&15;
 if(!nv2a_vertex_attribute_bytes(format))return 0;
 out[0]=out[1]=out[2]=0;out[3]=1;
 /* Float attributes are bit-preserving loads, not numeric conversions. */
 if(type==2){
  /* Constant-size, unaligned-safe bit copies lower to loads/stores instead of
   * a CRT call for every small float attribute. Defaults above remain intact. */
  switch(n){case 4:memcpy(out,data,16);break;case 3:memcpy(out,data,12);break;
            case 2:memcpy(out,data,8);break;case 1:memcpy(out,data,4);break;}
  return 1;
 }
 if(type==6) {
  uint32_t bits;memcpy(&bits,data,4);
  int x=(int32_t)(bits<<21)>>21,y=(int32_t)(bits<<10)>>21,z=(int32_t)bits>>22;
  /* These are signed integers, never NaNs: only the minimum encoding lies
   * below -1. Retain division rounding, without a general fmaxf call. */
  out[0]=x < -1023 ? -1.0f : x/1023.0f;
  out[1]=y < -1023 ? -1.0f : y/1023.0f;
  out[2]=z < -511 ? -1.0f : z/511.0f;return 1;
 }
 for(unsigned i=0;i<n;++i) {
  if(type==0 || type==4)out[i]=data[i]/255.0f;
  else {int16_t v;memcpy(&v,data+i*2,2);out[i]=type==1?(v == INT16_MIN ? -1.0f : v/32767.0f):(float)v;}
 }
 if(type==0 && n>=3){float t=out[0];out[0]=out[2];out[2]=t;}
 return 1;
}
/* Inline-array attributes are packed in attribute order with element alignment.
 * Array DMA strides do not describe this interleaved stream. */
static unsigned nv2a_inline_layout(const uint32_t formats[16],unsigned offsets[16]){
 unsigned size=0;
 for(unsigned a=0;a<16;++a){
  offsets[a]=UINT32_MAX;unsigned n=(formats[a]>>4)&15;if(!n)continue;
  unsigned bytes=nv2a_vertex_attribute_bytes(formats[a]);if(!bytes)return 0;
  unsigned type=formats[a]&15,align=(type==0||type==4)?1:(type==1||type==5)?2:4;
  size=(size+align-1)&~(align-1);offsets[a]=size;size+=bytes;
 }
 return size;
}
static int nv2a_inline_fetch(float out[16][4],const float current[16][4],const uint32_t formats[16],
 const unsigned offsets[16],const uint8_t*data,size_t bytes){
 memcpy(out,current,16*4*sizeof(float));
 for(unsigned a=0;a<16;++a){if(offsets[a]==UINT32_MAX)continue;
  unsigned n=nv2a_vertex_attribute_bytes(formats[a]);if(!n || offsets[a]>bytes || n>bytes-offsets[a])return 0;
  if(!nv2a_vertex_fetch(out[a],formats[a],data+offsets[a]))return 0;
 }
 return 1;
}
/* Draw-local inline plan. No cached guest data or lifetime across commands.
 * Caller validates complete records before fetching and initializes defaults
 * once. Every enabled attribute is overwritten on every vertex. */
typedef struct {unsigned count,stride;struct {unsigned slot,offset;uint32_t format;} entry[16];} NvInlinePlan;
static int nv2a_inline_prepare(NvInlinePlan*p,const uint32_t formats[16]){
 unsigned offsets[16];p->count=0;p->stride=nv2a_inline_layout(formats,offsets);
 if(!p->stride)return 0;
 for(unsigned a=0;a<16;++a)if(offsets[a]!=UINT32_MAX){
  unsigned i=p->count++;p->entry[i].slot=a;p->entry[i].offset=offsets[a];p->entry[i].format=formats[a];
 }
 return 1;
}
static int nv2a_inline_fetch_prepared(float out[16][4],const NvInlinePlan*p,const uint8_t*data,size_t bytes){
 if(!p->stride||bytes<p->stride)return 0;
 for(unsigned i=0;i<p->count;++i)
  if(!nv2a_vertex_fetch(out[p->entry[i].slot],p->entry[i].format,data+p->entry[i].offset))return 0;
 return 1;
}
#undef NV_FETCH_INLINE
#endif
