#include "hybrid_fire_jet_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern void sub_00022A10(void),sub_000236D0(void),sub_0003BD10(void),sub_00055EE0(void);
extern void sub_001A4F90(void),sub_001A50A0(void),sub_001A50F0(void);
extern void sub_001F7AA0(void),sub_001F8DC0(void),sub_001FB780(void),sub_001FB910(void),sub_001FB9D0(void),sub_001FBA40(void);
extern void sub_00207D70(void),sub_00208C50(void),sub_0020A140(void),sub_00214230(void),sub_00216020(void);
extern void sub_00218A60(void),sub_00218C40(void),sub_00218CE0(void),sub_00219530(void);
extern void sub_002D38F0(void),sub_002D3DC0(void),sub_002D57FB(void),sub_002D5CCD(void);
#define JET_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define JET_FRAME_CALL(fn,pc) do{g_ebp=frame;JET_CALL(fn,pc);}while(0)
#define JET_VCALL(target,pc,saved) do{uint32_t jet_target=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(jet_target,saved);}while(0)

/* The existing entity, two particle publications, source wrappers and beam
 * region remain authoritative. Damage1A57A0 and coroutine1A59F0 are unchanged.
 * Native decisions never retry after a retained service has had effects. */
static void jet_destroy(void){
 uint32_t saved=g_esp;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x3975b6);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x476ff8;
 g_eax=MEM32(0x51c398);g_ecx=MEM32(g_eax+12);g_eax=MEM32(g_esi+0x54);g_edx=MEM32(g_ecx);
 PUSH32(g_esp,0x46b800);PUSH32(g_esp,g_eax);MEM32(g_esp+0x18)=1;JET_VCALL(MEM32(g_edx+8),0x1a4fd1,saved);
 SET_LO8(g_eax,MEM8(g_esi+0xe4));
 if(LO8(g_eax)){g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);JET_CALL(sub_00055EE0,0x1a4fea);g_ecx=g_eax;JET_CALL(sub_00022A10,0x1a4ff1);MEM8(g_esi+0xe4)=0;}
 g_ecx=g_esi+0xe8;MEM8(g_esp+0x10)=0;JET_CALL(sub_001FBA40,0x1a5008);
 g_ecx=g_esi;MEM32(g_esp+0x10)=0xffffffffu;JET_CALL(sub_00218CE0,0x1a5017);
 g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void jet_query(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_eax=g_ecx;g_edi=0x45c2ac;g_ecx=15;g_edx=0;unsigned equal=0;
 while(g_ecx){equal=MEM8(g_esi)==MEM8(g_edi);g_esi++;g_edi++;g_ecx--;if(!equal)break;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);
 if(equal){unsigned active=(MEM32(g_eax+0x5c)&0x10000000)!=0;g_eax=MEM32(g_esp+8);MEM32(g_eax)=3;MEM8(g_eax+4)=(uint8_t)active;SET_LO8(g_eax,1);g_recomp_zf=!active;}
 else{SET_LO8(g_eax,0);g_recomp_zf=1;}g_esp+=12;
}
static void jet_source_destroy(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x3975c8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x47708c;
 g_ecx=MEM32(g_esi+8);unsigned child=g_ecx!=0;MEM32(g_esp+0x10)=0;
 if(child){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);JET_VCALL(MEM32(g_eax),0x1a50d8,saved);}
 g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x45c298;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void jet_source_matrix(void){
 uint32_t inherited=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);JET_CALL(sub_00207D70,0xda982);
 if(g_eax){g_ecx=MEM32(g_esi+4);PUSH32(g_esp,0);PUSH32(g_esp,0x45c2ac);PUSH32(g_esp,g_ecx);JET_CALL(sub_001F7AA0,0xda996);g_esp+=12;
  if(LO8(g_eax)){g_ecx=MEM32(g_esi+8);g_edx=MEM32(g_ecx);POP32(g_esp,g_esi);g_seh_ebp=inherited;RECOMP_ITAIL(MEM32(g_edx+8));return;}}
 g_eax=0;POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;
}

