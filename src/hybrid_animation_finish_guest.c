#include "hybrid_animation_finish_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/animation_blend.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
#include "hybrid_animation_finish_math.h"

enum { MIN_BONES=24,MAX_BONES=256,MAX_NODES=2048 };
static unsigned char*view(uint32_t p,unsigned bytes){
 return p>=0x10000u&&(uint64_t)p+bytes<=g_xbox_total_ram?(unsigned char*)XBOX_PTR(p):NULL;
}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return (uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int readable(uint32_t p,unsigned bytes,uint32_t records,unsigned record_bytes,uint32_t stack){
 return view(p,bytes)&&apart(p,bytes,records,record_bytes)&&apart(p,bytes,stack-304,324);
}
static int finite(uint32_t p,unsigned bytes){
 for(unsigned i=0;i<bytes;i+=4)if((MEM32(p+i)&0x7fffffffu)>=0x7f800000u)return 0;return 1;
}
static int finish(uint32_t state,uint32_t stack,uint32_t*first){
 if((stack&3)||stack<0x10130u||!view(stack-304,324)||!view(state,40))return 0;
 unsigned n=MEM32(state+36);if(n<MIN_BONES||n>MAX_BONES)return 0;
 uint32_t records=MEM32(state+32),bytes=n*48;
 if(!view(records,bytes)||!apart(records,bytes,stack-304,324)||!readable(state,40,records,bytes,stack))return 0;
 /* No cache: the cheap head scan declines sparse work before pose projection,
  * table comparison or linked-node reads. Thresholds are fixture-qualified. */
 unsigned populated=0;for(unsigned i=0;i<n&&populated<4;i++)populated+=MEM32(records+i*48+44)!=0;
 if(populated<4)return 0;
 if(!readable(0x51c158,12,records,bytes,stack)||!readable(0x51c178,1,records,bytes,stack)||MEM8(0x51c178))return 0;
 const uint32_t constants[][2]={{0x44bf94,0x3f800000},{0x44d024,0x3c23d70a},{0x456e28,0x427c0000}};
 for(unsigned i=0;i<3;i++)if(!readable(constants[i][0],4,records,bytes,stack)||MEM32(constants[i][0])!=constants[i][1])return 0;
 if(!readable(0x492dac,16392,records,bytes,stack)||MEM32(0x492dac)!=0x40490fdb||MEM32(0x496db0)||
    memcmp(view(0x492db0,16384),animation_finish_math,16384))return 0;
 uint32_t pool=MEM32(0x51c160),cursor=MEM32(0x51c15c);
 if(!readable(pool,0x18000,records,bytes,stack)||cursor<pool||cursor>pool+0x18000||(cursor-pool)%48)return 0;
 UCAnimationContribution nodes[MAX_NODES];UCAnimationKey output[MAX_BONES];unsigned nested[MAX_BONES],last[4];
 for(unsigned i=0;i<4;i++)last[i]=UINT32_MAX;
 unsigned used=0;
 for(unsigned bone=0;bone<n;bone++){
  uint32_t record=records+bone*48,node=MEM32(record+44);UCAnimationBlend b={0};
  /* Empty/copy-only chains may retain unread nonfinite prior pose bytes. */
  if(!finite(record+32,4))return 0;
  memcpy(&b.pose,view(record,28),28);b.accumulated=MEMF(record+32);if(b.accumulated<0||b.accumulated>1)return 0;
  unsigned start=used;
  while(node){
   if(used==MAX_NODES||node<cursor||node>=pool+0x18000||(node-pool)%48||!finite(node,36))return 0;
   UCAnimationContribution*c=nodes+used++;memcpy(c,view(node,36),36);if(c->weight<0||c->factor<0)return 0;
   node=MEM32(node+36);
  }
  b.contributions=nodes+start;b.count=b.capacity=used-start;
  for(size_t i=0;i<b.count/2;i++){UCAnimationContribution t=b.contributions[i];b.contributions[i]=b.contributions[b.count-1-i];b.contributions[b.count-1-i]=t;}
  UCAnimationBlendPath path;
  if(uc_animation_blend_finish_path(&b,(const UCAnimationBlendMath*)view(0x492db0,16384),&path)!=UC_ANIMATION_OK)return 0;
  output[bone]=b.pose;nested[bone]=path.last_interpolation;
  for(unsigned i=0;i<4;i++)if(path.flags&(1u<<i))last[i]=bone;
 }
 /* Retain the last common frame and last copy/interpolation/negative/table
  * writers. The unchanged suffix reconstructs every retired stack/CPU effect. */
 unsigned cut=n-1;for(unsigned i=0;i<4;i++)if(last[i]!=UINT32_MAX&&last[i]<cut)cut=last[i];
 if(!cut)return 0;
 for(unsigned i=0;i<cut;i++){
  memcpy(view(records+i*48,28),output+i,28);MEM32(records+i*48+32)=0;MEM32(records+i*48+44)=0;
 }
 uint32_t frame=stack-8,body=(frame&~15u)-80;
 g_ebp=nested[cut-1]?body-124:frame;*first=cut;return 1;
}
int uc_hybrid_animation_finish(uint32_t state,uint32_t stack,uint32_t*first){
 static int enabled=-1;
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_ANIMATION_FINISH");enabled=v&&!strcmp(v,"1")&&!getenv("UCRR_CONSTANT_SOURCE_TRACE");}
 /* Worker-disabled admission is a configuration check, not synchronization. */
 unsigned csr=_mm_getcsr();if(!enabled||!first||state!=g_esi||stack!=g_esp||(csr&~63u)!=0x1f80u)return 0;
 int ok=finish(state,stack,first);if(!ok)_mm_setcsr(csr);return ok;
}
