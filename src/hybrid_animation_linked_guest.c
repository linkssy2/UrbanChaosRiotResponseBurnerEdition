#include "hybrid_animation_linked_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/model_pose.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,unsigned n){return p>=0x10000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int separate(uint32_t a,unsigned n,uint32_t b,unsigned m){return(uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int readspan(uint32_t p,unsigned n,uint32_t out,unsigned bytes,uint32_t low,unsigned stackbytes){return span(p,n)&&separate(p,n,out,bytes)&&separate(p,n,low,stackbytes);}
static int finitebits(const void*p,unsigned bytes){for(unsigned i=0;i<bytes;i+=4){uint32_t v;memcpy(&v,(const unsigned char*)p+i,4);if((v&0x7fffffff)>=0x7f800000)return 0;}return 1;}
int uc_hybrid_animation_linked_enabled(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_ANIMATION_LINKED");enabled=v&&!strcmp(v,"1")&&!getenv("UCRR_CONSTANT_SOURCE_TRACE");}
 if(!enabled||!span(g_ecx,0x70))return 0;uint32_t h=MEM32(g_ecx+0x68);if(!span(h,0x44))return 0;
 uint32_t n=MEM32(h+12);return n>=4&&n<=UC_ANIMATION_LINKED_CAP;
}
static int stage(UCAnimationLinkedStage*q){
 uint32_t s=g_esp,m=g_ecx;if((s&3)||!span(s,16)||!span(m,0x70)||MEM32(s+8))return 0;
 uint32_t h=MEM32(m+0x68);if(!span(h,0x44))return 0;
 uint32_t n=MEM32(h+12);if(n<4||n>UC_ANIMATION_LINKED_CAP||s<0x10000+n*112)return 0;
 /* Each true sibling recursion consumes at most112 guest bytes. Child edges
  * stay in the same original frame. Exclude all active/retired frames, return
  * arguments and outputs from the complete dynamic source read spans. */
 uint32_t low=s-n*112,stackbytes=n*112+16,out=MEM32(s+4),bytes=n*64,root=MEM32(s+12),local=MEM32(m+0x6c),meta=MEM32(h+0x40);
 if(!span(out,bytes)||!span(low,stackbytes)||!separate(out,bytes,low,stackbytes)||
 !readspan(m,0x70,out,bytes,low,stackbytes)||!readspan(h,0x44,out,bytes,low,stackbytes)||
 !readspan(root,64,out,bytes,low,stackbytes)||!readspan(local,bytes,out,bytes,low,stackbytes)||
 !readspan(meta,n*160,out,bytes,low,stackbytes)||!readspan(0x51c178,1,out,bytes,low,stackbytes)||MEM8(0x51c178))return 0;
 /* The pinned title disables guest workers. This is a configuration guard,
  * not a lock or proof that a previously created worker has exited. Borrowed
  * inputs must remain stable for this synchronous callback-free call. */
 UCModel model={0};UCModelBone bones[UC_ANIMATION_LINKED_CAP];
 UCModelPoseVisit work[UC_ANIMATION_LINKED_CAP*2];uint8_t seen[UC_ANIMATION_LINKED_CAP];
 model.bone_count=n;model.bones=bones;
 for(unsigned i=0;i<n;i++){bones[i].first_child=MEM16(meta+i*160+24);bones[i].next_sibling=MEM16(meta+i*160+26);}
 if(uc_model_pose_world_linked_borrowed(&model,(const float(*)[16])XBOX_PTR(local),(const float*)XBOX_PTR(root),q->world,work,seen,UC_ANIMATION_LINKED_CAP,&q->count)!=UC_POSE_OK||q->count<4)return 0;
 for(unsigned i=0;i<q->count;i++)if(!finitebits(q->world[work[i].bone],64))return 0;
 q->last=work[q->count-1].bone;return 1;
}
int uc_hybrid_animation_linked(UCAnimationLinkedStage*q){
 if(!q||!uc_hybrid_animation_linked_enabled())return 0;unsigned csr=_mm_getcsr();if((csr&~63u)!=0x1f80)return 0;
 int ok=stage(q);if(!ok)_mm_setcsr(csr);return ok;
}
