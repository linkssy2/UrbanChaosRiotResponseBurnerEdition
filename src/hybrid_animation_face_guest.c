#include "hybrid_animation_face_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/character_face.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,unsigned n){return p>=0x10000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return(uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int finitebits(const void*p,unsigned bytes){for(unsigned i=0;i<bytes;i+=4){uint32_t v;memcpy(&v,(const unsigned char*)p+i,4);if((v&0x7fffffffu)>=0x7f800000u)return 0;}return 1;}
static uint32_t stage(uint32_t node,uint32_t stack,uint32_t frame){
 if(stack!=g_esp||frame!=g_ebp||node!=g_eax||g_ebx!=node+20||(stack&15)||(frame&3)||
    !span(node,20)||!span(frame,24)||stack+240!=(frame&~15u))return 0;
 uint32_t n=MEM32(node+16),local=MEM32(frame+12);
 if(n<2||n>24||g_ecx!=n||!span(node,20+n*28)||!span(stack,24)||MEM32(stack+20)!=node||MEM32(stack+16))return 0;
 uint32_t cut=0,active=0,highest=0;
 for(unsigned i=0;i<n;i++){int32_t bone=(int32_t)MEM32(node+20+i*28);if(bone<-1||bone>=256)return 0;if(bone>=0){cut=i;active++;if((uint32_t)bone>highest)highest=(uint32_t)bone;}}
 /* Decide sparse/empty fallback before projecting records or copying poses. */
 if(active<2||!cut||stack<0x10000+112)return 0;
 uint32_t low=stack-112,stackbytes=frame+24-low,bytes=(highest+1)*64;
 if(!span(low,stackbytes)||!span(local,bytes)||!apart(local,bytes,low,stackbytes)||
 !apart(node,20+n*28,local,bytes)||!apart(node,20+n*28,low,stackbytes)||
 !span(0x44bf94,4)||MEM32(0x44bf94)!=0x3f800000||!apart(0x44bf94,4,local,bytes)||!apart(0x44bf94,4,low,stackbytes)||
 !span(0x51e110,64)||!apart(0x51e110,64,local,bytes)||!apart(0x51e110,64,low,stackbytes)||
 !span(0x51c178,1)||MEM8(0x51c178)||!apart(0x51c178,1,local,bytes)||!apart(0x51c178,1,low,stackbytes))return 0;
 for(unsigned i=0;i<16;i++)if(MEM32(0x51e110+i*4)!=(i%5==0?0x3f800000u:0))return 0;
 /* Borrowed inputs stay stable during this synchronous pure call. The pinned
  * title's disabled-worker flag is a configuration guard, not a lock. The
  * bounded private map preserves sequential feedback for repeated bone IDs. */
 UCCharacterFacePose records[24];float output[24][16];uint32_t bone_ids[24],count=0;
 for(unsigned i=0;i<cut;i++){
  uint32_t at=node+20+i*28;int32_t bone=(int32_t)MEM32(at);
  /* Source copies but never interprets the six unused words of absent bones. */
  if(bone==-1){records[i]=(UCCharacterFacePose){-1,{0},{0}};continue;}
  if(!finitebits((const void*)XBOX_PTR(at+4),24))return 0;
  uint32_t mapped=0;while(mapped<count&&bone_ids[mapped]!=(uint32_t)bone)mapped++;
  if(mapped==count){bone_ids[count++]=(uint32_t)bone;memcpy(output[mapped],(const void*)XBOX_PTR(local+(uint32_t)bone*64),64);if(!finitebits(output[mapped],64))return 0;}
  memcpy(&records[i],(const void*)XBOX_PTR(at),28);records[i].bone=(int32_t)mapped;
 }
 if(uc_character_face_apply(output,count,records,cut))return 0;
 for(unsigned i=0;i<count;i++)if(!finitebits(output[i],64))return 0;
 for(unsigned i=0;i<count;i++)memcpy((void*)XBOX_PTR(local+bone_ids[i]*64),output[i],64);
 return cut;
}
uint32_t uc_hybrid_animation_face(uint32_t node,uint32_t stack,uint32_t frame){
 static int enabled=-1;
 if(enabled<0){
#if defined(UCRR_LEAN_RUNTIME) && UCRR_LEAN_RUNTIME == 1 && !defined(RECOMP_ICALL_FEEDBACK)
  const char*v=getenv("UCRR_NATIVE_ANIMATION_FACE");enabled=v&&!strcmp(v,"1")&&!getenv("UCRR_CONSTANT_SOURCE_TRACE");
#else
  /* Preserve the original constructor's ICALL diagnostic/feedback events. */
  enabled=0;
#endif
 }
 if(!enabled||g_fp_top<0||g_fp_top>7)return 0;
 unsigned csr=_mm_getcsr();if((csr&~63u)!=0x1f80)return 0;
 uint32_t cut=stage(node,stack,frame);if(!cut)_mm_setcsr(csr);return cut;
}
