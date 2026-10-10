#include "migration_animation_face_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
extern void sub_002D3DC0(void),sub_00011000(void);
/* Keep the guest destination NaN payload when both operands are NaNs.
 * C's commutative lowering otherwise changes payload selection in local SIMD
 * temporaries. Execute arithmetic first to retain floating exception flags. */
static RecompXmm face_ordered_add(RecompXmm a,RecompXmm b){RecompXmm v=XMM_ADD(a,b);for(unsigned i=0;i<4;i++)if((a.u[i]&0x7fffffffu)>0x7f800000u&&(b.u[i]&0x7fffffffu)>0x7f800000u)v.u[i]=a.u[i]|0x400000u;return v;}
static RecompXmm face_ordered_mul(RecompXmm a,RecompXmm b){RecompXmm v=XMM_MUL(a,b);for(unsigned i=0;i<4;i++)if((a.u[i]&0x7fffffffu)>0x7f800000u&&(b.u[i]&0x7fffffffu)>0x7f800000u)v.u[i]=a.u[i]|0x400000u;return v;}
static int face_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int face_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Private three-word copy keeps the original read-before-write alias behavior. */
static void face_vector(void){g_eax=MEM32(g_esp+8);g_xmm0=XMM_SCALAR(MEMF(g_eax+8));g_xmm1=XMM_SCALAR(MEMF(g_eax+4));g_ecx=MEM32(g_eax);g_eax=MEM32(g_esp+4);MEM32(g_eax)=g_ecx;MEMF(g_eax+4)=g_xmm1.f[0];MEMF(g_eax+8)=g_xmm0.f[0];g_esp+=4;}
int uc_migration_animation_face(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_FACE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,node=g_ecx;if(!enabled||(_mm_getcsr()&0x1f80)!=0x1f80||(s&3)||s<0x10800||!face_span(s-2048,2068)||!face_span(node,20)||!face_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t n=MEM32(node+16),out=MEM32(s+8),highest=0;int active=0;
 if(n>4096||!face_span(node,20+n*28)||!face_apart(node,20+n*28,s-2048,2068))return 0;
 for(unsigned i=0;i<n;i++){uint32_t bone=MEM32(node+20+i*28);if(bone==0xffffffffu)continue;if(bone>=4096)return 0;active=1;if(bone>highest)highest=bone;}
 if(active){unsigned bytes=(highest+1)*64;if(!face_span(out,bytes)||!face_apart(out,bytes,node,20+n*28)||!face_apart(out,bytes,s-2048,2068)||!face_span(0x51e110,64)||!face_apart(out,bytes,0x51e110,64)||!face_span(0x44bf94,4)||!face_apart(out,bytes,0x44bf94,4))return 0;}
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-228;g_eax=g_ecx;g_ecx=MEM32(g_eax+16);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(g_esp+20)=g_eax;MEM32(g_esp+16)=0;
 if(g_ecx){
  g_ebx=g_eax+20;
  do{
   g_ecx=7;g_esi=g_ebx;g_edi=g_esp+132;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),28);g_esi+=28;g_edi+=28;g_ecx=0;
   if(MEM32(g_esp+132)!=0xffffffffu){
    g_eax=g_esp+136;PUSH32(g_esp,g_eax);g_ecx=g_esp+40;PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x2b6340);face_vector();
    g_edx=g_esp+156;PUSH32(g_esp,g_edx);g_eax=g_esp+36;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x2b6352);face_vector();
    g_xmm0=XMM_SCALAR(MEMF(g_esp+40));g_edx=MEM32(g_esp+60);g_eax=MEM32(g_esp+56);g_esp+=16;MEMF(g_esp+48)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+28));g_ecx=g_esp+160;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+40);MEMF(g_esp+56)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+36));PUSH32(g_esp,g_edx);MEMF(g_esp+64)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_eax);MEMF(g_esp+72)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+60);PUSH32(g_esp,g_ecx);g_ecx=g_esp+192;XMM_STORE(g_esp+176,g_xmm0);g_ebp=frame;PUSH32(g_esp,0x2b63b7);sub_002D3DC0();
    PUSH32(g_esp,0x1f4b10);PUSH32(g_esp,4);PUSH32(g_esp,16);g_edx=g_esp+76;PUSH32(g_esp,g_edx);g_ebp=frame;PUSH32(g_esp,0x2b63ca);sub_00011000();
    g_xmm0=XMM_MEM(0x51e110);g_eax=MEM32(g_esp+132);g_ecx=MEM32(frame+12);XMM_STORE(g_esp+64,g_xmm0);
    for(unsigned row=1;row<4;row++){g_xmm0=XMM_MEM(0x51e110+row*16);XMM_STORE(g_esp+64+row*16,g_xmm0);}
    g_eax=(g_eax<<6)+g_ecx;
    /* Three matrix rows use the original pairwise addition order. Keep the
     * final scratch registers because the next constructor can observe them. */
    RecompXmm first=XMM_ZERO();
    for(unsigned row=0;row<3;row++){
     RecompXmm rotation=XMM_MEM(g_esp+176+row*16);
     RecompXmm a=face_ordered_mul(XMM_MEM(g_eax),XMM_SHUFFLE(rotation,rotation,0));
     RecompXmm b=face_ordered_mul(XMM_MEM(g_eax+16),XMM_SHUFFLE(rotation,rotation,0x55));
     RecompXmm c=face_ordered_mul(XMM_MEM(g_eax+32),XMM_SHUFFLE(rotation,rotation,0xaa));
     RecompXmm d=face_ordered_mul(XMM_MEM(g_eax+48),XMM_SHUFFLE(rotation,rotation,0xff));
     RecompXmm result=face_ordered_add(face_ordered_add(a,b),face_ordered_add(c,d));XMM_STORE(g_esp+64+row*16,result);
     if(!row)first=result;
     if(row==2){g_xmm2=XMM_SHUFFLE(rotation,rotation,0);g_xmm3=b;g_xmm4=face_ordered_add(c,d);}
    }
    g_xmm1=first;
    for(unsigned axis=0;axis<3;axis++){g_xmm0=XMM_SCALAR(MEMF(g_eax+48+axis*4));g_xmm0.f[0]+=MEMF(g_esp+224+axis*4);MEMF(g_esp+112+axis*4)=g_xmm0.f[0];}
    XMM_STORE(g_eax,g_xmm1);for(unsigned row=1;row<4;row++){g_xmm0=XMM_MEM(g_esp+64+row*16);XMM_STORE(g_eax+row*16,g_xmm0);}g_eax=MEM32(g_esp+20);
   }
   g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_eax+16);++g_ecx;g_ebx+=28;MEM32(g_esp+16)=g_ecx;
  }while(g_ecx<g_edx);
 }
 g_esi=MEM32(g_eax+16);g_edx=0;POP32(g_esp,g_edi);g_eax=g_esi!=0;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_eax==0;g_esp+=20;return 1;
}
