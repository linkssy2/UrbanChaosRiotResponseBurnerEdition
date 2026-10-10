#ifndef NV2A_EFFECT_ISOLATION_H
#define NV2A_EFFECT_ISOLATION_H
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef struct {uint32_t width,height,format,size;unsigned char bytes[32768];} NvEffectRef;
static struct {NvEffectRef refs[6];unsigned count,initialized,hidden,reported;uint64_t matched,skipped,protected_draws;} nv_effect;
static unsigned nv_effect_stage[4];
static int nv_effect_load(FILE*f){
 char magic[8];uint32_t count;nv_effect.count=0;
 if(!f||fread(magic,1,8,f)!=8||memcmp(magic,"FSFX0001",8)||fread(&count,4,1,f)!=1||!count||count>6)return 0;
 for(unsigned i=0;i<count;i++){
  NvEffectRef*r=&nv_effect.refs[i];
  if(fread(r,4,4,f)!=4||r->format!=15||!r->width||!r->height||!r->size||r->size>sizeof(r->bytes)||
     fread(r->bytes,1,r->size,f)!=r->size)return 0;
 }
 if(fgetc(f)!=EOF)return 0;
 nv_effect.count=count;return 1;
}
static void nv_effect_init(void){
 if(nv_effect.initialized)return;nv_effect.initialized=1;
 const char*path=getenv("UCRR_EFFECT_REFS");if(!path)return;
 FILE*f=fopen(path,"rb");int ok=nv_effect_load(f);if(f)fclose(f);
 fprintf(stderr,"[EFFECT-REFS] loaded=%d count=%u default=visible\n",ok,nv_effect.count);
}
static unsigned nv_effect_match(unsigned w,unsigned h,unsigned format,const void*bytes,size_t size){
 nv_effect_init();
 for(unsigned i=0;i<nv_effect.count;i++){
  const NvEffectRef*r=&nv_effect.refs[i];
  if(r->width!=w||r->height!=h||r->format!=format||!bytes||size>r->size)continue;
  /* The guest can bind fewer mip levels than the archive stores. Accept only
   * complete DXT5 level boundaries, and compare ALL bytes actually uploaded.
   * Never inspect beyond the validated source range or match a short prefix. */
  size_t boundary=0;unsigned mw=w,mh=h;
  for(;;){
   boundary+=(size_t)((mw+3)/4)*((mh+3)/4)*16;
   if(boundary==size){if(!memcmp(r->bytes,bytes,size))return i+1;break;}
   if(boundary>size||(mw==1&&mh==1))break;
   mw=mw>1?mw/2:1;mh=mh>1?mh/2:1;
  }
 }return 0;
}
static int nv_effect_should_skip(unsigned id,int blend,int indexed,int depth_write,int stencil,int query,unsigned shader){
 if(!id||id>nv_effect.count)return 0;
 ++nv_effect.matched;
 /* Exact assets alone don't authorize dropping query/depth/stencil work.
  * Restrict to ordinary single-stage 2D non-indexed transparent particles. */
 if(!blend||indexed||depth_write||stencil||query||shader!=1){
  if(nv_effect.protected_draws<12)fprintf(stderr,"[EFFECT-PROTECTED] id=%u blend=%d indexed=%d depth=%d stencil=%d query=%d shader=%08X\n",id,blend,indexed,depth_write,stencil,query,shader);
  ++nv_effect.protected_draws;return 0;
 }
 if(!nv_effect.hidden)return 0;
 ++nv_effect.skipped;return 1;
}
#endif
