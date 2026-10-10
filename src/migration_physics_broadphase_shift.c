/* Original broadphase quantized shift, endpoint clipping and origin correction. */
#include "migration_physics_broadphase_shift.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+16<=p);}
static int separate(uint32_t a,uint32_t an,uint32_t b,uint32_t bn){return (uint64_t)a+an<=b||(uint64_t)b+bn<=a;}
static void push_fp(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void pop_fp(void){g_fp_top=(g_fp_top+1)&7;}
#define TOP g_fp_stack[g_fp_top]
static void quantized_shift(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-52;g_eax=MEM32(frame+8);g_edx=MEM32(g_eax);MEM32(g_esp+36)=g_edx;g_edx=MEM32(g_eax+4);MEM32(g_esp+40)=g_edx;g_edx=MEM32(g_eax+8);g_eax=MEM32(g_eax+12);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);MEM32(g_esp+52)=g_edx;MEM32(g_esp+56)=g_eax;PUSH32(g_esp,g_edi);MEM32(g_esp+16)=g_ecx;g_edx=g_ecx+48;g_eax=0;
 do{push_fp(MEMF(g_edx));TOP*=MEMF(g_esp+g_eax+48);MEMF(g_esp+24)=(float)TOP;pop_fp();push_fp(MEMF(g_esp+24));SMEM32(g_esp+20)=(int32_t)llrint(TOP);pop_fp();g_esi=MEM32(g_esp+20)&~1u;MEM32(g_esp+24)=g_esi;push_fp((double)SMEM32(g_esp+24));MEM32(g_esp+g_eax+36)=g_esi;g_eax+=4;TOP/=MEMF(g_edx);g_edx+=4;MEMF(g_esp+g_eax+44)=(float)TOP;pop_fp();}while((int32_t)g_eax<12);
 g_edx=MEM32(frame+12);g_xmm0=XMM_MEM(g_esp+48);g_esi=MEM32(frame+16);XMM_STORE(g_edx,g_xmm0);g_eax=0;g_edx=g_ecx+80;MEM32(g_esp+24)=g_eax;MEM32(g_esp+12)=g_edx;
 do{g_edi=MEM32(g_esp+g_eax*4+36);g_ebx=MEM32(g_edx);MEM32(g_esp+28)=g_edi;g_edi=1;--g_ebx;MEM32(g_esp+20)=g_edi;
  if((int32_t)g_ebx>(int32_t)g_edi){do{g_eax=MEM32(g_edx-4);g_ebx=g_eax+g_edi*4;g_eax=MEM16(g_ebx);
   if((int32_t)g_eax>1&&(int32_t)g_eax<65532){g_edi=MEM32(g_esp+28)+g_eax;
    if((int32_t)g_edi<0){g_edx=MEM16(g_ebx+2);g_eax&=1;g_edx<<=4;g_edi=g_eax;g_eax=MEM32(g_ecx+64);g_ecx=g_edx+g_eax+12;g_edx=MEM32(g_esi+8)&0x3fffffff;g_eax=MEM32(g_esi+4);MEM32(g_esp+32)=g_ecx;if(g_eax==g_edx){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x3276b4);sub_002E0150();g_esp+=8;}g_edx=MEM32(g_esp+32);g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);g_edx=MEM32(g_edx);MEM32(g_ecx+g_eax*4)=g_edx;
     g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+12);++g_eax;MEM32(g_esi+4)=g_eax;
    }else if((int32_t)g_edi>=65532){g_ecx=MEM32(g_ecx+64);g_eax=(g_eax&1)|65532;g_edi=g_eax;g_eax=MEM16(g_ebx+2)<<4;g_edx=g_eax+g_ecx+12;g_eax=MEM32(g_esi+8)&0x3fffffff;g_ecx=MEM32(g_esi+4);MEM32(g_esp+32)=g_edx;if(g_ecx==g_eax){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x327702);sub_002E0150();g_esp+=8;}g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);g_eax=MEM32(g_eax);MEM32(g_edx+g_ecx*4)=g_eax;
     g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+12);++g_eax;MEM32(g_esi+4)=g_eax;
    }
    MEM16(g_ebx)=(uint16_t)g_edi;g_edi=MEM32(g_esp+20);
   }
   g_eax=MEM32(g_edx);++g_edi;--g_eax;MEM32(g_esp+20)=g_edi;
  }while((int32_t)g_edi<(int32_t)g_eax);g_eax=MEM32(g_esp+24);}
  ++g_eax;g_edx+=12;MEM32(g_esp+24)=g_eax;MEM32(g_esp+12)=g_edx;
 }while((int32_t)g_eax<3);
 g_recomp_zf=g_eax==3;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=16;
}
static void origin_shift(void){
 uint32_t saved=g_esp,frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-44;g_eax=MEM32(frame+8);g_xmm1=XMM_MEM(g_eax);g_eax=MEM32(frame+16);g_xmm0=XMM_MEM(0x49b790);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_edx=MEM32(g_esi);PUSH32(g_esp,g_eax);g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);g_eax=g_esp+24;g_xmm1=XMM_XOR(g_xmm1,g_xmm0);PUSH32(g_esp,g_eax);g_ecx=g_esi;XMM_STORE(g_esp+28,g_xmm1);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x324406);RECOMP_ICALL_SAFE(target,saved);
 push_fp(MEMF(0x44bf94));g_xmm1=XMM_MEM(g_esi+16);g_xmm0=XMM_MEM(g_esp+32);g_xmm1=XMM_ADD(g_xmm1,g_xmm0);XMM_STORE(g_esi+16,g_xmm1);TOP/=MEMF(g_esi+48);g_xmm2=XMM_MEM(g_esi+16);g_ecx=MEM32(frame+12);MEM32(g_esp+28)=0;MEMF(g_esp+16)=(float)TOP;pop_fp();push_fp(MEMF(0x44bf94));TOP/=MEMF(g_esi+52);MEMF(g_esp+20)=(float)TOP;pop_fp();push_fp(MEMF(0x44bf94));TOP/=MEMF(g_esi+56);MEMF(g_esp+24)=(float)TOP;pop_fp();g_xmm1=XMM_MEM(g_esp+16);g_xmm2=XMM_ADD(g_xmm2,g_xmm1);XMM_STORE(g_esi+32,g_xmm2);g_xmm1=XMM_MEM(0x49b790);g_xmm0=XMM_XOR(g_xmm0,g_xmm1);XMM_STORE(g_ecx,g_xmm0);POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=16;
}
int uc_migration_physics_broadphase_shift(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_SHIFT");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x3243d0&&entry!=0x3275c0)||!span(g_esp-256,272)||!safe(g_ecx,112)||!span(0x44bf94,4)||!span(0x49b790,16))return 0;
 uint32_t input=MEM32(g_esp+4),out=MEM32(g_esp+8),list=MEM32(g_esp+12);if(!safe(input,16)||!safe(out,16)||!safe(list,12)||!separate(out,16,g_ecx,112))return 0;
 uint32_t data=MEM32(list),count=MEM32(list+4),cap=MEM32(list+8)&0x3fffffff;if(count>cap||cap>65535||!safe(data,cap*4)||!separate(out,16,list,12)||!separate(out,16,data,cap*4))return 0;
 for(unsigned axis=0;axis<3;axis++){uint32_t p=MEM32(g_ecx+76+axis*12),n=MEM32(g_ecx+80+axis*12);if(n<2||n>65535||!safe(p,n*4)||!separate(out,16,p,n*4))return 0;for(uint32_t i=1;i+1<n;i++){uint64_t address=(uint64_t)MEM32(g_ecx+64)+MEM16(p+i*4+2)*16ull+12;if(address>0xffffffff||!safe((uint32_t)address,4)||!separate(out,16,(uint32_t)address,4))return 0;}}
 if(entry==0x3243d0){if(!span(MEM32(g_ecx),52))return 0;origin_shift();}else quantized_shift();return 1;
}
#undef TOP
