/* Animation pose -> original rigid-body transforms, preserving guest FP order. */
#include "migration_physics_ragdoll_pose.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002D5640(void);void sub_002F1BF0(void);void sub_002ECAA0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
#define SPLAT(x,i) XMM_SHUFFLE(x,x,i)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-512||(uint64_t)g_esp+32<=p);}
static void actor_axis(void){
 g_xmm0=XMM_MEM(g_esi+0x30);g_xmm2=SPLAT(g_xmm0,255);g_xmm1=XMM_MEM(0x51dab0);g_xmm3=XMM_MUL(g_xmm1,g_xmm2);
 g_xmm4=SPLAT(g_xmm0,170);g_xmm2=XMM_MEM(0x51daa0);g_xmm5=XMM_ADD(XMM_MUL(g_xmm2,g_xmm4),g_xmm3);
 g_xmm4=SPLAT(g_xmm0,85);g_xmm3=XMM_MEM(0x51da90);g_xmm6=XMM_MUL(g_xmm3,g_xmm4);g_xmm0=SPLAT(g_xmm0,0);g_xmm4=XMM_MEM(0x51da80);g_xmm7=XMM_MUL(g_xmm4,g_xmm0);g_xmm0=XMM_ADD(XMM_ADD(g_xmm7,g_xmm6),g_xmm5);XMM_STORE(g_esp+60,g_xmm0);
 for(unsigned row=1;row<3;row++){
  g_xmm0=XMM_MEM(g_esi+0x30+row*16);g_xmm5=SPLAT(g_xmm0,255);g_xmm6=XMM_MUL(g_xmm1,g_xmm5);g_xmm5=SPLAT(g_xmm0,170);g_xmm7=XMM_MUL(g_xmm2,g_xmm5);g_xmm5=SPLAT(g_xmm0,85);g_xmm7=XMM_ADD(g_xmm7,g_xmm6);g_xmm6=XMM_MUL(g_xmm3,g_xmm5);g_xmm5=SPLAT(g_xmm0,0);g_xmm0=XMM_ADD(XMM_ADD(XMM_MUL(g_xmm4,g_xmm5),g_xmm6),g_xmm7);XMM_STORE(g_esp+60+row*16,g_xmm0);
 }
 g_xmm0=XMM_MEM(g_esi+0x60);g_xmm5=SPLAT(g_xmm0,255);g_xmm1=XMM_MUL(g_xmm1,g_xmm5);PUSH32(g_esp,g_edi);g_eax=MEM32(g_esi+0x74);g_xmm5=SPLAT(g_xmm0,170);g_xmm2=XMM_ADD(XMM_MUL(g_xmm2,g_xmm5),g_xmm1);g_xmm1=SPLAT(g_xmm0,85);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);g_xmm1=SPLAT(g_xmm0,0);g_xmm4=XMM_ADD(XMM_MUL(g_xmm4,g_xmm1),g_xmm3);g_edi=0;g_xmm0=XMM_ADD(g_xmm4,g_xmm2);XMM_STORE(g_esp+112,g_xmm0);
}
static void bone_world(void){
 g_xmm0=XMM_MEM(g_eax);g_xmm3=XMM_MEM(g_esp+112);g_xmm5=XMM_MEM(g_esp+96);g_xmm1=SPLAT(g_xmm0,255);g_xmm2=XMM_MUL(g_xmm3,g_xmm1);g_xmm1=SPLAT(g_xmm0,170);g_xmm4=XMM_ADD(XMM_MUL(g_xmm5,g_xmm1),g_xmm2);g_xmm1=SPLAT(g_xmm0,85);g_xmm2=g_xmm4;g_xmm4=XMM_MUL(XMM_MEM(g_esp+80),g_xmm1);g_xmm1=g_xmm4;g_xmm0=SPLAT(g_xmm0,0);g_xmm4=XMM_MEM(g_esp+64);g_xmm6=XMM_MUL(g_xmm4,g_xmm0);g_xmm1=XMM_ADD(g_xmm6,g_xmm1);g_xmm0=XMM_MEM(g_eax+16);g_xmm1=XMM_ADD(g_xmm1,g_xmm2);
 g_xmm2=SPLAT(g_xmm0,255);g_xmm6=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=SPLAT(g_xmm0,170);g_xmm7=XMM_ADD(XMM_MUL(g_xmm5,g_xmm2),g_xmm6);g_xmm2=SPLAT(g_xmm0,85);g_xmm6=XMM_MUL(XMM_MEM(g_esp+80),g_xmm2);g_xmm2=SPLAT(g_xmm0,0);g_xmm2=XMM_MUL(g_xmm4,g_xmm2);g_xmm0=XMM_MEM(g_eax+32);g_xmm2=XMM_ADD(g_xmm2,g_xmm6);g_xmm6=SPLAT(g_xmm0,255);g_xmm3=XMM_MUL(g_xmm3,g_xmm6);g_xmm2=XMM_ADD(g_xmm2,g_xmm7);
 g_xmm6=SPLAT(g_xmm0,170);g_xmm7=XMM_ADD(XMM_MUL(g_xmm5,g_xmm6),g_xmm3);g_xmm3=SPLAT(g_xmm0,85);g_xmm6=XMM_MUL(XMM_MEM(g_esp+80),g_xmm3);g_xmm3=SPLAT(g_xmm0,0);g_xmm3=XMM_MUL(g_xmm4,g_xmm3);g_xmm0=XMM_MEM(g_eax+48);g_xmm3=XMM_ADD(XMM_ADD(g_xmm3,g_xmm6),g_xmm7);
 g_xmm6=SPLAT(g_xmm0,255);g_xmm7=XMM_MUL(XMM_MEM(g_esp+112),g_xmm6);g_xmm6=SPLAT(g_xmm0,170);g_xmm5=XMM_MUL(g_xmm5,g_xmm6);g_xmm6=SPLAT(g_xmm0,85);g_xmm5=XMM_ADD(g_xmm5,g_xmm7);g_xmm7=XMM_MUL(XMM_MEM(g_esp+80),g_xmm6);g_xmm6=SPLAT(g_xmm0,0);g_xmm4=XMM_MUL(g_xmm4,g_xmm6);g_xmm0=XMM_ADD(XMM_ADD(g_xmm4,g_xmm7),g_xmm5);
}
static void fp_push_pose(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void fp_add_pose(void){unsigned next=(g_fp_top+1)&7;g_fp_stack[next]+=g_fp_stack[g_fp_top];g_fp_top=(int)next;}
static void pose_to_bodies(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-180;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;actor_axis();
 while((int32_t)g_edi<(int32_t)g_eax){
  g_eax=MEM32(MEM32(g_esi+4)+g_edi*4);g_ecx=MEM32(g_esi+32);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x2a5331);RECOMP_ICALL_SAFE(target,saved);
  bone_world();g_eax=g_esp+128;PUSH32(g_esp,g_eax);g_ecx=g_esp+16;XMM_STORE(g_esp+132,g_xmm1);XMM_STORE(g_esp+148,g_xmm2);XMM_STORE(g_esp+164,g_xmm3);XMM_STORE(g_esp+180,g_xmm0);g_ebp=frame;CALL(0x2a548d,sub_002D5640);
  fp_push_pose(MEMF(g_esp+12));g_fp_stack[g_fp_top]*=MEMF(g_esp+12);g_ecx=MEM32(g_esi+112);fp_push_pose(MEMF(g_esp+16));g_edx=g_esp+32;g_fp_stack[g_fp_top]*=MEMF(g_esp+16);PUSH32(g_esp,g_edx);g_eax=g_esp+52;fp_add_pose();fp_push_pose(MEMF(g_esp+24));PUSH32(g_esp,g_eax);g_fp_stack[g_fp_top]*=MEMF(g_esp+28);fp_add_pose();fp_push_pose(MEMF(g_esp+32));g_fp_stack[g_fp_top]*=MEMF(g_esp+32);fp_add_pose();g_fp_stack[g_fp_top]=sqrt(g_fp_stack[g_fp_top]);g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];MEMF(g_esp+36)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
  g_xmm0=XMM_SCALAR(MEMF(g_esp+36));g_xmm1=g_xmm0;g_xmm1.f[0]*=MEMF(g_esp+20);MEMF(g_esp+20)=g_xmm1.f[0];MEMF(g_esp+40)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+184));g_xmm2=g_xmm0;g_xmm2.f[0]*=MEMF(g_esp+24);g_xmm3=g_xmm0;g_xmm0.f[0]*=MEMF(g_esp+32);g_xmm3.f[0]*=MEMF(g_esp+28);MEMF(g_esp+32)=g_xmm0.f[0];MEMF(g_esp+52)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44d024));g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+56)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+188));g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+60)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+192));g_xmm1.f[0]*=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+24)=g_xmm2.f[0];MEMF(g_esp+28)=g_xmm3.f[0];MEMF(g_esp+44)=g_xmm2.f[0];MEMF(g_esp+48)=g_xmm3.f[0];MEMF(g_esp+64)=g_xmm1.f[0];MEMF(g_esp+68)=g_xmm0.f[0];
  g_ebx=MEM32(g_ecx+g_edi*4);g_ecx=g_ebx;g_ebp=frame;CALL(0x2a5577,sub_002F1BF0);g_ecx=g_ebx;g_ebp=frame;CALL(0x2a557e,sub_002ECAA0);g_ecx=MEM32(g_ebx+80);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0x51e7c0);target=MEM32(g_edx+88);PUSH32(g_esp,0x2a558b);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esi+116);++g_edi;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;
}
int uc_migration_physics_ragdoll_pose(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_POSE");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2a51c0||!span(g_esp-512,544)||!safe(g_ecx,120)||!span(0x51da80,64)||!span(0x44bf94,4)||!span(0x44d024,4))return 0;
 uint32_t p=g_ecx,n=MEM32(p+116);if(n>65535)return 0;if(n){if(!safe(MEM32(p+4),n*4)||!safe(MEM32(p+112),n*4)||!safe(MEM32(p+32),4)||!safe(MEM32(MEM32(p+32)),12))return 0;for(unsigned i=0;i<n;i++){uint32_t b=MEM32(MEM32(p+112)+i*4);if(!safe(b,84)||!safe(MEM32(b+80),4)||!safe(MEM32(MEM32(b+80)),92))return 0;}}
 pose_to_bodies();return 1;
}
#undef SPLAT
#undef CALL
