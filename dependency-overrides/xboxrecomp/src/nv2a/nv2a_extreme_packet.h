#ifndef NV2A_EXTREME_PACKET_H
#define NV2A_EXTREME_PACKET_H
/* Private callback-free packet owner. Returns the exact successful prefix;
 * the caller publishes progress and resumes scalar at the first failed index. */
#define NV_EXTREME_PACKET_AVAILABLE 1
#if defined(_MSC_VER)
#define NV_EXT_INLINE static __forceinline
#else
#define NV_EXT_INLINE static inline
#endif
static int nv_extreme_packet_option=-1;
static int nv_extreme_layout_option=-1;
static int nv_extreme_layout_enabled(void){
 if(nv_extreme_layout_option<0){const char*s=getenv("UCRR_PACKET_LAYOUT_KERNEL");nv_extreme_layout_option=s&&!strcmp(s,"1");}
 return nv_extreme_layout_option;
}
static int nv_extreme_packet_enabled(void){
 if(nv_extreme_packet_option<0){const char*s=getenv("UCRR_EXTREME_PACKET");nv_extreme_packet_option=s&&!strcmp(s,"1");}
 return nv_extreme_packet_option;
}
typedef struct NvExtremePacket {
 NvPackedReuse *map;
 const NvVertexSnapshotCapturePlan *plan;
 const uint8_t *source[16];
 uint8_t *next;
 unsigned count,limit,stride,misses,shared,step,last_valid,last_index,last_row;
} NvExtremePacket;
NV_EXT_INLINE void nv_extreme_copy(NvExtremePacket*x,uint32_t index,unsigned kind,int shared){
 const NvVertexSnapshotCapturePlan*c=x->plan;
 size_t step=shared?(size_t)index*x->step:0;
#define EXT_COPY(I,N) nv_vertex_snapshot_copy_padded(x->next+c->entry[I].offset,x->source[I]+(shared?step:(size_t)index*c->entry[I].stride),N)
 switch(kind){
 case 1:EXT_COPY(0,6);EXT_COPY(1,4);EXT_COPY(2,16);EXT_COPY(3,8);EXT_COPY(4,6);EXT_COPY(5,2);break;
 case 2:EXT_COPY(0,6);EXT_COPY(1,6);EXT_COPY(2,2);break;
 case 3:EXT_COPY(0,6);EXT_COPY(1,4);EXT_COPY(2,4);EXT_COPY(3,8);EXT_COPY(4,6);EXT_COPY(5,2);break;
 case 4:EXT_COPY(0,16);EXT_COPY(1,16);EXT_COPY(2,8);break;
 case 5:EXT_COPY(0,16);EXT_COPY(1,4);EXT_COPY(2,8);break;
 case 6:EXT_COPY(0,12);break;
 default:
  if(!c->count){uint32_t zero=0;memcpy(x->next,&zero,4);}
  for(unsigned i=0;i<c->count;++i)EXT_COPY(i,c->entry[i].bytes);
 }
#undef EXT_COPY
}
NV_EXT_INLINE int nv_extreme_index(NvExtremePacket*x,uint32_t index,unsigned v,
 uint16_t*order16,uint32_t*order32,int compact,unsigned kind,int shared){
 if(index>x->plan->max_index)return 0;
 unsigned row,slot;
 if(x->last_valid&&x->last_index==index)row=x->last_row;
 else {
  slot=nv_packed_reuse_slot(index);
  if(nv_packed_reuse_key(x->map,slot,index))row=x->map->lean[slot].row;
  else {
   if(x->count>=x->limit)return 0;
   row=x->count;nv_extreme_copy(x,index,kind,shared);x->next+=x->stride;++x->count;++x->misses;
   x->map->lean[slot].generation=x->map->generation;
   x->map->lean[slot].index=index;x->map->lean[slot].row=row;
  }
 }
 if(row>=MAX_INLINE_VERTS)return 0;
 if(compact)order16[v]=(uint16_t)row;else order32[v]=row;
 x->last_valid=1;x->last_index=index;x->last_row=row;return 1;
}
NV_EXT_INLINE unsigned nv_extreme_loop(NvExtremePacket*x,const uint8_t*payload,
 unsigned words,unsigned v,uint16_t*order16,uint32_t*order32,int packed16,int compact,unsigned kind,int shared){
 unsigned done=0;
 for(unsigned i=0;i<words;++i){
  uint32_t word;memcpy(&word,payload+4u*i,4);
  if(packed16){
   if(!nv_extreme_index(x,word&65535u,v+done,order16,order32,compact,kind,shared))break;
   ++done;
   if(!nv_extreme_index(x,word>>16,v+done,order16,order32,compact,kind,shared))break;
   ++done;
  }else{
   if(!nv_extreme_index(x,word,v+done,order16,order32,compact,kind,shared))break;
   ++done;
  }
 }
 return done;
}
static unsigned nv_extreme_packet(const NvVertexSnapshotCapturePlan*c,NvPackedReuse*m,
 const uint8_t*payload,unsigned words,unsigned v,int packed16,int compact,
 uint8_t*data,size_t capacity,uint16_t*order16,uint32_t*order32,unsigned*misses){
 *misses=0;
 unsigned per=packed16?2u:1u;
 if(!c->valid||!c->ram||!c->stride||c->stride>256||c->count>16||
    !v||v>=MAX_INLINE_VERTS||words>(MAX_INLINE_VERTS-v)/per)return 0;
 size_t rows=capacity/c->stride;
 unsigned limit=rows<MAX_INLINE_VERTS?(unsigned)rows:MAX_INLINE_VERTS;
 if(m->count>limit)return 0;
 NvExtremePacket x={0};x.map=m;x.plan=c;x.count=m->count;x.limit=limit;
 x.stride=c->stride;x.next=data+(size_t)m->count*c->stride;x.shared=1;
 x.step=c->count?c->entry[0].stride:0;
 for(unsigned i=0;i<c->count;++i){x.source[i]=c->ram+c->entry[i].address;
  if(c->entry[i].stride!=x.step)x.shared=0;}
 unsigned done;
 /* Select the two six-attribute shared-stride kernels once per owned packet,
  * not once per newly captured vertex. No source lifetime is extended. Keep
  * all index/map/capacity checks and the generic mixed-stride path intact. */
#define NV_LAYOUT_LOOP(K) do { \
 if(packed16){ \
  if(compact)done=nv_extreme_loop(&x,payload,words,v,order16,order32,1,1,K,1); \
  else done=nv_extreme_loop(&x,payload,words,v,order16,order32,1,0,K,1); \
 }else{ \
  if(compact)done=nv_extreme_loop(&x,payload,words,v,order16,order32,0,1,K,1); \
  else done=nv_extreme_loop(&x,payload,words,v,order16,order32,0,0,K,1); \
 } }while(0)
 if(x.shared && nv_extreme_layout_enabled() && c->copy_kind==1){NV_LAYOUT_LOOP(1);}
 else if(x.shared && nv_extreme_layout_enabled() && c->copy_kind==3){NV_LAYOUT_LOOP(3);}
 else {
 if(packed16){
  if(compact)done=nv_extreme_loop(&x,payload,words,v,order16,order32,1,1,c->copy_kind,x.shared);
  else done=nv_extreme_loop(&x,payload,words,v,order16,order32,1,0,c->copy_kind,x.shared);
 }else{
  if(compact)done=nv_extreme_loop(&x,payload,words,v,order16,order32,0,1,c->copy_kind,x.shared);
  else done=nv_extreme_loop(&x,payload,words,v,order16,order32,0,0,c->copy_kind,x.shared);
 }
 }
#undef NV_LAYOUT_LOOP
 m->count=x.count;*misses=x.misses;return done;
}
#undef NV_EXT_INLINE
#endif
