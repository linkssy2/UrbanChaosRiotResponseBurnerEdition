/* Original unordered-pair cancellation and filtered added/removed dispatch.
 * Preserve tail-swap order for small lists and the original multiset policy
 * for large lists. Generic hash storage remains the shared service. */
#include "migration_physics_pairs.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E00E0(void);void sub_002E17A0(void);void sub_002E1C80(void);void sub_002E18F0(void);void sub_002E1420(void);void sub_002E1470(void);void sub_002E1490(void);void sub_002E1810(void);void sub_002E1980(void);void sub_002E17F0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-512,544);}
static int array(uint32_t p){if(!safe(p,12))return 0;uint32_t n=MEM32(p+4),cap=MEM32(p+8)&0x3fffffff;return n<=cap&&cap<=65536&&(!cap||(safe(MEM32(p),cap*8)&&apart(p,12,MEM32(p),cap*8)));}
static int valid(unsigned entry){
 if(!span(g_esp-512,544))return 0;
 if(entry==0x3191a0){uint32_t a=MEM32(g_esp+4),b=MEM32(g_esp+8);if(!array(a)||!array(b)||!apart(a,12,b,12))return 0;if(MEM32(a+4)>=32&&MEM32(b+4)>=32&&(!safe(0x51e6f4,4)||!safe(MEM32(0x51e6f4),4)||!safe(MEM32(MEM32(0x51e6f4)),24)))return 0;uint32_t p=MEM32(a),q=MEM32(b),n=(MEM32(a+8)&0x3fffffff)*8,m=(MEM32(b+8)&0x3fffffff)*8;return(!n||apart(p,n,b,12))&&(!m||apart(q,m,a,12))&&(!n||!m||apart(p,n,q,m));}
 uint32_t n=MEM32(g_esp+8),pairs=MEM32(g_esp+4);if(n>65536)return 0;if(!n)return 1;
 if(!safe(pairs,n*8)||!safe(g_ecx,256)||!apart(pairs,n*8,g_ecx,256))return 0;
 if(entry==0x319050){uint32_t filter=MEM32(g_esp+12);if(!safe(filter,4)||!safe(MEM32(filter),4))return 0;}
 for(unsigned i=0;i<n*2;i++){uint32_t p=MEM32(pairs+i*4);if(!safe(p,6)||MEM8(p+4)>7||!safe(p+(int8_t)MEM8(p+5),4))return 0;}
 for(unsigned i=0;i<64;i++){uint32_t p=MEM32(g_ecx+i*4);if(!safe(p,4)||!safe(MEM32(p),16))return 0;}return 1;
}
static void pair_events(unsigned entry){
 uint32_t frame=g_seh_ebp;g_eax=MEM32(g_esp+8)-1;PUSH32(g_esp,g_ebx);g_ebx=g_ecx;
 if((int32_t)g_eax>=0){
  if(entry==0x319050){PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);frame=g_eax+1;
   do{g_ecx=MEM32(g_esi);g_eax=(uint32_t)(int32_t)SMEM8(g_ecx+5);g_edx=MEM32(g_esi+4);g_eax+=g_ecx;g_ecx=(uint32_t)(int32_t)SMEM8(g_edx+5);g_ecx+=g_edx;g_edx=MEM32(g_edi);
    {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_eax=g_esp+32;PUSH32(g_esp,g_eax);g_ecx=g_edi;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x31908e);RECOMP_ICALL_SAFE(target,saved);}
    if(MEM8(g_eax)){g_ecx=MEM32(g_esi);g_eax=(uint32_t)(int32_t)SMEM8(g_ecx+4);g_edx=MEM32(g_esi+4);g_ecx=(uint32_t)(int32_t)SMEM8(g_edx+4);g_eax=g_ecx+g_eax*8;g_ecx=MEM32(g_ebx+g_eax*4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x3190ac);RECOMP_ICALL_SAFE(target,saved);}
    g_esi+=8;--frame;
   }while(frame);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);
  }else{PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);PUSH32(g_esp,g_edi);g_edi=g_eax+1;
   do{g_eax=MEM32(g_esi);g_ecx=MEM32(g_esi+4);g_eax=(uint32_t)(int32_t)SMEM8(g_eax+4);g_ecx=(uint32_t)(int32_t)SMEM8(g_ecx+4);g_edx=g_ecx+g_eax*8;g_ecx=MEM32(g_ebx+g_edx*4);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x3190ec);RECOMP_ICALL_SAFE(target,saved);}g_esi+=8;--g_edi;}while(g_edi);POP32(g_esp,g_edi);POP32(g_esp,g_esi);
  }
 }POP32(g_esp,g_ebx);g_esp+=entry==0x319050?16:12;
}
static void cancel(void){
 uint32_t frame=g_seh_ebp;g_esp-=20;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+28);g_ecx=MEM32(g_ebx+4);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+44);g_eax=MEM32(g_edi+4);if((int32_t)g_ecx>=(int32_t)g_eax)g_ecx=g_eax;
 if((int32_t)g_ecx<32){
  g_esi=0;MEM32(g_esp+44)=0;
  if((int32_t)g_eax>0)do{
   g_eax=MEM32(g_ebx+4);g_edx=0;
   if((int32_t)g_eax>0){g_eax=MEM32(g_edi);g_ecx=MEM32(g_eax+g_esi*8);g_eax+=g_esi*8;MEM32(g_esp+16)=g_eax;g_eax=MEM32(g_ebx);MEM32(g_esp+40)=g_ecx;g_ecx=g_eax;
    int found=0;do{
     g_esi=MEM32(g_esp+40);if(MEM32(g_ecx)==g_esi){g_esi=MEM32(g_ecx+4);frame=MEM32(g_esp+16);if(g_esi==MEM32(frame+4)){found=1;break;}}
     g_esi=MEM32(g_esp+40);if(MEM32(g_ecx+4)==g_esi){g_esi=MEM32(g_ecx);frame=MEM32(g_esp+16);if(g_esi==MEM32(frame+4)){found=1;break;}}
     g_esi=MEM32(g_ebx+4);++g_edx;g_ecx+=8;
    }while((int32_t)g_edx<(int32_t)g_esi);
    if(found){g_ecx=MEM32(g_ebx+4)-1;MEM32(g_ebx+4)=g_ecx;g_esi=MEM32(g_eax+g_ecx*8);MEM32(g_eax+g_edx*8)=g_esi;g_ecx=MEM32(g_eax+g_ecx*8+4);g_esi=MEM32(g_esp+44);MEM32(g_eax+g_edx*8+4)=g_ecx;g_ecx=MEM32(g_edi+4);g_eax=MEM32(g_edi);--g_ecx;MEM32(g_edi+4)=g_ecx;g_edx=MEM32(g_eax+g_ecx*8);MEM32(g_eax+g_esi*8)=g_edx;g_ecx=MEM32(g_eax+g_ecx*8+4);MEM32(g_eax+g_esi*8+4)=g_ecx;--g_esi;}
    else g_esi=MEM32(g_esp+44);
   }g_eax=MEM32(g_edi+4);++g_esi;MEM32(g_esp+44)=g_esi;
  }while((int32_t)g_esi<(int32_t)g_eax);
 }else{
  g_ecx=g_esp+24;CALL(0x31927e,sub_002E17A0);g_edx=MEM32(g_ebx+4);PUSH32(g_esp,g_edx);g_ecx=g_esp+28;CALL(0x31928b,sub_002E1C80);g_ecx=MEM32(g_ebx+4);g_eax=0;MEM32(g_esp+40)=0;
  if((int32_t)g_ecx>0)do{
   g_ecx=MEM32(g_ebx);frame=MEM32(g_ecx+g_eax*8);g_eax=MEM32(g_ecx+g_eax*8+4);MEM32(g_esp+20)=g_eax;if(frame>g_eax){g_ecx=frame;frame=g_eax;MEM32(g_esp+20)=g_ecx;g_eax=g_ecx;}
   PUSH32(g_esp,g_eax);PUSH32(g_esp,frame);g_ecx=g_esp+32;CALL(0x3192c8,sub_002E18F0);g_esi=g_eax;PUSH32(g_esp,g_esi);g_edx=g_esp+48;PUSH32(g_esp,g_edx);g_ecx=g_esp+32;CALL(0x3192d9,sub_002E1420);int exists=MEM8(g_eax)!=0;g_ecx=g_esp+24;
   if(exists){PUSH32(g_esp,g_esi);CALL(0x3192e8,sub_002E1470);++g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ecx=g_esp+36;CALL(0x3192fa,sub_002E1490);g_eax=MEM32(g_ebx);g_ecx=MEM32(g_esp+40);MEM32(g_eax+g_ecx*8)=0;}
   else{g_eax=(MEM32(g_esp+40)<<16)|1;g_edx=(int32_t)g_eax<0?0xffffffff:0;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+24);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,frame);CALL(0x319321,sub_002E1810);}
   g_eax=MEM32(g_esp+40);g_ecx=MEM32(g_ebx+4);++g_eax;MEM32(g_esp+40)=g_eax;
  }while((int32_t)g_eax<(int32_t)g_ecx);
  g_eax=MEM32(g_edi+4);g_esi=0;MEM32(g_esp+40)=0;
  if((int32_t)g_eax>0)do{
   g_ecx=MEM32(g_edi);g_eax=MEM32(g_ecx+g_esi*8);g_ecx=MEM32(g_ecx+g_esi*8+4);if(g_eax>g_ecx){g_edx=g_eax;g_eax=g_ecx;g_ecx=g_edx;}
   PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_ecx=g_esp+32;CALL(0x319364,sub_002E18F0);frame=g_eax;PUSH32(g_esp,frame);g_edx=g_esp+48;PUSH32(g_esp,g_edx);g_ecx=g_esp+32;CALL(0x319375,sub_002E1420);
   if(MEM8(g_eax)){
    PUSH32(g_esp,frame);g_ecx=g_esp+28;CALL(0x319384,sub_002E1470);g_esi=g_eax;g_eax&=0xffff;g_ecx=g_esp+24;
    if((int32_t)g_eax>1){--g_esi;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);PUSH32(g_esp,frame);CALL(0x3193a2,sub_002E1490);}
    else{PUSH32(g_esp,frame);CALL(0x3193aa,sub_002E1980);g_ecx=MEM32(g_ebx);g_esi=(uint32_t)((int32_t)g_esi>>16);MEM32(g_ecx+g_esi*8)=0;}
    g_ecx=MEM32(g_edi+4);g_eax=MEM32(g_edi);--g_ecx;MEM32(g_edi+4)=g_ecx;g_edx=g_ecx;g_esi=MEM32(g_eax+g_edx*8);g_ecx=MEM32(g_esp+40);MEM32(g_eax+g_ecx*8)=g_esi;g_edx=MEM32(g_eax+g_edx*8+4);MEM32(g_eax+g_ecx*8+4)=g_edx;--g_ecx;MEM32(g_esp+40)=g_ecx;g_esi=g_ecx;
   }g_eax=MEM32(g_edi+4);++g_esi;MEM32(g_esp+40)=g_esi;
  }while((int32_t)g_esi<(int32_t)g_eax);
  g_eax=MEM32(g_ebx+4);g_esi=0;g_ecx=0;
  if((int32_t)g_eax>0)do{g_eax=MEM32(g_ebx);g_edx=MEM32(g_eax+g_ecx*8);if(g_edx){MEM32(g_eax+g_esi*8)=g_edx;g_edx=MEM32(g_eax+g_ecx*8+4);MEM32(g_eax+g_esi*8+4)=g_edx;++g_esi;}g_eax=MEM32(g_ebx+4);++g_ecx;}while((int32_t)g_ecx<(int32_t)g_eax);
  g_eax=MEM32(g_ebx+8)&0x3fffffff;if((int32_t)g_eax<(int32_t)g_esi){g_eax*=2;if((int32_t)g_esi>=(int32_t)g_eax)g_eax=g_esi;PUSH32(g_esp,8);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);CALL(0x31942f,sub_002E00E0);g_esp+=12;}
  g_ecx=g_esp+24;MEM32(g_ebx+4)=g_esi;CALL(0x31943e,sub_002E17F0);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_pairs(unsigned entry){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_PAIRS");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x319050&&entry!=0x3190c0&&entry!=0x3191a0)||!valid(entry))return 0;if(entry==0x3191a0)cancel();else pair_events(entry);return 1;}
#undef CALL