/* Compose the authored quarter-turn with the live entity basis. These locals
 * are the exact original packed operations, including both addition trees.
 * The first row scratch store precedes later row loads for alias fidelity. */
static void jet_compose_basis(void){
 RecompXmm row=XMM_MEM(g_esp+0x40),w=XMM_MEM(g_eax+48);
 RecompXmm zpart=XMM_SHUFFLE(row,row,0xff),zcoef=XMM_SHUFFLE(row,row,0xaa),xcoef=XMM_SHUFFLE(row,row,0);
 RecompXmm wpart=XMM_MUL(w,zpart),z=XMM_MEM(g_eax+32);zpart=XMM_MUL(z,zcoef);zpart=XMM_ADD(zpart,wpart);
 RecompXmm y=XMM_MEM(g_eax+16),ycoef=XMM_SHUFFLE(row,row,0x55),ypart=XMM_MUL(y,ycoef),x=XMM_MEM(g_eax);
 RecompXmm result=XMM_MUL(x,xcoef);result=XMM_ADD(result,ypart);result=XMM_ADD(result,zpart);XMM_STORE(g_esp+0x80,result);
 row=XMM_MEM(g_esp+0x50);zpart=XMM_SHUFFLE(row,row,0xff);zcoef=XMM_SHUFFLE(row,row,0xaa);
 wpart=XMM_MUL(w,zpart);zpart=XMM_MUL(z,zcoef);zpart=XMM_ADD(zpart,wpart);ycoef=XMM_SHUFFLE(row,row,0x55);
 RecompXmm last_y_part=XMM_MUL(y,ycoef);xcoef=XMM_SHUFFLE(row,row,0);result=XMM_MUL(x,xcoef);result=XMM_ADD(result,last_y_part);result=XMM_ADD(result,zpart);
 RecompXmm middle=result;row=XMM_MEM(g_esp+0x60);zcoef=XMM_SHUFFLE(row,row,0xff);w=XMM_MUL(w,zcoef);zcoef=XMM_SHUFFLE(row,row,0xaa);z=XMM_MUL(z,zcoef);z=XMM_ADD(z,w);
 ycoef=XMM_SHUFFLE(row,row,0x55);row=XMM_SHUFFLE(row,row,0);x=XMM_MUL(x,row);row=XMM_MEM(g_esp+0x80);y=XMM_MUL(y,ycoef);x=XMM_ADD(x,y);x=XMM_ADD(x,z);
 XMM_STORE(g_esi+16,row);XMM_STORE(g_esi+32,middle);PUSH32(g_esp,12);XMM_STORE(g_esi+48,x);
 g_xmm0=row;g_xmm1=ycoef;g_xmm2=z;g_xmm3=zcoef;g_xmm4=y;g_xmm5=x;g_xmm6=middle;g_xmm7=last_y_part;
}

/* Publish jet and base particles through the live manager. Their ABI scratch
 * layouts differ, so keep the two phases explicit instead of caching owners. */
