/* Original body property replacement, ordered removal and 64-bit append. */
#include "migration_physics_body_properties.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+32<=p);}
static void remove_property(void){
 uint32_t frame=g_seh_ebp;g_esp-=16;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+72);g_eax=0;PUSH32(g_esp,g_edi);int found=0;
 if((int32_t)g_esi>0){g_edi=MEM32(g_ecx+68);g_ebx=MEM32(g_esp+40);g_edx=g_edi;do{if(MEM32(g_edx)==g_ebx){found=1;break;}++g_eax;g_edx+=16;}while((int32_t)g_eax<(int32_t)g_esi);}
 if(found){g_edx=g_eax<<4;g_edi+=g_edx;g_esi=MEM32(g_edi);MEM32(g_esp+16)=g_esi;g_esi=MEM32(g_edi+4);g_ebx=MEM32(g_edi+8);MEM32(g_esp+20)=g_esi;g_esi=MEM32(g_edi+12);MEM32(g_esp+28)=g_esi;g_esi=MEM32(g_ecx+72)-1;g_edi=g_esi;MEM32(g_ecx+72)=g_esi;g_esi=g_eax;
  while((int32_t)g_esi<(int32_t)g_edi){g_eax=MEM32(g_ecx+68)+g_edx;g_edi=g_eax+16;for(unsigned k=0;k<3;k++){frame=MEM32(g_edi+k*4);MEM32(g_eax+k*4)=frame;}g_edi=MEM32(g_edi+12);MEM32(g_eax+12)=g_edi;g_eax=MEM32(g_ecx+72);++g_esi;g_edx+=16;if((int32_t)g_esi>=(int32_t)g_eax)break;/* EDI is the moved word, not the loop bound. */g_edi=g_eax;}
  g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_esp+28);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(g_eax)=g_ebx;MEM32(g_eax+4)=g_ecx;
 }else{g_eax=MEM32(g_esp+36);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(g_eax)=0;MEM32(g_eax+4)=0;}
 POP32(g_esp,g_ebx);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void erase_if_present(void){
 g_ecx=MEM32(g_esp+4);g_esp-=8;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+72);g_eax=0;PUSH32(g_esp,g_edi);int found=0;if((int32_t)g_esi>0){g_edx=MEM32(g_ecx+68);g_edi=MEM32(g_esp+24);do{if(MEM32(g_edx)==g_edi){found=1;break;}++g_eax;g_edx+=16;}while((int32_t)g_eax<(int32_t)g_esi);}
 if(found){PUSH32(g_esp,g_edi);g_eax=g_esp+12;PUSH32(g_esp,g_eax);CALL(0x2a310f,remove_property);}POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,found);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void append_property(void){
 uint32_t frame=g_seh_ebp;g_edx=MEM32(g_ecx+72);g_esp-=16;PUSH32(g_esp,frame);frame=MEM32(g_esp+24);PUSH32(g_esp,g_esi);g_eax=0;PUSH32(g_esp,g_edi);int found=0;if((int32_t)g_edx>0){g_esi=MEM32(g_ecx+68);do{if(MEM32(g_esi)==frame){found=1;break;}++g_eax;g_esi+=16;}while((int32_t)g_eax<(int32_t)g_edx);}
 if(!found){g_eax=MEM32(g_ecx+76);g_edi=MEM32(g_esp+36);g_esi=g_ecx+68;g_ecx=MEM32(g_esi+4);g_eax&=0x3fffffff;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+44);if(g_ecx==g_eax){PUSH32(g_esp,16);PUSH32(g_esp,g_esi);CALL(0x2f5bf5,sub_002E0150);g_esp+=8;}g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);g_ecx=(g_ecx<<4)+g_edx;g_edx=MEM32(g_esp+20);MEM32(g_ecx)=frame;MEM32(g_ecx+4)=g_edx;MEM32(g_ecx+8)=g_edi;MEM32(g_ecx+12)=g_ebx;++MEM32(g_esi+4);POP32(g_esp,g_ebx);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=16;
}
static void replace_property(int signed_value){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);CALL(signed_value?0x2a3131:0x2a31a1,erase_if_present);g_eax=MEM32(g_esp+28);g_esp+=8;
 if(signed_value){g_edx=(int32_t)g_eax<0?0xffffffff:0;PUSH32(g_esp,g_edx);}else{g_ecx=0;PUSH32(g_esp,g_ecx);}PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ecx=g_edi;CALL(signed_value?0x2a3143:0x2a31b4,append_property);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=signed_value?g_esp==0:g_ecx==0;g_esp+=4;
}
int uc_migration_physics_body_properties(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_PROPERTIES");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-256,288))return 0;int wrapper=entry==0x2a30d0||entry==0x2a3120||entry==0x2a3190;if(!wrapper&&entry!=0x2f5af0&&entry!=0x2f5bb0)return 0;uint32_t p=wrapper?MEM32(g_esp+4):g_ecx;if(!safe(p,80))return 0;uint32_t n=MEM32(p+72),cap=MEM32(p+76)&0x3fffffff;if(n>65535||cap>65535||n>cap||(cap&&!safe(MEM32(p+68),cap*16)))return 0;if(entry==0x2f5af0&&!safe(MEM32(g_esp+4),8))return 0;
 if(entry==0x2a30d0)erase_if_present();else if(entry==0x2f5af0)remove_property();else if(entry==0x2f5bb0)append_property();else replace_property(entry==0x2a3120);return 1;
}
#undef CALL
