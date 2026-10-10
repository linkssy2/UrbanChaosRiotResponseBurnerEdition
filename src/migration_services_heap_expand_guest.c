#include "migration_services_heap_expand_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000827B6(void),sub_00082973(void),sub_00082C18(void);
static int expand_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void expand_check_fill(unsigned block,unsigned pc){
 g_eax=((unsigned)MEM16(block)<<4)-24;if((LO8(g_ecx)&2)&&g_eax>4)g_eax-=4;
 unsigned saved=g_esp;PUSH32(g_esp,0xfeeefeee);PUSH32(g_esp,g_eax);g_eax=block+24;PUSH32(g_esp,g_eax);unsigned target=MEM32(0x44b3f4);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);
}
static void expand_insert(unsigned f){
 g_ecx=MEM32(f-8);
 if(LO16(g_ecx)<128){
  g_eax=LO16(g_ecx);g_edi=g_ebx+g_eax*8+0x180;
  if(MEM32(g_edi)==g_edi){g_ecx=MEM16(g_esi);g_eax=g_ecx;g_ecx&=7;SET_LO8(g_edx,1);g_eax>>=3;SET_LO8(g_edx,LO8(g_edx)<<LO8(g_ecx));g_eax+=g_ebx+0x160;MEM8(g_eax)|=LO8(g_edx);}
 }else{
  g_eax=g_ebx+0x180;g_edi=MEM32(g_eax);while(g_eax!=g_edi&&LO16(g_ecx)>MEM16(g_edi-8))g_edi=MEM32(g_edi);
 }
 g_ecx=MEM32(g_edi+4);g_eax=g_esi+8;MEM32(g_eax)=g_edi;MEM32(g_esi+12)=g_ecx;MEM32(g_ecx)=g_eax;MEM32(g_edi+4)=g_eax;g_eax=MEM32(f-8);MEM32(g_ebx+48)+=g_eax;
}
int uc_migration_services_heap_expand(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_HEAP_EXPAND");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x82dbd||MEM8(0x51c178)||!expand_span(g_esp-256,284)||!expand_span(MEM32(g_esp+4),0x590)||!expand_span(MEM32(g_esp+12),16))return 0;
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;g_esp-=12;g_edx=MEM32(f+24);PUSH32(g_esp,g_ebx);g_ebx=MEM32(f+8);
 if(g_edx>MEM32(g_ebx+28)){SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_esp=f+28;return 1;}
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(f+16);g_eax=MEM16(g_edi);SET_LO8(g_ecx,MEM8(g_edi+5));g_esi=(g_eax<<4)+g_edi;MEM8(f-2)=LO8(g_ecx);
 if(LO8(g_ecx)&16){
  g_edx-=g_eax;PUSH32(g_esp,g_esi);g_edx<<=4;g_eax=f-8;PUSH32(g_esp,g_eax);g_eax=MEM8(g_edi+4);g_edx=(g_edx+4095)&0xfffff000;MEM32(f-8)=g_edx;PUSH32(g_esp,MEM32(g_ebx+4*g_eax+96));PUSH32(g_esp,g_ebx);g_ebp=f;PUSH32(g_esp,0x82e17);sub_000827B6();
  if(!g_eax)goto failure;
  MEM32(f-8)>>=4;PUSH32(g_esp,0);g_ecx=f-8;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_ebp=f;PUSH32(g_esp,0x82e2c);sub_00082973();
  g_edx=MEM16(g_edi);g_esi=MEM32(f-8);SET_LO8(g_ecx,MEM8(g_eax+5));g_edx+=g_esi;
  if(g_edx<MEM32(f+24)){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_ebp=f;PUSH32(g_esp,0x82e44);sub_00082C18();g_eax=MEM32(f-8);MEM32(g_ebx+48)+=g_eax;goto failure;}
  MEM32(f-8)=g_edx;g_edx=MEM32(f+24);
 }else{
  SET_LO8(g_ecx,MEM8(g_esi+5));MEM8(f+11)=LO8(g_ecx);if(LO8(g_ecx)&1)goto failure;
  g_ecx=MEM16(g_esi)+g_eax;MEM32(f-8)=g_ecx;if(g_ecx<g_edx)goto failure;
  g_eax=MEM32(g_esi+8);g_ecx=MEM32(g_esi+12);MEM32(g_ecx)=g_eax;MEM32(g_eax+4)=g_ecx;
  if(g_eax==g_ecx){SET_LO16(g_eax,MEM16(g_esi));if(LO16(g_eax)<128){g_ecx=LO16(g_eax);g_eax=(g_ecx>>3)+g_ebx+0x160;MEM32(f-12)=g_eax;g_ecx&=7;SET_LO8(g_eax,1);SET_LO8(g_eax,LO8(g_eax)<<LO8(g_ecx));SET_LO8(g_ecx,LO8(g_eax));g_eax=MEM32(f-12);MEM8(g_eax)^=LO8(g_ecx);}}
  SET_LO8(g_ecx,MEM8(g_esi+5));if(LO8(g_ecx)&4){expand_check_fill(g_esi,0x82edb);g_edx=MEM32(f+24);}
  g_eax=MEM16(g_esi);MEM32(g_ebx+48)-=g_eax;SET_LO8(g_ecx,MEM8(f+11));
 }
 g_eax=MEM16(g_edi);g_esi=MEM8(g_edi+6);MEM32(f-8)-=g_edx;g_eax=(g_eax<<4)-g_esi;MEM32(f-12)=g_eax;
 if(MEM32(f-8)<=2){g_edx+=MEM32(f-8);MEM32(f-8)=0;}
 if(MEM8(f-2)&2){g_eax=(unsigned)MEM16(g_edi)<<4;g_esi=g_eax+g_edi-16;g_eax=g_edx<<4;g_edi=g_eax+g_edi-16;for(unsigned i=0;i<4;i++){MEM32(g_edi)=MEM32(g_esi);g_esi+=4;g_edi+=4;}g_edi=MEM32(f+16);}
 SET_LO8(g_eax,(LO8(g_edx)<<4)-MEM8(f+20));MEM16(g_edi)=LO16(g_edx);MEM8(g_edi+6)=LO8(g_eax);
 if(!MEM32(f-8)){
  SET_LO8(g_ecx,LO8(g_ecx)&16);MEM8(g_edi+5)|=LO8(g_ecx);MEM8(f+27)=LO8(g_ecx);
  if(!LO8(g_ecx)){g_eax=(unsigned)LO16(g_edx)<<4;MEM16(g_eax+g_edi+2)=LO16(g_edx);}else{g_eax=MEM8(g_edi+4);g_eax=MEM32(g_ebx+4*g_eax+96);MEM32(g_eax+64)=g_edi;}
 }else{
  g_esi=(g_edx<<4)+g_edi;MEM16(g_esi+2)=LO16(g_edx);SET_LO8(g_eax,MEM8(g_edi+4));MEM8(g_esi+4)=LO8(g_eax);
  if(LO8(g_ecx)&16){
   g_eax=LO8(g_eax);g_eax=MEM32(g_ebx+4*g_eax+96);MEM32(g_eax+64)=g_esi;MEM8(g_esi+5)=LO8(g_ecx);SET_LO16(g_eax,MEM16(f-8));SET_LO8(g_ecx,LO8(g_ecx)&0xf8);MEM16(g_esi)=LO16(g_eax);MEM8(g_esi+5)=LO8(g_ecx);g_ecx=MEM32(f-8);
   if(LO16(g_ecx)<128){g_eax=LO16(g_ecx);g_edx=g_ebx+8*g_eax+0x180;if(MEM32(g_edx)==g_edx){g_ecx=MEM16(g_esi);g_eax=(g_ecx>>3)+g_ebx+0x160;MEM32(f+8)=g_eax;g_ecx&=7;SET_LO8(g_eax,1);SET_LO8(g_eax,LO8(g_eax)<<LO8(g_ecx));SET_LO8(g_ecx,LO8(g_eax));g_eax=MEM32(f+8);MEM8(g_eax)|=LO8(g_ecx);}}
   else{g_eax=g_ebx+0x180;g_edx=MEM32(g_eax);while(g_eax!=g_edx&&LO16(g_ecx)>MEM16(g_edx-8))g_edx=MEM32(g_edx);}
   g_ecx=MEM32(g_edx+4);g_eax=g_esi+8;MEM32(g_eax)=g_edx;MEM32(g_esi+12)=g_ecx;MEM32(g_ecx)=g_eax;MEM32(g_edx+4)=g_eax;g_eax=MEM32(f-8);MEM32(g_ebx+48)+=g_eax;
  }else{
   g_edi=(MEM32(f-8)<<4)+g_esi;SET_LO8(g_eax,MEM8(g_edi+5));
   if(LO8(g_eax)&1){
    SET_LO8(g_ecx,LO8(g_ecx)&0xef);MEM8(g_esi+5)=LO8(g_ecx);SET_LO16(g_eax,MEM16(f-8));MEM16(g_esi)=LO16(g_eax);g_eax=MEM32(f-8);g_ecx=g_eax<<4;MEM16(g_ecx+g_esi+2)=LO16(g_eax);MEM8(g_esi+5)&=0xf8;expand_insert(f);
   }else{
    g_ecx=MEM32(g_edi+12);MEM8(f+11)=LO8(g_eax);g_eax=MEM32(g_edi+8);MEM32(g_ecx)=g_eax;MEM32(g_eax+4)=g_ecx;
    if(g_eax==g_ecx){SET_LO16(g_eax,MEM16(g_edi));if(LO16(g_eax)<128){g_ecx=LO16(g_eax);g_eax=g_ecx;g_ecx&=7;SET_LO8(g_edx,1);g_eax>>=3;SET_LO8(g_edx,LO8(g_edx)<<LO8(g_ecx));g_eax+=g_ebx+0x160;MEM8(g_eax)^=LO8(g_edx);}}
    SET_LO8(g_ecx,MEM8(g_edi+5));if(LO8(g_ecx)&4)expand_check_fill(g_edi,0x830ec);
    g_eax=MEM16(g_edi);MEM32(g_ebx+48)-=g_eax;g_eax=MEM16(g_edi);MEM32(f-8)+=g_eax;SET_LO8(g_ecx,MEM8(f+11));MEM8(g_esi+5)=LO8(g_ecx);g_eax=MEM32(f-8);
    if(g_eax>0xff00){PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);g_ebp=f;PUSH32(g_esp,0x83172);sub_00082C18();}
    else{MEM16(g_esi)=LO16(g_eax);if(!(LO8(g_ecx)&16)){g_eax=MEM32(f-8);g_ecx=g_eax<<4;MEM16(g_ecx+g_esi+2)=LO16(g_eax);}else{g_eax=MEM8(g_esi+4);g_eax=MEM32(g_ebx+4*g_eax+96);MEM32(g_eax+64)=g_esi;}MEM8(g_esi+5)&=0xf8;expand_insert(f);}
   }
   g_edi=MEM32(f+16);
  }
 }
 g_edx=MEM32(f+12);
 if(LO8(g_edx)&8){g_esi=MEM32(f-12);g_ecx=MEM32(f+20);if(g_ecx>g_esi){g_ecx-=g_esi;g_edi+=g_esi+16;g_esi=g_ecx;g_ecx>>=2;g_eax=0;memset((void*)XBOX_PTR(g_edi),0,g_ecx*4);g_edi+=g_ecx*4;g_ecx=g_esi&3;memset((void*)XBOX_PTR(g_edi),0,g_ecx);g_edi+=g_ecx;g_ecx=0;g_edi=MEM32(f+16);}}
 g_eax=g_edx>>4;SET_LO8(g_eax,LO8(g_eax)^MEM8(g_edi+5));g_edx>>=4;SET_LO8(g_eax,LO8(g_eax)&31);SET_LO8(g_eax,LO8(g_eax)^LO8(g_edx));MEM8(g_edi+5)=LO8(g_eax);SET_LO8(g_eax,1);goto done;
failure:SET_LO8(g_eax,0);
done:POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+28;return 1;
}
