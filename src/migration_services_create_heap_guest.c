#include "migration_services_create_heap_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000831EA(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void invoke(unsigned slot,unsigned pc,unsigned saved){uint32_t target=MEM32(slot);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
static void query(unsigned f,unsigned base,unsigned pc){uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,base);if(pc==0x838e0)g_esi=MEM32(0x44b3e0);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(g_esi,saved);(void)f;}
static void create_heap(void){
 uint32_t f=g_esp-4,old=g_seh_ebp;PUSH32(g_esp,104);PUSH32(g_esp,0x4556e8);PUSH32(g_esp,0x83745);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=104;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x83745;MEM32(f-4)=0xffffffff;MEM32(f-8)=0x4556e8;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;
 g_esi=0;MEM32(f-28)=0;g_ebx=0;PUSH32(g_esp,12);POP32(g_esp,g_ecx);g_eax=0;g_edi=f-120;memset((void*)XBOX_PTR(g_edi),0,48);g_edi+=48;g_ecx=0;g_eax=MEM32(f+28);
 if(g_eax){
  MEM32(f-4)=g_esi;
  if(MEM32(g_eax)==48){
   /* The parameter record is exactly48 bytes and the local destination is
    * word aligned. Preserve the shared copy routine's frame and dead slots. */
   PUSH32(g_esp,48);PUSH32(g_esp,g_eax);g_eax=f-120;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x83771);PUSH32(g_esp,g_ebp);uint32_t copy=g_esp;g_ebp=copy;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);
   g_esi=MEM32(copy+12);g_ecx=MEM32(copy+16);g_edi=MEM32(copy+8);g_eax=g_ecx+g_esi;g_edx=0;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;g_eax=MEM32(copy+8);POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_esp=copy+8;g_esp+=12;
  }
  MEM32(f-4)=0xffffffff;
 }
 if(MEM8(0x5206ca)&32)MEM8(f+8)|=128;
 static const unsigned defaults[4]={0x4ea4d0,0x4ea4d4,0x4ea4dc,0x4ea4d8};
 for(unsigned i=0;i<4;i++)if(!MEM32(f-116+i*4)){g_eax=MEM32(defaults[i]);MEM32(f-116+i*4)=g_eax;}
 if(!MEM32(f-100))MEM32(f-100)=0x7ffdefff;
 if(!MEM32(f-96)||MEM32(f-96)>0xff000)MEM32(f-96)=0xff000;
 g_eax=MEM32(f+20);
 if(!g_eax){MEM32(f+20)=0x1000;g_eax=MEM32(f+16);if(!g_eax)MEM32(f+16)=0x40000;else{g_eax=(g_eax+0xfff)&~0xfffu;MEM32(f+16)=g_eax;}}
 else{g_eax+=0xfff;g_ecx=0xfffff000;g_eax&=g_ecx;MEM32(f+20)=g_eax;g_edx=MEM32(f+16);if(!g_edx){g_eax=(g_eax+0xffff)&~0xffffu;MEM32(f+16)=g_eax;}else{g_edx=(g_edx+0xfff)&g_ecx;MEM32(f+16)=g_edx;}}
 MEM32(f-36)=0x590;
 if(!(MEM8(f+8)&1)){if(MEM32(f+24))MEM8(f+11)|=128;else{MEM32(f-36)=0x5ac;MEM32(f+24)=0xffffffff;}}
 else if(MEM32(f+24))goto failed;
 g_edi=MEM32(f+12);
 if(g_edi){
  if(MEM32(f-84)){
   g_ebx=MEM32(f-92);if(!g_ebx)goto failed;g_eax=MEM32(f-88);if(!g_eax||g_ebx>g_eax||(MEM8(f+8)&2))goto failed;
   MEM32(f-32)=g_edi;g_ebx+=g_edi;MEM32(f+16)=g_eax;g_ecx=0x400;g_eax=0;memset((void*)XBOX_PTR(g_edi),0,4096);g_edi+=4096;g_ecx=0;
  }else{
   g_eax=f-72;query(f,MEM32(f+12),0x838e0);if((int32_t)g_eax<0)goto failed;
   g_edi=MEM32(f-72);if(g_edi!=MEM32(f+12)||MEM32(f-56)==0x10000)goto failed;
   MEM32(f-32)=g_edi;g_eax=0x1000;
   if(MEM32(f-56)==g_eax){
    g_ecx=0x400;g_eax=0;memset((void*)XBOX_PTR(g_edi),0,4096);g_edi+=4096;g_ecx=0;g_eax=MEM32(f-60);MEM32(f+20)=g_eax;g_ecx=MEM32(f-32);g_ebx=g_eax+g_ecx;g_eax=f-72;query(f,g_ebx,0x8392a);
    g_ecx=MEM32(f+20);MEM32(f+16)=g_ecx;if((int32_t)g_eax>=0&&MEM32(f-56)==0x2000){g_eax=MEM32(f-60)+g_ecx;MEM32(f+16)=g_eax;}
   }else{MEM32(f+20)=g_eax;g_ebx=g_edi;}
  }
  MEM32(f-40)=1;g_eax=MEM32(f+12);MEM32(f-28)=g_eax;
 }else{
  if(MEM32(f-84))goto failed;
  {uint32_t saved=g_esp;PUSH32(g_esp,4);PUSH32(g_esp,0x2000);g_eax=f+16;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_eax=f-28;PUSH32(g_esp,g_eax);invoke(0x44b3d8,0x8397a,saved);}
  if((int32_t)g_eax<0)goto failed;MEM32(f-40)=g_esi;if(!MEM32(f+20))MEM32(f+20)=0x1000;g_ebx=MEM32(f-28);MEM32(f-32)=g_ebx;
 }
 if(MEM32(f-32)==g_ebx){
  {uint32_t saved=g_esp;PUSH32(g_esp,4);PUSH32(g_esp,0x1000);g_eax=f+20;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_eax=f-32;PUSH32(g_esp,g_eax);invoke(0x44b3d8,0x839b3,saved);}
  if((int32_t)g_eax<0){if(!MEM32(f+12)){uint32_t saved=g_esp;PUSH32(g_esp,0x8000);g_eax=f+16;PUSH32(g_esp,g_eax);g_eax=f-28;PUSH32(g_esp,g_eax);invoke(0x44b3dc,0x839d4,saved);}goto failed;}
  g_ebx+=MEM32(f+20);
 }
 g_edx=MEM32(f-28);g_eax=(g_edx+0x597)&~7u;g_esi=MEM32(f-36);g_edi=128;g_esi+=g_edi;g_edx+=0x4c;PUSH32(g_esp,8);POP32(g_esp,g_ecx);
 do{MEM32(g_edx)=g_eax;g_edx=g_eax;g_eax+=16;g_ecx--;}while(g_ecx);g_ecx=g_eax;MEM32(g_edx)=0;
 if(MEM8(0x5206c9)&8){g_eax=(g_eax+7)&~7u;g_ecx=MEM32(f-28);MEM32(g_ecx+0x17c)=g_eax;g_eax=0x60c;g_esi+=g_eax;g_ecx=MEM32(f-28);g_ecx=MEM32(g_ecx+0x17c)+g_eax;}
 g_esi=(g_esi+15)&~15u;g_eax=g_esi>>4;g_edx=MEM32(f-28);MEM16(g_edx)=LO16(g_eax);g_eax=MEM32(f-28);MEM8(g_eax+5)=1;g_eax=MEM32(f-28);MEM32(g_eax+16)=0xeeffeeff;
 g_edx=MEM32(f-28);g_eax=MEM32(f+8);MEM32(g_edx+20)=g_eax;g_eax&=0x6001007d;g_edx=MEM32(f-28);MEM32(g_edx+24)=g_eax;g_eax=MEM32(f-28);MEM16(g_eax+0x170)=0xffff;
 g_edx=g_ecx;g_eax=MEM32(f-28);g_edx-=g_eax;MEM16(g_eax+0x3a)=LO16(g_edx);g_eax=MEM32(f-28);MEM32(g_eax+0x3c)=0;g_eax=MEM32(f-28)+0x180;
 do{MEM32(g_eax+4)=g_eax;MEM32(g_eax)=g_eax;g_eax+=8;g_edi--;}while(g_edi);
 g_eax=MEM32(f-28);g_edx=g_eax+0x58;MEM32(g_eax+0x5c)=g_edx;g_eax=MEM32(f-28);g_edx=MEM32(g_eax+0x5c);MEM32(g_eax+0x58)=g_edx;g_edi=MEM32(f+24);
 if(g_edi==0xffffffff){g_edi=g_ecx;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);invoke(0x44b3f8,0x83abb,saved);}
 g_eax=MEM32(f-28);MEM32(g_eax+0x580)=g_edi;g_eax=MEM32(f-32);g_ecx=MEM32(f+16)+g_eax;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(f-40));PUSH32(g_esp,0);g_eax=MEM32(f-28);g_esi+=g_eax;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x83ae0);sub_000831EA();if(!LO8(g_eax))goto failed;
 g_eax=MEM32(f-28);MEM16(g_eax+0x38)=0;g_eax=MEM32(f-28);g_ecx=MEM32(f-116);MEM32(g_eax+32)=g_ecx;g_eax=MEM32(f-28);g_ecx=MEM32(f-112);MEM32(g_eax+36)=g_ecx;
 g_eax=MEM32(f-108)>>4;g_ecx=MEM32(f-28);MEM32(g_ecx+40)=g_eax;g_eax=MEM32(f-104)>>4;g_ecx=MEM32(f-28);MEM32(g_ecx+44)=g_eax;
 g_eax=MEM32(f-28);g_ecx=MEM32(f-100);MEM32(g_eax+52)=g_ecx;g_eax=(MEM32(f-96)+15)>>4;g_ecx=MEM32(f-28);MEM32(g_ecx+28)=g_eax;
 g_eax=MEM32(f-28);g_ecx=MEM32(f-84);MEM32(g_eax+0x584)=g_ecx;g_eax=MEM32(f-28);MEM32(g_eax+0x50)=31;g_eax=MEM32(f-28);MEM32(g_eax+0x54)=0xfffffff0;g_eax=MEM32(f-28);goto done;
failed:g_eax=0;
done:g_seh_ebp=f;PUSH32(g_esp,0x83b5a);MEM32(0)=MEM32(f-16);POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);MEM32(g_esp-4)=g_ecx;g_esp+=28;
}
int uc_migration_services_create_heap(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_CREATE_HEAP");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x83739||(g_esp&3)||!span(g_esp-512,544)||MEM8(0x51c178))return 0;
 uint32_t param=MEM32(g_esp+24),base=MEM32(g_esp+8),reserve=MEM32(g_esp+12),commit=MEM32(g_esp+16);
 if(param&&(!span(param,48)||(param<g_esp+32&&(uint64_t)param+48>g_esp-512)))return 0;
 if(reserve>0x200000||commit>0x200000||(base&&!span(base,reserve?reserve:0x40000)))return 0;
 create_heap();return 1;
}