static void jet_publish_particles(uint32_t frame){
 JET_FRAME_CALL(sub_002D57FB,0x1a532f);g_edi=g_eax;g_esp+=4;MEM32(g_esp+0x14)=g_edi;unsigned allocated=g_edi!=0;MEM8(g_esp+0x22c)=2;
 if(allocated){
  g_ecx=MEM32(0x51c4dc);JET_FRAME_CALL(sub_00208C50,0x1a534f);g_edx=MEM32(g_esi+0x54);MEM32(g_esp+0x10)=g_edx;g_edx=MEM32(g_eax);
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_eax;JET_VCALL(MEM32(g_edx+0x20),0x1a535e,saved);}
  g_ecx=MEM32(g_esp+0x10);MEM32(g_edi)=0x47708c;MEM32(g_edi+4)=g_ecx;MEM32(g_edi+8)=g_eax;MEM32(g_esp+0x10)=g_edi;
 }else MEM32(g_esp+0x10)=0;
 g_ecx=MEM32(0x51c4dc);MEM8(g_esp+0x22c)=1;JET_FRAME_CALL(sub_00208C50,0x1a538f);g_ecx=MEM32(0x51c4dc);g_edi=g_eax;JET_FRAME_CALL(sub_00208C50,0x1a539c);
 g_edx=MEM32(g_esi+0x54);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_esp+0xc4;MEM32(g_esp+0x1c)=g_eax;g_eax=MEM32(g_eax);MEM32(g_esp+0x20)=g_edx;g_edx=MEM32(g_edi);PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM32(g_esp+0x1c)=g_eax;JET_VCALL(MEM32(g_edx+0x1c),0x1a53bd,saved);}
 g_edx=MEM32(g_esp+0x1c);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);JET_FRAME_CALL(sub_00219530,0x1a53ca);g_ecx=MEM32(g_esp+0x24);g_esp+=8;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x18);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x20);JET_VCALL(MEM32(g_eax+0x14),0x1a53de,saved);}
 PUSH32(g_esp,12);JET_FRAME_CALL(sub_002D57FB,0x1a53e5);g_edi=g_eax;g_esp+=4;MEM32(g_esp+0x14)=g_edi;allocated=g_edi!=0;MEM8(g_esp+0x22c)=3;
 if(allocated){
  g_ecx=MEM32(0x51c4dc);JET_FRAME_CALL(sub_00208C50,0x1a5405);g_edx=MEM32(g_eax);g_ebx=MEM32(g_esi+0x54);g_ecx=g_esi+0xd8;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;JET_VCALL(MEM32(g_edx+0x20),0x1a5416,saved);}
  MEM32(g_edi)=0x47708c;MEM32(g_edi+4)=g_ebx;MEM32(g_edi+8)=g_eax;MEM32(g_esp+0x10)=g_edi;
 }else MEM32(g_esp+0x10)=0;
 g_ecx=MEM32(0x51c4dc);MEM8(g_esp+0x22c)=1;JET_FRAME_CALL(sub_00208C50,0x1a5443);g_ecx=MEM32(0x51c4dc);g_ebx=g_eax;JET_FRAME_CALL(sub_00208C50,0x1a5450);
 g_edx=MEM32(g_ebx);g_edi=g_eax;g_eax=MEM32(g_esi+0x54);MEM32(g_esp+0x14)=g_eax;g_eax=MEM32(g_edi);MEM32(g_esp+0x18)=g_eax;g_eax=g_esi+0xd8;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=g_esp+0xc4;PUSH32(g_esp,g_eax);g_ecx=g_ebx;JET_VCALL(MEM32(g_edx+0x1c),0x1a5475,saved);}
 g_ecx=MEM32(g_esp+0x14);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);JET_FRAME_CALL(sub_00219530,0x1a5482);g_edx=MEM32(g_esp+0x1c);g_esp+=8;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x20);PUSH32(g_esp,g_edx);g_ecx=g_edi;JET_VCALL(MEM32(g_eax+0x14),0x1a5494,saved);}
}
static void jet_construct(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x397618);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x218;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x3c)=g_esi;JET_FRAME_CALL(sub_00218A60,0x1a511f);
 g_eax=0;MEM32(g_esi)=0x476ff8;g_ebx=g_esi+0xd4;MEM32(g_ebx)=g_eax;g_ecx=g_esi+0xe8;MEM32(g_esp+0x22c)=g_eax;MEM32(g_esi+0xd8)=g_eax;JET_FRAME_CALL(sub_001FB780,0x1a5147);
 g_edi=MEM32(frame+8);g_eax=g_esp+0x10;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x44d1d0);PUSH32(g_esp,3);g_ecx=g_edi;MEM8(g_esp+0x238)=1;JET_FRAME_CALL(sub_001F8DC0,0x1a5165);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+0x10)&0xff;SET_LO8(g_eax,g_eax!=0);}
 g_ecx=g_esp+0x10;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x4770c4);PUSH32(g_esp,9);g_ecx=g_edi;MEM8(g_esi+0xdc)=LO8(g_eax);MEM32(g_esp+0x1c)=0;JET_FRAME_CALL(sub_001F8DC0,0x1a519a);
 g_edx=MEM32(g_esp+0x10);g_eax=g_esp+0x10;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4770b4);PUSH32(g_esp,9);g_ecx=g_edi;MEM32(g_ebx)=g_edx;MEM32(g_esp+0x1c)=0;JET_FRAME_CALL(sub_001F8DC0,0x1a51bb);
 g_ecx=MEM32(g_esp+0x10);g_edx=g_esp+0x10;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4770a0);MEM32(g_esi+0xd8)=g_ecx;PUSH32(g_esp,2);g_ecx=g_edi;JET_FRAME_CALL(sub_001F8DC0,0x1a51d8);
 unsigned authored=LO8(g_eax)!=0;g_xmm0=XMM_SCALAR(MEMF(0x44d744));if(authored)g_xmm0=XMM_SCALAR(MEMF(g_esp+0x10));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c178);
 g_eax=g_esp+0x20;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);MEMF(g_esi+0xd0)=g_xmm0.f[0];g_xmm0=XMM_ZERO();PUSH32(g_esp,0x3fc90fdb);
 MEMF(g_esp+0x2c)=g_xmm0.f[0];MEMF(g_esp+0x30)=g_xmm0.f[0];MEMF(g_esp+0x34)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,0);g_ecx=g_esp+0x50;MEMF(g_esp+0x3c)=g_xmm0.f[0];JET_FRAME_CALL(sub_002D3DC0,0x1a5234);
 g_ecx=g_esp+0x80;PUSH32(g_esp,g_ecx);g_ecx=g_esi;JET_FRAME_CALL(sub_0003BD10,0x1a5243);
 jet_compose_basis();jet_publish_particles(frame);
 PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_esi+0xe8;JET_FRAME_CALL(sub_001FB9D0,0x1a54a2);
 PUSH32(g_esp,0x100);g_ecx=g_esi;JET_FRAME_CALL(sub_0020A140,0x1a54ae);PUSH32(g_esp,0x2000);g_ecx=g_esi;JET_FRAME_CALL(sub_0020A140,0x1a54ba);
 g_ecx=MEM32(frame+8);g_eax=g_esp+0x10;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x44ca30);PUSH32(g_esp,6);MEM32(g_esp+0x1c)=0;JET_FRAME_CALL(sub_001F8DC0,0x1a54d6);
 g_ecx=MEM32(g_esp+0x10);PUSH32(g_esp,0);g_edx=g_esp+0x18;MEM32(g_esp+0x18)=g_ecx;PUSH32(g_esp,g_edx);g_ecx=g_esi;JET_FRAME_CALL(sub_00216020,0x1a54ec);
 MEM32(g_esi+0xe0)=g_eax;g_eax=MEM32(0x51c398);g_ecx=MEM32(g_eax+12);g_eax=MEM32(g_esi+0x54);g_edx=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,0x46b800);PUSH32(g_esp,g_eax);JET_VCALL(MEM32(g_edx+4),0x1a5508,saved);}
 g_ecx=MEM32(g_esp+0x224);POP32(g_esp,g_edi);MEM8(g_esi+0xe4)=0;g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=8;
}

