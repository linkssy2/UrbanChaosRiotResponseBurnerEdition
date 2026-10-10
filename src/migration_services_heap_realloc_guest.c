#include "migration_services_heap_realloc_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00082C18(void),sub_00082D12(void),sub_00082DBD(void),sub_00083B5D(void),sub_000842D8(void),sub_00084BE4(void),sub_002D5990(void);
static int realloc_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Three original insertion sites expose different scratch slots to callbacks. */
static void realloc_insert(unsigned f,unsigned kind){
 static const unsigned headslot[]={64,72,68},scan[]={88,112,92},nodeslot[]={236,228,208},linkslot[]={180,204,216},prevslot[]={220,156,224};
 if(LO16(g_edi)<128){
  if(kind==0){g_ecx=LO16(g_edi);g_eax+=8*g_ecx+0x180;}
  else if(kind==1){g_eax=LO16(g_edi);g_ecx=MEM32(f+8);g_eax=g_ecx+8*g_eax+0x180;}
  else{g_eax=LO16(g_edi);g_eax=g_edx+8*g_eax+0x180;}
  MEM32(f-headslot[kind])=g_eax;
  if(MEM32(g_eax)==g_eax){
   g_ecx=MEM16(g_esi);
   if(kind<2){g_edx=g_ecx>>3;MEM32(f-(kind?252:212))=g_edx;g_ecx&=7;g_ebx=1u<<g_ecx;MEM32(f-(kind?196:172))=g_ebx;g_ecx=MEM32(f+8)+g_edx+0x160;g_edx=MEM8(g_ecx)|g_ebx;MEM8(g_ecx)=LO8(g_edx);}
   else{g_ebx=g_ecx>>3;MEM32(f-84)=g_ebx;g_ecx&=7;g_ebx=1u<<g_ecx;MEM32(f-200)=g_ebx;g_ecx=MEM32(f-84);g_edx+=g_ecx+0x160;g_ecx=MEM8(g_edx)|g_ebx;MEM8(g_edx)=LO8(g_ecx);}
  }
 }else{
  if(kind==0)g_edx=g_eax+0x180;else if(kind==1)g_edx=MEM32(f+8)+0x180;else g_edx+=0x180;
  MEM32(f-headslot[kind])=g_edx;g_eax=MEM32(g_edx);
  for(;;){MEM32(f-scan[kind])=g_eax;if(g_edx==g_eax)break;g_ecx=g_eax-8;MEM32(f-nodeslot[kind])=g_ecx;if(LO16(g_edi)<=MEM16(g_ecx))break;g_eax=MEM32(g_eax);}
  MEM32(f-headslot[kind])=g_eax;
 }
 MEM32(f-linkslot[kind])=g_eax;g_edx=MEM32(g_eax+4);MEM32(f-prevslot[kind])=g_edx;g_ecx=g_esi+8;
 MEM32(g_ecx)=g_eax;MEM32(g_esi+12)=g_edx;MEM32(g_edx)=g_ecx;MEM32(g_eax+4)=g_ecx;g_eax=MEM32(f-44);MEM32(g_eax+48)+=g_edi;
}
static void realloc_clear(unsigned count){g_ecx=count>>2;memset((void*)XBOX_PTR(g_edi),0,g_ecx*4);g_edi+=g_ecx*4;g_ecx=count&3;memset((void*)XBOX_PTR(g_edi),0,g_ecx);g_edi+=g_ecx;g_ecx=0;}
int uc_migration_services_heap_realloc(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_HEAP_REALLOC");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x844cc||MEM8(0x51c178)||!realloc_span(g_esp-512,536)||!realloc_span(MEM32(g_esp+4),0x584))return 0;
 unsigned ptr=MEM32(g_esp+12);if(ptr&&!realloc_span(ptr-48,48))return 0;
 unsigned f=g_esp-4,old=g_seh_ebp;
 PUSH32(g_esp,316);PUSH32(g_esp,0x455718);PUSH32(g_esp,0x844db);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=316;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x844db;MEM32(f-4)=0xffffffff;MEM32(f-8)=0x455718;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;
 g_eax=MEM32(f+8);MEM32(f-44)=g_eax;MEM8(f-29)=0;g_esi=MEM32(f+16);if(!g_esi){g_eax=0;goto leave;}
 MEM32(f-116)=g_esi;g_ecx=MEM32(g_eax+24);MEM32(f+12)|=g_ecx;g_edx=MEM32(f+20);if(g_edx>0x7fffffff){g_eax=0;goto leave;}
 if(!g_edx)g_edx=1;g_ecx=(MEM32(g_eax+80)+g_edx)&MEM32(g_eax+84);MEM32(f-28)=g_ecx;
 if((MEM32(f+12)&0x3c000100)||MEM32(g_eax+0x17c)||(MEM8(g_esi-11)&2)){g_ecx+=16;MEM32(f-28)=g_ecx;}
 MEM32(f-4)=0;if(!(MEM8(f+12)&1)){unsigned saved=g_esp;PUSH32(g_esp,MEM32(g_eax+0x580));unsigned target=MEM32(0x44b3f0);PUSH32(g_esp,0x8454b);RECOMP_ICALL_SAFE(target,saved);MEM8(f-29)=1;MEM32(f+12)^=1;}
 MEM32(f-4)=1;g_ebx=g_esi-16;MEM32(f-100)=g_ebx;SET_LO8(g_eax,MEM8(g_ebx+5));if(!(LO8(g_eax)&1))goto cleanup;
 SET_LO8(g_ecx,LO8(g_eax)&8);
 if(LO8(g_ecx)){g_edx=g_ebx-32;MEM32(f-240)=g_edx;g_eax=MEM16(g_ebx);g_esi=MEM32(g_edx+24)-g_eax;MEM32(f-48)=g_esi;g_edx=(g_eax+g_esi)>>4;MEM32(f-80)=g_edx;MEM32(f-28)+=32;g_edi=(MEM32(f-28)+4095)&0xfffff000;MEM32(f-28)=g_edi;}
 else{g_eax=MEM16(g_ebx);g_edx=g_eax;MEM32(f-80)=g_edx;g_edi=MEM8(g_ebx+6);g_esi=(g_edx<<4)-g_edi;MEM32(f-48)=g_esi;}
 g_edi=MEM32(f-28)>>4;MEM32(f-56)=g_edi;
 if(g_edi>g_edx)goto grow;
 g_edi++;if(g_edi==g_edx){MEM32(f-56)=g_edi;MEM32(f-28)+=16;}
 if(LO8(g_ecx)){g_eax=MEM32(f-28)-MEM32(f+20);MEM16(g_ebx)=LO16(g_eax);}
 else{
  if(MEM8(g_ebx+5)&2){g_eax=(g_eax<<4)+g_ebx-16;MEM32(f-76)=g_eax;g_ecx=MEM32(f-56)<<4;g_edi=g_ecx+g_ebx-16;MEM32(f-108)=g_edi;g_esi=g_eax;for(unsigned i=0;i<4;i++){MEM32(g_edi)=MEM32(g_esi);g_esi+=4;g_edi+=4;}}
  g_eax=MEM32(f-28)-MEM32(f+20);MEM8(g_ebx+6)=LO8(g_eax);
 }
 g_esi=MEM32(f-48);g_ecx=MEM32(f+20);
 if(g_ecx>g_esi&&(MEM8(f+12)&8)){g_ecx-=g_esi;g_eax=0;g_edi=MEM32(f+16)+g_esi;g_esi=g_ecx;realloc_clear(g_esi);}
 g_ecx=MEM32(f-56);if(g_ecx==g_edx)goto result;
 g_eax=MEM8(g_ebx+5)&0xfe;MEM8(f-57)=LO8(g_eax);
 if(LO8(g_eax)&8){
  g_ebx-=32;MEM32(f-152)=g_ebx;g_eax=MEM32(f-28);g_ecx=g_ebx+g_eax;MEM32(f-40)=g_ecx;g_edx=(g_edx<<4)-g_eax;MEM32(f-36)=g_edx;
  unsigned saved=g_esp;PUSH32(g_esp,0x8000);g_eax=f-36;PUSH32(g_esp,g_eax);g_eax=f-40;PUSH32(g_esp,g_eax);unsigned target=MEM32(0x44b3dc);PUSH32(g_esp,0x84683);RECOMP_ICALL_SAFE(target,saved);MEM32(f-248)=g_eax;
  if((int32_t)g_eax>=0){g_eax=MEM32(f-36);MEM32(g_ebx+24)-=g_eax;}goto result;
 }
 g_esi=(g_ecx<<4)+g_ebx;MEM32(f-244)=g_esi;MEM8(g_esi+5)=LO8(g_eax);MEM16(g_esi+2)=LO16(g_ecx);SET_LO8(g_edx,MEM8(g_ebx+4));MEM8(g_esi+4)=LO8(g_edx);g_edi=MEM16(g_ebx)-g_ecx;MEM32(f-120)=g_edi;MEM16(g_ebx)=LO16(g_ecx);SET_LO8(g_ecx,MEM8(g_ebx+5)&0xef);MEM8(g_ebx+5)=LO8(g_ecx);
 if(LO8(g_eax)&16){
  g_ecx=MEM8(g_esi+4);g_eax=MEM32(f+8);g_ecx=MEM32(g_eax+4*g_ecx+96);MEM32(f-164)=g_ecx;MEM32(g_ecx+64)=g_esi;MEM16(g_esi)=LO16(g_edi);SET_LO8(g_ecx,MEM8(g_esi+5)&0xf8);MEM8(g_esi+5)=LO8(g_ecx);realloc_insert(f,0);goto result;
 }
 g_eax=g_edi<<4;g_ebx=g_eax+g_esi;MEM32(f-188)=g_ebx;SET_LO8(g_eax,MEM8(g_ebx+5));
 if(LO8(g_eax)&1){MEM16(g_esi)=LO16(g_edi);MEM16(g_ebx+2)=LO16(g_edi);SET_LO8(g_eax,MEM8(g_esi+5)&0xf8);MEM8(g_esi+5)=LO8(g_eax);realloc_insert(f,1);goto result;}
 MEM8(g_esi+5)=LO8(g_eax);g_eax=MEM32(g_ebx+8);MEM32(f-160)=g_eax;g_ecx=MEM32(g_ebx+12);MEM32(f-168)=g_ecx;MEM32(g_ecx)=g_eax;MEM32(g_eax+4)=g_ecx;
 if(g_eax==g_ecx){SET_LO16(g_eax,MEM16(g_ebx));if(LO16(g_eax)<128){g_ecx=LO16(g_eax);g_eax=g_ecx>>3;MEM32(f-176)=g_eax;g_ecx&=7;g_edx=1u<<g_ecx;MEM32(f-96)=g_edx;g_edx=MEM32(f+8);g_eax+=g_edx+0x160;MEM32(f-136)=g_eax;g_eax=MEM8(g_eax)^MEM32(f-96);g_ecx=MEM32(f-136);MEM8(g_ecx)=LO8(g_eax);}else g_edx=MEM32(f+8);}else g_edx=MEM32(f+8);
 SET_LO8(g_ecx,MEM8(g_ebx+5));
 if(LO8(g_ecx)&4){
  g_eax=((unsigned)MEM16(g_ebx)<<4)-24;MEM32(f-104)=g_eax;if((LO8(g_ecx)&2)&&g_eax>4){g_eax-=4;MEM32(f-104)=g_eax;}
  unsigned saved=g_esp;PUSH32(g_esp,0xfeeefeee);PUSH32(g_esp,g_eax);g_eax=g_ebx+24;PUSH32(g_esp,g_eax);unsigned target=MEM32(0x44b3f4);PUSH32(g_esp,0x848bd);RECOMP_ICALL_SAFE(target,saved);MEM32(f-184)=g_eax;g_edx=MEM32(f+8);
 }
 g_eax=MEM16(g_ebx);MEM32(g_edx+48)-=g_eax;g_eax=MEM16(g_ebx);g_edi+=g_eax;MEM32(f-120)=g_edi;
 if(g_edi>0xff00){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,0x849ab);sub_00082C18();goto result;}
 MEM16(g_esi)=LO16(g_edi);if(!(MEM8(g_esi+5)&16)){g_eax=g_edi<<4;MEM16(g_eax+g_esi+2)=LO16(g_edi);}else{g_eax=MEM8(g_esi+4);g_eax=MEM32(g_edx+4*g_eax+96);MEM32(f-192)=g_eax;MEM32(g_eax+64)=g_esi;}
 SET_LO8(g_eax,MEM8(g_esi+5)&0xf8);MEM8(g_esi+5)=LO8(g_eax);realloc_insert(f,2);goto result;

