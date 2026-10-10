#include "migration_interface_presence.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_0022C2C0(void); /* Existing host-input merge remains authoritative. */
static int presence_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int presence_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PRESENCE");v=s&&!strcmp(s,"1");}return v;}
static int presence_device(uint32_t object,uint32_t port){
 if(!presence_span(object,8))return 0;
 return presence_span((uint64_t)MEM32(object+4)+(uint64_t)port*4+16,4);
}
int migration_interface_presence(unsigned op){
 if(!presence_enabled()||op>2||g_esp<0x10020||!presence_span(g_esp-16,24))return 0;
 if(op){
  uint32_t object=g_ecx,port=MEM32(g_esp+4);
  if(op==1){if(!presence_span(object,4)||!presence_span(MEM32(object),8))return 0;object=MEM32(MEM32(object)+4);}
  if((op==2||object)&&!presence_device(object,port))return 0;
  if(op==1){g_eax=MEM32(g_ecx);g_ecx=MEM32(g_eax+4);if(!g_ecx){g_eax=0;g_recomp_zf=1;g_esp+=8;return 1;}}
  g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_eax+g_ecx*4+16);g_esp+=8;return 1;
 }
 if(!presence_span(0x5096e0,4))return 0;
 uint32_t mapping=MEM32(0x5096e0);
 if(!presence_span(mapping,25)||!presence_span(g_edi,4)||!presence_span(MEM32(g_edi),8))return 0;
 uint32_t device=MEM32(MEM32(g_edi)+4);
 for(unsigned i=0;i<4;i++){uint32_t port=MEM8(mapping+24)?(uint32_t)(int32_t)SMEM8(mapping+12+i):i;if(MEM8(mapping+24)&&MEM8(mapping+16+i)==255)continue;if(device&&!presence_device(device,port))return 0;}
 g_eax=mapping;SET_LO8(g_ecx,MEM8(g_eax+24));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=0;
 if(LO8(g_ecx)){
  SET_LO8(g_ebx,255);
  for(;g_esi<4;g_esi++)if(MEM8(g_eax+g_esi+16)!=LO8(g_ebx)){
   g_eax=(uint32_t)(int32_t)SMEM8(g_eax+g_esi+12);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x5aa26);sub_0022C2C0();g_edx=MEM32(0x5096e0);SET_LO8(g_ecx,g_eax!=0);MEM8(g_edx+g_esi+20)=LO8(g_ecx);g_eax=MEM32(0x5096e0);
  }
  g_recomp_zf=1;
 }else{
  for(g_ebx=0;g_ebx<4;g_ebx++){
   PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x5aa4d);sub_0022C2C0();
   if(g_eax){g_eax=MEM32(0x5096e0);MEM8(g_eax+g_esi+12)=LO8(g_ebx);g_ecx=MEM32(0x5096e0);MEM8(g_ecx+g_esi+16)=LO8(g_ebx);g_edx=MEM32(0x5096e0);MEM8(g_edx+g_esi+20)=1;g_esi++;}
  }
  g_eax=g_esi;if(g_esi<4){SET_LO8(g_ebx,255);for(;g_eax<4;g_eax++){g_ecx=MEM32(0x5096e0);MEM8(g_ecx+g_eax+12)=LO8(g_eax);g_edx=MEM32(0x5096e0);MEM8(g_edx+g_eax+16)=LO8(g_ebx);g_ecx=MEM32(0x5096e0);MEM8(g_ecx+g_eax+20)=0;}}
 }
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=4;return 1;
}
