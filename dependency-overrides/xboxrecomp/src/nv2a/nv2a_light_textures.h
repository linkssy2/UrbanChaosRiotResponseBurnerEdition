#ifndef NV2A_LIGHT_TEXTURES_H
#define NV2A_LIGHT_TEXTURES_H
#include "nv2a_light_gpu.h"
#ifndef NV_LIGHT_COMPARE
#define NV_LIGHT_COMPARE(a,b,n,offset,format,signs) memcmp(a,b,n)
#endif
/* Optional observation only: 0=interval hit, 1=full exact comparison equal,
 * 2=exact comparison changed, 3=cold lookup attempt (not an upload success).
 * No hook may replace comparisons, alter cache validity, or read guest bytes. */
#ifndef NV_LIGHT_WITNESS
#define NV_LIGHT_WITNESS(kind,ram,ram_size,offset,format,signs,size) ((void)0)
#endif
/* Scoped single-level, border-color A8 volume / ARGB8 cube resources.
 * Mips, compressed faces and texture borders are rejected. RAM-backed only:
 * GPU-written sources require a separate ownership/coherency path. */
typedef struct NvLightTexture {uint32_t offset,format,signs;size_t size;uint8_t*source;ID3D11Resource*resource;ID3D11ShaderResourceView*view;uint64_t validated;} NvLightTexture;
typedef struct NvLightOverflow {struct NvLightOverflow*next;NvLightTexture texture;} NvLightOverflow;
typedef struct NvLightTextures {NvLightTexture entries[8];size_t bytes;int profile,interval_cache;uint64_t generation,interval_hits,hits,changes,uploads,compare_bytes,upload_bytes;const uint8_t*validated_ram;size_t validated_size;int gpu_enabled;uint64_t gpu_uploads;NvLightGPU gpu;NvLightOverflow*overflow;} NvLightTextures;
/* Entries and retained certificates keep stable addresses. Overflow metadata
 * uses the existing byte budget, with no eviction of queued resources. */
typedef struct NvLightIterator {NvLightTexture*texture,*slots;NvLightOverflow*next;unsigned index;} NvLightIterator;
static NvLightIterator nv_light_iter(NvLightTextures*c){NvLightIterator it={c->entries,c->entries,c->overflow,0};return it;}
static void nv_light_iter_next(NvLightIterator*it){
 if(++it->index<8)it->texture=&it->slots[it->index];
 else if(it->next){it->texture=&it->next->texture;it->next=it->next->next;}
 else it->texture=NULL;
}
/* Only borrow validation while the synchronous decoder owns RAM. Guest
 * reentry/mapping changes and host report/semaphore writes end that interval. */
static void nv_light_invalidate(NvLightTextures*c){
 if(++c->generation==0){for(NvLightIterator it=nv_light_iter(c);it.texture;nv_light_iter_next(&it))it.texture->validated=0;c->generation=1;}
}
/* A completed host write has a known physical RAM range. Expire only entries
 * it overlaps; unknown guest writes still require nv_light_invalidate. Zero is
 * reserved for invalid entries, including before the first ownership epoch. */
static void nv_light_written(NvLightTextures*c,const uint8_t*ram,size_t ram_size,uint32_t offset,size_t size){
 if(!size)return;
 if(!c->generation||c->validated_ram!=ram||c->validated_size!=ram_size||offset>ram_size||size>ram_size-offset){nv_light_invalidate(c);return;}
 for(NvLightIterator it=nv_light_iter(c);it.texture;nv_light_iter_next(&it)){NvLightTexture*t=it.texture;
  if(t->view&&(uint64_t)offset<(uint64_t)t->offset+t->size&&(uint64_t)t->offset<(uint64_t)offset+size)t->validated=0;
 }
}
static void nv_light_shutdown(NvLightTextures*c){
 for(NvLightIterator it=nv_light_iter(c);it.texture;nv_light_iter_next(&it)){NvLightTexture*t=it.texture;if(t->view)ID3D11ShaderResourceView_Release(t->view);if(t->resource)ID3D11Resource_Release(t->resource);free(t->source);}
 NvLightOverflow*n=c->overflow;while(n){NvLightOverflow*next=n->next;free(n);n=next;}
 nv_light_gpu_shutdown(&c->gpu);memset(c,0,sizeof(*c));
}
static size_t nv_light_morton(unsigned x,unsigned y,unsigned z,unsigned w,unsigned h,unsigned d){
 size_t result=0,place=1;for(unsigned bit=1;bit<w||bit<h||bit<d;bit<<=1){
  if(bit<w){if(x&bit)result|=place;place<<=1;}if(bit<h){if(y&bit)result|=place;place<<=1;}if(bit<d){if(z&bit)result|=place;place<<=1;}
 }return result;
}
/* NV_texture_shader documents NVIDIA signed 8-bit texture components as
 * two's-complement / 128, not biased UNORM or DXGI SNORM (/127). Decode before
 * interpolation; post-sample conversion crosses the 0x7f/0x80 discontinuity.
 * Combiner fixed-point rounding is a separate, currently float-backed contract. */
