#include "migration_services_sidecar_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D631D(void),sub_002D69BB(void);
static int sidecar_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void sidecar_array(unsigned destructor,unsigned count,unsigned stride,unsigned base,unsigned pc){
 PUSH32(g_esp,destructor);PUSH32(g_esp,count);PUSH32(g_esp,stride);PUSH32(g_esp,base);PUSH32(g_esp,pc);sub_002D69BB();
}
static void sidecar_free(unsigned ptr,unsigned pc,int use_eax){
 g_ecx=MEM32(0x51e6f4);unsigned method;
 if(use_eax){g_eax=MEM32(g_ecx);method=MEM32(g_eax+4);}else{g_edx=MEM32(g_ecx);method=MEM32(g_edx+4);}
 unsigned saved=g_esp;PUSH32(g_esp,ptr);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(method,saved);
}
int uc_migration_services_sidecar(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_SIDECAR");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!sidecar_span(g_esp-256,272)||!sidecar_span(g_ecx,52))return 0;
 if(entry!=0x2273c0&&entry!=0x227390&&entry!=0x227230&&entry!=0x227260)return 0;
 if(entry==0x2273c0){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+28);PUSH32(g_esp,g_edi);
  for(unsigned i=0;i<3;i++){
   if(i)g_eax=MEM32(g_esi+28+4*i);
   if(g_eax){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2273d1+16*i);sub_002D631D();g_esp+=4;}
  }
  g_eax=MEM32(g_esi+20);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2273fd);sub_002D631D();
  g_ecx=MEM32(g_esi+24);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x227406);sub_002D631D();
  g_eax=MEM32(g_esi+40);g_esp+=8;
  if(g_eax){g_edx=MEM32(g_eax-4);g_edi=g_eax-4;sidecar_array(0x227390,g_edx,8,g_eax,0x227424);sidecar_free(g_edi,0x227430,1);}
  g_eax=MEM32(g_esi+44);
  if(g_eax){g_ecx=MEM32(g_eax-4);g_edi=g_eax-4;sidecar_array(0x227230,g_ecx,8,g_eax,0x22744b);sidecar_free(g_edi,0x227457,0);}
  g_esi=MEM32(g_esi+48);int empty=!g_esi;
  if(g_esi){g_eax=MEM32(g_esi-4);g_edi=g_esi-4;sidecar_array(0x227260,g_eax,12,g_esi,0x227472);sidecar_free(g_edi,0x22747e,0);}
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=empty;
 }else if(entry==0x227260){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);int empty=!g_eax;
  if(g_eax){g_ecx=MEM32(g_eax-4);PUSH32(g_esp,g_edi);g_edi=g_eax-4;sidecar_array(0x2a3d90,g_ecx,16,g_eax,0x22727e);sidecar_free(g_edi,0x22728a,0);POP32(g_esp,g_edi);}
  MEM32(g_esi+4)=0;MEM32(g_esi+8)=0xffffffff;POP32(g_esp,g_esi);g_recomp_zf=empty;
 }else{
  g_eax=MEM32(g_ecx+(entry==0x227390?4:0));int empty=!g_eax;
  if(g_eax){g_ecx=MEM32(g_eax-16);PUSH32(g_esp,g_esi);g_esi=g_eax-16;sidecar_array(0x2a7910,g_ecx,80,g_eax,entry==0x227390?0x2273ac:0x22724b);sidecar_free(g_esi,entry==0x227390?0x2273b8:0x227257,0);POP32(g_esp,g_esi);}
  g_recomp_zf=empty;
 }
 g_esp+=4;return 1;
}
