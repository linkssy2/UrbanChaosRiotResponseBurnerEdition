#ifndef NV2A_MIP_UPLOAD_H
#define NV2A_MIP_UPLOAD_H
#include "nv2a_texture_mips.h"
#include "nv2a_palette.h"
/* Remaining levels of validated 2D DXT or P8 chains. Base level continues
 * through the existing texture interface. No guest-memory reads by the GPU. */
static int nv_mip_upload(ID3D11DeviceContext*ctx,ID3D11Texture2D*texture,
 const uint8_t*source,size_t source_size,const NvTextureMips*plan,unsigned level,
 unsigned block,const uint8_t*palette,size_t palette_size){
 if(!ctx||!texture||!source||!plan||level>=plan->count||plan->count>13)return 0;
 const NvTextureMip*m=&plan->level[level];
 if(m->offset>source_size||m->size>source_size-m->offset||!m->width||!m->height||m->width>4096||m->height>4096)return 0;
 if(block){
  if((block!=8&&block!=16)||m->pitch!=(m->width+3)/4*block||m->rows!=(m->height+3)/4||m->size!=(size_t)m->pitch*m->rows)return 0;
  ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)texture,level,NULL,source+m->offset,m->pitch,(UINT)m->size);return 1;
 }
 if(m->width>4096||m->height>4096||(m->width&(m->width-1))||(m->height&(m->height-1))||m->size!=(size_t)m->width*m->height)return 0;
 size_t pixels=(size_t)m->width*m->height;uint8_t*linear=malloc(pixels),*rgba=malloc(pixels*4);
 if(!linear||!rgba){free(linear);free(rgba);return 0;}
 for(unsigned y=0;y<m->height;++y)for(unsigned x=0;x<m->width;++x){
  size_t index=0,place=1;for(unsigned bit=1;bit<m->width||bit<m->height;bit<<=1){
   if(bit<m->width){if(x&bit)index|=place;place<<=1;}if(bit<m->height){if(y&bit)index|=place;place<<=1;}}
  linear[(size_t)y*m->width+x]=source[m->offset+index];
 }
 int ok=nv_palette_row(rgba,pixels*4,linear,pixels,palette,palette_size);
 if(ok)ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)texture,level,NULL,rgba,m->width*4,(UINT)(pixels*4));
 free(linear);free(rgba);return ok;
}
#endif
