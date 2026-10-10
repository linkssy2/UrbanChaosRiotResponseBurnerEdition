#include "migration_services_grow_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000827B6(void),sub_000831EA(void),sub_00082973(void),sub_00082C18(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static void reserve(uint32_t f,unsigned pc){
 uint32_t saved=g_esp;PUSH32(g_esp,4);PUSH32(g_esp,g_ebx);g_eax=f+8;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_eax=f-4;PUSH32(g_esp,g_eax);uint32_t target=g_esi;PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);
}
static void grow_heap(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp-=16;PUSH32(g_esp,g_ebx);g_ebx=MEM32(f+12);PUSH32(g_esp,g_esi);g_esi=(g_ebx+0xfff)>>12;g_eax=g_esi<<12;PUSH32(g_esp,g_edi);g_edi=MEM32(f+8);MEM32(f-8)=g_eax;MEM8(f-12)=64;MEM8(f+15)=0;
 do{
  g_ecx=MEM8(f+15);g_ecx=MEM32(g_edi+g_ecx*4+0x60);MEM32(f-4)=g_ecx;
  if(g_ecx){
   if(g_esi<=MEM32(g_ecx+0x30)&&g_eax<=MEM32(g_ecx+0x1c)){
    PUSH32(g_esp,0);g_eax=f-8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ebp=f;PUSH32(g_esp,0x83356);sub_000827B6();
    if(g_eax){
     MEM32(f-8)>>=4;PUSH32(g_esp,0);g_ecx=f-8;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ebp=f;PUSH32(g_esp,0x83463);sub_00082973();
     PUSH32(g_esp,MEM32(f-8));g_esi=g_eax;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_ebp=f;PUSH32(g_esp,0x8346f);sub_00082C18();g_eax=g_esi;goto done;
    }
    g_eax=MEM32(f-8);
   }
  }else if(MEM8(f-12)==64){SET_LO8(g_ecx,MEM8(f+15));MEM8(f-12)=LO8(g_ecx);}
  MEM8(f+15)++;
 }while(MEM8(f+15)<64);
 if(MEM8(f-12)==64||!(MEM8(g_edi+20)&2))goto failed;
 g_ecx=MEM32(g_edi+32);MEM32(f-4)=0;g_eax=g_ebx+0x1000;MEM32(f+12)=g_eax;MEM32(f+8)=g_eax;if(g_eax<=g_ecx)MEM32(f+8)=g_ecx;
 g_esi=MEM32(0x44b3d8);g_ebx=0x2000;reserve(f,0x833c0);
 while((int32_t)g_eax<0){
  g_ecx=MEM32(f+12);if(MEM32(f+8)==g_ecx)goto failed;
  MEM32(f+8)>>=1;if(MEM32(f+8)<g_ecx)MEM32(f+8)=g_ecx;reserve(f,0x833e6);
 }
 g_eax=MEM32(f+8);MEM32(g_edi+32)+=g_eax;g_ecx=MEM32(f+12);g_eax=MEM32(g_edi+36);MEM32(f-16)=g_ecx;if(g_ecx<=g_eax)MEM32(f-16)=g_eax;
 {uint32_t saved=g_esp;PUSH32(g_esp,4);PUSH32(g_esp,0x1000);g_eax=f-16;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_eax=f-4;PUSH32(g_esp,g_eax);uint32_t target=g_esi;PUSH32(g_esp,0x8341b);RECOMP_ICALL_SAFE(target,saved);}
 g_esi=g_eax;
 if((int32_t)g_esi>=0){
  g_eax=MEM32(f-4);g_ecx=MEM32(f+8)+g_eax;PUSH32(g_esp,g_ecx);g_ecx=MEM32(f-16)+g_eax;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,MEM32(f-12));PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ebp=f;PUSH32(g_esp,0x8343d);sub_000831EA();
  if(!LO8(g_eax))g_esi=0xc0000017;
  if((int32_t)g_esi>=0){g_eax=MEM32(f-4);g_eax=MEM32(g_eax+0x28);goto done;}
 }
 {uint32_t saved=g_esp;PUSH32(g_esp,0x8000);g_eax=f+8;PUSH32(g_esp,g_eax);g_eax=f-4;PUSH32(g_esp,g_eax);uint32_t target=MEM32(0x44b3dc);PUSH32(g_esp,0x83486);RECOMP_ICALL_SAFE(target,saved);}
failed:g_eax=0;
done:POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+16;
}
int uc_migration_services_grow(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_GROW");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x83308)return 0;
 uint32_t s=g_esp;if((s&3)||!span(s-512,528)||MEM8(0x51c178))return 0;
 uint32_t heap=MEM32(s+4),bytes=MEM32(s+8);
 if((heap&15)||!span(heap,0x588)||!apart(heap,0x588,s-512,528)||!bytes||bytes>0xfe000||MEM32(heap+32)>0x1000000||MEM32(heap+36)>0x1000000)return 0;
 for(unsigned i=0;i<64;i++){uint32_t seg=MEM32(heap+0x60+i*4);if(seg&&(!span(seg,0x44)||!apart(seg,0x44,s-512,528)||MEM32(seg+24)!=heap))return 0;}
 grow_heap();return 1;
}
