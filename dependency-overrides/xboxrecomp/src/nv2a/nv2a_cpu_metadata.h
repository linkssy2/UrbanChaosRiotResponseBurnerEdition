#ifndef NV_CPU_METADATA_H
#define NV_CPU_METADATA_H
/* These reviewed methods only update pixel/constant state or delimit draws. All unknown
 * methods invalidate. Guest writes and drain/reentry boundaries invalidate too. */
static int nv_cpu_metadata_method(unsigned m){
 /* Fetch plans contain array formats/addresses, validated DMA ranges and
  * immediate attribute defaults only. These reviewed pixel/raster stores do
  * not change any of those inputs. Preserve the descriptor, never captured
  * vertices: the independent attribute epoch still advances as before. */
 if(m&3u)return 0;
 if((m>=NV097_SET_COMBINER_ALPHA_ICW&&m<NV097_SET_COMBINER_ALPHA_ICW+32)||
    (m>=NV097_SET_COMBINER_FACTOR0&&m<NV097_SET_COMBINER_COLOR_ICW+32)||
    (m>=NV097_SET_COMBINER_COLOR_OCW&&m<NV097_SET_COMBINER_COLOR_OCW+32))return 1;
 if(m>=NV097_SET_TEXTURE_OFFSET&&m<NV097_SET_TEXTURE_OFFSET+0x100){
  unsigned stage=(m-NV097_SET_TEXTURE_OFFSET)/0x40;
  switch(m-stage*0x40){
   case NV097_SET_TEXTURE_OFFSET:case NV097_SET_TEXTURE_FORMAT:
   case NV097_SET_TEXTURE_ADDRESS:case NV097_SET_TEXTURE_CONTROL0:
   case NV097_SET_TEXTURE_CONTROL1:case NV097_SET_TEXTURE_FILTER:
   case NV097_SET_TEXTURE_IMAGE_RECT:return 1;
  }
 }
 /* Constant values/load cursor do not participate in fetch-plan construction.
  * Preserve descriptors, NOT vertex contents or shader outputs. Aligned methods
  * only; array/DMA/default-attribute and unknown methods still invalidate. */
 if(!(m&3u)&&((m>=NV097_SET_TRANSFORM_CONSTANT&&m<NV097_SET_TRANSFORM_CONSTANT+0x80u)||
    (m>=0xA20u&&m<=0xA2Cu)||(m>=0xAF0u&&m<=0xAFCu)))return 1;
 switch(m){
 case NV097_SET_COMBINER_SPECULAR_FOG_CW0:case NV097_SET_COMBINER_SPECULAR_FOG_CW1:
 case NV097_SET_COMBINER_CONTROL:case NV097_SET_CONTROL0:
 case NV097_SET_STENCIL_TEST_ENABLE:case NV097_SET_BLEND_COLOR:case NV097_SET_BLEND_EQUATION:
 case NV097_SET_STENCIL_MASK:case NV097_SET_STENCIL_FUNC:case NV097_SET_STENCIL_FUNC_REF:
 case NV097_SET_STENCIL_FUNC_MASK:case NV097_SET_STENCIL_OP_FAIL:
 case NV097_SET_STENCIL_OP_ZFAIL:case NV097_SET_STENCIL_OP_ZPASS:
 case NV097_SET_POLYGON_OFFSET_SCALE_FACTOR:case NV097_SET_POLYGON_OFFSET_BIAS:
 case NV097_SET_FRONT_POLYGON_MODE:case NV097_SET_BACK_POLYGON_MODE:
 case NV097_SET_CULL_FACE:case NV097_SET_FRONT_FACE:case NV097_SET_CULL_FACE_ENABLE:
 case NV097_SET_LOGIC_OP_ENABLE:case NV097_SET_LOGIC_OP:
 case NV097_SET_SHADER_STAGE_PROGRAM:return 1;
 case NV097_SET_TRANSFORM_CONSTANT_LOAD:
 case NV097_SET_BEGIN_END:
 case NV097_SET_ALPHA_TEST_ENABLE:case NV097_SET_BLEND_ENABLE:
 case NV097_SET_DEPTH_TEST_ENABLE:case NV097_SET_ALPHA_FUNC:case NV097_SET_ALPHA_REF:
 case NV097_SET_BLEND_FUNC_SFACTOR:case NV097_SET_BLEND_FUNC_DFACTOR:
 case NV097_SET_DEPTH_FUNC:case NV097_SET_DEPTH_MASK:case NV097_SET_COLOR_MASK:return 1;
 default:return 0;
 }
}
static int nv_cpu_metadata_enabled(void){
 static int enabled=-1;
 if(enabled<0){const char*s=getenv("UCRR_CPU_METADATA");enabled=s&&!strcmp(s,"1");}
 return enabled;
}
static int nv_metadata_invalid_skip_enabled(void){
 static int enabled=-1;
 if(enabled<0){const char*s=getenv("UCRR_METADATA_INVALID_SKIP");enabled=s&&s[0]=='1'&&!s[1];}
 return enabled;
}
/* Descriptor-only retention. A nonzero NO_OPERATION is a software callback,
 * not padding: it must take the original invalidation path. Vertex contents
 * still invalidate independently, and guest reentry still clears everything. */
static int nv_cpu_metadata_preserve(unsigned method,unsigned param){
 return nv_cpu_metadata_enabled() &&
     ((method==NV097_NO_OPERATION && param==0) || nv_cpu_metadata_method(method));
}
#endif