grow:
 if(!LO8(g_ecx)){
  PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(f+20));PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(f+12));PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,0x849c4);sub_00082DBD();if(LO8(g_eax))goto result;g_esi=MEM32(f-48);
 }
 if(MEM8(f+12)&16){MEM32(f+16)=0;goto result;}
 MEM16(f+14)&=0xc003;g_eax=MEM8(g_ebx+5);
 if(LO8(g_eax)&2){
  MEM8(f+13)&=0xf1;g_eax=((g_eax&0xe0)|16)<<4;MEM32(f+12)|=g_eax;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x84a05);sub_00082D12();MEM32(f-76)=g_eax;MEM32(f-4)=2;SET_LO16(g_eax,MEM16(g_eax+2));
  if(LO16(g_eax)&&!(HI8(g_eax)&0x80)){g_eax=(unsigned)LO16(g_eax)<<18;MEM32(f+12)|=g_eax;}MEM32(f-4)=1;
 }else{SET_LO8(g_eax,MEM8(g_ebx+7));if(LO8(g_eax)){g_eax=(unsigned)LO8(g_eax)<<18;MEM32(f+12)|=g_eax;}}
 PUSH32(g_esp,MEM32(f+20));g_eax=MEM32(f+12)&0xfffffff7;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,0x84a6e);sub_00083B5D();MEM32(f-52)=g_eax;
 if(g_eax){
  g_eax-=16;MEM32(f-232)=g_eax;
  if(MEM8(g_eax+5)&2){
   PUSH32(g_esp,g_eax);PUSH32(g_esp,0x84a8e);sub_00082D12();g_edi=g_eax;MEM32(f-108)=g_edi;
   if(MEM8(g_ebx+5)&2){PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x84a9f);sub_00082D12();MEM32(f-76)=g_eax;g_eax=MEM32(g_eax+4);MEM32(g_edi+4)=g_eax;}
   else{g_eax=0;memset((void*)XBOX_PTR(g_edi),0,16);g_edi+=16;}
  }
  g_eax=MEM32(f+20);if(g_eax>=g_esi)g_eax=g_esi;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(f+16));PUSH32(g_esp,MEM32(f-52));PUSH32(g_esp,0x84ac5);sub_002D5990();g_esp+=12;
  if(MEM32(f+20)>g_esi&&(MEM8(f+12)&8)){g_ecx=MEM32(f+20)-g_esi;g_eax=0;g_edx=MEM32(f-52);g_edi=g_edx+g_esi;g_edx=g_ecx;realloc_clear(g_edx);}
  PUSH32(g_esp,MEM32(f+16));PUSH32(g_esp,MEM32(f+12));PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,0x84afc);sub_000842D8();g_eax=MEM32(f-52);
 }
 MEM32(f+16)=g_eax;