/* Prepare the beam endpoint and the exact public ray arguments. The temporary
 * W lane and all packed operation order remain observable through CPU state. */
static void jet_prepare_ray(void){
 g_xmm0=XMM_SCALAR(MEMF(0x44db94));g_xmm1=XMM_MEM(g_esi+16);g_xmm3=XMM_MEM(g_esi+32);
 MEMF(g_esp+0x24)=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+0x28)=g_xmm0.f[0];MEMF(g_esp+0x2c)=g_xmm0.f[0];g_eax=g_esp+0x24;
 RecompXmm point=XMM_MEM(g_eax),factor=XMM_SHUFFLE(point,point,0),first=XMM_MUL(g_xmm1,factor);
 factor=XMM_SHUFFLE(point,point,0x55);RecompXmm second=XMM_MUL(g_xmm3,factor);factor=XMM_SHUFFLE(point,point,0xaa);
 point=XMM_MEM(g_esi+48);g_eax=0;SET_LO16(g_eax,MEM16(g_esi+0x94));point=XMM_MUL(point,factor);factor=XMM_MEM(g_esi+64);
 PUSH32(g_esp,0);point=XMM_ADD(point,factor);PUSH32(g_esp,0x4000);first=XMM_ADD(first,second);first=XMM_ADD(first,point);PUSH32(g_esp,g_eax);XMM_STORE(g_esp+0x1c,first);
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0x1c));MEMF(g_esp+0x30)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x20));PUSH32(g_esp,9);g_ecx=g_esp+0x34;PUSH32(g_esp,g_ecx);MEMF(g_esp+0x3c)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0x2c));g_edi=g_esi+64;PUSH32(g_esp,g_edi);MEMF(g_esp+0x44)=g_xmm0.f[0];g_xmm1=first;g_xmm2=factor;g_xmm3=second;
}
static void jet_clip_length(void){
 float dx=MEMF(g_eax);dx=dx-MEMF(g_edi);float dy=MEMF(g_eax+4);dy=dy-MEMF(g_edi+4);float dz=MEMF(g_eax+8);dz=dz-MEMF(g_edi+8);
 float xx=dx*dx,yy=dy*dy;float sum=xx+yy;float zz=dz*dz;sum=sum+zz;MEMF(g_esp+12)=sum;
 g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=MEMF(g_esp+12);g_fp_stack[g_fp_top]=sqrt(g_fp_stack[g_fp_top]);MEMF(g_esp+12)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;
 g_xmm0=XMM_SCALAR(dz);g_xmm1=XMM_SCALAR(zz);g_xmm2=XMM_SCALAR(yy);g_xmm3=XMM_SCALAR(sum);
}
/* This original point transform is followed by the same two by-value vectors
 * and world-region registration. No detached region or replacement world. */
