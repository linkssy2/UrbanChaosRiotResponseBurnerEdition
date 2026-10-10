/* Original rigid-body construction defaults and live motion descriptor export. */
#include "migration_physics_body_descriptor.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002EC470(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+32<=p);}
/* Clone callers construct descriptors above their argument/return slots. */
static int destination(uint32_t p,unsigned arguments){return span(p,181)&&((uint64_t)p+181<=g_esp-128||(uint64_t)g_esp+arguments<=p);}
static void defaults(void){
 g_eax=g_ecx;g_xmm0=XMM_ZERO();XMM_STORE(g_eax+16,g_xmm0);XMM_STORE(g_eax+32,g_xmm0);g_ecx=0x3f800000;MEM32(g_eax+44)=g_ecx;for(unsigned o=48;o<=112;o+=16)XMM_STORE(g_eax+o,g_xmm0);MEM32(g_eax+80)=MEM32(g_eax+100)=MEM32(g_eax+120)=g_ecx;g_edx=0x43480000;MEM32(g_eax+144)=g_ecx;g_ecx=0;MEM32(g_eax+164)=MEM32(g_eax+168)=g_edx;SET_LO8(g_edx,2);XMM_STORE(g_eax+128,g_xmm0);MEM32(g_eax+148)=0;MEM32(g_eax+152)=0x3d4ccccd;MEM32(g_eax+156)=0x3f000000;MEM32(g_eax+160)=0x3ecccccd;MEM32(g_eax+172)=0xbf800000;MEM8(g_eax+176)=1;MEM8(g_eax+177)=MEM8(g_eax+178)=(uint8_t)g_edx;MEM8(g_eax+179)=0;MEM8(g_eax+8)=1;MEM16(g_eax+10)=0xffff;MEM32(g_eax)=MEM32(g_eax+4)=0;MEM8(g_eax+180)=0;g_recomp_zf=1;g_esp+=4;
}
static void export_descriptor(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+144));PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);MEM8(g_edi+180)=(uint8_t)g_eax;SET_LO16(g_ecx,MEM16(g_esi+142));MEM16(g_edi+10)=(uint16_t)g_ecx;g_ecx=MEM32(g_esi+100);int no_shape=g_ecx==0;uint32_t saved,target;
 if(g_ecx){g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+16);PUSH32(g_esp,0x2f1ebb);RECOMP_ICALL_SAFE(target,saved);}else g_eax=1;
 MEM8(g_edi+177)=(uint8_t)g_eax;g_eax=MEM32(g_esi+92);MEM32(g_edi+156)=g_eax;SET_LO8(g_ecx,MEM8(g_esi+88));MEM8(g_edi+8)=(uint8_t)g_ecx;g_edx=MEM32(g_esi+96);MEM32(g_edi+160)=g_edx;g_eax=MEM32(g_esi+80);g_ecx=MEM32(g_eax+200);MEM32(g_edi+148)=g_ecx;g_edx=MEM32(g_esi+80);g_eax=MEM32(g_edx+204);MEM32(g_edi+152)=g_eax;g_ecx=MEM32(g_esi+80);g_xmm0=XMM_MEM(g_ecx+208);XMM_STORE(g_edi+48,g_xmm0);g_edx=MEM32(g_esi+80);g_xmm0=XMM_MEM(g_edx+224);XMM_STORE(g_edi+64,g_xmm0);g_ecx=MEM32(g_esi+80);PUSH32(g_esp,0x2f1f22);sub_002EC470();MEMF(g_edi+144)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 g_ecx=MEM32(g_esi+80);g_eax=MEM32(g_ecx);g_edx=g_edi+80;saved=g_esp;PUSH32(g_esp,g_edx);target=MEM32(g_eax+40);PUSH32(g_esp,0x2f1f34);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+80);g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+8);PUSH32(g_esp,0x2f1f3c);RECOMP_ICALL_SAFE(target,saved);
 MEM8(g_edi+176)=(uint8_t)g_eax;g_ecx=MEM32(g_esi+80);SET_LO8(g_edx,MEM8(g_ecx+188));MEM8(g_edi+178)=(uint8_t)g_edx;g_eax=MEM32(g_esi+80);g_ecx=MEM32(g_eax+180);MEM32(g_edi+164)=g_ecx;g_edx=MEM32(g_esi+80);g_eax=MEM32(g_edx+184);MEM32(g_edi+168)=g_eax;g_ecx=MEM32(g_esi+80);g_xmm0=XMM_MEM(g_ecx+64);XMM_STORE(g_edi+16,g_xmm0);g_edx=MEM32(g_esi+80);g_xmm0=XMM_MEM(g_edx+128);XMM_STORE(g_edi+32,g_xmm0);g_eax=MEM32(g_esi+80);g_xmm0=XMM_MEM(g_eax+144);XMM_STORE(g_edi+128,g_xmm0);g_ecx=MEM32(g_esi+20);MEM32(g_edi+4)=g_ecx;g_edx=MEM32(g_esi+48);MEM32(g_edi)=g_edx;g_eax=MEM32(g_esi+52);MEM32(g_edi+172)=g_eax;SET_LO8(g_ecx,MEM8(g_esi+46));MEM8(g_edi+179)=(uint8_t)g_ecx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=no_shape;g_esp+=8;
}
int uc_migration_physics_body_descriptor(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_DESCRIPTOR");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-128,160))return 0;if(entry==0x2f2f90){if(!destination(g_ecx,4))return 0;defaults();return 1;}if(entry!=0x2f1e90||!safe(g_ecx,145)||!destination(MEM32(g_esp+4),8))return 0;uint32_t motion=MEM32(g_ecx+80),shape=MEM32(g_ecx+100);if(!safe(motion,240)||!safe(MEM32(motion),44)||(shape&&(!safe(shape,4)||!safe(MEM32(shape),20))))return 0;export_descriptor();return 1;
}
