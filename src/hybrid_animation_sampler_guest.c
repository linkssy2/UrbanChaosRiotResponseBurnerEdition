#include "hybrid_animation_sampler_guest.h"
#include "../native/animation_asset.h"
#include "../native/animation_blend.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
typedef struct Track {UCAnimationContribution nodes[2];uint32_t record,head,count;float accumulated;} Track;
typedef struct ReadGuard {uint32_t stack,records,record_bytes,pool,pool_bytes;} ReadGuard;
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<=g_xbox_total_ram&&n<=g_xbox_total_ram-p;}
static int overlaps(uint32_t a,uint32_t n,uint32_t b,uint32_t m){return (uint64_t)a<(uint64_t)b+m&&(uint64_t)b<(uint64_t)a+n;}
static int readable(const ReadGuard*g,uint32_t p,uint32_t n){
 return span(p,n)&&!overlaps(p,n,g->stack-52,228)&&!overlaps(p,n,g->records,g->record_bytes)&&!overlaps(p,n,g->pool,g->pool_bytes)&&!overlaps(p,n,0x51c15c,4);
}
static uint32_t sample(uint32_t clip,uint32_t s,uint32_t lower){
 if((s&3)||g_fp_top<0||g_fp_top>7||!span(s-52,228)||!span(clip,0x58)||!span(0x51c158,12)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(lower!=MEM32(s+0x18)||g_ebx!=MEM32(s+0x3c)||g_xmm0.u[0]!=MEM32(s+0x28))return 0;
 uint32_t count=MEM32(clip+0x14),state=MEM32(s+0x98);
 if(count<24||count>256||!span(state,0x28))return 0;
 uint32_t bones=MEM32(state+0x24),records=MEM32(state+0x20),mask=MEM32(state+0xc),ids=MEM32(clip+0x18);
 if(!bones||bones>256||!span(records,bones*48)||!span(mask,bones)||!span(ids,count*4))return 0;
 uint32_t cursor=MEM32(0x51c15c),base=MEM32(0x51c160),potential=count*96;
 if(cursor<base||cursor-base<potential||!span(cursor-potential,potential))return 0;
 ReadGuard guard={s,records,bones*48,cursor-potential,potential};
 if(overlaps(records,bones*48,s-52,228)||overlaps(records,bones*48,guard.pool,potential)||overlaps(guard.pool,potential,s-52,228)||
    overlaps(records,bones*48,0x51c158,12)||overlaps(guard.pool,potential,0x51c158,12)||overlaps(s-52,228,0x51c158,12)||
    !readable(&guard,0x51c178,1)||!readable(&guard,clip,0x58)||!readable(&guard,state,0x28)||!readable(&guard,mask,bones)||!readable(&guard,ids,count*4))return 0;
 const uint32_t constants[][2]={{0x47faf0,0x3ab504f3},{0x47faf4,0x383504f3},{0x47fad0,0x3f3504f3},{0x44bf94,0x3f800000},{0x44c174,0}};
 for(unsigned i=0;i<5;i++)if(!readable(&guard,constants[i][0],4)||MEM32(constants[i][0])!=constants[i][1])return 0;
 const uint32_t root_track=MEM32(clip+0x54);
 uint8_t seen[256]={0};unsigned enabled=0,root_enabled=0;
 for(unsigned i=0;i<count;i++){
  int32_t id=SMEM32(ids+i*4);uint32_t bone=(uint32_t)(id<0?~id:id);
  if(bone>=bones||seen[bone])return 0;seen[bone]=1;enabled+=MEM8(mask+bone)!=0;
  if(i==root_track&&MEM8(mask+bone))root_enabled=1;
 }
 if(enabled<4)return 0;
 unsigned csr=_mm_getcsr();if((csr&~0x3fu)!=0x1f80)return 0;
 #define DECLINE() do{_mm_setcsr(csr);return 0;}while(0)
 const float fraction=MEMF(s+0x28),weight=MEMF(s+0xa0);float rootfactor=1.0f;
 /* The source only copies a used root factor. Avoid an extra denormal operand
  * status from native factor<0 validation; unused factor bytes stay unread. */
 if(root_enabled){
  uint32_t bits=MEM32(s+0xa4),magnitude=bits&0x7fffffffu;
  if(magnitude>=0x7f800000u||((bits>>31)&&magnitude)||(magnitude&&magnitude<0x800000u))DECLINE();
  memcpy(&rootfactor,&bits,4);
 }
 if(!isfinite(fraction)||fraction<0||fraction>=1||!isfinite(weight)||weight<0)DECLINE();
 const unsigned parts=fraction==0?1:2,bytes=MEM8(s+0x13)?6:4;
 uint32_t low=MEM32(s+0x14),high=MEM32(s+0x24),position=MEM32(s+0x18),next=MEM32(s+0x40),first=MEM32(s+0x20),frame=MEM32(s+0x3c);
 if((int32_t)frame<0||!readable(&guard,low,count*bytes)||(parts==2&&!readable(&guard,high,count*bytes))||!readable(&guard,first,count*12))DECLINE();
 Track tracks[256];uint32_t last_enabled=UINT32_MAX,last_hole=UINT32_MAX,last_alloc=UINT32_MAX;
 float weights[2]={weight,0};if(parts==2){weights[0]=(1.0f-fraction)*weight;weights[1]=fraction*weight;}
 for(unsigned i=0;i<count;i++){
  int32_t id=SMEM32(ids+i*4);uint32_t bone=(uint32_t)(id<0?~id:id),p0=position,p1=next;
  if(id<0&&frame>0){p0=p1=first+i*12;}else{position+=12;if(parts==2){if(id<0)p1=first+i*12;else next+=12;}}
  Track*t=tracks+i;t->record=records+bone*48;t->count=0;t->head=MEM32(t->record+44);t->accumulated=MEMF(t->record+32);
  if(!MEM8(mask+bone))continue;
  last_enabled=i;
  if(!readable(&guard,p0,12)||(parts==2&&!readable(&guard,p1,12)))DECLINE();
  UCAnimationBlend blend={0};blend.capacity=2;blend.contributions=t->nodes;blend.accumulated=t->accumulated;blend.limit=MEMF(t->record+36);blend.scale=MEMF(t->record+40);
  if(!isfinite(blend.accumulated)||!isfinite(blend.limit)||!isfinite(blend.scale)||blend.accumulated<0||blend.limit<blend.accumulated||blend.scale<0)DECLINE();
  for(unsigned j=0;j<parts;j++){
   uint32_t packed=(j?high:low)+i*bytes;UCAnimationKey key;
   if((MEM8(packed)&3)!=2)last_hole=i;
   if(uc_animation_quaternion_decode((const void*)(g_xbox_mem_offset+packed),bytes,key.rotation)!=UC_ANIMATION_OK)DECLINE();
   memcpy(key.translation,(const void*)(g_xbox_mem_offset+(j?p1:p0)),12);
   if(uc_animation_blend_add(&blend,&key,weights[j],i==root_track?rootfactor:1.0f)!=UC_ANIMATION_OK)DECLINE();
  }
  t->count=(uint32_t)blend.count;t->accumulated=blend.accumulated;
  if(t->count)last_alloc=i;
 }
 uint32_t cut=last_enabled;
 if(last_hole<cut)cut=last_hole;if(last_alloc<cut)cut=last_alloc;
 if(cut==UINT32_MAX||cut<4)DECLINE();
 /* Keep the final source writer of decoder scratch and successful pool frames.
  * Admission requires enough pool for every track, so no failed allocation
  * can introduce a separate persistent writer class. */
 position=MEM32(s+0x18);next=MEM32(s+0x40);
 for(unsigned i=0;i<cut;i++){
  Track*t=tracks+i;
  for(unsigned j=0;j<t->count;j++){
   cursor-=48;memcpy((void*)(g_xbox_mem_offset+cursor),t->nodes+j,36);MEM32(cursor+36)=t->head;t->head=cursor;
  }
  if(t->count){MEMF(t->record+32)=t->accumulated;MEM32(t->record+44)=t->head;}
  if(SMEM32(ids+i*4)>=0||!frame)position+=12;
  if(parts==2&&SMEM32(ids+i*4)>=0)next+=12;
 }
 MEM32(0x51c15c)=cursor;MEM32(s+0x14)=low+cut*bytes;MEM32(s+0x18)=position;MEM32(s+0x20)=first+cut*12;
 if(parts==2){MEM32(s+0x24)=high+cut*bytes;MEM32(s+0x40)=next;}
 return cut;
 #undef DECLINE
}

uint32_t uc_hybrid_animation_sampler(uint32_t clip,uint32_t stack,uint32_t lower){
 static int enabled=-1;
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_ANIMATION_SAMPLER");enabled=v&&!strcmp(v,"1")&&!getenv("UCRR_CONSTANT_SOURCE_TRACE");}
 if(!enabled||clip!=g_edi||stack!=g_esp)return 0;
 /* Borrowed data must stay stable during this callback-free calculation.
  * Worker-disabled admission is a configuration check, not a lock. */
 return sample(clip,stack,lower);
}
