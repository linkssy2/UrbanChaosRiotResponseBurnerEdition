/* Original endpoint-coordinate AABB reconstruction, including x87 rounding. */
#include "migration_physics_broadphase_bounds.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+12<=p);}
static void push_fp(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void pop_fp(void){g_fp_top=(g_fp_top+1)&7;}
#define TOP g_fp_stack[g_fp_top]
#define ST(i) g_fp_stack[(g_fp_top+(i))&7]
static void reconstruct(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-36;
 push_fp(MEMF(0x44bf94));g_eax=MEM32(frame+8);TOP/=MEMF(g_ecx+48);g_edx=MEM32(g_ecx+76);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM16(g_eax+8);g_esi=MEM16(g_edx+g_esi*4);MEM32(g_esp+20)=g_esi;g_esi=MEM32(g_ecx+88);PUSH32(g_esp,g_edi);g_edi=MEM16(g_eax);g_edi=MEM16(g_esi+g_edi*4);g_ebx=MEM16(g_eax+2);g_xmm2=XMM_MEM(g_ecx+16);MEM32(g_esp+44)=0;
 push_fp((double)SMEM32(g_esp+24));MEM32(g_esp+24)=g_edi;g_edi=MEM32(g_ecx+100);g_ebx=MEM16(g_edi+g_ebx*4);TOP*=ST(1);MEM32(g_esp+28)=g_ebx;MEMF(g_esp+32)=(float)TOP;pop_fp();
 push_fp(MEMF(0x44bf94));TOP/=MEMF(g_ecx+52);push_fp((double)SMEM32(g_esp+24));TOP*=ST(1);MEMF(g_esp+36)=(float)TOP;pop_fp();
 push_fp(MEMF(0x44bf94));TOP/=MEMF(g_ecx+56);g_ecx=MEM16(g_eax+10);g_edx=MEM16(g_edx+g_ecx*4);g_ecx=MEM16(g_eax+4);g_eax=MEM16(g_eax+6);MEMF(g_esp+24)=(float)TOP;pop_fp();
 push_fp((double)SMEM32(g_esp+28));MEM32(g_esp+28)=g_edx;g_edx=MEM16(g_esi+g_ecx*4);g_ecx=MEM16(g_edi+g_eax*4);TOP*=MEMF(g_esp+24);g_eax=MEM32(frame+12);MEMF(g_esp+40)=(float)TOP;pop_fp();g_xmm0=XMM_MEM(g_esp+32);
 push_fp((double)SMEM32(g_esp+28));MEM32(g_esp+28)=g_edx;POP32(g_esp,g_edi);MEM32(g_esp+40)=0;TOP*=ST(2);POP32(g_esp,g_esi);g_xmm0=XMM_SUB(g_xmm0,g_xmm2);XMM_STORE(g_eax,g_xmm0);MEMF(g_esp+24)=(float)TOP;pop_fp();POP32(g_esp,g_ebx);
 push_fp((double)SMEM32(g_esp+16));MEM32(g_esp+16)=g_ecx;TOP*=ST(1);MEMF(g_esp+24)=(float)TOP;pop_fp();pop_fp();pop_fp();push_fp((double)SMEM32(g_esp+16));TOP*=MEMF(g_esp+12);MEMF(g_esp+28)=(float)TOP;pop_fp();g_xmm1=XMM_MEM(g_esp+20);g_xmm1=XMM_SUB(g_xmm1,g_xmm2);XMM_STORE(g_eax+16,g_xmm1);
 g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_physics_broadphase_bounds(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_BOUNDS");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x3242d0||!span(g_esp-128,140)||!safe(g_ecx,104)||!span(0x44bf94,4))return 0;
 uint32_t record=MEM32(g_esp+4),output=MEM32(g_esp+8);if(!safe(record,12)||!safe(output,32))return 0;static const unsigned offsets[3][2]={{8,10},{0,4},{2,6}};
 for(unsigned axis=0;axis<3;axis++){uint32_t data=MEM32(g_ecx+76+axis*12);for(unsigned side=0;side<2;side++){uint64_t address=(uint64_t)data+MEM16(record+offsets[axis][side])*4ull;if(address>0xffffffff||!safe((uint32_t)address,2))return 0;}}
 reconstruct();return 1;
}
#undef TOP
#undef ST
