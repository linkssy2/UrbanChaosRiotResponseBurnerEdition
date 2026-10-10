#include "migration_interface_settings.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00016FB0(void),sub_000643E0(void),sub_00057FC0(void),sub_00087940(void),sub_00263230(void),sub_002633A0(void);
static int settings_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void settings_call(uint32_t target,unsigned ret,uint32_t saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void settings_apply(void){
 g_ecx=MEM32(0x5096e0);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x26323c);sub_00016FB0();g_eax=MEM32(0x51c4dc);g_esi=MEM32(g_eax+108);
 if(g_esi){PUSH32(g_esp,g_edi);const unsigned profile_ret[]={0x263254,0x263284,0x2632b4,0x2632e3,0x263313},call_ret[]={0x263274,0x2632a4,0x2632d4,0x263303,0x263333},names[]={0x450f68,0x450f88,0x450f80,0x450f70,0x487600};
  for(unsigned i=0;i<5;i++){
   if(i==1||i==4){g_edx=MEM32(0x51c4dc);g_esi=MEM32(g_edx+108);}else if(i==2){g_ecx=MEM32(0x51c4dc);g_esi=MEM32(g_ecx+108);}else if(i==3){g_eax=MEM32(0x51c4dc);g_esi=MEM32(g_eax+108);}g_edi=MEM32(g_esi);PUSH32(g_esp,profile_ret[i]);sub_000643E0();
   uint32_t value=(uint32_t)(int32_t)SMEM8(g_eax+56+(i<2?i:2));if(i==0||i==3)g_ecx=value;else if(i==2)g_edx=value;else g_eax=value;
   uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)value;g_xmm0.f[0]*=MEMF(0x44ff80);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,names[i]);g_ecx=g_esi;settings_call(MEM32(g_edi+72),call_ret[i],saved);
  }
  g_xmm0=XMM_SCALAR(MEMF(g_esp+12));double a=g_xmm0.f[0],b=MEMF(0x44c174);int below=a!=a||b!=b||a<b;POP32(g_esp,g_edi);
  if(!below){SET_LO8(g_eax,MEM8(g_esp+12));if(LO8(g_eax)){g_ecx=MEM32(0x51c4dc);g_ecx=MEM32(g_ecx+108);g_xmm0.f[0]*=MEMF(0x44ff80);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x4875f8);settings_call(MEM32(g_edx+72),0x26336c,saved);PUSH32(g_esp,0x4875f0);PUSH32(g_esp,0x263376);sub_00057FC0();g_esp+=4;}}
  PUSH32(g_esp,0x26337e);sub_000643E0();g_eax=(uint32_t)(int32_t)SMEM8(g_eax+59);g_xmm0.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]*=MEMF(0x44ff80);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x263399);sub_00087940();g_esp+=4;
 }
 POP32(g_esp,g_esi);g_esp+=4;
}
static void settings_display(void){g_eax=MEM32(0x5150b0);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax);PUSH32(g_esp,0x2633ad);sub_000643E0();g_ecx=MEM32(g_eax+64);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x2633b6);sub_000643E0();g_edx=MEM32(g_eax+60);g_ecx=MEM32(0x5150b0);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);settings_call(MEM32(g_esi+172),0x2633c6,saved);POP32(g_esp,g_esi);g_esp+=4;}
int migration_interface_settings(unsigned op){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_SETTINGS");enabled=s&&!strcmp(s,"1");}if(!enabled||op>3)return 0;uint32_t s=g_esp;if(s<0x10400||(s&3)||!settings_span(s-1024,1040)||!settings_span(0x515270,1024))return 0;
 if(op<2){uint32_t p=op?MEM32(s+4):MEM32(0x515278);if(!settings_span(p,op?412:1660)||(!op&&MEM32(s+4)>=4))return 0;}
 else{if(!settings_span(0x51c4dc,4))return 0;uint32_t p=MEM32(op==2?0x51c4dc:0x5150b0);if(!settings_span(p,op==2?112:4))return 0;p=op==2?MEM32(p+108):p;if(p&&(!settings_span(p,4)||!settings_span(MEM32(p),op==2?76:176)))return 0;}
 if(op==2){settings_apply();return 1;}if(op==3){settings_display();return 1;}
 if(!op){uint32_t ebp=g_seh_ebp;g_ecx=MEM32(g_esp+4);g_eax=MEM32(0x515278);g_ecx*=412;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx+g_eax+12;g_ecx=103;g_edi=0x5152b0;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),412);g_esi+=412;g_edi+=412;g_ecx=0;g_edx=MEM32(0x5152c8);MEM32(0x5152cc)=g_edx;g_eax=MEM32(g_eax+4);PUSH32(g_esp,1);PUSH32(g_esp,0xbf800000);MEM32(0x515270)=g_eax;MEM8(0x51527c)=1;PUSH32(g_esp,0x64278);sub_00263230();g_esp+=8;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_seh_ebp=ebp;sub_002633A0();return 1;}
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);PUSH32(g_esp,1);g_ecx=103;g_edi=0x5152b0;PUSH32(g_esp,0xbf800000);memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),412);g_esi+=412;g_edi+=412;g_ecx=0;PUSH32(g_esp,0x642ae);sub_00263230();g_esp+=8;PUSH32(g_esp,0x642b6);sub_002633A0();POP32(g_esp,g_edi);MEM8(0x51527c)=0;POP32(g_esp,g_esi);g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
