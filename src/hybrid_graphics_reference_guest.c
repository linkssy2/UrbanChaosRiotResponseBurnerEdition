#include "hybrid_graphics_reference_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_003E5570(void),sub_003E55B0(void),sub_003E5450(void);
extern void sub_003E9890(void),sub_003E5400(void),sub_000825C8(void);
#define REF_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)

/* Resource headers retain two distinct counts: low16 public references and
 * bits19..22 device bindings. A surface's parent is retained before its first
 * public reference, and released before its last reference retires. Calls and
 * header reloads stay live; completing services may alter registers and memory. */
static void reference_get(uint32_t entry){
 g_eax=MEM32(0x3f4238);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+(entry==0x3e35d0?0x1a04:0x1a08));
 unsigned empty=g_esi==0;
 if(!empty){PUSH32(g_esp,g_esi);if(entry==0x3e35d0){REF_CALL(sub_003E5570,0x3e35e6);}else{REF_CALL(sub_003E5570,0x3e360a);}}
 g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=empty;g_esp+=4;
}
static void reference_bind(void){
 g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_eax);
 if(!(g_ecx&0x780000)){
  g_ecx&=0x70000;
  if(g_ecx==0x50000){g_ecx=MEM32(g_eax+20);if(g_ecx)MEM32(g_ecx)+=0x80000;}
 }
 MEM32(g_eax)+=0x80000;g_esp+=8;
}
static void reference_acquire(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi);
 if(!(g_eax&0xffff)){
  g_eax&=0x70000;
  if(g_eax==0x50000){g_eax=MEM32(g_esi+20);if(g_eax){PUSH32(g_esp,g_eax);REF_CALL(sub_003E5570,0x3e5597);}}
 }
 g_edx=MEM32(g_esi)+1;g_eax=g_edx;MEM32(g_esi)=g_edx;g_eax&=0xffff;
 POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;
}
static void reference_release(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi);g_ecx=g_eax&0xffff;
 if(g_ecx==1){
  g_eax&=0x70000;
  if(g_eax==0x50000){g_eax=MEM32(g_esi+20);if(g_eax){PUSH32(g_esp,g_eax);REF_CALL(sub_003E55B0,0x3e55dd);}}
  g_eax=MEM32(g_esi);
  if(!(g_eax&0x780000)){
   PUSH32(g_esp,g_esi);REF_CALL(sub_003E5450,0x3e55ec);g_eax=0;
   POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return;
  }
 }
 --g_eax;MEM32(g_esi)=g_eax;g_eax&=0xffff;
 POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;
}
static void reference_unbind(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi);g_ecx=g_eax&0x780000;
 if(g_ecx==0x80000){
  g_eax&=0x70000;
  if(g_eax==0x50000){
   g_eax=MEM32(g_esi+20);
   if(g_eax){g_edx=MEM32(g_eax)-0x80000;g_ecx=g_edx;MEM32(g_eax)=g_edx;
    if(!(g_ecx&0x78ffff)){PUSH32(g_esp,g_eax);REF_CALL(sub_003E5450,0x3e5714);}}
  }
  g_eax=MEM32(g_esi);
  if(!(g_eax&0xffff)){PUSH32(g_esp,g_esi);REF_CALL(sub_003E5450,0x3e5723);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return;}
 }
 g_eax-=0x80000;MEM32(g_esi)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;
}
static void reference_retire(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_eax=MEM32(g_edi);g_esi=g_eax&0x70000;
 if(g_esi!=0x50000||(int32_t)g_eax<0){PUSH32(g_esp,g_edi);REF_CALL(sub_003E9890,0x3e5472);}
 PUSH32(g_esp,g_edi);REF_CALL(sub_003E5400,0x3e5478);
 if(g_esi==0x50000){
  if((int32_t)MEM32(g_edi)<0){g_eax=MEM32(g_edi+4);PUSH32(g_esp,0xa1800000u);g_eax|=0x80000000u;PUSH32(g_esp,g_eax);REF_CALL(sub_000825C8,0x3e54d1);}
 }else if(g_esi==0x20000){
  if((int32_t)MEM32(g_edi)>=0){g_ecx=MEM32(g_edi+4);PUSH32(g_esp,0xa1800000u);PUSH32(g_esp,g_ecx);REF_CALL(sub_000825C8,0x3e54d1);}
 }else if(g_esi!=0x10000&&g_esi!=0x60000){
  g_edx=MEM32(g_edi+4);PUSH32(g_esp,0xa1800000u);g_edx|=0x80000000u;PUSH32(g_esp,g_edx);REF_CALL(sub_000825C8,0x3e54d1);
 }
 PUSH32(g_esp,0x24800000);PUSH32(g_esp,g_edi);REF_CALL(sub_000825C8,0x3e54dc);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
static int reference_enabled(void){static int value=-1;if(value<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_REFERENCE");value=v&&!strcmp(v,"1");}return value;}
static int reference_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int reference_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_graphics_reference(uint32_t entry){
 if(!reference_enabled())return 0;
 unsigned getter=entry==0x3e35d0||entry==0x3e35f0;
 if(!getter&&entry!=0x3e5450&&entry!=0x3e5530&&entry!=0x3e5570&&entry!=0x3e55b0&&entry!=0x3e56d0)return 0;
 uint32_t s=g_esp,p;
 if((s&3)||s<0x10100u||!reference_span(s-256,272)||!reference_span(0x51c178,1)||MEM8(0x51c178)||!reference_apart(s-256,272,0x51c178,1))return 0;
 if(getter){
  if(!reference_span(0x3f4238,4)||!reference_apart(s-256,272,0x3f4238,4))return 0;
  uint32_t device=MEM32(0x3f4238);
  if(!reference_span(device,0x1a0c)||!reference_apart(device,0x1a0c,s-256,272))return 0;
  p=MEM32(device+(entry==0x3e35d0?0x1a04:0x1a08));
 }else p=MEM32(s+4);
 if((p||!getter)&&(!reference_span(p,24)||!reference_apart(p,24,s-256,272)||!reference_apart(p,24,0x51c178,1)))return 0;
 /* Descendants and retained service allocations keep the same valid-lifetime
  * contract as the original. No preflight snapshot or post-effect replay. */
 if(getter)reference_get(entry);
 else if(entry==0x3e5530)reference_bind();
 else if(entry==0x3e5570)reference_acquire();
 else if(entry==0x3e55b0)reference_release();
 else if(entry==0x3e56d0)reference_unbind();
 else reference_retire();
 return 1;
}
#undef REF_CALL