static void jet_prepare_region(void){
 RecompXmm first=XMM_MEM(g_eax),second=XMM_MEM(g_eax+16);g_ecx=g_esp+16;
 RecompXmm point=XMM_MEM(g_ecx),factor=XMM_SHUFFLE(point,point,0);first=XMM_MUL(first,factor);factor=XMM_SHUFFLE(point,point,0x55);second=XMM_MUL(second,factor);
 factor=XMM_SHUFFLE(point,point,0xaa);point=XMM_MEM(g_eax+32);point=XMM_MUL(point,factor);factor=XMM_MEM(g_eax+48);point=XMM_ADD(point,factor);
 PUSH32(g_esp,1);g_esp-=12;g_eax=g_esp;first=XMM_ADD(first,second);first=XMM_ADD(first,point);XMM_STORE(g_esp+32,first);
 g_xmm0=XMM_SCALAR(MEMF(g_esp+32));MEMF(g_eax)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+36));MEMF(g_eax+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+40));MEMF(g_eax+8)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0x58));g_esp-=12;g_eax=g_esp;MEMF(g_eax)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x68));MEMF(g_eax+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x6c));MEMF(g_eax+8)=g_xmm0.f[0];
 g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);g_xmm1=first;g_xmm2=factor;g_xmm3=second;
}
static void jet_update(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x98;PUSH32(g_esp,g_esi);g_esi=g_ecx;
 unsigned active=(MEM32(g_esi+0x5c)&0x10000000)!=0;SET_LO8(g_eax,MEM8(g_esi+0xe4));PUSH32(g_esp,g_edi);
 if(active){
  if(!LO8(g_eax)){
   jet_prepare_ray();JET_FRAME_CALL(sub_00214230,0x1a5626);g_esp+=24;if(g_eax)jet_clip_length();
   PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ecx=g_esp+0x38;JET_FRAME_CALL(sub_002D38F0,0x1a5681);
   g_xmm0=XMM_SCALAR(MEMF(g_esp+12));MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44d8f8));g_edx=g_esp+0x60;MEMF(g_esp+20)=g_xmm0.f[0];
   g_xmm0=XMM_SCALAR(MEMF(0x467798));PUSH32(g_esp,g_edx);g_ecx=g_esi;MEMF(g_esp+28)=g_xmm0.f[0];JET_FRAME_CALL(sub_0003BD10,0x1a56b5);
   jet_prepare_region();JET_FRAME_CALL(sub_00055EE0,0x1a5752);g_ecx=g_eax;JET_FRAME_CALL(sub_000236D0,0x1a5759);MEM8(g_esi+0xe4)=1;
  }
 }else if(LO8(g_eax)){
  g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);JET_FRAME_CALL(sub_00055EE0,0x1a5775);g_ecx=g_eax;JET_FRAME_CALL(sub_00022A10,0x1a577c);MEM8(g_esi+0xe4)=0;
 }
 g_ecx=g_esi+0xe8;JET_FRAME_CALL(sub_001FB910,0x1a578e);g_ecx=g_esi;JET_FRAME_CALL(sub_00218C40,0x1a5795);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_esp+=4;
}

