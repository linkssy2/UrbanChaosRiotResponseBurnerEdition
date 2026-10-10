#include "migration_services_free_full_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00082973(void),sub_0008348F(void),sub_00082C18(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static void invoke(unsigned slot,unsigned pc,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
static void release(uint32_t s){
 uint32_t f=s-4,old=g_seh_ebp;
 PUSH32(g_esp,92);PUSH32(g_esp,0x455708);PUSH32(g_esp,0x842e4);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);
 MEM32(f)=old;g_esp-=92;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x842e4;
 MEM32(f-4)=0xffffffffu;MEM32(f-8)=0x455708;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;
 g_esi=MEM32(f+8);g_ebx=g_esi;MEM32(f-36)=g_ebx;MEM8(f-25)=0;MEM8(f-26)=1;g_edi=MEM32(f+16);
 if(!g_edi){SET_LO8(g_eax,1);goto finish;}
 MEM32(f-108)=g_edi;MEM32(f-104)=0;MEM32(f-100)=0;g_eax=MEM32(MEM32(0x20)+0x250);
 if(g_eax){g_ecx=f-108;uint32_t saved=g_esp,target=MEM32(g_eax+20);PUSH32(g_esp,g_ecx);PUSH32(g_esp,11);PUSH32(g_esp,0x84326);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(f+12)|MEM32(g_esi+24);g_edi-=16;MEM32(f-4)=0;
 if(!(g_eax&1)){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esi+0x580));invoke(0x44b3f0,0x84343,saved);MEM8(f-25)=1;}
 if(MEM8(g_edi+5)&8){
  g_eax=g_edi-32;MEM32(f-44)=g_eax;g_ecx=MEM32(g_eax);MEM32(f-88)=g_ecx;g_eax=MEM32(g_eax+4);MEM32(f-92)=g_eax;MEM32(g_eax)=g_ecx;MEM32(g_ecx+4)=g_eax;
  if(MEM8(f-25)){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esi+0x580));invoke(0x44b3ec,0x8447c,saved);MEM8(f-25)=0;}
  MEM32(f-32)=0;uint32_t saved=g_esp;PUSH32(g_esp,0x8000);g_eax=f-32;PUSH32(g_esp,g_eax);g_eax=f-44;PUSH32(g_esp,g_eax);invoke(0x44b3dc,0x84497,saved);
  MEM32(f-96)=g_eax;if((int32_t)g_eax<0)MEM8(f-26)=0;
 }else{
  g_eax=MEM16(g_edi);MEM32(f-32)=g_eax;PUSH32(g_esp,0);g_eax=f-32;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x84364);sub_00082973();
  MEM32(f-48)=g_eax;g_ecx=MEM32(f-32);
  if(g_ecx<128){
   g_ecx=MEM8(g_eax+5)&16;MEM8(g_eax+5)=(uint8_t)g_ecx;g_ecx=MEM16(f-32);g_edx=g_esi+g_ecx*8+0x180;MEM32(f-52)=g_edx;
   if(MEM32(g_edx)==g_edx){g_ecx=MEM16(g_eax);g_edi=g_ecx>>3;MEM32(f-56)=g_edi;g_ecx&=7;g_ebx=1u<<g_ecx;MEM32(f-60)=g_ebx;g_edi+=g_esi+0x160;g_ecx=MEM8(g_edi)|g_ebx;MEM8(g_edi)=(uint8_t)g_ecx;g_ebx=MEM32(f-36);}
   MEM32(f-64)=g_edx;g_edi=MEM32(g_edx+4);MEM32(f-68)=g_edi;g_ecx=g_eax+8;MEM32(g_ecx)=g_edx;MEM32(g_eax+12)=g_edi;MEM32(g_edi)=g_ecx;MEM32(g_edx+4)=g_ecx;
   g_eax=MEM32(f-32);MEM32(g_esi+0x30)+=g_eax;
  }else{
   if(g_ecx>=MEM32(g_esi+0x28)){
    g_edx=MEM32(g_esi+0x30)+g_ecx;
    if(g_edx>=MEM32(g_esi+0x2c)){PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x843ef);sub_0008348F();goto cleanup;}
   }
   if(g_ecx>0xff00){PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x84452);sub_00082C18();goto cleanup;}
   g_ecx=MEM8(g_eax+5)&16;MEM8(g_eax+5)=(uint8_t)g_ecx;g_esi+=0x180;MEM32(f-72)=g_esi;g_ecx=MEM32(g_esi);
   for(;;){MEM32(f-40)=g_ecx;if(g_ecx==g_esi)break;g_edx=g_ecx-8;MEM32(f-76)=g_edx;SET_LO16(g_edi,MEM16(f-32));if(LO16(g_edi)<=MEM16(g_edx))break;g_ecx=MEM32(g_ecx);}
   MEM32(f-80)=g_ecx;g_esi=MEM32(g_ecx+4);MEM32(f-84)=g_esi;g_edx=g_eax+8;MEM32(g_edx)=g_ecx;MEM32(g_eax+12)=g_esi;MEM32(g_esi)=g_edx;MEM32(g_ecx+4)=g_edx;
   g_eax=MEM32(f-32);MEM32(g_ebx+0x30)+=g_eax;
  }
 }
