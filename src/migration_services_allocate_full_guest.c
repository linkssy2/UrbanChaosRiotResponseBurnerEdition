#include "migration_services_allocate_full_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00082C18(void),sub_00083308(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static void unlock(unsigned pc){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_ebx+0x580));uint32_t target=MEM32(0x44b3ec);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
/* The three insertion contexts have distinct observable scratch slots. The
 * list operation is shared; this table preserves their original stack ABI. */
static void insert_remainder(uint32_t f,unsigned context){
 static const unsigned small[3][6]={{200,132,264,100,208,296},{280,108,152,140,156,164},{228,112,236,120,244,252}};
 static const unsigned large[3][5]={{216,124,272,224,312},{172,116,180,188,196},{260,128,268,276,284}};
 g_eax=MEM8(g_esi+5)&16;MEM8(g_esi+5)=(uint8_t)g_eax;
 if(LO16(g_edx)<128){
  const unsigned*o=small[context];g_eax=LO16(g_edx);g_edi=g_ebx+g_eax*8+0x180;MEM32(f-o[0])=g_edi;
  if(MEM32(g_edi)==g_edi){
   g_ecx=MEM16(g_esi);g_eax=g_ecx>>3;MEM32(f-o[1])=g_eax;g_ecx&=7;g_eax=1u<<g_ecx;MEM32(f-o[2])=g_eax;
   g_ecx=MEM32(f-o[1])+g_ebx+0x160;MEM32(f-o[3])=g_ecx;g_ecx=MEM8(g_ecx)|g_eax;g_eax=MEM32(f-o[3]);MEM8(g_eax)=(uint8_t)g_ecx;
  }
  MEM32(f-o[4])=g_edi;g_ecx=MEM32(g_edi+4);MEM32(f-o[5])=g_ecx;g_eax=g_esi+8;
  MEM32(g_eax)=g_edi;MEM32(g_esi+12)=g_ecx;MEM32(g_ecx)=g_eax;MEM32(g_edi+4)=g_eax;
 }else{
  const unsigned*o=large[context];g_ecx=g_ebx+0x180;MEM32(f-o[0])=g_ecx;g_eax=MEM32(g_ecx);
  for(;;){MEM32(f-o[1])=g_eax;if(g_eax==g_ecx)break;g_edi=g_eax-8;MEM32(f-o[2])=g_edi;if(LO16(g_edx)<=MEM16(g_edi))break;g_eax=MEM32(g_eax);}
  MEM32(f-o[3])=g_eax;g_edi=MEM32(g_eax+4);MEM32(f-o[4])=g_edi;g_ecx=g_esi+8;
  MEM32(g_ecx)=g_eax;MEM32(g_esi+12)=g_edi;MEM32(g_edi)=g_ecx;MEM32(g_eax+4)=g_ecx;
 }
 MEM32(g_ebx+0x30)+=g_edx;
}
static void allocate(uint32_t s){
 uint32_t f=s-4,old=g_seh_ebp;
 PUSH32(g_esp,384);PUSH32(g_esp,0x4556f8);PUSH32(g_esp,0x83b6c);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=384;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x83b6c;MEM32(f-4)=0xffffffffu;MEM32(f-8)=0x4556f8;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;
 g_esi=MEM32(f+8);g_ebx=g_esi;MEM32(f-136)=g_ebx;MEM8(f-26)=0;g_ecx=MEM32(f+12)|MEM32(g_esi+24);MEM32(f+12)=g_ecx;
 g_eax=MEM32(f+16);if(!g_eax)g_eax=1;g_eax=(g_eax+31)&~15u;MEM32(f-32)=g_eax;g_edi=g_eax>>4;MEM32(f-40)=g_edi;MEM32(f-4)=0;
 if(!(g_ecx&1)){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esi+0x580));uint32_t target=MEM32(0x44b3f0);PUSH32(g_esp,0x83bb2);RECOMP_ICALL_SAFE(target,saved);MEM8(f-26)=1;}
 if(g_edi<128){
  g_eax=g_esi+g_edi*8+0x180;MEM32(f-44)=g_eax;
  if(MEM32(g_eax)!=g_eax){
   g_eax=MEM32(g_eax+4)-8;MEM32(f-52)=g_eax;SET_LO8(g_edx,MEM8(g_eax+5));MEM8(f-25)=LO8(g_edx);
   g_ecx=MEM32(g_eax+8);MEM32(f-288)=g_ecx;g_edi=MEM32(g_eax+12);MEM32(f-316)=g_edi;MEM32(g_edi)=g_ecx;MEM32(g_ecx+4)=g_edi;
   if(g_ecx==g_edi){g_ecx=MEM16(g_eax);g_edi=g_ecx>>3;MEM32(f-144)=g_edi;g_ecx&=7;g_edi=1u<<g_ecx;MEM32(f-240)=g_edi;g_ecx=MEM32(f-144)+g_esi+0x160;MEM32(f-76)=g_ecx;g_ecx=MEM8(g_ecx)^g_edi;g_edi=MEM32(f-76);MEM8(g_edi)=(uint8_t)g_ecx;}
   g_ecx=MEM32(f-40);MEM32(g_esi+0x30)-=g_ecx;MEM32(f-56)=g_eax;g_edx=(g_edx&16)|1;MEM8(g_eax+5)=(uint8_t)g_edx;
   g_ecx=MEM32(f-32)-MEM32(f+16);MEM8(g_eax+6)=(uint8_t)g_ecx;MEM8(g_eax+7)=0;goto finish;
  }
  g_ecx=MEM32(f-40);g_edx=g_ecx>>5;MEM32(f-160)=g_edx;g_edi=g_esi+g_edx*4+0x160;MEM32(f-48)=g_edi;g_ecx&=31;
  g_eax=(0xffffffffu<<g_ecx)&MEM32(g_edi);MEM32(f-64)=g_eax;g_edi+=4;MEM32(f-48)=g_edi;unsigned word=g_edx;g_edx=0;
  while(!g_eax&&word<3){word++;g_eax=MEM32(g_edi);MEM32(f-64)=g_eax;g_edi+=4;MEM32(f-48)=g_edi;}
  if(g_eax){
   g_edi=g_esi+0x180+word*256;MEM32(f-44)=g_edi;g_ecx=g_eax;MEM32(g_esp-4)=0x83cf2;g_eax=RECOMP_BSF32(g_ecx);g_recomp_zf=0;
   g_eax=g_edi+g_eax*8;MEM32(f-44)=g_eax;g_eax=MEM32(g_eax+4)-8;MEM32(f-52)=g_eax;g_ecx=MEM32(g_eax+8);MEM32(f-304)=g_ecx;g_edx=MEM32(g_eax+12);MEM32(f-168)=g_edx;MEM32(g_edx)=g_ecx;MEM32(g_ecx+4)=g_edx;
   if(g_ecx==g_edx){g_ecx=MEM16(g_eax);g_edx=g_ecx>>3;MEM32(f-248)=g_edx;g_ecx&=7;g_edi=1u<<g_ecx;MEM32(f-176)=g_edi;g_esi=g_edx+g_esi+0x160;g_ecx=MEM8(g_esi)^g_edi;MEM8(g_esi)=(uint8_t)g_ecx;}
   goto split;
  }
 }
 if(MEM32(f-40)>=128&&g_edi>MEM32(g_esi+0x1c))goto virtual_allocate;
 g_edx=g_esi+0x180;MEM32(f-44)=g_edx;g_eax=MEM32(g_edx+4);MEM32(f-68)=g_eax;
 if(g_edx==g_eax)goto grow;
 g_eax-=8;MEM32(f-52)=g_eax;g_eax=MEM16(g_eax);if(g_eax<MEM32(f-40))goto grow;g_ecx=MEM32(g_edx);
 for(;;){MEM32(f-68)=g_ecx;if(g_edx==g_ecx)goto grow;g_eax=g_ecx-8;MEM32(f-52)=g_eax;g_esi=MEM16(g_eax);if(g_esi>=MEM32(f-40))break;g_ecx=MEM32(g_ecx);}
 g_ecx=MEM32(g_eax+8);MEM32(f-148)=g_ecx;g_edx=MEM32(g_eax+12);MEM32(f-184)=g_edx;goto unlink_fit;