result:
 if(!MEM32(f+16)&&(MEM8(f+12)&4)){
  MEM32(f-332)=0xc0000017;MEM32(f-324)=0;MEM32(f-316)=1;MEM32(f-328)=0;g_eax=MEM32(f-28);MEM32(f-312)=g_eax;g_eax=f-332;
  unsigned saved=g_esp;PUSH32(g_esp,g_eax);unsigned target=MEM32(0x44b3fc);PUSH32(g_esp,0x84b46);RECOMP_ICALL_SAFE(target,saved);
 }
cleanup:
 MEM32(f-4)=0;MEM32(f-4)=0xffffffff;PUSH32(g_esp,0x84b70);sub_00084BE4();g_ecx=MEM32(f+16);
 if(MEM32(f-116)==g_ecx){
  MEM32(f-148)=g_ecx;MEM32(f-144)=0;MEM32(f-140)=0;g_eax=MEM32(0x20);g_eax=MEM32(g_eax+0x250);
  if(g_eax){g_ecx=f-148;unsigned saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,11);unsigned target=MEM32(g_eax+20);PUSH32(g_esp,0x84ba8);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(f+16);}
  MEM32(f-132)=g_ecx;g_eax=MEM32(f+20);MEM32(f-128)=g_eax;MEM32(f-124)=0;g_eax=MEM32(0x20);g_eax=MEM32(g_eax+0x250);
  if(g_eax){g_ecx=f-132;unsigned saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,10);unsigned target=MEM32(g_eax+20);PUSH32(g_esp,0x84bd7);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(f+16);}
 }
 g_eax=g_ecx;
leave:
 g_seh_ebp=f;PUSH32(g_esp,0x84be1);g_ecx=MEM32(f-16);MEM32(0)=g_ecx;POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);PUSH32(g_esp,g_ecx);g_esp+=24;return 1;
}
