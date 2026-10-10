#include "migration_interface_transitions.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D5CCD(void);
static int transitions_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned transitions_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
int migration_interface_transitions(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_TRANSITIONS");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!transitions_span(s-1024,1032)||!transitions_span(g_ecx,12)||!transitions_span(0x44c174,4))return 0;
 unsigned head=MEM32(g_ecx+4);if(!transitions_span(head,12))return 0;unsigned p=MEM32(head),count=0;while(p!=head){if(++count>4096||!transitions_span(p,12)||!transitions_span(MEM32(p+4),12))return 0;unsigned child=MEM32(p+8);if(!transitions_span(child,32)||(op&&!transitions_span(MEM32(child),8)))return 0;p=MEM32(p);}
 if(!op){g_edx=MEM32(g_ecx+4);g_eax=MEM32(g_edx);if(g_eax!=g_edx){SET_LO8(g_edx,1);PUSH32(g_esp,g_esi);do{g_esi=MEM32(g_eax+8);MEM8(g_esi+28)=LO8(g_edx);g_eax=MEM32(g_eax);}while(g_eax!=MEM32(g_ecx+4));POP32(g_esp,g_esi);}g_esp+=4;return 1;}
 unsigned frame=g_seh_ebp;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+4);g_esi=MEM32(g_eax);
 if(g_esi!=g_eax){PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+20);do{
  g_ecx=MEM32(g_esi+8);SET_LO8(g_eax,MEM8(g_ecx+28));g_ebx=MEM32(g_esi);int expired=0;if(LO8(g_eax)){g_xmm0=XMM_ZERO();expired=!(transitions_cmp(g_xmm0.f[0],MEMF(g_ecx+8))&1);}g_eax=expired?1:0;g_xmm0.f[0]=(float)(int32_t)g_eax;SET_HI8(g_eax,transitions_cmp(g_xmm0.f[0],MEMF(0x44c174)));
  if(RECOMP_PARITY8(HI8(g_eax)&0x44)){if(g_ecx){g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,0x27436c);RECOMP_ICALL_SAFE(MEM32(g_eax),saved);}if(g_esi!=MEM32(g_edi+4)){g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);MEM32(g_ecx)=g_edx;g_eax=MEM32(g_esi);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_esi);MEM32(g_eax+4)=g_ecx;PUSH32(g_esp,0x274386);sub_002D5CCD();g_eax=MEM32(g_edi+8);g_esp+=4;g_eax--;MEM32(g_edi+8)=g_eax;}}
  else{g_edx=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,frame);PUSH32(g_esp,0x274398);RECOMP_ICALL_SAFE(MEM32(g_edx+4),saved);}g_esi=g_ebx;
 }while(g_ebx!=MEM32(g_edi+4));POP32(g_esp,frame);POP32(g_esp,g_ebx);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;return 1;
}
