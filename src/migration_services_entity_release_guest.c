#include "migration_services_entity_release_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002A1E00(void),sub_001F19F0(void),sub_002273C0(void),sub_002D5CCD(void),sub_00207FF0(void),sub_001F9150(void);
#define RELEASE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int release_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void release_icall(uint32_t target,uint32_t pc,uint32_t saved){PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
static void release_body_memory(uint32_t target_object,uint32_t pc,int through_eax){
 g_ecx=MEM32(0x51e6f4);if(through_eax)g_eax=MEM32(g_ecx);else g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,36);PUSH32(g_esp,176);PUSH32(g_esp,target_object);release_icall(MEM32((through_eax?g_eax:g_edx)+20),pc,saved);
}
static void release_children(void){
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_eax=MEM32(g_ebx+60);g_ecx=0;
 if(g_eax){MEM32(g_esp+4)=0;if((int32_t)MEM32(g_eax+4)>0){PUSH32(g_esp,g_seh_ebp);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);uint32_t offset=0;
  do{g_eax=MEM32(g_ebx+60);g_esi=MEM32(g_eax);g_edi=MEM32(g_esi+offset+12);g_esi+=offset;if(g_edi!=g_ecx){g_ecx=g_edi;RELEASE_CALL(sub_002A1E00,0x21aca6);release_body_memory(g_edi,0x21acb9,0);g_ecx=0;}
   g_eax=MEM32(g_esp+16);MEM32(g_esi+12)=g_ecx;g_edx=MEM32(g_ebx+60);g_esi=MEM32(g_edx+4);g_eax++;offset+=16;MEM32(g_esp+16)=g_eax;
  }while((int32_t)g_eax<(int32_t)g_esi);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
 }MEM32(g_ebx+60)=g_ecx;}POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=4;
}
static void release_bodies(void){
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_seh_ebp);g_ebx=g_ecx;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx+52);
 if(g_esi){g_ecx=g_esi;RELEASE_CALL(sub_002A1E00,0x21acf6);release_body_memory(g_esi,0x21ad09,1);MEM32(g_ebx+52)=0;}
 g_eax=MEM32(g_ebx+60);if(g_eax){MEM32(g_esp+12)=0;if((int32_t)MEM32(g_eax+4)>0){PUSH32(g_esp,g_edi);uint32_t offset=0;
  do{g_ecx=MEM32(g_ebx+60);g_esi=MEM32(g_ecx);g_edi=MEM32(g_esi+offset+12);g_esi+=offset;if(g_edi){g_ecx=g_edi;RELEASE_CALL(sub_002A1E00,0x21ad36);release_body_memory(g_edi,0x21ad49,0);}
   g_eax=MEM32(g_esp+16);MEM32(g_esi+12)=0;g_ecx=MEM32(g_ebx+60);g_edx=MEM32(g_ecx+4);g_eax++;offset+=16;MEM32(g_esp+16)=g_eax;
  }while((int32_t)g_eax<(int32_t)g_edx);POP32(g_esp,g_edi);
 }MEM32(g_ebx+60)=0;}POP32(g_esp,g_esi);g_esp+=4;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=4;
}
static void release_attachment(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi);PUSH32(g_esp,g_edi);
 if(g_ecx){g_eax=MEM32(g_ecx);release_icall(MEM32(g_eax+36),0x21b1af,g_esp);if(g_eax){g_edx=MEM32(g_eax);g_ecx=g_eax;release_icall(MEM32(g_edx+40),0x21b1ba,g_esp);if(g_eax){g_ecx=g_eax;RELEASE_CALL(sub_001F19F0,0x21b1c5);}}}
 if(MEM32(g_esi)){g_ecx=MEM32(0x5150b0);g_eax=MEM32(g_ecx);release_icall(MEM32(g_eax+48),0x21b1d5,g_esp);g_ecx=MEM32(g_esi);g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;release_icall(MEM32(g_edx+68),0x21b1df,saved);}
 g_ecx=g_esi;PUSH32(g_esp,0x21b1e6);release_bodies();g_ecx=g_esi;PUSH32(g_esp,0x21b1ed);release_children();g_edi=MEM32(g_esi+4);
 if(g_edi){g_ecx=g_edi;RELEASE_CALL(sub_002273C0,0x21b1fb);PUSH32(g_esp,g_edi);RELEASE_CALL(sub_002D5CCD,0x21b201);g_esp+=4;}
 g_ecx=MEM32(g_esi);int missing=!g_ecx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);if(!missing){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);release_icall(MEM32(g_edx),0x21b212,saved);}g_recomp_zf=missing;g_esp+=4;
}
static void release_extension(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+0x78);int missing=!g_ecx;if(!missing){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);release_icall(MEM32(g_eax),0x216010,saved);}MEM32(g_esi+0x78)=0;POP32(g_esp,g_esi);g_recomp_zf=missing;g_esp+=4;
}
static void release_placement(void){g_eax=MEM32(g_esp+4);if((int32_t)g_eax<(int32_t)MEM32(g_ecx+16)){g_ecx=MEM32(g_ecx+4);g_eax=MEM32(g_ecx+g_eax*4);g_edx=(MEM32(g_eax+8)&~15u)|16;MEM32(g_eax+8)=g_edx;}g_esp+=8;}
static void release_base(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esi)=0x4805f8;PUSH32(g_esp,0x20a48f);release_extension();g_edi=MEM32(g_esi+0x74);if(g_edi){g_ecx=g_edi;PUSH32(g_esp,0x20a49d);release_attachment();PUSH32(g_esp,g_edi);RELEASE_CALL(sub_002D5CCD,0x20a4a3);g_esp+=4;MEM32(g_esi+0x74)=0;}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
}
static void release_unregister(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_edx=MEM32(g_esi+0x5c)|0x8100;PUSH32(g_esp,g_edi);MEM32(g_esi+0x5c)=g_edx;PUSH32(g_esp,0x20a635);release_extension();g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);RELEASE_CALL(sub_00207FF0,0x20a644);g_eax=MEM32(g_esi+0x5c);
 if(g_eax&0x1000){g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,0x20a657);g_eax=MEM32(g_ecx+4);g_eax=MEM32(g_eax+0x2054);g_esp+=4;g_ecx=MEM32(g_esi+0x50);PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x20a662);release_placement();}
 g_eax=MEM32(g_esi+0x64);g_edi=g_esi+0x64;if(g_eax){g_edx=MEM32(g_esi+0x54);g_eax=MEM32(0x51c398);g_ecx=MEM32(g_eax+8);PUSH32(g_esp,g_edx);RELEASE_CALL(sub_001F9150,0x20a67d);g_ecx=0;MEM32(g_edi)=0;MEM32(g_edi+4)=0;MEM32(g_edi+8)=0;}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
}
int uc_migration_services_entity_release(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_ENTITY_RELEASE");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!release_span(g_esp-256,280)||!release_span(g_ecx,0x7c))return 0;
 switch(entry){case 0x21ac70:release_children();break;case 0x21ace0:release_bodies();break;case 0x21b1a0:release_attachment();break;case 0x216000:release_extension();break;case 0x210f10:release_placement();break;case 0x20a480:release_base();break;case 0x20a620:release_unregister();break;case 0x2084e0:g_eax=MEM32(g_ecx+4);g_eax=MEM32(g_eax+0x2054);g_esp+=4;break;default:return 0;}return 1;
}
#undef RELEASE_CALL
