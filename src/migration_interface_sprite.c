#include "migration_interface_sprite.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273730(void),sub_00087100(void),sub_00065550(void),sub_00065760(void);
int migration_interface_sprite(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_SPRITE");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||(uint64_t)s+16>g_xbox_total_ram||g_ecx<0x10000||(uint64_t)g_ecx+80>g_xbox_total_ram)return 0;
 g_esp-=32;g_eax=MEM32(g_esp+36);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_esi=g_ecx;PUSH32(g_esp,0x2739b0);sub_00273730();
 g_xmm3=XMM_SCALAR(MEMF(g_esi+32));g_xmm4=XMM_SCALAR(MEMF(g_esi+36));g_xmm0=XMM_SCALAR(MEMF(g_esi+64));g_xmm1=XMM_SCALAR(MEMF(g_esi+68));g_xmm5=XMM_SCALAR(MEMF(g_esi+52));g_xmm0.f[0]+=g_xmm3.f[0];g_xmm1.f[0]+=g_xmm4.f[0];g_xmm2=g_xmm0;g_xmm2.f[0]+=g_xmm3.f[0];g_xmm3=g_xmm1;g_xmm3.f[0]+=g_xmm4.f[0];g_xmm4=XMM_SCALAR(MEMF(0x44c1f8));g_xmm2.f[0]*=g_xmm4.f[0];g_xmm3.f[0]*=g_xmm4.f[0];g_xmm0.f[0]-=g_xmm2.f[0];g_xmm0.f[0]*=g_xmm5.f[0];g_xmm4=g_xmm2;g_xmm4.f[0]-=g_xmm0.f[0];MEMF(g_esp+4)=g_xmm4.f[0];g_ecx=MEM32(g_esp+4);g_xmm1.f[0]-=g_xmm3.f[0];g_xmm1.f[0]*=g_xmm5.f[0];g_xmm0.f[0]+=g_xmm2.f[0];MEMF(g_esp+4)=g_xmm0.f[0];g_eax=MEM32(g_esp+4);g_xmm4=g_xmm3;g_xmm4.f[0]-=g_xmm1.f[0];MEM32(g_esp+20)=g_ecx;MEMF(g_esp+8)=g_xmm4.f[0];g_edx=MEM32(g_esp+8);g_xmm1.f[0]+=g_xmm3.f[0];MEMF(g_esp+8)=g_xmm1.f[0];g_ecx=MEM32(g_esp+8);MEM32(g_esp+12)=g_eax;g_eax=MEM32(g_esi+72);MEM32(g_esp+16)=g_ecx;g_ecx=g_eax>>16;MEM32(g_esp+24)=g_edx;g_edx=ZX8(HI8(g_ecx));g_xmm0.f[0]=(float)(int32_t)g_edx;g_xmm0.f[0]*=MEMF(g_esi+56);g_edx=(int32_t)g_xmm0.f[0];PUSH32(g_esp,g_edx);g_edx=ZX8(LO8(g_eax));MEM32(g_esp+8)=g_eax;g_eax=ZX8(HI8(g_eax));PUSH32(g_esp,g_edx);g_ecx=ZX8(LO8(g_ecx));PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esp+56;PUSH32(g_esp,0x273a7f);sub_00087100();
 SET_LO8(g_eax,MEM8(g_esi+76));g_edx=g_esp+40;PUSH32(g_esp,0);
 if(LO8(g_eax)){g_xmm0=XMM_ZERO();g_xmm1=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,0);PUSH32(g_esp,g_edx);g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_ecx=g_esp+44;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+60);g_edx=g_esp+32;PUSH32(g_esp,g_edx);g_eax=g_esp+44;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);MEMF(g_esp+36)=g_xmm0.f[0];MEMF(g_esp+40)=MEMF(g_esp+60)=g_xmm1.f[0];MEMF(g_esp+64)=g_xmm0.f[0];PUSH32(g_esp,0x273acf);sub_00065550();g_esp+=32;}
 else{PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+60);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x273aed);sub_00065760();g_esp+=20;}
 POP32(g_esp,g_esi);g_esp+=32;g_recomp_zf=!g_esp;g_esp+=8;return 1;
}
