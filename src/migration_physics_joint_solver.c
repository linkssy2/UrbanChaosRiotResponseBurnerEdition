/* Exact joint opcode runs inside the original ordered solver. No copied world,
 * new integration policy, changed limits, or independent impulse authority. */
#include "migration_physics_joint_solver.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int bounded(uint32_t p,unsigned n){for(unsigned i=0;i<n;i++)if((MEM32(p+i*4)&0x7fffffff)>0x47800000)return 0;return 1;}
static void push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static double pop(void){double v=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;return v;}
static void status(void){SET_LO16(g_eax,(uint16_t)(((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000)));}
static void friction(uint32_t frame){
 g_ecx=MEM32(frame+8);g_eax=MEM32(g_esp+24);g_ecx+=12;MEM32(g_esp+36)=g_ecx;g_ecx=g_eax+48;MEM32(g_esp+20)=g_ecx;g_ecx=g_eax+32;
 do{
  g_xmm1=XMM_MEM(g_esi);push(MEMF(g_esi+28));g_xmm0=XMM_MEM(g_edx+32);g_xmm2=XMM_MEM(g_ecx);g_eax=MEM32(g_ebx+4);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm1=XMM_MEM(g_esi+16);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);MEM32(g_esp+60)=g_eax;g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm2=g_xmm0;g_eax=g_esp+136;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_xmm2.f[0]+=g_xmm1.f[0];MEMF(g_eax)=g_xmm2.f[0];g_eax=MEM32(g_esp+36);g_fp_stack[g_fp_top]*=MEMF(g_eax);g_eax=MEM32(frame+8);push(MEMF(g_esp+136));g_fp_stack[g_fp_top]*=MEMF(g_eax+8);g_fp_stack[(g_fp_top+1)&7]-=g_fp_stack[g_fp_top];pop();g_fp_stack[g_fp_top]*=MEMF(g_esi+12);push(g_fp_stack[g_fp_top]);g_fp_stack[g_fp_top]=fabs(g_fp_stack[g_fp_top]);g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(g_esp+60));status();
  if(!((g_eax>>8)&0x41)){g_fp_stack[g_fp_top]=MEMF(g_esp+60)/g_fp_stack[g_fp_top];push(g_fp_stack[g_fp_top]);g_fp_stack[(g_fp_top+2)&7]*=g_fp_stack[g_fp_top];pop();g_fp_stack[g_fp_top]*=MEMF(g_esi+28);MEMF(g_esi+28)=(float)pop();}else pop();
  g_xmm2=XMM_MEM(g_edx+48);MEMF(g_esp+32)=(float)g_fp_stack[g_fp_top];g_xmm1=XMM_SCALAR(MEMF(g_esp+32));g_eax=MEM32(g_esp+20);g_xmm0=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=XMM_MEM(g_eax);g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm2);g_xmm2=XMM_MEM(g_esi);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=XMM_MEM(g_esi+16);g_xmm1=XMM_MUL(g_xmm1,g_xmm2);g_xmm2=XMM_MEM(g_edx+32);g_xmm2=XMM_ADD(g_xmm2,g_xmm0);XMM_STORE(g_edx+32,g_xmm2);g_xmm0=XMM_MEM(g_ecx);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);XMM_STORE(g_ecx,g_xmm0);g_fp_stack[g_fp_top]+=MEMF(g_edi);g_ebx+=8;g_esi+=32;MEMF(g_edi)=(float)pop();SET_LO8(g_eax,MEM8(g_ebx));g_edi+=4;
 }while((uint8_t)g_eax==9);
}
static void limits(uint32_t frame){
 g_ecx=MEM32(g_esp+24)+32;
 do{
  g_xmm0=XMM_MEM(g_edx+32);g_xmm1=XMM_MEM(g_ecx);g_xmm2=XMM_MEM(g_esi);g_xmm3=XMM_MEM(g_esi+16);g_xmm1=XMM_MUL(g_xmm1,g_xmm3);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm4=g_xmm0;g_eax=g_esp+72;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm0,0xaa);g_xmm4.f[0]+=g_xmm1.f[0];MEMF(g_eax)=g_xmm4.f[0];push(MEMF(g_esp+72));g_eax=MEM32(frame+8);g_fp_stack[g_fp_top]*=MEMF(g_eax+8);push(MEMF(g_esi+28));g_fp_stack[g_fp_top]-=MEMF(g_ebx+4);g_fp_stack[g_fp_top]*=MEMF(g_ebx+12);g_fp_stack[g_fp_top]-=g_fp_stack[(g_fp_top+1)&7];MEMF(g_esp+36)=(float)g_fp_stack[g_fp_top];g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x44c174));pop();status();
  int apply=0;
  if(!((g_eax>>8)&0x41)){pop();push(MEMF(g_esp+36));g_fp_stack[g_fp_top]*=MEMF(g_esi+12);MEMF(g_esp+36)=(float)g_fp_stack[g_fp_top];g_xmm1=XMM_SCALAR(MEMF(g_esp+36));apply=1;}
  else{push(MEMF(g_esi+28));g_fp_stack[g_fp_top]-=MEMF(g_ebx+8);g_fp_stack[g_fp_top]*=MEMF(g_ebx+12);g_fp_stack[(g_fp_top+1)&7]=g_fp_stack[g_fp_top]-g_fp_stack[(g_fp_top+1)&7];pop();g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x44c174));status();if(!RECOMP_PARITY8(((g_eax>>8)&255)&5)){g_fp_stack[g_fp_top]*=MEMF(g_esi+12);MEMF(g_esp+36)=(float)g_fp_stack[g_fp_top];g_xmm1=XMM_SCALAR(MEMF(g_esp+36));apply=1;}}
  if(apply){g_xmm4=XMM_MEM(g_edx+48);g_eax=MEM32(g_esp+24);g_xmm0=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm0=XMM_MUL(g_xmm0,g_xmm4);g_xmm4=XMM_MEM(g_eax+48);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=XMM_MEM(g_edx+32);g_xmm2=XMM_ADD(g_xmm2,g_xmm0);XMM_STORE(g_edx+32,g_xmm2);g_xmm0=XMM_MEM(g_ecx);g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm4);g_xmm1=XMM_MUL(g_xmm1,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);XMM_STORE(g_ecx,g_xmm0);g_fp_stack[g_fp_top]+=MEMF(g_edi);MEMF(g_edi)=(float)pop();}else pop();
  SET_LO8(g_eax,MEM8(g_ebx+16));g_ebx+=16;g_esi+=32;g_edi+=4;
 }while((uint8_t)g_eax==10);
}
static void alignment(uint32_t frame){
 g_ecx=MEM32(g_esp+24);g_eax=g_ecx+48;g_ecx+=32;MEM32(g_esp+20)=g_eax;g_eax=g_edx+32;MEM32(g_esp+28)=g_ecx;
 do{
  g_xmm2=XMM_MEM(g_esi);push(MEMF(g_esi+12));g_xmm3=XMM_MEM(g_esi+16);push(MEMF(g_esi+28));g_xmm0=XMM_MEM(g_edx+32);g_ecx=MEM32(g_esp+28);g_xmm1=XMM_MEM(g_ecx);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm1=XMM_MUL(g_xmm1,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm1.f[0]+=g_xmm0.f[0];g_xmm4=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm4.f[0]+=g_xmm1.f[0];g_ecx=g_esp+152;MEMF(g_ecx)=g_xmm4.f[0];g_ecx=MEM32(frame+8);g_fp_stack[g_fp_top]*=MEMF(g_ecx+4);g_xmm4=XMM_MEM(g_edx+48);push(MEMF(g_esp+152));g_ebx+=4;g_fp_stack[g_fp_top]*=MEMF(g_ecx+8);g_ecx=MEM32(g_esp+20);g_esi+=32;g_edi+=4;g_fp_stack[(g_fp_top+1)&7]-=g_fp_stack[g_fp_top];pop();g_fp_stack[(g_fp_top+1)&7]*=g_fp_stack[g_fp_top];pop();MEMF(g_esp+36)=(float)g_fp_stack[g_fp_top];g_xmm1=XMM_SCALAR(MEMF(g_esp+36));g_xmm0=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm0=XMM_MUL(g_xmm0,g_xmm4);g_xmm4=XMM_MEM(g_ecx);g_ecx=MEM32(g_esp+28);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=XMM_MEM(g_eax);g_xmm2=XMM_ADD(g_xmm2,g_xmm0);XMM_STORE(g_eax,g_xmm2);g_xmm0=XMM_MEM(g_ecx);g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm4);g_xmm1=XMM_MUL(g_xmm1,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);XMM_STORE(g_ecx,g_xmm0);g_fp_stack[g_fp_top]+=MEMF(g_edi-4);MEMF(g_edi-4)=(float)pop();SET_LO8(g_ecx,MEM8(g_ebx));
 }while((uint8_t)g_ecx==12);
}
static void points(uint32_t frame){
 g_ecx=MEM32(g_esp+24);g_eax=g_ecx+48;g_ecx+=16;MEM32(g_esp+40)=g_ecx;g_ecx=MEM32(g_esp+24)+32;MEM32(g_esp+20)=g_eax;g_eax=g_edx+16;MEM32(g_esp+28)=g_ecx;
 do{
  g_xmm1=XMM_MEM(g_edx+16);push(MEMF(g_esi+28));g_xmm2=XMM_MEM(g_esi+16);push(MEMF(g_esi+12));g_xmm3=XMM_MEM(g_esi+32);g_ecx=MEM32(g_esp+40);g_xmm0=XMM_MEM(g_ecx);g_ecx=MEM32(g_esp+28);g_xmm4=XMM_MEM(g_ecx);g_xmm1=XMM_SUB(g_xmm1,g_xmm0);g_xmm0=XMM_MEM(g_edx+32);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=XMM_MEM(g_esi);g_xmm1=XMM_MUL(g_xmm1,g_xmm2);g_xmm4=XMM_MUL(g_xmm4,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm4);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm1.f[0]+=g_xmm0.f[0];g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm3.f[0]+=g_xmm1.f[0];g_ecx=g_esp+240;MEMF(g_ecx)=g_xmm3.f[0];g_ecx=MEM32(frame+8);g_fp_stack[g_fp_top]*=MEMF(g_ecx+4);g_xmm3=XMM_MEM(g_edx+48);push(MEMF(g_esp+240));g_ebx+=4;g_fp_stack[g_fp_top]*=MEMF(g_ecx+8);g_ecx=MEM32(g_esp+20);g_esi+=48;g_edi+=4;g_fp_stack[(g_fp_top+1)&7]-=g_fp_stack[g_fp_top];pop();g_fp_stack[(g_fp_top+1)&7]*=g_fp_stack[g_fp_top];pop();MEMF(g_esp+36)=(float)g_fp_stack[g_fp_top];g_xmm1=XMM_SCALAR(MEMF(g_esp+36));g_xmm0=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm0=XMM_MUL(g_xmm0,g_xmm3);g_xmm3=XMM_MEM(g_ecx);g_ecx=MEM32(g_esp+40);g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm3);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xff);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm4=XMM_SHUFFLE(g_xmm1,g_xmm1,0xff);g_xmm4=XMM_MUL(g_xmm4,g_xmm2);g_xmm2=XMM_MEM(g_eax);g_xmm2=XMM_ADD(g_xmm2,g_xmm3);XMM_STORE(g_eax,g_xmm2);g_xmm2=XMM_MEM(g_ecx);g_xmm2=XMM_SUB(g_xmm2,g_xmm4);XMM_STORE(g_ecx,g_xmm2);g_xmm2=XMM_MEM(g_esi-32);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=XMM_MEM(g_esi-16);g_ecx=g_edx+32;g_xmm1=XMM_MUL(g_xmm1,g_xmm2);g_xmm2=XMM_MEM(g_ecx);g_xmm2=XMM_ADD(g_xmm2,g_xmm0);XMM_STORE(g_ecx,g_xmm2);g_ecx=MEM32(g_esp+28);g_xmm0=XMM_MEM(g_ecx);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);XMM_STORE(g_ecx,g_xmm0);g_fp_stack[g_fp_top]+=MEMF(g_edi-4);MEMF(g_edi-4)=(float)pop();SET_LO8(g_ecx,MEM8(g_ebx));
 }while((uint8_t)g_ecx==13);
}
int uc_migration_physics_joint_solver(unsigned type,uint32_t frame){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_JOINT_SOLVER");enabled=v&&!strcmp(v,"1");}
 if(!enabled||(type!=9&&type!=10&&type!=12&&type!=13)||frame!=g_ebp||(unsigned)g_fp_top>7||!span(g_esp,256)||!span(frame+8,4)||!span(0x51c178,1)||MEM8(0x51c178)||!span(0x44c174,4)||MEM32(0x44c174))return 0;
 uint32_t b=MEM32(g_esp+24),cfg=MEM32(frame+8),step=type==9?8:type==10?16:4,rowsize=type==13?48:32,word=type==9?0x40809:type==10?0x4100a:type==12?0x4040c:0x3040d,count=0;
 if((g_edx|b|g_esi)&15)return 0;
 while(count<64){uint32_t p=g_ebx+count*step;if(!span(p,4))return 0;if(MEM8(p)!=type)break;if(MEM32(p)!=word)return 0;++count;}
 if(!count||count==64)return 0;
 const uint32_t ranges[][2]={{g_esp,256},{frame+8,4},{g_edx+16,48},{b+16,48},{cfg+4,12},{g_ebx,count*step+1},{g_esi,count*rowsize},{g_edi,count*4},{0x44c174,4},{0x51c178,1}};
 for(unsigned i=0;i<10;i++){if(!span(ranges[i][0],ranges[i][1]))return 0;for(unsigned j=0;j<i;j++)if(!apart(ranges[i][0],ranges[i][1],ranges[j][0],ranges[j][1]))return 0;}
 if(!bounded(g_edx+16,12)||!bounded(b+16,12)||!bounded(cfg+4,3)||!bounded(g_esi,count*rowsize/4)||!bounded(g_edi,count))return 0;
 for(unsigned i=0;i<count;i++)if(!bounded(g_ebx+i*step+4,step/4-1))return 0;
 if(type==9)friction(frame);else if(type==10)limits(frame);else if(type==12)alignment(frame);else points(frame);return 1;
}
