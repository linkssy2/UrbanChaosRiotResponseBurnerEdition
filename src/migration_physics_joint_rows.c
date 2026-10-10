/* Authored Havok ragdoll joints: original cone, plane, twist and friction
 * parameters, original ordered solver rows, and original guest runtime. */
#include "migration_physics_joint_rows.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_0034B850(void);void sub_00389510(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-2048,2080);}
static int finite_values(uint32_t p,unsigned n){if(!safe(p,n*4))return 0;for(unsigned i=0;i<n;i++)if((MEM32(p+i*4)&0x7fffffff)>0x47800000)return 0;return 1;}
static void fp_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static double fp_pop(void){double v=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;return v;}
static void status(void){SET_LO16(g_eax,(uint16_t)(((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000)));}
static void cross_axes(void){
 g_xmm1=XMM_SHUFFLE(g_xmm2,g_xmm2,0xc9);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xd2);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);
 g_xmm1=XMM_SHUFFLE(g_xmm2,g_xmm2,0xd2);g_xmm4=XMM_SHUFFLE(g_xmm0,g_xmm0,0xc9);g_xmm4=XMM_MUL(g_xmm4,g_xmm1);g_xmm1=XMM_SUB(g_xmm4,g_xmm3);
}
static void rotate_angular(uint32_t body){
 g_xmm2=XMM_MEM(body+112);g_xmm3=XMM_MEM(body+96);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MEM(body+80);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);
}
static void mass_sum(uint32_t a,uint32_t b){
 g_xmm0=XMM_MEM(g_eax+16);g_xmm1=XMM_MEM(g_eax);g_xmm2=XMM_MEM(b+48);g_xmm3=XMM_MUL(g_xmm0,g_xmm0);g_xmm0=XMM_MEM(a+48);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_MUL(g_xmm1,g_xmm1);g_xmm1=XMM_MUL(g_xmm2,g_xmm0);g_xmm0=XMM_ADD(g_xmm1,g_xmm3);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm2=g_xmm0;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_xmm2.f[0]+=g_xmm1.f[0];
}
static void angular_limit(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=~15u;PUSH32(g_esp,g_ecx);g_edx=MEM32(frame+12);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_edx+24);PUSH32(g_esp,g_esi);g_esi=MEM32(g_edx+20);SET_LO8(g_edx,MEM8(g_esi+12));PUSH32(g_esp,g_edi);g_edi=MEM32(frame+16);g_eax=MEM32(g_edi);g_ecx=MEM32(g_edi+4);int direct=(uint8_t)g_edx!=0;g_edx=MEM32(frame+8);g_xmm0=XMM_MEM(g_edx);
 if(!direct)rotate_angular(g_esi);XMM_STORE(g_eax,g_xmm0);
 if(MEM8(g_ebx+12)){g_xmm0=XMM_MEM(0x49b790);g_xmm1=XMM_MEM(g_edx);g_xmm1=XMM_XOR(g_xmm1,g_xmm0);XMM_STORE(g_eax+16,g_xmm1);}
 else{g_xmm1=XMM_MEM(0x49b790);g_xmm0=XMM_MEM(g_edx);g_xmm2=XMM_MEM(g_ebx+112);g_xmm3=XMM_MEM(g_ebx+96);g_xmm0=XMM_XOR(g_xmm0,g_xmm1);rotate_angular(g_ebx);XMM_STORE(g_eax+16,g_xmm0);}
 mass_sum(g_esi,g_ebx);g_esi=g_esp+12;MEMF(g_esi)=g_xmm2.f[0];fp_push(MEMF(g_esp+12));g_fp_stack[g_fp_top]+=MEMF(0x49e5c8);g_esi=MEM32(frame+12);g_ecx+=16;g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];MEMF(g_eax+12)=(float)fp_pop();
 fp_push(MEMF(g_esi+4));g_fp_stack[g_fp_top]=-g_fp_stack[g_fp_top];fp_push(g_fp_stack[g_fp_top]);g_fp_stack[g_fp_top]*=MEMF(g_edx+16);fp_push(g_fp_stack[(g_fp_top+1)&7]);g_fp_stack[g_fp_top]*=MEMF(g_edx+20);
 {double t=g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=t;}
 MEMF(g_ecx-12)=(float)fp_pop();MEM32(g_ecx-16)=0x4100a;MEM32(g_ecx-4)=0x3f000000;MEMF(g_ecx-8)=(float)fp_pop();MEM32(g_edi+4)=g_ecx;g_ecx=g_eax+32;MEM32(g_edi)=g_ecx;g_fp_stack[g_fp_top]*=MEMF(g_edx+24);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEMF(g_eax+28)=(float)fp_pop();POP32(g_esp,g_ebx);fp_push(MEMF(g_eax+12));g_fp_stack[g_fp_top]*=MEMF(0x44c1f8);MEMF(g_eax+12)=(float)fp_pop();g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_ecx==0;g_esp+=4;
}
static void friction_rows(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-20;g_edx=MEM32(frame+8);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_edx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_edx+4);MEM32(g_esp+20)=g_esi;g_esi=MEM32(g_edx+12)-1;uint32_t last=g_esi;PUSH32(g_esp,g_edi);g_edi=MEM32(frame+16);g_eax=MEM32(g_edi);g_ecx=MEM32(g_edi+4);
 if((int32_t)g_esi>=0){
  g_edi=MEM32(g_esp+24);g_edx=MEM32(frame+12);g_edi+=4;++g_esi;MEM32(g_esp+20)=g_edi;MEM32(g_esp+24)=g_esi;
  do{
   g_esi=MEM32(g_edx+20);g_edi=MEM32(g_edx+24);g_xmm0=XMM_MEM(g_ebx);if(!MEM8(g_esi+12))rotate_angular(g_esi);XMM_STORE(g_eax,g_xmm0);
   if(MEM8(g_edi+12)){g_xmm0=XMM_MEM(0x49b790);g_xmm1=XMM_MEM(g_ebx);g_xmm1=XMM_XOR(g_xmm1,g_xmm0);XMM_STORE(g_eax+16,g_xmm1);}
   else{g_xmm1=XMM_MEM(0x49b790);g_xmm0=XMM_MEM(g_ebx);g_xmm2=XMM_MEM(g_edi+112);g_xmm3=XMM_MEM(g_edi+96);g_xmm0=XMM_XOR(g_xmm0,g_xmm1);rotate_angular(g_edi);XMM_STORE(g_eax+16,g_xmm0);}
   mass_sum(g_esi,g_edi);g_esi=g_esp+28;MEMF(g_esi)=g_xmm2.f[0];fp_push(MEMF(g_esp+28));g_fp_stack[g_fp_top]+=MEMF(0x49e5c8);g_esi=MEM32(frame+8);g_ecx+=8;g_ebx+=16;g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];g_eax+=32;MEMF(g_eax-20)=(float)fp_pop();
   fp_push(MEMF(g_esi+8));g_fp_stack[g_fp_top]*=MEMF(g_edx);g_esi=MEM32(g_esp+20);MEM32(g_ecx-8)=0x40809;g_esi+=8;MEM32(g_esp+20)=g_esi;MEMF(g_ecx-4)=(float)fp_pop();fp_push(MEMF(g_esi-8));g_esi=MEM32(g_esp+24);g_fp_stack[g_fp_top]*=MEMF(g_edx+4);--g_esi;last=g_esi;MEM32(g_esp+24)=g_esi;MEMF(g_eax-4)=(float)fp_pop();
  }while(g_esi);
  g_edx=MEM32(frame+16);MEM32(g_edx)=g_eax;MEM32(g_edx+4)=g_ecx;
 }else{MEM32(g_edi)=g_eax;MEM32(g_edi+4)=g_ecx;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=last==0;g_esp+=4;
}
static void point_rows(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-20;g_eax=MEM32(frame+16);g_edx=MEM32(frame+20);g_ecx=MEM32(g_eax+20);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+24);g_eax=MEM32(g_edx);PUSH32(g_esp,g_edi);g_edi=MEM32(g_edx+4);g_edx=0x505660;MEM32(g_esp+20)=3;
 do{
  g_xmm0=XMM_MEM(g_edx);g_ebx=MEM32(frame+8);XMM_STORE(g_eax,g_xmm0);g_xmm0=XMM_MEM(g_ebx);g_xmm1=XMM_MEM(g_ecx+64);g_ebx=MEM32(frame+12);g_xmm2=XMM_MEM(g_esi+64);g_xmm0=XMM_SUB(g_xmm0,g_xmm1);g_xmm1=XMM_MEM(g_ebx);SET_LO8(g_ebx,MEM8(g_ecx+12));int direct=(uint8_t)g_ebx!=0;g_xmm1=XMM_SUB(g_xmm1,g_xmm2);g_xmm2=XMM_MEM(g_edx);g_xmm4=g_xmm0;g_xmm3=XMM_SHUFFLE(g_xmm2,g_xmm2,0xc9);g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm0,0xd2);g_xmm4=XMM_MUL(g_xmm4,g_xmm3);g_xmm3=XMM_SHUFFLE(g_xmm2,g_xmm2,0xd2);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0xc9);g_xmm0=XMM_MUL(g_xmm2,g_xmm3);g_xmm0=XMM_SUB(g_xmm0,g_xmm4);
  if(!direct){g_xmm3=XMM_MEM(g_ecx+112);g_xmm4=XMM_MEM(g_ecx+96);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm4=XMM_MUL(g_xmm4,g_xmm2);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MEM(g_ecx+80);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm0=XMM_ADD(g_xmm0,g_xmm4);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);}
  XMM_STORE(g_eax+16,g_xmm0);g_xmm0=XMM_MEM(g_edx);SET_LO8(g_ebx,MEM8(g_esi+12));direct=(uint8_t)g_ebx!=0;g_xmm3=g_xmm0;g_xmm2=XMM_SHUFFLE(g_xmm1,g_xmm1,0xc9);g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm0,0xd2);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_SHUFFLE(g_xmm1,g_xmm1,0xd2);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0xc9);g_xmm0=XMM_MUL(g_xmm1,g_xmm2);g_xmm0=XMM_SUB(g_xmm0,g_xmm3);if(!direct)rotate_angular(g_esi);
  g_xmm1=XMM_MEM(g_eax+16);XMM_STORE(g_eax+32,g_xmm0);fp_push(MEMF(g_esi+60));g_xmm0=XMM_MEM(g_eax+32);g_fp_stack[g_fp_top]+=MEMF(g_ecx+60);g_xmm2=XMM_MEM(g_esi+48);g_xmm3=g_xmm0;g_fp_stack[g_fp_top]+=MEMF(0x49e5c8);g_xmm3=XMM_MUL(g_xmm3,g_xmm0);g_xmm0=XMM_MEM(g_ecx+48);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_MUL(g_xmm1,g_xmm1);g_xmm1=XMM_MUL(g_xmm2,g_xmm0);g_xmm0=XMM_ADD(g_xmm1,g_xmm3);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm2.f[0]+=g_xmm1.f[0];g_ebx=g_esp+24;MEMF(g_ebx)=g_xmm2.f[0];g_fp_stack[g_fp_top]+=MEMF(g_esp+24);g_ebx=MEM32(frame+8);g_edi+=4;MEMF(g_eax+44)=(float)g_fp_stack[g_fp_top];g_eax+=48;fp_push(MEMF(0x44bf94));g_edx+=16;g_fp_stack[g_fp_top]/=g_fp_stack[(g_fp_top+1)&7];MEMF(g_eax-20)=(float)fp_pop();g_xmm0=XMM_MEM(g_ebx);g_ebx=MEM32(frame+12);g_xmm2=XMM_MEM(g_ebx);fp_pop();g_xmm1=XMM_MEM(g_edx-16);g_xmm2=XMM_SUB(g_xmm2,g_xmm0);g_xmm0=XMM_MUL(g_xmm2,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm2=g_xmm0;g_ebx=g_esp+28;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_xmm2.f[0]+=g_xmm1.f[0];MEMF(g_ebx)=g_xmm2.f[0];fp_push(MEMF(g_esp+28));g_ebx=MEM32(frame+16);g_fp_stack[g_fp_top]*=MEMF(g_ebx+4);g_ebx=MEM32(g_esp+20)-1;MEM32(g_esp+20)=g_ebx;MEMF(g_eax-36)=(float)fp_pop();MEM32(g_edi-4)=0x3040d;
 }while(g_ebx);
 g_ecx=MEM32(frame+20);MEM32(g_ecx+4)=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_ecx)=g_eax;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=4;
}
static void setup_rows(void){
 g_ecx=MEM32(g_esp+8);g_eax=MEM32(g_ecx+4);g_edx=MEM32(g_ecx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+24);g_esi=MEM32(g_esi+20);MEM32(g_eax+12)=g_esi;g_esi=MEM32(g_esp+20);MEM32(g_eax+8)=g_esi;g_esi=MEM32(g_esp+24);MEM32(g_eax+16)=g_edi;MEM32(g_eax+20)=g_esi;MEM32(g_eax)=0x11801;MEM32(g_eax+4)=g_edx;g_eax+=24;POP32(g_esp,g_edi);MEM32(g_ecx+4)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=4;
}
static void cone_plane(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-0x44;
 g_eax=MEM32(frame+12);g_xmm2=XMM_MEM(g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=MEM32(frame+24);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+8);g_xmm0=XMM_MEM(g_edi);cross_axes();g_esi=g_ecx;
 fp_push(MEMF(g_esi+0x70));XMM_STORE(g_esp+32,g_xmm1);g_xmm1=XMM_MUL(g_xmm1,g_xmm1);g_fp_stack[g_fp_top]=-g_fp_stack[g_fp_top];g_xmm3=g_xmm1;MEMF(g_esp+24)=(float)fp_pop();
 g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm1,0x55);g_xmm4=g_xmm1;g_xmm3.f[0]+=g_xmm1.f[0];g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm1,0xaa);g_ecx=g_esp+16;g_xmm4.f[0]+=g_xmm3.f[0];MEMF(g_ecx)=g_xmm4.f[0];fp_push(MEMF(g_esp+16));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x49de7c));fp_pop();status();
 if(RECOMP_PARITY8(((g_eax>>8)&255)&5)){
  fp_push(MEMF(g_esp+16));g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_fp_stack[g_fp_top]=sqrt(g_fp_stack[g_fp_top]);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm2=g_xmm0;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_edx=g_esp+20;g_xmm2.f[0]+=g_xmm1.f[0];MEMF(g_edx)=g_xmm2.f[0];g_eax=MEM32(g_esp+20);PUSH32(g_esp,g_eax);MEMF(g_esp+20)=(float)fp_pop();g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x31e513);sub_00389510();
  g_fp_stack[g_fp_top]=-g_fp_stack[g_fp_top];fp_push(MEMF(0x44bf94));g_eax=MEM32(frame+20);g_fp_stack[g_fp_top]/=MEMF(g_esp+24);g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);g_ecx=g_esp+64;PUSH32(g_esp,g_ecx);MEM32(g_esp+84)=g_edx;MEM32(g_esp+88)=0x42c80000;MEMF(g_esp+40)=(float)fp_pop();g_xmm0=XMM_SCALAR(MEMF(g_esp+40));g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);MEMF(g_esp+92)=(float)fp_pop();g_xmm1=XMM_MUL(g_xmm1,XMM_MEM(g_esp+52));XMM_STORE(g_esp+68,g_xmm1);g_ebp=frame;PUSH32(g_esp,0x31e55d);angular_limit();g_esp+=20;
 }
 g_xmm0=XMM_MEM(g_edi);g_ecx=MEM32(frame+16);g_xmm2=XMM_MEM(g_ecx);g_edx=MEM32(g_esi+0x78);g_eax=MEM32(g_esi+0x74);cross_axes();XMM_STORE(g_esp+32,g_xmm1);
 g_xmm1=XMM_MUL(g_xmm1,g_xmm1);g_xmm3=g_xmm1;MEM32(g_esp+28)=g_edx;g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm1,0x55);g_xmm4=g_xmm1;g_xmm3.f[0]+=g_xmm1.f[0];g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm1,0xaa);g_edx=g_esp+16;g_xmm4.f[0]+=g_xmm3.f[0];MEMF(g_edx)=g_xmm4.f[0];fp_push(MEMF(g_esp+16));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x49de7c));fp_pop();MEM32(g_esp+20)=g_eax;status();
 if(RECOMP_PARITY8(((g_eax>>8)&255)&5)){
  fp_push(MEMF(g_esp+16));g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_fp_stack[g_fp_top]=sqrt(g_fp_stack[g_fp_top]);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm2=g_xmm0;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_eax=g_esp+24;g_xmm2.f[0]+=g_xmm1.f[0];MEMF(g_eax)=g_xmm2.f[0];g_edx=MEM32(g_esp+24);MEMF(g_esp+16)=(float)fp_pop();g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ebp=frame;PUSH32(g_esp,0x31e614);sub_00389510();
  fp_push(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_esp+24);g_eax=MEM32(g_esp+28);g_edx=MEM32(frame+20);g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_ebx);MEM32(g_esp+76)=g_eax;PUSH32(g_esp,g_edx);g_eax=g_esp+64;PUSH32(g_esp,g_eax);MEM32(g_esp+88)=g_ecx;MEMF(g_esp+44)=(float)fp_pop();g_xmm0=XMM_SCALAR(MEMF(g_esp+44));g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);MEMF(g_esp+92)=(float)fp_pop();g_xmm1=XMM_MUL(g_xmm1,XMM_MEM(g_esp+52));XMM_STORE(g_esp+68,g_xmm1);g_ebp=frame;PUSH32(g_esp,0x31e65c);angular_limit();g_esp+=20;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=24;
}
static void transform_basis(void){
 do{
  g_xmm0=XMM_MEM(g_eax);g_xmm4=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm5=XMM_MUL(g_xmm1,g_xmm4);g_xmm4=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm6=XMM_MUL(g_xmm2,g_xmm4);g_xmm4=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MUL(g_xmm3,g_xmm4);g_xmm0=XMM_ADD(g_xmm0,g_xmm6);g_xmm0=XMM_ADD(g_xmm0,g_xmm5);XMM_STORE(g_ecx+g_eax,g_xmm0);g_eax+=16;--g_esi;
 }while(g_esi);
}
static void ragdoll(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-0xb4;SET_LO8(g_eax,MEM8(0x51f5a8));int active=(uint8_t)g_eax!=0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_esi;
 if(!active){PUSH32(g_esp,1);g_ebp=frame;PUSH32(g_esp,0x31e865);sub_0034B850();g_esp+=4;MEM8(0x51f5a8)=(uint8_t)g_eax;active=(uint8_t)g_eax!=0;}
 if(active){
  g_ebx=MEM32(frame+8);g_eax=MEM32(g_ebx+40);g_ecx=MEM32(frame+12);PUSH32(g_esp,8);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebp=frame;PUSH32(g_esp,0x31e888);setup_rows();
  g_edx=MEM32(g_ebx+28);g_eax=g_esi+16;g_ecx=12;g_esi=g_edx;g_edi=g_esp+64;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;g_xmm1=XMM_MEM(g_esp+96);g_xmm2=XMM_MEM(g_esp+80);g_xmm3=XMM_MEM(g_esp+64);g_ecx=g_esp+160;g_esp+=16;g_ecx-=g_eax;g_esi=3;transform_basis();
  g_xmm0=XMM_MEM(g_edx+48);g_xmm1=XMM_MEM(g_esp+144);g_edx=MEM32(g_ebx+32);g_eax=MEM32(g_esp+12);g_xmm1=XMM_ADD(g_xmm1,g_xmm0);XMM_STORE(g_esp+144,g_xmm1);g_ecx=12;g_esi=g_edx;g_edi=g_esp+48;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;g_xmm1=XMM_MEM(g_esp+80);g_xmm2=XMM_MEM(g_esp+64);g_xmm3=XMM_MEM(g_esp+48);g_eax+=64;g_ecx=g_esp+96-g_eax;g_esi=3;transform_basis();
  g_xmm0=XMM_MEM(g_edx+48);g_edi=MEM32(frame+12);g_xmm1=XMM_MEM(g_esp+96);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);g_edx=g_esp+120;PUSH32(g_esp,g_edx);g_eax=g_esp+140;PUSH32(g_esp,g_eax);g_ecx=g_esp+192;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+32);g_xmm1=XMM_ADD(g_xmm1,g_xmm0);XMM_STORE(g_esp+116,g_xmm1);g_ebp=frame;PUSH32(g_esp,0x31e9b0);cone_plane();
  g_xmm0=XMM_MEM(g_esp+176);g_xmm2=XMM_MEM(g_esp+128);g_esi=MEM32(g_esp+12);g_eax=MEM32(g_esi+128);g_edx=MEM32(g_esi+124);XMM_STORE(g_esp+48,g_xmm0);g_xmm1=XMM_ADD(g_xmm2,g_xmm0);g_xmm0=XMM_MUL(g_xmm1,g_xmm1);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm4=g_xmm0;g_xmm3.f[0]+=g_xmm0.f[0];g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm0,0xaa);g_xmm0=g_xmm4;g_xmm0.f[0]+=g_xmm3.f[0];XMM_STORE(g_esp+16,g_xmm0);g_xmm0.f[0]=sqrtf(g_xmm0.f[0]);MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+16);g_ecx=g_esp+12;MEMF(g_ecx)=g_xmm0.f[0];fp_push(MEMF(g_esp+12));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x49de84));fp_pop();MEM32(g_esp+68)=g_eax;MEM32(g_esp+64)=g_edx;status();
  if(!((g_eax>>8)&0x41)){fp_push(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_esp+12);MEMF(g_esp+12)=(float)fp_pop();g_xmm0=XMM_SCALAR(MEMF(g_esp+12));g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);XMM_STORE(g_esp+16,g_xmm2);}else{XMM_STORE(g_esp+16,g_xmm2);g_xmm2=XMM_MEM(g_esp+16);}
  g_xmm0=XMM_MEM(g_esp+112);g_xmm1=XMM_SHUFFLE(g_xmm2,g_xmm2,0xd2);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xc9);g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm2,0xc9);g_xmm4=XMM_MUL(g_xmm1,g_xmm3);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xd2);g_xmm0=XMM_MUL(g_xmm2,g_xmm3);g_xmm0=XMM_SUB(g_xmm0,g_xmm4);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xd2);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0xc9);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm1=g_xmm2;g_xmm2=XMM_MEM(g_esp+160);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm1=XMM_SUB(g_xmm1,g_xmm3);g_xmm3=g_xmm0;g_xmm1=XMM_MUL(g_xmm1,g_xmm2);g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm0,0x55);g_xmm4=g_xmm0;g_xmm3.f[0]+=g_xmm0.f[0];g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm0,0xaa);g_xmm0=XMM_SHUFFLE(g_xmm1,g_xmm1,0x55);g_xmm2=g_xmm1;g_xmm0.f[0]+=g_xmm1.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm1,0xaa);g_eax=g_esp+12;g_xmm2.f[0]+=g_xmm0.f[0];g_edx=g_esp+44;MEMF(g_eax)=g_xmm2.f[0];g_ecx=MEM32(g_esp+12);g_xmm4.f[0]+=g_xmm3.f[0];MEMF(g_edx)=g_xmm4.f[0];g_edx=MEM32(g_esp+44);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ebp=frame;PUSH32(g_esp,0x31eafb);sub_00389510();
  MEMF(g_esp+80)=(float)fp_pop();g_xmm0=XMM_MEM(g_esp+24);PUSH32(g_esp,g_edi);g_eax=g_esp+60;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);XMM_STORE(g_esp+68,g_xmm0);g_ebp=frame;PUSH32(g_esp,0x31eb15);angular_limit();
  PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);g_ecx=g_esp+124;PUSH32(g_esp,g_ecx);g_edx=g_esp+176;PUSH32(g_esp,g_edx);g_ebp=frame;PUSH32(g_esp,0x31eb29);point_rows();
  fp_push(MEMF(g_esi+132));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x44c174));fp_pop();g_esp+=36;status();
  if(!((g_eax>>8)&0x41)){g_eax=MEM32(g_esi+132);g_edx=MEM32(g_ebx+40);g_ecx=MEM32(g_ebx+28);PUSH32(g_esp,g_edi);MEM32(g_esp+28)=g_eax;g_eax=g_esp+20;g_edx+=48;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);MEM32(g_esp+28)=g_ecx;MEM32(g_esp+40)=3;MEM32(g_esp+32)=g_edx;g_ebp=frame;PUSH32(g_esp,0x31eb6e);friction_rows();g_esp+=12;}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=12;
}
static void alignment_row(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=~15u;PUSH32(g_esp,g_ecx);g_ecx=MEM32(frame+8);g_xmm0=XMM_MEM(g_ecx+16);PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+12);g_edx=MEM32(g_ebx+20);int direct=MEM8(g_edx+12)!=0;PUSH32(g_esp,g_esi);g_esi=MEM32(frame+16);g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ebx+24);if(!direct)rotate_angular(g_edx);XMM_STORE(g_eax,g_xmm0);
 if(MEM8(g_edi+12)){g_xmm0=XMM_MEM(0x49b790);g_xmm1=XMM_MEM(g_ecx+16);g_xmm1=XMM_XOR(g_xmm1,g_xmm0);XMM_STORE(g_eax+16,g_xmm1);}
 else{g_xmm1=XMM_MEM(0x49b790);g_xmm0=XMM_MEM(g_ecx+16);g_xmm2=XMM_MEM(g_edi+112);g_xmm3=XMM_MEM(g_edi+96);g_xmm0=XMM_XOR(g_xmm0,g_xmm1);rotate_angular(g_edi);XMM_STORE(g_eax+16,g_xmm0);}
 mass_sum(g_edx,g_edi);g_edx=g_esp+12;MEMF(g_edx)=g_xmm2.f[0];fp_push(MEMF(g_esp+12));g_fp_stack[g_fp_top]+=MEMF(0x49e5c8);g_edx=g_eax+32;POP32(g_esp,g_edi);g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];MEMF(g_eax+12)=(float)fp_pop();
 fp_push(MEMF(g_ecx+40));g_fp_stack[g_fp_top]*=MEMF(g_ecx+8);fp_push(MEMF(g_ecx+36));g_fp_stack[g_fp_top]*=MEMF(g_ecx+4);g_fp_stack[(g_fp_top+1)&7]+=g_fp_stack[g_fp_top];fp_pop();fp_push(MEMF(g_ecx+32));g_fp_stack[g_fp_top]*=MEMF(g_ecx);g_ecx=MEM32(g_esi+4);MEM32(g_ecx)=0x4040c;MEM32(g_esi)=g_edx;g_fp_stack[(g_fp_top+1)&7]+=g_fp_stack[g_fp_top];fp_pop();g_ecx+=4;MEM32(g_esi+4)=g_ecx;g_fp_stack[g_fp_top]=-g_fp_stack[g_fp_top];POP32(g_esp,g_esi);g_fp_stack[g_fp_top]*=MEMF(g_ebx+4);POP32(g_esp,g_ebx);MEMF(g_eax+28)=(float)fp_pop();g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_ecx==0;g_esp+=4;
}
static void limited_hinge(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-180;PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+8);g_eax=MEM32(g_ebx+40);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,8);g_esi=g_ecx;g_ecx=MEM32(frame+12);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);MEM32(g_esp+28)=g_esi;g_ebp=frame;PUSH32(g_esp,0x2f4b18);setup_rows();
 g_edx=MEM32(g_ebx+28);g_eax=g_esi+32;g_ecx=12;g_esi=g_edx;g_edi=g_esp+32;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;g_xmm1=XMM_MEM(g_esp+64);g_xmm2=XMM_MEM(g_esp+48);g_xmm3=XMM_MEM(g_esp+32);g_ecx=g_esp+96;g_esp+=16;g_ecx-=g_eax;g_esi=4;transform_basis();
 g_xmm0=XMM_MEM(g_edx+48);g_xmm1=XMM_MEM(g_esp+80);g_edx=MEM32(g_ebx+32);g_eax=MEM32(g_esp+12);g_xmm1=XMM_ADD(g_xmm1,g_xmm0);XMM_STORE(g_esp+80,g_xmm1);g_ecx=12;g_esi=g_edx;g_edi=g_esp+16;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;g_xmm1=XMM_MEM(g_esp+48);g_xmm2=XMM_MEM(g_esp+32);g_xmm3=XMM_MEM(g_esp+16);g_eax+=96;g_ecx=g_esp+144-g_eax;g_esi=3;transform_basis();
 g_xmm0=XMM_MEM(g_edx+48);g_xmm1=XMM_MEM(g_esp+144);g_esi=MEM32(frame+12);g_xmm1=XMM_ADD(g_xmm1,g_xmm0);g_xmm0=XMM_MEM(g_esp+112);XMM_STORE(g_esp+32,g_xmm0);g_xmm0=XMM_MEM(g_esp+128);PUSH32(g_esp,g_esi);g_edx=g_esp+20;XMM_STORE(g_esp+20,g_xmm0);g_xmm0=XMM_MEM(g_esp+164);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edx);XMM_STORE(g_esp+156,g_xmm1);XMM_STORE(g_esp+60,g_xmm0);g_ebp=frame;PUSH32(g_esp,0x2f4c54);alignment_row();
 g_xmm0=XMM_MEM(g_esp+140);g_xmm1=XMM_MEM(g_esp+172);XMM_STORE(g_esp+44,g_xmm0);g_xmm0=XMM_MEM(g_esp+124);PUSH32(g_esp,g_esi);XMM_STORE(g_esp+32,g_xmm0);g_xmm0=XMM_MEM(0x49b790);g_eax=g_esp+32;PUSH32(g_esp,g_ebx);g_xmm1=XMM_XOR(g_xmm1,g_xmm0);PUSH32(g_esp,g_eax);XMM_STORE(g_esp+72,g_xmm1);g_ebp=frame;PUSH32(g_esp,0x2f4c8e);alignment_row();
 g_xmm1=XMM_MEM(g_esp+200);g_xmm0=XMM_MEM(g_esp+120);g_xmm2=XMM_MEM(g_esp+136);g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_eax+12);g_edx=MEM32(g_eax+16);XMM_STORE(g_esp+40,g_xmm0);g_edi=MEM32(g_ebx+40);g_xmm0=XMM_MUL(g_xmm1,g_xmm2);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm3.f[0]+=g_xmm0.f[0];g_xmm4=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm0=XMM_MEM(g_esp+184);g_xmm4.f[0]+=g_xmm3.f[0];g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xc9);g_eax=g_esp+96;MEMF(g_eax)=g_xmm4.f[0];g_eax=MEM32(g_esp+96);g_xmm4=XMM_SHUFFLE(g_xmm1,g_xmm1,0xd2);g_xmm4=XMM_MUL(g_xmm4,g_xmm3);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xd2);g_xmm0=XMM_SHUFFLE(g_xmm1,g_xmm1,0xc9);g_xmm0=XMM_MUL(g_xmm0,g_xmm3);g_xmm0=XMM_SUB(g_xmm0,g_xmm4);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm1=g_xmm0;MEM32(g_esp+56)=g_ecx;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,0x55);g_xmm2=g_xmm0;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_ecx=g_esp+100;g_xmm2.f[0]+=g_xmm1.f[0];MEM32(g_esp+60)=g_edx;MEMF(g_ecx)=g_xmm2.f[0];g_edx=MEM32(g_esp+100);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x2f4d3e);sub_00389510();
 MEMF(g_esp+72)=(float)g_fp_stack[g_fp_top];fp_push(MEMF(g_edi+56));g_esp+=32;g_fp_stack[g_fp_top]-=g_fp_stack[(g_fp_top+1)&7];g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x49a9d0));fp_pop();status();
 if(!((g_eax>>8)&0x41)){g_fp_stack[g_fp_top]+=MEMF(0x44d95c);MEMF(g_esp+40)=(float)g_fp_stack[g_fp_top];}
 else{fp_push(g_fp_stack[g_fp_top]);g_fp_stack[g_fp_top]-=MEMF(g_edi+56);g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x49a9d0));fp_pop();status();if(!((g_eax>>8)&0x41)){g_fp_stack[g_fp_top]-=MEMF(0x44d95c);MEMF(g_esp+40)=(float)g_fp_stack[g_fp_top];}}
 PUSH32(g_esp,g_esi);MEMF(g_edi+56)=(float)fp_pop();g_ecx=g_esp+20;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x2f4d8a);angular_limit();
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);g_edx=g_esp+164;PUSH32(g_esp,g_edx);g_eax=g_esp+104;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x2f4d9e);point_rows();
 fp_push(MEMF(0x44c174));g_ecx=MEM32(g_esp+40);fp_push(MEMF(g_ecx+20));g_esp+=28;g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);fp_pop();fp_pop();status();
 if(RECOMP_PARITY8(((g_eax>>8)&255)&0x44)){g_eax=MEM32(g_ecx+20);PUSH32(g_esp,g_esi);g_ecx=g_esp+20;g_edx=g_esp+100;PUSH32(g_esp,g_ebx);g_edi+=48;PUSH32(g_esp,g_ecx);MEM32(g_esp+28)=g_edx;MEM32(g_esp+36)=g_eax;MEM32(g_esp+40)=1;MEM32(g_esp+32)=g_edi;g_ebp=frame;PUSH32(g_esp,0x2f4de1);friction_rows();g_esp+=12;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=12;
}
int uc_migration_physics_joint_rows(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_JOINT_ROWS");enabled=v&&!strcmp(v,"1");}
 if(!enabled||(g_esp&3)||g_esp<0x10800||!span(g_esp-2048,2080)||g_fp_top<0||g_fp_top>7||!safe(0x440000,0xe0000))return 0;
 if(entry==0x345d30){uint32_t context=MEM32(g_esp+4),out=MEM32(g_esp+8);if(!safe(context,28)||!safe(out,8)||!safe(MEM32(out+4),24))return 0;setup_rows();return 1;}
 if(entry!=0x31e840&&entry!=0x31e450&&entry!=0x2f4af0)return 0;
 if((g_ecx&15)||!finite_values(g_ecx+16,30)||!safe(0x51f5a8,1))return 0;
 if(entry==0x2f4af0&&(!finite_values(g_ecx+12,1)||!finite_values(g_ecx+136,2)))return 0;
 uint32_t context=MEM32(g_esp+(entry==0x31e450?16:4)),out=MEM32(g_esp+(entry==0x31e450?20:8));
 if(!safe(context,44)||!safe(out,12)||!safe(MEM32(out),4096)||!safe(MEM32(out+4),4096)||!safe(MEM32(context+40),72))return 0;
 if(!finite_values(MEM32(context+40),18))return 0;
 if(!finite_values(context,2)||!finite_values(MEM32(context+20)+16,28)||!finite_values(MEM32(context+24)+16,28))return 0;
 if(!apart(MEM32(out),4096,MEM32(out+4),4096))return 0;
 if(entry!=0x31e450){if(!finite_values(MEM32(context+28),16)||!finite_values(MEM32(context+32),16))return 0;if(entry==0x31e840)ragdoll();else limited_hinge();}
 else{for(unsigned i=1;i<=3;i++)if(!finite_values(MEM32(g_esp+i*4),4))return 0;cone_plane();}
 return 1;
}
