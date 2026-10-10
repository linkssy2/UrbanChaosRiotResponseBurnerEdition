#ifndef NV_STATE_BLOCK_IMPL_H
#define NV_STATE_BLOCK_IMPL_H
#include "nv2a_state_block.h"
#include "nv2a_preparation_dirty.h"
#include <stddef.h>
static int nv_state_options=-1;
static unsigned nv_state_flags(void){
 if(nv_state_options<0){nv_state_options=0;const char*s=getenv("UCRR_STATE_WIDE");if(s&&!strcmp(s,"1"))nv_state_options|=1;
  s=getenv("UCRR_STATE_SEQUENCE");if(s&&!strcmp(s,"1"))nv_state_options|=2;
  s=getenv("UCRR_STATE_CONSTANTS");if(s&&!strcmp(s,"1"))nv_state_options|=4;}
 return (unsigned)nv_state_options;
}
/* Only compare when an owned draw is actually pending. The renderer supplies
 * this expression; standalone users keep the original mandatory flush path. */
#ifndef NV_STATE_BATCH_PENDING
#define NV_STATE_BATCH_PENDING() 0
#endif
static int nv_equal_state_batch_option=-1;
static int nv_equal_state_batch_enabled(void){
 if(nv_equal_state_batch_option<0){const char*s=getenv("UCRR_EQUAL_STATE_BATCH");nv_equal_state_batch_option=s&&!strcmp(s,"1");}
 return nv_equal_state_batch_option;
}
/* Raw trace bits, normalized booleans, and texture enable side fields are all
 * observable consumers. Equal raw values alone do not repair stale mirrors. */
static int nv_state_value_equal(const uint32_t*trace,const void*destination,
 const int*enabled,unsigned boolean,uint32_t value){
 if(*trace!=value||(enabled&&*enabled!=(int)((value>>30)&1)))return 0;
 if(destination){uint32_t current;memcpy(&current,destination,4);
  if(current!=(boolean?!!value:value))return 0;}
 return 1;
}
/* Cache only a header's interpretation and stable host field destinations.
 * Never cache guest payload/address or assume unchanged guest memory. Called
 * solely by the synchronous decoder with diagnostics inactive. */
