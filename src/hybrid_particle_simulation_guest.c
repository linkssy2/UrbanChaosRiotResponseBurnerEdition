#include "hybrid_particle_simulation_guest.h"
#include "hybrid_particle_motion_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D38F0(void);
static int simulation_enabled(void){static int flag=-1;if(flag<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_SIMULATION");flag=p&&!strcmp(p,"1");}return flag;}
/* Complete239330 live record simulation, retaining original bounds service.
 * Structured native loop preserves packed-lane arithmetic and stable in-place
 * compaction. No failure is allowed after the original prologue starts. */
static int simulation_view(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int simulation_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_particle_simulation(void){
 if(!simulation_enabled())return 0;
 uint32_t s=g_esp,o=g_ecx;
 if((s&3)||s<0x10800u||!simulation_view(s-2048,2084)||!simulation_view(0x10000,0x5f0000)||
    !simulation_view(o,0x94)||!simulation_apart(o,0x94,s-2048,2084)||MEM8(0x51c178)||
    !simulation_apart(s-2048,2084,0x51c178,1))return 0;
 uint32_t count=MEM32(o+0x90);
 if(count){
  uint32_t gravity=MEM32(s+16),wind=MEM32(s+20),bounds=MEM32(s+32);
  if(!simulation_view(gravity,12)||!simulation_view(wind,12)||!simulation_view(bounds,48)||
     !simulation_apart(gravity,12,s-2048,2084)||!simulation_apart(wind,12,s-2048,2084)||
     !simulation_apart(bounds,48,s-2048,2084)||!simulation_apart(bounds,48,0x51c178,1))return 0;
  if((int32_t)count>0){
   uint32_t data=MEM32(o+4),block=MEM32(o+0x14);
   if(count>0x7fffffffu/32u||!simulation_view(data,count*32)||!simulation_view(block,0x8c)||
      !simulation_apart(data,count*32,o,0x94)||!simulation_apart(data,count*32,block,0x8c)||
      !simulation_apart(data,count*32,s-2048,2084)||!simulation_apart(block,0x8c,s-2048,2084)||
      !simulation_apart(data,count*32,0x51c178,1))return 0;
  }
 }
 uint32_t frame=g_ebp;
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    g_esp = g_esp - 0x74;
    g_edx = MEM32(g_ecx + 0x90);
    PUSH32(g_esp, g_ebx);
    PUSH32(g_esp, g_esi);
    g_esi = 0;
    PUSH32(g_esp, g_edi);
    if(!g_edx){
    g_eax = 0;
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_recomp_zf = !!((g_eax == 0));
    g_esp += 36; return 1;
    }
    g_xmm0 = XMM_SCALAR(MEMF(0x44C724));
    g_xmm5 = XMM_SCALAR(MEMF(frame + 0x20));
    g_xmm5.f[0] = g_xmm5.f[0] * MEMF(0x482FBC);
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44FFDC));
    g_eax = g_esp + 0x18;
    g_xmm3 = XMM_SCALAR(MEMF(g_eax));
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    g_eax = g_esp + 0x18;
    g_xmm4 = XMM_SCALAR(MEMF(g_eax));
    g_eax = MEM32(frame + 0x14);
    g_xmm0 = XMM_SCALAR(MEMF(g_eax));
    MEMF(g_esp + 0x20) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4));
    MEMF(g_esp + 0x24) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8));
    g_eax = MEM32(frame + 0x18);
    g_xmm1 = XMM_SCALAR(MEMF(g_eax));
    MEMF(g_esp + 0x28) = g_xmm0.f[0];
    g_xmm0 = XMM_ZERO();
    MEMF(g_esp + 0x2C) = g_xmm0.f[0];
    g_xmm6 = XMM_MEM(g_esp + 0x20);
    MEMF(g_esp + 0x20) = g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_eax + 4));
    MEMF(g_esp + 0x24) = g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_eax + 8));
    MEMF(g_esp + 0x28) = g_xmm1.f[0];
    MEMF(g_esp + 0x2C) = g_xmm0.f[0];
    g_xmm7 = XMM_MEM(g_esp + 0x20);
    g_xmm3 = XMM_SHUFFLE(g_xmm3, g_xmm3, 0);
    g_xmm4 = XMM_SHUFFLE(g_xmm4, g_xmm4, 0);
    MEM32(g_esp + 0x14) = g_esi;
    XMM_STORE(g_esp + 0x40, g_xmm3);
    XMM_STORE(g_esp + 0x30, g_xmm4);
    MEM32(g_esp + 0x18) = g_esi;
    if((int32_t)g_edx>0){
    g_edx = 0;
    if(!(uc_hybrid_particle_motion()))do{
    g_eax = MEM32(g_ecx + 4);
    g_edi = MEM32(frame + 0x10);
    g_ebx = 0;
    SET_LO16(g_ebx, MEM16(g_esi + g_eax + 0x1C));
    g_edi = g_edi + g_ebx;
    if(LO16(g_edi)>=LO16(g_ebx)){
    g_xmm0 = XMM_MEM(g_esi + g_eax + 0x10);
    SET_LO16(g_ebx, MEM16(g_esi + g_eax + 0x1E));
    g_xmm2 = XMM_MEM(0x51E4A0);
    g_xmm1 = XMM_SCALAR(MEMF(frame + 0x1C));
    g_xmm2 = XMM_XOR(g_xmm2, g_xmm0);
    g_xmm0 = XMM_ADD(g_xmm0, g_xmm6);
    g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm1, 0);
    g_xmm2 = XMM_MUL(g_xmm2, g_xmm1);
    g_xmm1 = g_xmm0;
    g_xmm0 = XMM_SCALAR(MEMF(frame + 8));
    g_xmm1 = XMM_ADD(g_xmm1, g_xmm2);
    MEM16(g_esp + 0x1C) = LO16(g_ebx);
    g_ebx = ZX16(LO16(g_edi));
    g_xmm0 = XMM_SHUFFLE(g_xmm0, g_xmm0, 0);
    g_xmm2 = g_xmm1;
    g_xmm2 = XMM_MUL(g_xmm2, g_xmm0);
    g_xmm0 = XMM_MEM(g_esi + g_eax);
    g_xmm0 = XMM_ADD(g_xmm0, g_xmm2);
    g_xmm2.f[0] = (float)(int32_t)g_ebx;
    g_ebx = MEM32(g_ecx + 0x14);
    g_xmm0 = XMM_ADD(g_xmm0, g_xmm7);
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm5.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] + MEMF(g_ebx + 0x88);
    g_xmm3 = XMM_MIN(g_xmm3, g_xmm0);
    XMM_STORE(g_esp + 0x40, g_xmm3);
    g_xmm4 = XMM_MAX(g_xmm4, g_xmm0);
    XMM_STORE(g_esp + 0x30, g_xmm4);
    XMM_STORE(g_edx + g_eax, g_xmm0);
    XMM_STORE(g_edx + g_eax + 0x10, g_xmm1);
    MEM16(g_edx + g_eax + 0x1C) = LO16(g_edi);
    SET_LO16(g_edi, MEM16(g_esp + 0x1C));
    MEM16(g_edx + g_eax + 0x1E) = LO16(g_edi);
    g_edi = MEM32(g_esp + 0x14);
    MEMF(g_edx + g_eax + 0xC) = g_xmm2.f[0];
    g_xmm3 = XMM_MEM(g_esp + 0x40);
    g_xmm4 = XMM_MEM(g_esp + 0x30);
    ++g_edi;
    MEM32(g_esp + 0x14) = g_edi;
    g_edx = g_edx + 0x20;
    }
    g_eax = MEM32(g_esp + 0x18);
    g_edi = MEM32(g_ecx + 0x90);
    ++g_eax;
    g_esi = g_esi + 0x20;
    MEM32(g_esp + 0x18) = g_eax;
    }while((int32_t)g_eax<(int32_t)g_edi);
    }
    g_ecx = g_esp + 0x30;
    PUSH32(g_esp, g_ecx);
    g_edx = g_esp + 0x44;
    PUSH32(g_esp, g_edx);
    g_ecx = g_esp + 0x58;
    g_ebp = frame;
    PUSH32(g_esp, 0x002394F6u); sub_002D38F0();
    g_edi = MEM32(frame + 0x24);
    g_esi = g_eax;
    g_eax = MEM32(g_esp + 0x14);
    g_ecx = 0xC;
    memcpy((void*)XBOX_PTR(g_edi), (void*)XBOX_PTR(g_esi), g_ecx * 4);
    g_esi += g_ecx * 4; g_edi += g_ecx * 4; g_ecx = 0;
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_esp += 36; return 1;
}