static int jet_enabled(void){static int value=-1;if(value<0){const char*v=getenv("UCRR_NATIVE_FIRE_JET");value=v&&!strcmp(v,"1");}return value;}
static int jet_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int jet_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_fire_jet(uint32_t entry){
 if(!jet_enabled())return 0;
 switch(entry){case 0xda970:case 0x1a4f90:case 0x1a5030:case 0x1a5080:case 0x1a50a0:case 0x1a50f0:case 0x1a5530:case 0x1a5550:case 0x1a5990:case 0x218f70:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx;unsigned source=entry==0xda970||entry==0x218f70||entry==0x1a5080||entry==0x1a50a0,n=source?12:0xf0;
 if((s&3)||s<0x12000u||!jet_span(s-8192,8224)||!jet_span(0x51c178,1)||MEM8(0x51c178)||!jet_apart(s-8192,8224,0x51c178,1)||(unsigned)g_fp_top>7)return 0;
 if(entry!=0x1a5990&&(!jet_span(p,n)||!jet_apart(p,n,s-8192,8224)||!jet_apart(p,n,0x51c178,1)))return 0;
 if((entry==0xda970||entry==0x218f70)&&MEM32(p)!=0x47708c)return 0;
 switch(entry){
 case 0xda970:jet_source_matrix();break;
 case 0x218f70:{uint32_t frame=g_seh_ebp;g_ecx=MEM32(g_ecx+8);g_eax=MEM32(g_ecx);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_eax+4));break;}
 case 0x1a4f90:jet_destroy();break;
 case 0x1a5030:jet_query();break;
 case 0x1a50a0:jet_source_destroy();break;
 case 0x1a50f0:jet_construct();break;
 case 0x1a5550:jet_update();break;
 case 0x1a5080:case 0x1a5530:
  PUSH32(g_esp,g_esi);g_esi=g_ecx;if(entry==0x1a5080){JET_CALL(sub_001A50A0,0x1a5088);}else{JET_CALL(sub_001A4F90,0x1a5538);}
  if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);if(entry==0x1a5080){JET_CALL(sub_002D5CCD,0x1a5095);}else{JET_CALL(sub_002D5CCD,0x1a5545);}g_esp+=4;}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;break;
 case 0x1a5990:
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39765b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xf0);JET_CALL(sub_002D57FB,0x1a59b0);g_esp+=4;MEM32(g_esp)=g_eax;
  {unsigned allocated=g_eax!=0;MEM32(g_esp+12)=0;if(allocated){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;JET_CALL(sub_001A50F0,0x1a59ce);}else g_eax=0;g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;}break;
 }
 return 1;
}
#undef JET_CALL
#undef JET_FRAME_CALL
#undef JET_VCALL