cleanup:
 MEM32(f-4)=0xffffffffu;PUSH32(g_esp,0x844ab);
 if(MEM8(f-25)){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_ebx+0x580));invoke(0x44b3ec,0x844cb,saved);g_recomp_zf=0;}else g_recomp_zf=1;
 g_esp+=4;SET_LO8(g_eax,MEM8(f-26));
finish:
 g_seh_ebp=f;PUSH32(g_esp,0x844b3);MEM32(0)=MEM32(f-16);POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);MEM32(g_esp-4)=g_ecx;g_esp+=16;
}
int uc_migration_services_free_full(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FREE_FULL");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x842d8)return 0;
 uint32_t s=g_esp;if((s&3)||!span(s-256,272)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t h=MEM32(s+4),ptr=MEM32(s+12);if(!ptr){release(s);return 1;}
 uint32_t t=MEM32(0x20),b=ptr-16;
 if((h&15)||(b&15)||!span(h,0x584)||!span(t,0x254)||!span(b-32,48)||!apart(h,0x584,s-256,272)||!apart(b-32,48,s-256,272))return 0;
 if(MEM8(b+5)&8){
  uint32_t n=MEM32(b-32),p=MEM32(b-28);
  if(!span(n,8)||!span(p,8)||!apart(n,8,s-256,272)||!apart(p,8,s-256,272)||MEM32(n+4)!=b-32||MEM32(p)!=b-32)return 0;
 }else{
  unsigned units=MEM16(b),merged=units;uint32_t first=b,prev=b-MEM16(b+2)*16u,next=b+units*16u;
  if(!(MEM8(b+5)&1)||units<2||units>0xff00||!span(prev,16)||!span(b,units*16)||!apart(b,units*16,s-256,272)||!apart(b,units*16,h,0x584))return 0;
  if(prev!=b&&!(MEM8(prev+5)&1)&&merged+MEM16(prev)<=0xff00){merged+=MEM16(prev);first=prev;}
  if(!(MEM8(b+5)&16)){
   if(!span(next,16))return 0;
   if(!(MEM8(next+5)&1)&&merged+MEM16(next)<=0xff00)merged+=MEM16(next);
  }

  uint32_t head=h+0x180+(merged<128?merged*8:0),node=MEM32(head),back=head;unsigned visited=0;
  while(node!=head){if(++visited>4096||node<8||!span(node-8,16)||!apart(node-8,16,s-256,272)||MEM32(node+4)!=back)return 0;back=node;node=MEM32(node);}
  if(MEM32(head+4)!=back||!span(first,merged*16+16)||!apart(first,merged*16+16,s-256,272))return 0;
 }
 if(MEM32(t+0x250)&&!span(MEM32(t+0x250),24))return 0;
 release(s);return 1;
}
