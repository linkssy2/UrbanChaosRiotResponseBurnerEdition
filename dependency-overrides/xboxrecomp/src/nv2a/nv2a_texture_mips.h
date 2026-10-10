#ifndef NV2A_TEXTURE_MIPS_H
#define NV2A_TEXTURE_MIPS_H
#include <stddef.h>
#include <stdint.h>
/* Packed, border-color 2D mip chains only. Cube face alignment, embedded
 * borders and volume layouts have different contracts. */
typedef struct NvTextureMip {unsigned width,height,pitch,rows;size_t offset,size;} NvTextureMip;
typedef struct NvTextureMips {unsigned count;size_t size,host_bytes;NvTextureMip level[13];} NvTextureMips;
static int nv_texture_mips(NvTextureMips*out,unsigned w,unsigned h,unsigned count,unsigned bpp,unsigned block){
 if(!out||!w||!h||w>4096||h>4096||(w&(w-1))||(h&(h-1))||!count||count>13)return 0;
 if(block?(block!=8&&block!=16):(bpp!=1&&bpp!=2&&bpp!=4))return 0;
 unsigned max=w>h?w:h,limit=1;while(max>1){max>>=1;++limit;}if(count>limit)return 0;
 NvTextureMips p={0};p.count=count;
 for(unsigned i=0;i<count;++i){
  NvTextureMip*m=&p.level[i];m->width=w;m->height=h;m->pitch=block?((w+3)/4)*block:w*bpp;
  m->rows=block?(h+3)/4:h;m->offset=p.size;m->size=(size_t)m->pitch*m->rows;
  p.size+=m->size;p.host_bytes+=(size_t)w*h*4;
  w=w>1?w/2:1;h=h>1?h/2:1;
 }
 *out=p;return 1;
}
#endif