grow:
 PUSH32(g_esp,MEM32(f-32));PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x83db5);sub_00083308();MEM32(f-52)=g_eax;
 if(!g_eax){MEM32(f-72)=0xc0000017;goto failed;}
 g_ecx=MEM32(g_eax+8);MEM32(f-256)=g_ecx;g_edx=MEM32(g_eax+12);MEM32(f-192)=g_edx;
unlink_fit:MEM32(g_edx)=g_ecx;MEM32(g_ecx+4)=g_edx;
split:
 SET_LO8(g_ecx,MEM8(g_eax+5));MEM8(f-25)=LO8(g_ecx);g_ecx=MEM16(g_eax);MEM32(g_ebx+0x30)-=g_ecx;MEM32(f-56)=g_eax;MEM8(g_eax+5)=1;
 g_edx=MEM16(g_eax);g_ecx=MEM32(f-40);g_edx-=g_ecx;MEM32(f-104)=g_edx;MEM16(g_eax)=(uint16_t)g_ecx;g_ecx=MEM32(f-32)-MEM32(f+16);MEM8(g_eax+6)=(uint8_t)g_ecx;MEM8(g_eax+7)=0;
 if(g_edx==1){g_ecx=MEM16(g_eax)+1;MEM16(g_eax)=(uint16_t)g_ecx;g_ecx=MEM8(g_eax+6)+16;MEM8(g_eax+6)=(uint8_t)g_ecx;}
 else if(g_edx>1){
  g_esi=MEM32(f-40)*16+g_eax;MEM32(f-320)=g_esi;SET_LO8(g_ecx,MEM8(f-25));MEM8(g_esi+5)=LO8(g_ecx);SET_LO16(g_edi,MEM16(f-40));MEM16(g_esi+2)=LO16(g_edi);SET_LO8(g_eax,MEM8(g_eax+4));MEM8(g_esi+4)=LO8(g_eax);MEM16(g_esi)=LO16(g_edx);
  if(MEM8(g_esi+5)&16)insert_remainder(f,0);
  else{
   g_eax=g_edx*16+g_esi;MEM32(f-232)=g_eax;SET_LO8(g_ecx,MEM8(g_eax+5));
   if(LO8(g_ecx)&1){MEM16(g_eax+2)=LO16(g_edx);insert_remainder(f,1);}
   else{
    MEM8(g_esi+5)=LO8(g_ecx);g_ecx=MEM32(g_eax+8);MEM32(f-204)=g_ecx;g_edi=MEM32(g_eax+12);MEM32(f-212)=g_edi;MEM32(g_edi)=g_ecx;MEM32(g_ecx+4)=g_edi;
    if(g_ecx==g_edi){SET_LO16(g_ecx,MEM16(g_eax));if(LO16(g_ecx)<128){g_ecx=LO16(g_ecx);g_edi=g_ecx>>3;MEM32(f-80)=g_edi;g_ecx&=7;g_edi=1u<<g_ecx;MEM32(f-220)=g_edi;g_ecx=MEM32(f-80)+g_ebx+0x160;MEM32(f-96)=g_ecx;g_ecx=MEM8(g_ecx)^g_edi;g_edi=MEM32(f-96);MEM8(g_edi)=(uint8_t)g_ecx;}}
    g_ecx=MEM16(g_eax);MEM32(g_ebx+0x30)-=g_ecx;g_eax=MEM16(g_eax);g_edx+=g_eax;MEM32(f-104)=g_edx;
    if(g_edx>0xff00){PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x8411b);sub_00082C18();}
    else{MEM16(g_esi)=LO16(g_edx);if(!(MEM8(g_esi+5)&16)){g_eax=g_edx*16;MEM16(g_eax+g_esi+2)=LO16(g_edx);}insert_remainder(f,2);}
   }
  }
  MEM8(f-25)=0;if(MEM8(g_esi+5)&16){g_eax=MEM8(g_esi+4);g_eax=MEM32(g_ebx+g_eax*4+0x60);MEM32(f-292)=g_eax;MEM32(g_eax+0x40)=g_esi;}
 }
 if(MEM8(f-25)&16){g_eax=MEM32(f-56);SET_LO8(g_ecx,MEM8(g_eax+5));SET_LO8(g_ecx,LO8(g_ecx)|16);MEM8(g_eax+5)=LO8(g_ecx);}