static float nv_light_component(uint8_t value,int is_signed){
 return is_signed?(float)((int)value-(value>=128?256:0))/128.0f:(float)value/255.0f;
}
static ID3D11ShaderResourceView*nv_light_texture(NvLightTextures*c,ID3D11Device*dev,const uint8_t*ram,size_t ram_size,uint32_t offset,uint32_t format,uint32_t filter){
 unsigned signs=filter>>28;
 unsigned cube=!!(format&4),dim=(format>>4)&15,fmt=(format>>8)&255;
 unsigned w=1u<<((format>>20)&15),h=1u<<((format>>24)&15),d=cube?1:1u<<((format>>28)&15);
 if(!(format&8)||((format>>16)&15)!=1 || (cube?(dim!=2||fmt!=6||w!=h||w<8):(dim!=3||fmt!=0x19)) || w>256||h>256||d>64)return NULL;
 /* Only the authored ARGB8 cube signed route is implemented here. */
 if(signs&&!cube)return NULL;
 unsigned host_pixel=signs?16:4;
 DXGI_FORMAT host_format=signs?DXGI_FORMAT_R32G32B32A32_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;
 size_t pixels=(size_t)w*h*d,source_size=pixels*(cube?24:1),host_size=pixels*(cube?6:1)*host_pixel;
 if(host_size>8u*1024*1024 || offset>ram_size || source_size>ram_size-offset)return NULL;
 if(c->validated_ram!=ram||c->validated_size!=ram_size){nv_light_invalidate(c);c->validated_ram=ram;c->validated_size=ram_size;}
 NvLightTexture*t=NULL;
 for(NvLightIterator it=nv_light_iter(c);it.texture;nv_light_iter_next(&it))if(it.texture->view&&it.texture->offset==offset&&it.texture->format==format&&it.texture->signs==signs){t=it.texture;break;}
 if(t){
  if(c->interval_cache && t->validated==c->generation){NV_LIGHT_WITNESS(0,ram,ram_size,offset,format,signs,source_size);if(c->profile)++c->interval_hits;return t->view;}
  if(c->profile)c->compare_bytes+=source_size;
  int compare_result=NV_LIGHT_COMPARE(t->source,ram+offset,source_size,offset,format,signs);
  NV_LIGHT_WITNESS(compare_result?2:1,ram,ram_size,offset,format,signs,source_size);
  if(!compare_result){if(c->profile)++c->hits;t->validated=c->generation;return t->view;}
  if(c->profile)++c->changes;
 }
 if(!t)NV_LIGHT_WITNESS(3,ram,ram_size,offset,format,signs,source_size);
 NvLightOverflow*pending=NULL;
 if(!t){
  if(c->bytes+host_size>32u*1024*1024)return NULL;
  for(unsigned i=0;i<8;++i)if(!c->entries[i].view){t=&c->entries[i];break;}
  if(!t){if(c->bytes+host_size+sizeof(*pending)>32u*1024*1024)return NULL;
   pending=(NvLightOverflow*)calloc(1,sizeof(*pending));if(!pending)return NULL;t=&pending->texture;}
 }
 uint8_t*linear=NULL,*copy=malloc(source_size);if(!copy){free(pending);return NULL;}memcpy(copy,ram+offset,source_size);
 ID3D11Resource*resource=NULL;ID3D11ShaderResourceView*view=NULL;HRESULT hr;int converted=0;
 if(c->gpu_enabled&&nv_light_gpu_convert(&c->gpu,dev,copy,source_size,w,h,d,cube,signs,&resource,&view)){
  converted=1;++c->gpu_uploads;goto publish;
 }
 linear=malloc(host_size);if(!linear){free(copy);free(pending);return NULL;}
 for(unsigned face=0;face<(cube?6u:1u);++face)for(unsigned z=0;z<d;++z)for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
  size_t from=nv_light_morton(x,y,z,w,h,d),to=((size_t)face*pixels+((size_t)z*h+y)*w+x)*host_pixel;
  if(cube){const uint8_t*p=copy+((size_t)face*pixels+from)*4;
   if(signs){float*q=(float*)(linear+to);q[0]=nv_light_component(p[2],signs&2);q[1]=nv_light_component(p[1],signs&4);q[2]=nv_light_component(p[0],signs&8);q[3]=nv_light_component(p[3],signs&1);}
   else{linear[to]=p[2];linear[to+1]=p[1];linear[to+2]=p[0];linear[to+3]=p[3];}}
  else{linear[to]=linear[to+1]=linear[to+2]=255;linear[to+3]=copy[from];}
 }
 if(signs){static unsigned samples;static int trace=-1;if(trace<0)trace=getenv("UCRR_LIGHTING_TRACE")!=NULL;
  if(trace&&samples<4){++samples;size_t center=(h/2*w+w/2)*host_pixel;const float*q=(const float*)(linear+center);
   fprintf(stderr,"[SIGNED-CUBE-UPLOAD] offset=%08X format=%08X signs=%X size=%u center_rgba=%g,%g,%g,%g\n",offset,format,signs,w,q[0],q[1],q[2],q[3]);}}
 if(cube){D3D11_TEXTURE2D_DESC td={0};td.Width=w;td.Height=h;td.MipLevels=1;td.ArraySize=6;td.Format=host_format;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;td.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;
  D3D11_SUBRESOURCE_DATA data[6]={0};for(unsigned i=0;i<6;++i){data[i].pSysMem=linear+i*pixels*host_pixel;data[i].SysMemPitch=w*host_pixel;}
  ID3D11Texture2D*tex=NULL;hr=ID3D11Device_CreateTexture2D(dev,&td,data,&tex);resource=(ID3D11Resource*)tex;
 }else{D3D11_TEXTURE3D_DESC td={0};td.Width=w;td.Height=h;td.Depth=d;td.MipLevels=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA data={linear,w*4,w*h*4};ID3D11Texture3D*tex=NULL;hr=ID3D11Device_CreateTexture3D(dev,&td,&data,&tex);resource=(ID3D11Resource*)tex;
 }
 if(SUCCEEDED(hr)){D3D11_SHADER_RESOURCE_VIEW_DESC sd={0};sd.Format=host_format;sd.ViewDimension=cube?D3D11_SRV_DIMENSION_TEXTURECUBE:D3D11_SRV_DIMENSION_TEXTURE3D;if(cube)sd.TextureCube.MipLevels=1;else sd.Texture3D.MipLevels=1;hr=ID3D11Device_CreateShaderResourceView(dev,resource,&sd,&view);}
 free(linear);if(FAILED(hr)){if(view)ID3D11ShaderResourceView_Release(view);if(resource)ID3D11Resource_Release(resource);free(copy);free(pending);return NULL;}
publish:
 if(t->view){ID3D11ShaderResourceView_Release(t->view);ID3D11Resource_Release(t->resource);free(t->source);}else c->bytes+=host_size;
 if(c->profile){++c->uploads;c->upload_bytes+=converted?source_size:host_size;}
 *t=(NvLightTexture){offset,format,signs,source_size,copy,resource,view,c->generation};
 if(pending){pending->next=c->overflow;c->overflow=pending;c->bytes+=sizeof(*pending);}
 return view;
}
#endif