typedef struct NvStateWrite {void *destination;int *enabled;unsigned method,boolean,ignored;} NvStateWrite;
typedef struct NvStatePlan {uint32_t header;unsigned valid,count,sub,inc,preserve,flags,kind,ignored;NvStateWrite write[32];} NvStatePlan;
static NvStatePlan nv_state_plans[64];
typedef struct NvStateSequence {uintptr_t address;unsigned count,flags,seen;NvStatePlan plan[32];} NvStateSequence;
static NvStateSequence nv_state_sequences[16];
static int nv_bundle_state_option=-1,nv_bundle_arrays_option=-1;
static int nv_bundle_state_enabled(void){
 if(nv_bundle_state_option<0){const char*s=getenv("UCRR_CPU_BUNDLE_STATE");nv_bundle_state_option=s&&!strcmp(s,"1");}
 return nv_bundle_state_option;
}
static int nv_bundle_arrays_enabled(void){
 if(nv_bundle_arrays_option<0){const char*s=getenv("UCRR_CPU_BUNDLE_ARRAYS");nv_bundle_arrays_option=s&&!strcmp(s,"1");}
 return nv_bundle_arrays_option;
}
static int nv_bundle_array_method(unsigned m){
 return !(m&3)&&((m>=NV097_SET_VERTEX_DATA_ARRAY_FORMAT&&m<NV097_SET_VERTEX_DATA_ARRAY_FORMAT+64)||
                (m>=NV097_SET_VERTEX_DATA_ARRAY_OFFSET&&m<NV097_SET_VERTEX_DATA_ARRAY_OFFSET+64));
}
/* Descriptor equality, never a certificate for vertex contents. */
static int nv_bundle_array_equal(int sub,unsigned m,uint32_t value){
 if(!vertex_fetch_plan.valid||!nv_bundle_arrays_enabled()||!nv_cpu_metadata_enabled()||
    !g_pg.initialized||!ordered_gpu_enabled()||sub<0||sub>=8||object_classes[sub]!=0x97||!nv_bundle_array_method(m))return 0;
 if(m>=NV097_SET_VERTEX_DATA_ARRAY_FORMAT&&m<NV097_SET_VERTEX_DATA_ARRAY_FORMAT+64)
  return g_pg.array_format[(m-NV097_SET_VERTEX_DATA_ARRAY_FORMAT)/4]==value;
 return g_pg.array_offset[(m-NV097_SET_VERTEX_DATA_ARRAY_OFFSET)/4]==value;
}
static int nv_state_trace_only(unsigned m){
 /* Explicit registers whose scalar path only stores trace state then marks
  * ignored. First-use diagnostics still run through the original handler. */
 if((m>=NV097_SET_COMBINER_ALPHA_ICW&&m<NV097_SET_COMBINER_ALPHA_ICW+32)||
    (m>=NV097_SET_COMBINER_FACTOR0&&m<NV097_SET_COMBINER_COLOR_ICW+32)||
    (m>=NV097_SET_COMBINER_COLOR_OCW&&m<NV097_SET_COMBINER_COLOR_OCW+32))return 1;
 switch(m){
 case NV097_SET_COMBINER_SPECULAR_FOG_CW0:case NV097_SET_COMBINER_SPECULAR_FOG_CW1:
 case NV097_SET_COMBINER_CONTROL:case NV097_SET_CONTROL0:case NV097_SET_STENCIL_TEST_ENABLE:
 case NV097_SET_BLEND_COLOR:case NV097_SET_BLEND_EQUATION:case NV097_SET_DEPTH_FUNC:case NV097_SET_DEPTH_MASK:
 case NV097_SET_STENCIL_MASK:case NV097_SET_STENCIL_FUNC:case NV097_SET_STENCIL_FUNC_REF:case NV097_SET_STENCIL_FUNC_MASK:
 case NV097_SET_STENCIL_OP_FAIL:case NV097_SET_STENCIL_OP_ZFAIL:case NV097_SET_STENCIL_OP_ZPASS:
 case NV097_SET_POLYGON_OFFSET_SCALE_FACTOR:case NV097_SET_POLYGON_OFFSET_BIAS:
 case NV097_SET_FRONT_POLYGON_MODE:case NV097_SET_BACK_POLYGON_MODE:case NV097_SET_CULL_FACE:case NV097_SET_FRONT_FACE:
 case NV097_SET_LOGIC_OP_ENABLE:case NV097_SET_LOGIC_OP:return 1;
 default:return 0;
 }
}
static int nv_state_destination(unsigned m,NvStateWrite*w){
 memset(w,0,sizeof(*w));w->method=m;
 if((nv_state_flags()&1)&&nv_state_trace_only(m)){w->ignored=1;return 1;}
#define STATE_FIELD(M,F,B) case M:w->destination=&g_pg.F;w->boolean=B;return 1
 switch(m){
 STATE_FIELD(NV097_SET_DEPTH_TEST_ENABLE,depth_test,1);
 STATE_FIELD(NV097_SET_BLEND_ENABLE,blend_enable,1);
 STATE_FIELD(NV097_SET_BLEND_FUNC_SFACTOR,blend_sfactor,0);
 STATE_FIELD(NV097_SET_BLEND_FUNC_DFACTOR,blend_dfactor,0);
 STATE_FIELD(NV097_SET_CULL_FACE_ENABLE,cull_enable,1);
 STATE_FIELD(NV097_SET_ALPHA_TEST_ENABLE,alpha_test,1);
 STATE_FIELD(NV097_SET_ALPHA_FUNC,alpha_func,0);
 STATE_FIELD(NV097_SET_ALPHA_REF,alpha_ref,0);
 STATE_FIELD(NV097_SET_COLOR_MASK,color_mask,0);
 STATE_FIELD(NV097_SET_COLOR_CLEAR_VALUE,clear_color,0);
 STATE_FIELD(NV097_SET_CLEAR_RECT_HORIZONTAL,clear_rect_h,0);
 STATE_FIELD(NV097_SET_CLEAR_RECT_VERTICAL,clear_rect_v,0);
 STATE_FIELD(NV097_SET_SURFACE_CLIP_HORIZONTAL,surface_clip_h,0);
 STATE_FIELD(NV097_SET_SURFACE_CLIP_VERTICAL,surface_clip_v,0);
 STATE_FIELD(NV097_SET_SHADER_STAGE_PROGRAM,shader_stage_program,0);
 }
#undef STATE_FIELD
 /* Viewport methods also write shader constants in the scalar prefix and may
  * reject an in-progress indexed draw. Deliberately leave them on that path. */
 if(m>=NV097_SET_TEXTURE_OFFSET&&m<NV097_SET_TEXTURE_OFFSET+0x100){
  unsigned stage=(m-NV097_SET_TEXTURE_OFFSET)/0x40;
  switch(m-stage*0x40){
   case NV097_SET_TEXTURE_OFFSET:w->destination=&g_pg.tex[stage].offset;return 1;
   case NV097_SET_TEXTURE_FORMAT:w->destination=&g_pg.tex[stage].format;return 1;
   case NV097_SET_TEXTURE_ADDRESS:w->destination=&g_pg.tex[stage].address;return 1;
   case NV097_SET_TEXTURE_CONTROL0:w->destination=&g_pg.tex[stage].control0;w->enabled=&g_pg.tex[stage].enabled;return 1;
   case NV097_SET_TEXTURE_CONTROL1:w->destination=&g_pg.tex[stage].control1;return 1;
   case NV097_SET_TEXTURE_FILTER:w->destination=&g_pg.tex[stage].filter;return 1;
   case NV097_SET_TEXTURE_IMAGE_RECT:w->destination=&g_pg.tex[stage].image_rect;return 1;
  }
 }
 if(nv_state_flags()&1){
  if(m>=NV097_SET_VERTEX_DATA_ARRAY_FORMAT&&m<NV097_SET_VERTEX_DATA_ARRAY_FORMAT+64){w->destination=&g_pg.array_format[(m-NV097_SET_VERTEX_DATA_ARRAY_FORMAT)/4];return 1;}
  if(m>=NV097_SET_VERTEX_DATA_ARRAY_OFFSET&&m<NV097_SET_VERTEX_DATA_ARRAY_OFFSET+64){w->destination=&g_pg.array_offset[(m-NV097_SET_VERTEX_DATA_ARRAY_OFFSET)/4];return 1;}
 }
 return 0;
}
static NvStatePlan *nv_state_plan(uint32_t header){
 NvStatePlan*p=&nv_state_plans[(header^(header>>18)^(header>>8))&63];
 unsigned flags=nv_state_flags();
 if(p->valid&&p->header==header&&p->flags==flags)return p;
 p->valid=1;p->header=header;p->count=0;p->flags=flags;p->kind=p->ignored=0;
 uint32_t kind=header&0xE0030003u;
 if(kind!=0&&kind!=0x40000000u)return p;
 unsigned count=(header>>18)&0x7ff,method=header&0x1ffc;
 if(!count||count>32)return p;
 p->sub=(header>>13)&7;p->inc=kind==0;p->preserve=1;
 if(flags&4){
  if(method==NV097_SET_TRANSFORM_CONSTANT_LOAD&&count==1){p->kind=1;p->count=1;return p;}
  if(p->inc&&method>=NV097_SET_TRANSFORM_CONSTANT&&method<NV097_SET_TRANSFORM_CONSTANT+0x80&&
     count<=(NV097_SET_TRANSFORM_CONSTANT+0x80-method)/4){p->kind=2;p->ignored=count;p->count=count;return p;}
 }
 for(unsigned i=0;i<count;++i){unsigned m=method+(p->inc?4*i:0);
  if(!nv_state_destination(m,&p->write[i]))return p;
  p->ignored+=p->write[i].ignored;
  if(!nv_cpu_metadata_method(m))p->preserve=0;
 }
 p->count=count;return p;
}
#include "nv2a_state_program.h"
NvStateBlockResult pgraph_d3d11_state_block(const uint8_t*source,unsigned words){
 NvStateBlockResult r={0};
 if(!source||words<2||!g_pg.initialized||!ordered_gpu_enabled())return r;
 /* Fail before probing program ways or rebuilding a state plan when the first
  * header cannot possibly be admitted by any state-block mode. This is a
  * syntax-only rejection, never a cached assumption about mutable payload. */
 uint32_t first_header;memcpy(&first_header,source,4);
 unsigned first_kind=first_header&0xE0030003u,first_count=(first_header>>18)&0x7ff;
 if((first_kind!=0&&first_kind!=0x40000000u)||!first_count||first_count>32||first_count>=words)return r;
 /* At most32 packets/256words. No callbacks, target operations, queries,
  * program uploads, draws or unknown methods are admitted. */
 if(words>256)words=256;
 NvStatePlan*first_plan=NULL;
 if(words>=4&&nv_state_program_enabled()){
  NvStateBlockResult program=nv_state_program_run_first(source,words,&first_plan);
  if(program.words)return program;
 }
 unsigned flags=nv_state_flags(),flushed=0;NvStateSequence*sequence=NULL;
 if(flags&2){sequence=&nv_state_sequences[((uintptr_t)source>>4)&15];
  if(sequence->address!=(uintptr_t)source||sequence->flags!=flags){sequence->address=(uintptr_t)source;sequence->flags=flags;sequence->count=0;sequence->seen=1;sequence=NULL;}
  else if(sequence->seen<2){++sequence->seen;sequence=NULL;}}
 while(words-r.words>=2&&r.packets<32){
  uint32_t header;memcpy(&header,source+4*r.words,4);
  NvStatePlan*p;
  if(sequence&&r.packets<sequence->count&&sequence->plan[r.packets].header==header)p=&sequence->plan[r.packets];
  else {if(sequence)sequence->count=r.packets;
   /* Compilation may replace this direct-mapped cache slot. Recheck its tag
    * and flags before reusing metadata; payload and class stay fresh below. */
   if(!r.words&&first_plan&&first_plan->valid&&first_plan->header==header&&first_plan->flags==flags)p=first_plan;
   else p=nv_state_plan(header);}
  if(!p->count||p->count>=words-r.words||object_classes[p->sub]!=0x97)break;
  const uint8_t*payload=source+4*(r.words+1);unsigned component=((header&0x1ffc)-NV097_SET_TRANSFORM_CONSTANT)/4&3;
  if(p->kind){
   if(g_pg.in_draw&&g_pg.indexed_draw&&gpu_vertices_active&&g_pg.inline_count)break;
   if(p->kind==2){
    if(g_pg.constant_load>=192||p->count>(192-g_pg.constant_load)*4-component)break;
    unsigned valid=1;for(unsigned i=0;i<p->count;++i){uint32_t value;memcpy(&value,payload+4*i,4);
     if((value&0x7f800000u)==0x7f800000u||!g_pg.missing_method_reported[(header&0x1ffc)/4+i]){valid=0;break;}}
    if(!valid)break;
   }
  }else if(p->ignored){unsigned valid=1;for(unsigned i=0;i<p->count;++i){NvStateWrite*w=&p->write[i];
    if(w->ignored&&!g_pg.missing_method_reported[w->method/4]){valid=0;break;}}
   if(!valid)break;
  }
  /* Each admitted pure packet has no callbacks or state reads. Keep a pending
   * group only if every authored store already agrees with all its mirrors.
   * Constants and changed/unknown state retain a pre-mutation barrier. Once a
   * barrier ran, no packet in this callback-free block can create another draw. */
  unsigned preserve=p->preserve;
  if(!preserve&&!p->kind&&vertex_fetch_plan.valid&&nv_bundle_arrays_enabled()&&nv_cpu_metadata_enabled()){
   preserve=1;for(unsigned i=0;i<p->count;i++){
    NvStateWrite*w=&p->write[i];uint32_t value;memcpy(&value,payload+4*i,4);
    if(!nv_cpu_metadata_method(w->method)&&!nv_bundle_array_equal((int)p->sub,w->method,value)){preserve=0;break;}
   }
  }
  unsigned equal=0;
  if(!flushed&&!p->kind&&nv_equal_state_batch_enabled()&&NV_STATE_BATCH_PENDING()){
   equal=1;for(unsigned i=0;i<p->count;++i){uint32_t value;NvStateWrite*w=&p->write[i];
    memcpy(&value,payload+4*i,4);
    if(!nv_state_value_equal(&g_pg.trace_methods[w->method/4],w->destination,w->enabled,w->boolean,value)){equal=0;break;}}
  }
  if(!equal&&!flushed){gameplay_batch_flush();flushed=1;}
  if(equal){/* Only stores are omitted; original accounting below still runs. */}
  else if(p->kind==1){uint32_t value;memcpy(&value,payload,4);g_pg.constant_load=value;g_pg.trace_methods[NV097_SET_TRANSFORM_CONSTANT_LOAD/4]=value;}
  else if(p->kind==2){
   memcpy(g_pg.trace_methods+(header&0x1ffc)/4,payload,4*p->count);
   float*dest=&g_pg.vertex_constants[g_pg.constant_load][component];
   if(!vertex_constant_tracking){memcpy(dest,payload,4*p->count);raw_snapshot_dirty|=2;}
   else for(unsigned i=0;i<p->count;++i){uint32_t value;unsigned at=component+i;memcpy(&value,payload+4*i,4);vertex_constant_store(&g_pg.vertex_constants[g_pg.constant_load+at/4][at%4],value);}
   g_pg.constant_load+=(component+p->count)/4;
  }else for(unsigned i=0;i<p->count;++i){uint32_t value;NvStateWrite*w=&p->write[i];
   nv_tx_dirty_range(w->method,1,1);
   memcpy(&value,payload+4*i,4);g_pg.trace_methods[w->method/4]=value;
   if(w->enabled)*w->enabled=(value>>30)&1;
   if(w->boolean)value=!!value;
   if(w->destination)memcpy(w->destination,&value,4);
  }
  /* Exact scalar epoch progression, including the skipped zero on wrap.
   * No attribute-cache lookup occurs inside a state-only packet. */
  uint32_t next=attribute_cache.epoch+p->count;
  if(next<attribute_cache.epoch){memset(attribute_cache.tags,0,sizeof(attribute_cache.tags));++next;}
  attribute_cache.epoch=next;
  if(!preserve||!nv_cpu_metadata_enabled())vertex_fetch_plan.valid=0;
  g_pg.stats.methods_handled+=p->count;r.methods+=p->count;
  g_pg.stats.methods_ignored+=p->ignored;
  if(sequence&&sequence->count==r.packets){memcpy(&sequence->plan[r.packets],p,offsetof(NvStatePlan,write)+(p->kind?0:p->count*sizeof(NvStateWrite)));++sequence->count;}
  r.words+=p->count+1;++r.packets;r.subchannel=p->sub;r.incrementing=p->inc;
  r.method=(header&0x1ffc)+(p->inc?4*p->count:0);
 }
 return r;
}
#endif