finish:
 g_esi=MEM32(f-56)+16;MEM32(f-60)=g_esi;if(MEM8(f-26)){unlock(0x84163);MEM8(f-26)=0;}
 if(MEM8(f+12)&8){g_edx=MEM32(f+16);memset((void*)XBOX_PTR(g_esi),0,g_edx);g_edi=g_esi+g_edx;g_ecx=0;g_eax=0;}
 goto cleanup;
virtual_allocate:
 if(!(MEM8(g_esi+20)&2)){MEM32(f-72)=0xc0000023;goto failed;}
 MEM32(f-36)=0;MEM32(f-32)+=32;PUSH32(g_esp,4);g_eax=((~(MEM32(f+12)<<20))&0x800000)|0x1000;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=f-32;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_eax=f-36;PUSH32(g_esp,g_eax);uint32_t target=MEM32(0x44b3d8);PUSH32(g_esp,0x841ce);RECOMP_ICALL_SAFE(target,saved);}
 MEM32(f-72)=g_eax;if((int32_t)g_eax<0)goto failed;
 PUSH32(g_esp,12);POP32(g_esp,g_ecx);g_eax=0;g_edi=MEM32(f-36);memset((void*)XBOX_PTR(g_edi),0,48);g_edi+=48;g_ecx=0;
 g_eax=MEM32(f-32)-MEM32(f+16);g_ecx=MEM32(f-36);MEM16(g_ecx+32)=LO16(g_eax);g_eax=MEM32(f-36);MEM8(g_eax+37)=11;
 g_eax=MEM32(f-32);g_ecx=MEM32(f-36);MEM32(g_ecx+24)=g_eax;g_eax=MEM32(f-36);g_ecx=MEM32(f-32);MEM32(g_eax+28)=g_ecx;
 g_esi+=0x58;MEM32(f-300)=g_esi;g_eax=MEM32(g_esi+4);MEM32(f-308)=g_eax;g_ecx=MEM32(f-36);MEM32(g_ecx)=g_esi;g_ecx=MEM32(f-36);MEM32(g_ecx+4)=g_eax;
 g_ecx=MEM32(f-36);MEM32(g_eax)=g_ecx;g_eax=MEM32(f-36);MEM32(g_esi+4)=g_eax;g_esi=MEM32(f-36)+48;MEM32(f-60)=g_esi;goto cleanup;
