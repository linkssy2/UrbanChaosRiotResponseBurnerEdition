/* Original collision-agent block transfer and reciprocal link relocation. */
#include "migration_physics_agent_transfer.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);void sub_002E01D0(void);void sub_002440D0(void);void sub_00320700(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-512,544);}
static int record_valid(uint32_t p,uint32_t stride){
 if(!safe(p,stride))return 0;for(unsigned side=0;side<2;side++){uint32_t b=MEM32(p+20+side*4),i=MEM16(p+12+side*2);if(!safe(b,48))return 0;uint32_t n=MEM32(b+40),cap=MEM32(b+44)&0x3fffffff;if(n>cap||cap>65536||i>=n||!safe(MEM32(b+36),cap*8))return 0;}return 1;
}
static int stream_valid(uint32_t p){
 if(!safe(p,24))return 0;uint32_t n=MEM32(p+4),cap=MEM32(p+8)&0x3fffffff,stride=MEM16(p+20),size=MEM16(p+22),used=MEM32(p+16);
 if(n>cap||cap>65536||stride<32||stride>512||size<stride||size%stride||used>size||used%stride||(!n&&used!=size)||(n&&!used)||(cap&&!safe(MEM32(p),cap*4)))return 0;
 for(unsigned i=0;i<n;i++){uint32_t block=MEM32(MEM32(p)+i*4),bytes=i+1==n?used:size;if(!safe(block,size))return 0;for(unsigned j=0;j<bytes;j+=stride)if(!record_valid(block+j,stride))return 0;}return 1;
}
static void append(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_eax=MEM16(g_edi+22);
 if(MEM32(g_edi+16)==g_eax){
  g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0x24);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x32993d);RECOMP_ICALL_SAFE(target,saved);
  g_ecx=MEM32(g_edi+4);g_esi=g_eax;g_eax=MEM32(g_edi+8)&0x3fffffff;
  if(g_ecx==g_eax){PUSH32(g_esp,4);PUSH32(g_esp,g_edi);CALL(0x329956,sub_002E0150);g_esp+=8;}
  g_ecx=MEM32(g_edi+4);g_edx=MEM32(g_edi);MEM32(g_edx+g_ecx*4)=g_esi;++MEM32(g_edi+4);MEM32(g_edi+16)=0;
 }
 g_ecx=MEM32(g_edi+4);g_edx=MEM32(g_edi);g_esi=MEM32(g_edx+g_ecx*4-4);g_ecx=MEM16(g_edi+20);g_eax=MEM32(g_edi+16);g_esi+=g_eax;PUSH32(g_esp,g_ecx);g_eax+=g_ecx;g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);MEM32(g_edi+16)=g_eax;CALL(0x32998e,sub_002440D0);
 g_edx=MEM16(g_esi+12);g_ecx=MEM32(g_esi+20);SET_LO16(g_eax,MEM16(g_esi+14));g_ecx=MEM32(g_ecx+36);MEM32(g_ecx+g_edx*8)=g_esi;g_edx=MEM32(g_esi+24);g_ecx=MEM32(g_edx+36);g_eax=(uint16_t)g_eax;g_esp+=12;MEM32(g_ecx+g_eax*8)=g_esi;POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;
}
static void transfer(void){
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);g_eax=MEM16(g_ebx+22);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);
 if(MEM32(g_ebx+16)!=g_eax){PUSH32(g_esp,g_edi);do{
  g_eax=MEM32(g_esi+4);if(!g_eax)break;
  g_ecx=MEM32(g_esi);g_edx=MEM16(g_esi+20);g_eax=MEM32(g_ecx+g_eax*4-4);g_ecx=MEM32(g_esi+16);g_eax-=g_edx;g_eax+=g_ecx;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);CALL(0x32a493,append);
  g_ecx=MEM16(g_esi+20);g_eax=MEM32(g_esi+16);g_esp+=8;g_eax-=g_ecx;MEM32(g_esi+16)=g_eax;
  if(!g_eax){
   g_eax=MEM32(g_esi+4);g_edx=MEM32(g_esi);g_edi=MEM32(g_edx+g_eax*4-4);g_ecx=g_eax-1;g_eax=MEM32(g_esi+8);MEM32(g_esi+4)=g_ecx;
   if((int32_t)g_eax>=0){++g_ecx;int shrink=(int32_t)g_ecx<1;if(!shrink){g_eax&=0x3fffffff;g_ecx*=2;shrink=(int32_t)g_ecx<(int32_t)g_eax;}if(shrink){PUSH32(g_esp,1);g_edx=g_esi+12;PUSH32(g_esp,g_edx);PUSH32(g_esp,4);PUSH32(g_esp,g_esi);CALL(0x32a4d9,sub_002E01D0);g_esp+=16;}}
   g_edx=MEM16(g_esi+22);g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0x24);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x32a4ef);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM16(g_esi+22);MEM32(g_esi+16)=g_eax;
  }
  g_ecx=MEM16(g_ebx+22);
 }while(MEM32(g_ebx+16)!=g_ecx);POP32(g_esp,g_edi);}
 g_eax=MEM32(g_esi+4);int zero=g_eax==0;
 if(g_eax){g_edx=MEM32(g_ebx+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edx);g_ecx=g_ebx;CALL(0x32a517,sub_00320700);g_eax=MEM32(g_esi+16);MEM32(g_ebx+16)=g_eax;MEM32(g_esi+4)=0;g_ecx=MEM16(g_ebx+22);MEM32(g_esi+16)=g_ecx;}
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=zero;g_esp+=4;
}
int uc_migration_physics_agent_transfer(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_AGENT_TRANSFER");enabled=v&&!strcmp(v,"1");}
 if(!enabled||!span(g_esp-512,544)||!span(0x51e6f4,4))return 0;uint32_t a=MEM32(g_esp+4),b=MEM32(g_esp+8),alloc=MEM32(0x51e6f4);if(!safe(alloc,4)||!safe(MEM32(alloc),24)||!stream_valid(a))return 0;
 if(entry==0x329920){if(!record_valid(b,MEM16(a+20)))return 0;append();return 1;}
 if(entry!=0x32a450||a==b||!stream_valid(b)||MEM16(a+20)!=MEM16(b+20)||MEM16(a+22)!=MEM16(b+22)||MEM32(a+4)+MEM32(b+4)>65535)return 0;
 transfer();return 1;
}
#undef CALL
