/* Original joint residual updates and ordered impulse/velocity export. */
#include "migration_physics_joint_finish.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int bounded(uint32_t p,unsigned n){for(unsigned i=0;i<n;i++)if((MEM32(p+i*4)&0x7fffffff)>0x47800000)return 0;return 1;}
static void push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static double pop(void){double v=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;return v;}
static unsigned width(unsigned op){return op==9?8:op==10?16:op==12||op==13?4:0;}
static void residual(unsigned group){
 do{
  if(group==3){
   g_xmm1=XMM_MEM(g_eax+16);g_xmm0=XMM_MEM(g_esi+32);g_xmm2=XMM_MEM(g_edi+16);g_xmm3=XMM_MEM(g_edi+32);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm1=XMM_MEM(g_esi+16);g_xmm1=XMM_SUB(g_xmm1,g_xmm2);g_xmm2=XMM_MEM(g_eax);g_xmm1=XMM_MUL(g_xmm1,g_xmm2);g_xmm2=XMM_MEM(g_eax+32);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);
  }else{
   g_xmm1=XMM_MEM(g_eax);g_xmm0=XMM_MEM(g_esi+32);g_xmm2=XMM_MEM(g_edi+32);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm1=XMM_MEM(g_eax+16);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);
  }
  g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm2=g_xmm0;g_edx=g_esp+(group==3?244:252);g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_xmm2.f[0]+=g_xmm1.f[0];MEMF(g_edx)=g_xmm2.f[0];
  push(MEMF(g_eax+(group==3?12:28)));g_fp_stack[g_fp_top]-=MEMF(g_edx);g_eax+=group==3?48:32;MEMF(g_eax-(group==3?36:4))=(float)pop();g_edx=MEM8(g_ecx+1);g_ecx+=g_edx;
 }while(MEM8(g_ecx+2)==group);
}
static void export_rows(unsigned op,uint32_t frame){
 if(op==13){g_ecx=MEM32(frame+8)+244;MEM32(g_esp+20)=g_ecx;}
 else{g_edx=MEM32(frame+8)+244;MEM32(g_esp+20)=g_edx;}
 do{
  if(op==13){
   g_edx=MEM32(g_edi);g_ecx=MEM32(g_esp+20);MEM32(g_ebx)=g_edx;push(MEMF(g_eax+12));g_fp_stack[g_fp_top]*=MEMF(g_ecx);g_edx=MEM32(g_esp+44);PUSH32(g_esp,1);g_ecx=g_eax;MEMF(g_ebx+4)=(float)pop();g_ebx+=g_edx;PUSH32(g_esp,0x34a1ff);g_eax=MEM32(g_esp+4);g_eax+=g_eax*2;g_eax=g_eax*16+g_ecx;g_recomp_zf=g_eax==0;g_esp+=8;
   g_edx=MEM8(g_esi+1);g_ecx=(uint32_t)(int32_t)SMEM8(g_esi+g_edx);g_esi+=g_edx;g_edx=MEM32(g_esp+48);g_edi+=4;
  }else{
   g_ecx=MEM32(g_edi);g_edx=MEM32(g_esp+20);MEM32(g_ebx)=g_ecx;push(MEMF(g_eax+28));g_fp_stack[g_fp_top]*=MEMF(g_edx);g_ecx=MEM32(g_esp+44);PUSH32(g_esp,1);MEMF(g_ebx+4)=(float)pop();g_ebx+=g_ecx;g_ecx=g_eax;PUSH32(g_esp,0x34a245);g_eax=MEM32(g_esp+4);g_eax=g_eax*32+g_ecx;g_recomp_zf=g_eax==0;g_esp+=8;
   g_ecx=MEM8(g_esi+1);g_edx=(uint32_t)(int32_t)SMEM8(g_esi+g_ecx);g_esi+=g_ecx;g_ecx=MEM32(g_esp+48);g_edi+=4;
  }
 }while(g_ecx==g_edx);
}
int uc_migration_physics_joint_finish(unsigned target,uint32_t frame){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_JOINT_FINISH");enabled=v&&!strcmp(v,"1");}
 int repeat=target==0x34a032||target==0x34a0a1,exporting=target==0x34a1cd||target==0x34a219;
 if(!enabled||(!repeat&&!exporting)||frame!=g_ebp||(unsigned)g_fp_top>7||!span(g_esp-8,264)||!span(frame+8,4)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t cmd=repeat?g_ecx:g_esi,group=(target==0x34a032||target==0x34a1cd)?3:4,rowsize=group==3?48:32,count=0,bytes=0;
 if(!span(cmd,4))return 0;unsigned op=MEM8(cmd);if(!width(op)||(group==3)!=(op==13))return 0;
 do{
  if(!span(cmd+bytes,4))return 0;unsigned next=MEM8(cmd+bytes);if(repeat?MEM8(cmd+bytes+2)!=group:next!=op)break;
  unsigned step=width(next);if(!step||(group==3)!=(next==13)||MEM32(cmd+bytes)!=(next|(step<<8)|(group<<16)))return 0;
  bytes+=step;++count;
 }while(count<64);
 if(!count||count==64||(g_eax&15))return 0;
 uint32_t ranges[9][2]={{g_esp-8,264},{frame+8,4},{cmd,bytes+4},{g_eax,count*rowsize},{0x51c178,1}};unsigned used=5;
 if(repeat){if((g_esi|g_edi)&15)return 0;ranges[used][0]=g_esi+16;ranges[used++][1]=32;ranges[used][0]=g_edi+16;ranges[used++][1]=32;}
 else{
  uint32_t stride=MEM32(g_esp+44),cfg=MEM32(frame+8);if(stride<8||stride>256||MEM32(g_esp+48)!=op)return 0;
  ranges[used][0]=g_edi;ranges[used++][1]=count*4;ranges[used][0]=g_ebx;ranges[used++][1]=(count-1)*stride+8;ranges[used][0]=cfg+244;ranges[used++][1]=4;
 }
 for(unsigned i=0;i<used;i++){if(!span(ranges[i][0],ranges[i][1]))return 0;for(unsigned j=0;j<i;j++)if(!apart(ranges[i][0],ranges[i][1],ranges[j][0],ranges[j][1]))return 0;}
 if(!bounded(g_eax,count*rowsize/4))return 0;
 if(repeat){if(!bounded(g_esi+16,8)||!bounded(g_edi+16,8))return 0;residual(group);}
 else{if(!bounded(MEM32(frame+8)+244,1))return 0;export_rows(op,frame);}return 1;
}
