#include "migration_graphics_commands.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003ECF90(void),sub_003F1280(void),sub_003ED100(void),sub_003ED480(void),sub_003ED670(void),sub_003ED850(void),sub_003EE000(void),sub_003EE350(void),sub_003EE550(void),sub_003E9810(void);
#define CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_GRAPHICS_COMMANDS");v=s&&!strcmp(s,"1");}return v;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

/* Ordered dirty groups share the live EBX/device state across callbacks. The
 * shader-group service returns a replacement mask; reading the global instead
 * would lose the original ordering and callback feedback contract. */
static void prepare_dirty_groups(void){
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(0x3f3d98);PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);
 if(g_ebx&0x100){PUSH32(g_esp,g_esi);CALL(sub_003ECF90,0x3ee569);}
 if(g_ebx&0x800){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);CALL(sub_003F1280,0x3ee575);g_ebx=g_eax;}
 if(g_ebx&0x4000){PUSH32(g_esp,g_esi);CALL(sub_003ED100,0x3ee582);}
 if(g_ebx&15){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);CALL(sub_003ED480,0x3ee58e);}
 if(g_ebx&0x2000){PUSH32(g_esp,g_esi);CALL(sub_003ED670,0x3ee599);}
 if(g_ebx&0x400){PUSH32(g_esp,g_esi);CALL(sub_003ED850,0x3ee5a4);}
 if(g_ebx&0xff1000){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);CALL(sub_003EE000,0x3ee5b3);}
 if(g_ebx&0x200){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);CALL(sub_003EE350,0x3ee5bf);}
 g_ebx&=0xc0000070u;POP32(g_esp,g_esi);MEM32(0x3f3d98)=g_ebx;POP32(g_esp,g_ebx);g_recomp_zf=g_ebx==0;g_esp+=4;
}

/* Complete stream format/address packet ownership, including both zero-base and
 * offset-base routes. Guest scratch and pointer reloads preserve valid aliases.
 * Packet growth and lower dirty-group emitters remain completing services. */
static void prepare_streams(void){
 uint32_t frame=g_seh_ebp;
 g_esp-=12;PUSH32(g_esp,g_ebx);g_ebx=MEM32(0x3f3d98);g_eax=g_ebx;PUSH32(g_esp,g_esi);g_eax&=0xffffffaf;g_esi=g_ecx;MEM32(g_esp+16)=g_esi;MEM32(0x3f3d98)=g_eax;
 if(g_ebx&0x3fffff8f){CALL(sub_003EE550,0x3ee5f8);}
 if(!(g_ebx&0x40000000)){
  PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);
  if((g_ebx&0x40)||MEM32(g_esi+32)!=g_edi){
   g_eax=MEM32(g_esi+0x794);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,frame);MEM32(g_esi+32)=g_edi;
   frame=(MEM32(g_eax+4)&16)+0x4eaa58;MEM32(g_esp+16)=g_eax;g_eax=MEM32(g_esi);
   if(g_eax>=g_ecx){CALL(sub_003E9810,0x3ee63f);}
   g_ecx=g_eax;
   if(g_ebx&16){
    MEM32(g_eax)=0x401760;g_ecx=g_eax+4;g_edx=0;
    do{
     g_eax=MEM8(g_edx+frame);g_esi=MEM32(g_esp+16);g_eax<<=4;g_ebx=MEM32(g_eax+g_esi+28);g_eax+=g_esi;
     g_esi=MEM32(g_eax+20);g_esi*=3;g_esi=MEM32(g_esi*4+0x3f66e0);g_esi=(g_esi<<8)+g_ebx;MEM32(g_ecx)=g_esi;g_ecx+=4;++g_edx;
    }while(g_edx<16);
   }
   g_ebx=0;
   if(!g_edi){
    g_edi=0x41720;
    do{
     g_eax=MEM8(g_ebx+frame);g_edx=MEM32(g_esp+16);g_eax<<=4;g_edx+=g_eax;
     if(MEM32(g_edx+28)!=2){
      g_eax=MEM32(g_edx+20);g_esi=g_eax*3;g_eax=MEM32(g_esi*4+0x3f66e8);g_esi=g_esi*4+0x3f66e0;
      if(g_eax){g_eax=MEM32(g_eax+4);g_eax+=MEM32(g_edx+24);g_eax+=MEM32(g_esi+4);MEM32(g_ecx)=g_edi;MEM32(g_ecx+4)=g_eax;g_ecx+=8;}
     }
     g_edi+=4;++g_ebx;
    }while(g_edi<0x41760);
    g_eax=MEM32(g_esp+24);POP32(g_esp,frame);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_eax)=g_ecx;POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=8;return;
   }
   MEM32(g_esp+20)=g_ebx;g_ebx=0x41720;
   do{
    g_eax=MEM32(g_esp+20);g_eax=MEM8(g_eax+frame);g_edx=MEM32(g_esp+16);g_eax<<=4;g_esi=g_eax+g_edx;
    if(MEM32(g_esi+28)!=2){
     g_eax=MEM32(g_esi+20);g_edx=g_eax*3;g_edi=MEM32(g_edx*4+0x3f66e8);g_edx=g_edx*4+0x3f66e0;
     if(g_edi){g_eax=MEM32(g_edx);g_eax*=MEM32(g_esp+32);g_eax+=MEM32(g_edi+4);g_edi=MEM32(g_esi+24);g_esi=MEM32(g_edx+4);g_eax+=g_edi;g_eax+=g_esi;MEM32(g_ecx)=g_ebx;MEM32(g_ecx+4)=g_eax;g_ecx+=8;}
    }
    g_esi=MEM32(g_esp+20)+1;g_ebx+=4;MEM32(g_esp+20)=g_esi;
   }while(g_ebx<0x41760);
   g_eax=MEM32(g_esp+24);MEM32(g_eax)=g_ecx;POP32(g_esp,frame);
  }
  POP32(g_esp,g_edi);
 }
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_graphics_commands(uint32_t entry){
 if(!enabled()||(entry!=0x3ee550&&entry!=0x3ee5d0))return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10100||!span(s-128,140)||!span(0x51c178,1)||MEM8(0x51c178)||!span(0x3f4238,4)||!span(0x3f3d98,4))return 0;
 uint32_t device=entry==0x3ee550?MEM32(0x3f4238):g_ecx;
 if(!span(device,entry==0x3ee550?12:0x798))return 0;
 if(entry==0x3ee550)prepare_dirty_groups();else prepare_streams();
 return 1;
}
#undef CALL
