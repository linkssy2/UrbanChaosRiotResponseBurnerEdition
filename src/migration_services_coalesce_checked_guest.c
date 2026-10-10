#include "migration_services_coalesce_checked_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_SERVICES_COALESCE_CHECKED");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
/* Detach a free entry; preserve the high register bytes used by the original
 * small-bin bitmap operations, even though they are not pointer values. */
static void detach(uint32_t block,uint32_t heap,uint32_t count,unsigned pc){
 g_ecx=MEM32(block+8);g_edx=MEM32(block+12);
 MEM32(g_edx)=g_ecx;MEM32(g_ecx+4)=g_edx;
 if(g_ecx==g_edx){
  SET_LO16(g_ecx,MEM16(block));
  if(LO16(g_ecx)<128){g_ecx=LO16(g_ecx);g_eax=g_ecx;g_ecx&=7;SET_LO8(g_edx,1u<<g_ecx);g_eax=(g_eax>>3)+heap+0x160;MEM8(g_eax)^=LO8(g_edx);g_eax=count;}
 }
 SET_LO8(g_ecx,MEM8(block+5));
 if(LO8(g_ecx)&4){
  g_eax=MEM16(block)*16u-24;if((LO8(g_ecx)&2)&&g_eax>4)g_eax-=4;
  uint32_t saved=g_esp;PUSH32(g_esp,0xfeeefeee);PUSH32(g_esp,g_eax);g_eax=block+24;PUSH32(g_esp,g_eax);
  uint32_t target=MEM32(0x44b3f4);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_ebp+16);
 }
}
static void remove_current(uint32_t heap,uint32_t block,uint32_t count,unsigned pc){
 detach(block,heap,count,pc);g_ecx=MEM16(block);MEM32(heap+0x30)-=g_ecx;
}
static void coalesce(void){
 uint32_t s=g_esp,f=s-4;
 PUSH32(g_esp,g_ebp);g_ebp=f;PUSH32(g_esp,g_ebx);g_ebx=MEM32(f+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=MEM32(f+12);g_eax=MEM16(g_edi+2)*16u;g_esi=g_edi-g_eax;
 uint32_t heap=g_ebx,count=MEM32(f+16);
 if(g_esi!=g_edi&&!(MEM8(g_esi+5)&1)){
  g_ecx=MEM16(g_esi);g_eax=count;g_ecx+=MEM32(count);
  if(g_ecx<=0xff00){
   if(MEM8(f+20)){remove_current(heap,g_edi,count,0x82a18);MEM8(f+20)=0;}
   detach(g_esi,heap,count,0x82a87);
   SET_LO8(g_ecx,MEM8(g_edi+5)&16);MEM8(g_esi+5)=LO8(g_ecx);
   if(LO8(g_ecx)){g_ecx=MEM8(g_esi+4);g_ecx=MEM32(heap+0x60+4*g_ecx);MEM32(g_ecx+0x40)=g_esi;}
   g_ecx=MEM16(g_esi);MEM32(g_eax)+=g_ecx;g_ecx=MEM16(g_esi);MEM32(heap+0x30)-=g_ecx;
   SET_LO16(g_ecx,MEM16(g_eax));g_edi=g_esi;MEM16(g_esi)=LO16(g_ecx);
   if(!(MEM8(g_esi+5)&16)){g_ecx=MEM32(g_eax);SET_LO16(g_edx,MEM16(g_eax));g_ecx*=16;MEM16(g_ecx+g_esi+2)=LO16(g_edx);}
  }
 }else g_eax=count;
 if(!(MEM8(g_edi+5)&16)){
  g_ecx=MEM32(g_eax);g_esi=g_edi+16*g_ecx;
  if(!(MEM8(g_esi+5)&1)){
   g_edx=MEM16(g_esi)+g_ecx;
   if(g_edx<=0xff00){
    if(MEM8(f+20))remove_current(heap,g_edi,count,0x82b64);
    SET_LO8(g_ecx,MEM8(g_esi+5)&16);MEM8(g_edi+5)=LO8(g_ecx);
    if(LO8(g_ecx)){g_ecx=MEM8(g_edi+4);g_ecx=MEM32(heap+0x60+4*g_ecx);MEM32(g_ecx+0x40)=g_edi;}
    detach(g_esi,heap,count,0x82be8);
    g_ecx=MEM16(g_esi);MEM32(g_eax)+=g_ecx;g_ecx=MEM16(g_esi);MEM32(heap+0x30)-=g_ecx;
    SET_LO16(g_ecx,MEM16(g_eax));MEM16(g_edi)=LO16(g_ecx);
    if(!(MEM8(g_edi+5)&16)){g_ecx=MEM32(g_eax);SET_LO16(g_eax,MEM16(g_eax));g_ecx*=16;MEM16(g_ecx+g_edi+2)=LO16(g_eax);}
   }
  }
 }
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=24;
}
static int free_links(uint32_t p,uint32_t heap,uint32_t stack,uint32_t count){
 uint32_t n=MEM32(p+8),b=MEM32(p+12);
 if(!span(n,8)||!span(b,8)||!apart(n,8,stack-16,40)||!apart(b,8,stack-16,40)||!apart(n,8,count,4)||!apart(b,8,count,4))return 0;
 if(MEM32(n+4)!=p+8||MEM32(b)!=p+8)return 0;
 if(MEM16(p)<128&&!(MEM8(heap+0x160+(MEM16(p)>>3))&(1u<<(MEM16(p)&7))))return 0;
 return 1;
}
int uc_migration_services_coalesce_checked(unsigned entry){
 if(!enabled()||entry!=0x82973)return 0;
 uint32_t s=g_esp;
 if((s&3)||!span(s-16,40)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t h=MEM32(s+4),p=MEM32(s+8),c=MEM32(s+12);
 if((h&15)||(p&15)||!span(h,0x584)||!span(p,16)||!span(c,4)||!apart(h,0x584,s-16,40)||!apart(p,16,s-16,40)||!apart(c,4,s-16,40)||!apart(c,4,h,0x584))return 0;
 unsigned units=MEM32(c);if(units<2||units>0xff00||MEM16(p)!=units)return 0;
 uint32_t prev=p-MEM16(p+2)*16u,next=p+units*16u;
 if(!span(prev,16)||!span(p,units*16)||!apart(prev,16,h,0x584)||!apart(p,units*16,h,0x584)||!apart(p,units*16,c,4)||!apart(p,units*16,s-16,40))return 0;
 /* Preserve the guest debug comparison callback, including its live ABI. */
 if(MEM8(p+5)&8)return 0;
 if(prev!=p&&!(MEM8(prev+5)&1)){
  if(!MEM16(prev)||prev+MEM16(prev)*16u!=p||(MEM8(prev+5)&8)||!apart(prev,MEM16(prev)*16u,c,4)||!apart(prev,MEM16(prev)*16u,s-16,40)||!free_links(prev,h,s,c))return 0;
 }
 if(!(MEM8(p+5)&16)){
  if(!span(next,16)||!apart(next,16,h,0x584)||!apart(next,16,s-16,40)||!apart(next,16,c,4))return 0;
  if(!(MEM8(next+5)&1)){
   unsigned n=MEM16(next);if(n<2||!span(next,n*16+16)||(MEM8(next+5)&8)||!apart(next,n*16+16,h,0x584)||!apart(next,n*16+16,c,4)||!apart(next,n*16+16,s-16,40)||!free_links(next,h,s,c))return 0;
  }
 }
 if(MEM8(s+16)&&!free_links(p,h,s,c))return 0;
 uint32_t seg=MEM32(h+0x60+4*MEM8(p+4));
 if(!span(seg,0x44)||!apart(seg,0x44,s-16,40)||!apart(seg,0x44,c,4))return 0;
 if(prev!=p&&MEM8(prev+4)!=MEM8(p+4))return 0;
 coalesce();return 1;
}