failed:
 if(MEM8(f+12)&4){MEM32(f-400)=0xc0000017;MEM32(f-392)=0;MEM32(f-384)=1;MEM32(f-396)=0;g_eax=MEM32(f-32);MEM32(f-380)=g_eax;g_eax=f-400;
  uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(0x44b3fc);PUSH32(g_esp,0x8427a);RECOMP_ICALL_SAFE(target,saved);
 }
 g_esi=0;MEM32(f-60)=g_esi;
cleanup:
 MEM32(f-4)=0xffffffffu;PUSH32(g_esp,0x84288);
 if(MEM8(f-26)){unlock(0x842d7);g_recomp_zf=0;}else g_recomp_zf=1;g_esp+=4;
 if(g_esi){MEM32(f-92)=g_esi;g_eax=MEM32(f+16);MEM32(f-88)=g_eax;MEM32(f-84)=0;g_eax=MEM32(MEM32(0x20)+0x250);
  if(g_eax){g_ecx=f-92;uint32_t saved=g_esp,target=MEM32(g_eax+20);PUSH32(g_esp,g_ecx);PUSH32(g_esp,10);PUSH32(g_esp,0x842b2);RECOMP_ICALL_SAFE(target,saved);}
 }
 g_eax=g_esi;g_seh_ebp=f;PUSH32(g_esp,0x842b9);MEM32(0)=MEM32(f-16);POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);MEM32(g_esp-4)=g_ecx;g_esp+=16;
}
int uc_migration_services_allocate_full(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_ALLOCATE_FULL");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x83b5d)return 0;uint32_t s=g_esp;
 if((s&3)||!span(s-512,528)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t h=MEM32(s+4),size=MEM32(s+12),thread=MEM32(0x20);
 if((h&15)||!span(h,0x584)||!apart(h,0x584,s-512,528)||!span(thread,0x254)||size>0x200000)return 0;
 unsigned units=((size?size:1)+31)>>4;
 unsigned visited=0,total=0;int fit=0;
 for(unsigned bin=0;bin<128;bin++){
  uint32_t head=h+0x180+bin*8,prev=head,node=MEM32(head);unsigned previous_size=0;
  if(bin&&!!(MEM8(h+0x160+(bin>>3))&(1u<<(bin&7)))!=(node!=head))return 0;
  while(node!=head){
   if(++visited>4096||node<8||!span(node-8,16)||!apart(node-8,16,h,0x584)||!apart(node-8,16,s-512,528)||MEM32(node+4)!=prev)return 0;
   uint32_t b=node-8;unsigned n=MEM16(b);if(n<2||n>0xff00||(bin?n!=bin:(n<128||n<previous_size))||(MEM8(b+5)&9)||!span(b,n*16+16)||!apart(b,n*16+16,s-512,528)||!apart(b,n*16+16,h,0x584))return 0;
   uint32_t segment=MEM32(h+0x60+4*MEM8(b+4));if(!span(segment,0x44)||!apart(segment,0x44,s-512,528))return 0;
   if(!(MEM8(b+5)&16)&&!(MEM8(b+n*16+5)&1)){
    uint32_t next=b+n*16;unsigned nn=MEM16(next);if(nn<2||!span(next,nn*16+16)||!apart(next,nn*16+16,s-512,528))return 0;
   }
   if(n>=units)fit=1;total+=n;previous_size=n;prev=node;node=MEM32(node);
  }
  if(MEM32(head+4)!=prev)return 0;
 }
 if(MEM32(h+0x30)<total)return 0;
 if(!fit){
  if(!span(h,0x588))return 0;
  for(unsigned i=0;i<64;i++){uint32_t seg=MEM32(h+0x60+i*4);if(seg&&!span(seg,0x44))return 0;}
 }
 if(MEM32(thread+0x250)&&!span(MEM32(thread+0x250),24))return 0;
 uint32_t tail=MEM32(h+0x5c);if(!span(tail,8)||MEM32(tail)!=h+0x58)return 0;
 allocate(s);return 1;
}
