#include "hybrid_bullet_tracer_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_00069F30(void),sub_00069F50(void),sub_0006A3E0(void),sub_0006A450(void);
extern void sub_0006AB90(void),sub_00214F40(void),sub_0022B210(void),sub_002E0E00(void);
extern void sub_00069E80(void),sub_002A7B50(void),sub_002D57FB(void),sub_002D5CCD(void);
static int tracer_owner_enabled(void){static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_BULLET_TRACER_OWNER");enabled=p&&!strcmp(p,"1");}return enabled;}
static int tracer_owner_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int tracer_owner_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define TRACER_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define TRACER_VCALL(target,pc,saved) do{uint32_t tracer_target=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(tracer_target,saved);}while(0)

static void tracer_owner_construct(void){
 uint32_t saved=g_esp;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38b513);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=12;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;g_edx=0;g_eax=g_esi+4;MEM32(g_esp+0x1c)=g_edx;
 MEM32(g_eax)=0x4527c8;g_ecx=MEM32(g_esp+0x24);MEM32(g_esi)=0x452824;MEM32(g_eax)=0x452808;MEM32(g_esi+12)=g_ecx;
 PUSH32(g_esp,0x3f800000);MEM32(g_esi+16)=g_edx;g_ecx=MEM32(0x5150b0);g_edi=MEM32(g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);MEM8(g_esp+0x28)=1;
 TRACER_VCALL(MEM32(g_edi+0x90),0x69edf,saved);
 MEM32(g_esi+8)=g_eax;g_edx=MEM32(0x5150b0);g_edi=MEM32(g_edx);TRACER_CALL(sub_0022B210,0x69eef);g_ecx=g_eax;TRACER_CALL(sub_002E0E00,0x69ef6);
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4527e4);g_ecx=g_esp+0x14;TRACER_CALL(sub_002A7B50,0x69f05);g_ecx=MEM32(0x5150b0);
 saved=g_esp;PUSH32(g_esp,g_eax);TRACER_VCALL(MEM32(g_edi+0x98),0x69f12,saved);
 g_ecx=MEM32(g_esp+0x14);MEM32(g_esi+0x420)=g_eax;POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=0x18;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void tracer_owner_destroy(void){
 uint32_t saved=g_esp;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38b54f);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+8)=g_esi;MEM32(g_esi)=0x452824;MEM32(g_esi+4)=0x452808;
 g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);MEM32(g_esp+0x18)=1;
 TRACER_VCALL(MEM32(g_eax+0x94),0x69f96,saved);
 g_ecx=MEM32(g_esp+12);MEM32(g_esi+4)=0x4527c8;MEM32(g_esi)=0x4527bc;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=0x14;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void tracer_owner_erase(void){
 g_eax=MEM32(g_ecx);g_edx=MEM32(g_esp+4);g_edx<<=6;g_eax--;MEM32(g_ecx)=g_eax;PUSH32(g_esp,g_esi);g_eax<<=6;g_eax+=g_ecx+16;g_ecx+=g_edx+16;PUSH32(g_esp,g_edi);
 g_edx=g_eax;g_edi=MEM32(g_edx);g_esi=g_ecx;MEM32(g_esi)=g_edi;g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edx=MEM32(g_edx+8);MEM32(g_esi+8)=g_edx;
 g_edx=g_eax+12;g_edi=MEM32(g_edx);g_esi=g_ecx+12;MEM32(g_esi)=g_edi;g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edx=MEM32(g_edx+8);MEM32(g_esi+8)=g_edx;
 g_xmm0=XMM_MEM(g_eax+0x20);XMM_STORE(g_ecx+0x20,g_xmm0);g_edx=MEM32(g_eax+0x30);MEM32(g_ecx+0x30)=g_edx;g_edx=MEM32(g_eax+0x34);MEM32(g_ecx+0x34)=g_edx;g_eax=MEM32(g_eax+0x38);POP32(g_esp,g_edi);MEM32(g_ecx+0x38)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;
}
static void tracer_owner_step(void){
 g_eax=MEM32(g_ecx+16);PUSH32(g_esp,g_ebx);g_ebx=g_ecx+16;PUSH32(g_esp,g_esi);g_esi=0;
 if(g_eax){
  PUSH32(g_esp,g_edi);g_edi=g_ecx+0x4c;
  do{
   g_xmm0=XMM_SCALAR(MEMF(g_edi));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c47c);MEMF(g_edi)=g_xmm0.f[0];
   g_xmm0=XMM_SCALAR(MEMF(g_edi+8));g_xmm0.f[0]=g_xmm0.f[0]-MEMF(0x44c1e0);MEMF(g_edi+8)=g_xmm0.f[0];g_xmm0=XMM_ZERO();
   double a=g_xmm0.f[0],b=MEMF(g_edi+8);unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
   int remove=!(flags&1u);
   if(!remove){g_xmm0=XMM_SCALAR(MEMF(0x44c538));a=g_xmm0.f[0];b=MEMF(g_edi);flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;remove=!(flags&0x41u);}
   if(remove){PUSH32(g_esp,g_esi);g_ecx=g_ebx;TRACER_CALL(sub_0006A450,0x6a440);}
   g_eax=MEM32(g_ebx);g_esi++;g_edi+=0x40;
  }while(g_esi<g_eax);
  POP32(g_esp,g_edi);
 }
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=4;
}
static void tracer_owner_append(void);
static void tracer_owner_copy(void){
 g_eax=g_ecx;g_ecx=MEM32(g_esp+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edx=g_ecx;g_edi=MEM32(g_edx);g_esi=g_eax;MEM32(g_esi)=g_edi;g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edx=MEM32(g_edx+8);MEM32(g_esi+8)=g_edx;
 g_edx=g_ecx+12;g_edi=MEM32(g_edx);g_esi=g_eax+12;MEM32(g_esi)=g_edi;g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edx=MEM32(g_edx+8);MEM32(g_esi+8)=g_edx;
 g_xmm0=XMM_MEM(g_ecx+0x20);XMM_STORE(g_eax+0x20,g_xmm0);g_edx=MEM32(g_ecx+0x30);MEM32(g_eax+0x30)=g_edx;g_edx=MEM32(g_ecx+0x34);MEM32(g_eax+0x34)=g_edx;g_ecx=MEM32(g_ecx+0x38);
 POP32(g_esp,g_edi);MEM32(g_eax+0x38)=g_ecx;POP32(g_esp,g_esi);g_esp+=8;
}
static void tracer_owner_factory(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38b56b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x430);
 TRACER_CALL(sub_002D57FB,0x6ac10);g_esp+=4;MEM32(g_esp)=g_eax;uint32_t allocated=g_eax;MEM32(g_esp+12)=0;
 if(allocated){g_ecx=MEM32(g_esp+0x14);PUSH32(g_esp,g_ecx);g_ecx=g_eax;TRACER_CALL(sub_00069E80,0x6ac2e);}else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=0x10;g_recomp_zf=g_esp==0;g_esp+=4;
}

int uc_hybrid_bullet_tracer_owner(uint32_t entry){
 if(!tracer_owner_enabled())return 0;
 switch(entry){case 0x69e80:case 0x69f30:case 0x69f50:case 0x69fc0:case 0x6a3c0:case 0x6a3e0:case 0x6a450:case 0x6a4c0:case 0x6ab90:case 0x6abf0:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;
 if((s&3)||s<0x12000u||!tracer_owner_span(s-8192,8224)||!tracer_owner_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||!tracer_owner_apart(s-8192,8224,0x51c178,1))return 0;
 if(entry==0x6abf0){tracer_owner_factory();return 1;}
 if(entry==0x6ab90){if(!tracer_owner_span(p,60)||!tracer_owner_span(MEM32(s+4),60)||!tracer_owner_apart(p,60,0x51c178,1))return 0;tracer_owner_copy();return 1;}
 if(entry==0x69fc0)p-=4;
 unsigned bytes=entry==0x6a450?0x410:0x430;
 if(!tracer_owner_span(p,bytes)||!tracer_owner_apart(p,bytes,s-8192,8224)||!tracer_owner_apart(p,bytes,0x51c178,1))return 0;
 if(entry==0x6a3c0||entry==0x6a3e0||entry==0x6a4c0){if(MEM32(p+16)>16)return 0;}
 if(entry==0x6a450){uint32_t count=MEM32(p);if(!count||count>16||MEM32(s+4)>=count)return 0;}
 if(entry==0x6a4c0&&(!tracer_owner_span(MEM32(s+4),12)||!tracer_owner_span(MEM32(s+8),12)||!tracer_owner_span(MEM32(s+12),16)))return 0;
 if(entry==0x69e80){tracer_owner_construct();return 1;}
 if(entry==0x69f50){tracer_owner_destroy();return 1;}
 if(entry==0x69fc0){g_ecx-=4;g_seh_ebp=frame;sub_00069F30();return 1;}
 if(entry==0x69f30){PUSH32(g_esp,g_esi);g_esi=g_ecx;TRACER_CALL(sub_00069F50,0x69f38);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);TRACER_CALL(sub_002D5CCD,0x69f45);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 if(entry==0x6a3c0){g_xmm0=XMM_SCALAR(MEMF(g_esp+4));double a=g_xmm0.f[0],b=MEMF(0x44c174);unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;if(!(flags&0x41u))TRACER_CALL(sub_0006A3E0,0x6a3d4);g_recomp_zf=!!(flags&0x40u);g_esp+=8;return 1;}
 if(entry==0x6a3e0){tracer_owner_step();return 1;}
 if(entry==0x6a450){tracer_owner_erase();return 1;}
 tracer_owner_append();return 1;
}

/* Append preserves the original mixed SSE/virtual-x87 instruction schedule,
 * including the full-capacity audio path and ordered alias-sensitive stores.
 * Only unused lift bookkeeping is removed; callbacks remain public services. */
static void tracer_owner_append(void)
{
    uint32_t frame = g_ebp;
    uint32_t _sse_flags = 0x02u;
    uint32_t _incdec_result = 0;
    uint32_t _fa = 0, _fb = 0;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    g_esp = g_esp - 0xA4;
    PUSH32(g_esp, g_ebx);
    PUSH32(g_esp, g_esi);
    g_esi = MEM32(frame + 8);
    PUSH32(g_esp, g_edi);
    g_edi = MEM32(frame + 0xC);
    g_xmm0 = XMM_SCALAR(MEMF(g_edi));
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = MEMF(g_esi);
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    SET_HI8(g_eax, _sse_flags);
    _fa = (uint32_t)(HI8(g_eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    g_ebx = g_ecx;
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_0006A506;
    g_xmm0 = XMM_SCALAR(MEMF(g_edi + 4));
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = MEMF(g_esi + 4);
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    SET_HI8(g_eax, _sse_flags);
    _fa = (uint32_t)(HI8(g_eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_0006A506;
    g_xmm0 = XMM_SCALAR(MEMF(g_edi + 8));
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = MEMF(g_esi + 8);
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    SET_HI8(g_eax, _sse_flags);
    _fa = (uint32_t)(HI8(g_eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_0006AB86;
loc_0006A506: ;
    fp_push(MEMF(g_esi));
    g_xmm0 = XMM_SCALAR(MEMF(g_esi));
    fp_top() = fp_top() - MEMF(g_edi);
    g_eax = MEM32(frame + 0x10);
    fp_push(MEMF(g_esi + 4));
    MEMF(g_esp + 0x70) = g_xmm0.f[0];
    fp_top() = fp_top() - MEMF(g_edi + 4);
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 4));
    fp_push(MEMF(g_esi + 8));
    MEMF(g_esp + 0x74) = g_xmm0.f[0];
    fp_top() = fp_top() - MEMF(g_edi + 8);
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 8));
    MEMF(g_esp + 0x78) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edi));
    { double _t = g_fp_stack[(g_fp_top + 2) & 7]; fp_push(_t); }
    MEMF(g_esp + 0x7C) = g_xmm0.f[0];
    g_fp_stack[(g_fp_top + 3) & 7] = g_fp_stack[(g_fp_top + 3) & 7] * fp_top(); fp_pop();
    g_xmm0 = XMM_SCALAR(MEMF(g_edi + 4));
    { double _t = fp_top(); fp_push(_t); }
    MEMF(g_esp + 0x80) = g_xmm0.f[0];
    fp_top() = fp_top() * fp_st1();
    g_xmm0 = XMM_SCALAR(MEMF(g_edi + 8));
    MEMF(g_esp + 0x84) = g_xmm0.f[0];
    g_fp_stack[(g_fp_top + 3) & 7] = g_fp_stack[(g_fp_top + 3) & 7] + fp_top(); fp_pop();
    { double _t = fp_st1(); fp_push(_t); }
    g_xmm0 = XMM_MEM(g_eax);
    fp_top() = fp_top() * g_fp_stack[(g_fp_top + 2) & 7];
    g_eax = MEM32(g_ebx + 0x10);
    _fa = (uint32_t)(g_eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10) & 0xFFFFFFFFu;
    XMM_STORE(g_esp + 0x90, g_xmm0);
    g_fp_stack[(g_fp_top + 3) & 7] = g_fp_stack[(g_fp_top + 3) & 7] + fp_top(); fp_pop();
    { double _t = fp_top(); fp_top() = g_fp_stack[(g_fp_top + 2) & 7]; g_fp_stack[(g_fp_top + 2) & 7] = _t; }
    g_xmm0 = XMM_SCALAR(MEMF(frame + 0x14));
    MEMF(g_esp + 0xA0) = g_xmm0.f[0];
    fp_top() = sqrt(fp_top());
    g_fp_stack[(g_fp_top + 2) & 7] = fp_top(); fp_pop();
    fp_pop();
    MEMF(g_esp + 0xA4) = (float)fp_top();
    MEMF(g_esp + 0xA8) = (float)fp_top(); fp_pop();
    if (CMP_EQ(_fa, _fb)) goto loc_0006A5BC;
    g_edx = g_eax;
    g_ecx = g_esp + 0x70;
    g_edx = g_edx << 6;
    PUSH32(g_esp, g_ecx);
    g_ecx = g_edx + g_ebx + 0x20;
    g_ebp = frame;
    PUSH32(g_esp, 0x0006A5B9u); sub_0006AB90();
    _incdec_result = (uint32_t)((uint32_t)(MEM32(g_ebx + 0x10)) + 1u);
    MEM32(g_ebx + 0x10) = _incdec_result;
loc_0006A5BC: ;
    g_ecx = MEM32(0x51C4DC);
    g_eax = MEM32(g_ecx + 0x6C);
    _fa = (uint32_t)(g_eax) & 0xFFFFFFFFu; _fb = (uint32_t)(g_eax) & 0xFFFFFFFFu;
    if (TEST_Z(_fa, _fb)) goto loc_0006AB86;
    PUSH32(g_esp, 0);
    g_ebp = frame;
    PUSH32(g_esp, 0x0006A5D4u); sub_00214F40();
    g_xmm4 = XMM_SCALAR(MEMF(g_eax + 0x40));
    g_xmm7 = XMM_SCALAR(MEMF(g_eax + 0x48));
    g_xmm1 = XMM_SCALAR(MEMF(g_eax + 0x44));
    g_eax = g_eax + 0x40;
    g_xmm0 = g_xmm4;
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esi);
    g_xmm3 = g_xmm0;
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0];
    MEMF(g_esp + 0x64) = g_xmm0.f[0];
    g_xmm2 = g_xmm7;
    g_xmm2.f[0] = g_xmm2.f[0] - MEMF(g_esi + 8);
    g_xmm0 = g_xmm2;
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm2.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + g_xmm0.f[0];
    MEMF(g_esp + 0x14) = g_xmm1.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esi + 4);
    g_xmm0 = g_xmm1;
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44DF68));
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = g_xmm3.f[0];
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    MEMF(g_esp + 0x10) = g_xmm4.f[0];
    MEMF(g_esp + 0x18) = g_xmm7.f[0];
    MEMF(g_esp + 0x68) = g_xmm1.f[0];
    MEMF(g_esp + 0x6C) = g_xmm2.f[0];
    if (((_sse_flags & 0x41u) == 0)) goto loc_0006AB86;
    g_xmm0 = XMM_SCALAR(MEMF(g_edi));
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esi);
    g_xmm1 = XMM_SCALAR(MEMF(g_edi + 4));
    g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esi + 4);
    g_xmm2 = XMM_SCALAR(MEMF(g_edi + 8));
    g_xmm2.f[0] = g_xmm2.f[0] - MEMF(g_esi + 8);
    g_xmm3 = g_xmm0;
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0];
    MEMF(g_esp + 0x5C) = g_xmm3.f[0];
    g_xmm6 = g_xmm1;
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm1.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + g_xmm6.f[0];
    MEMF(g_esp + 0x58) = g_xmm6.f[0];
    g_xmm6 = XMM_SCALAR(MEMF(g_esi));
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm4.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm0.f[0];
    g_xmm5 = g_xmm2;
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm2.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + g_xmm5.f[0];
    MEMF(g_esp + 0x54) = g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(0x44C530));
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm5.f[0];
    MEMF(g_esp + 0x2C) = g_xmm3.f[0];
    g_xmm3 = XMM_SCALAR(MEMF(g_esi + 4));
    g_xmm3.f[0] = g_xmm3.f[0] - MEMF(g_esp + 0x14);
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm5.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + g_xmm6.f[0];
    g_xmm6 = XMM_SCALAR(MEMF(g_esi + 8));
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm7.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm2.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm5.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + g_xmm6.f[0];
    g_xmm6 = XMM_ZERO();
    g_xmm5 = g_xmm3;
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm3.f[0];
    MEMF(g_esp + 0x1C) = g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esi));
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm4.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esi + 4));
    g_xmm5.f[0] = g_xmm5.f[0] * MEMF(g_esp + 0x14);
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esi + 8));
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm7.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm5.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] * MEMF(0x44C530);
    g_xmm5 = XMM_SCALAR(MEMF(g_esi + 8));
    g_xmm7 = g_xmm5;
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esi + 4));
    g_xmm6.f[0] = g_xmm6.f[0] + g_xmm7.f[0];
    g_xmm7 = g_xmm5;
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esi));
    g_xmm6.f[0] = g_xmm6.f[0] + g_xmm7.f[0];
    g_xmm7 = g_xmm5;
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x18));
    g_xmm6.f[0] = g_xmm6.f[0] + g_xmm7.f[0];
    g_xmm7 = g_xmm5;
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x14));
    g_xmm6.f[0] = g_xmm6.f[0] + g_xmm7.f[0];
    g_xmm7 = g_xmm5;
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm5.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] + g_xmm7.f[0];
    g_xmm5 = g_xmm4;
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm4.f[0];
    g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x1C));
    g_xmm6.f[0] = g_xmm6.f[0] + g_xmm5.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] - MEMF(0x44DF68);
    g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x2C));
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm5.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] * MEMF(0x44EC38);
    g_xmm4.f[0] = g_xmm4.f[0] - g_xmm6.f[0];
    g_xmm6 = XMM_ZERO();
    MEMF(g_esp + 0x50) = g_xmm0.f[0];
    MEMF(g_esp + 0x4C) = g_xmm1.f[0];
    MEMF(g_esp + 0x60) = g_xmm2.f[0];
    MEMF(g_esp + 0x1C) = g_xmm4.f[0];
    { double _sse_a = g_xmm6.f[0];
      double _sse_b = g_xmm4.f[0];
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    if (((_sse_flags & 0x41u) == 0)) goto loc_0006AB86;
    fp_push(MEMF(g_esp + 0x1C));
    g_xmm5.f[0] = g_xmm5.f[0] * MEMF(0x44C530);
    fp_top() = sqrt(fp_top());
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm3.f[0];
    MEMF(g_esp + 0x1C) = (float)fp_top(); fp_pop();
    g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x1C));
    g_xmm4 = g_xmm7;
    g_xmm4.f[0] = g_xmm4.f[0] - g_xmm3.f[0];
    g_xmm3 = XMM_SCALAR(MEMF(0x44BF94));
    g_xmm4.f[0] = g_xmm4.f[0] / g_xmm5.f[0];
    { double _sse_a = g_xmm4.f[0];
      double _sse_b = g_xmm3.f[0];
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm7.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] / g_xmm5.f[0];
    MEMF(g_esp + 0x30) = g_xmm4.f[0];
    MEMF(g_esp + 0x1C) = g_xmm6.f[0];
    if (((_sse_flags & 0x41u) == 0)) goto loc_0006AB86;
    { double _sse_a = g_xmm6.f[0];
      double _sse_b = g_xmm3.f[0];
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    if (((_sse_flags & 0x41u) == 0)) goto loc_0006AB86;
    g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x6C));
    g_xmm3 = g_xmm4;
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + MEMF(g_esi + 4);
    MEMF(g_esp + 0x44) = g_xmm3.f[0];
    g_xmm3 = g_xmm4;
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm2.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + MEMF(g_esi + 8);
    MEMF(g_esp + 0x48) = g_xmm3.f[0];
    g_xmm3 = g_xmm6;
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + MEMF(g_esi);
    MEMF(g_esp + 0x34) = g_xmm3.f[0];
    g_xmm3 = g_xmm6;
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + MEMF(g_esi + 4);
    MEMF(g_esp + 0x38) = g_xmm3.f[0];
    g_xmm3 = g_xmm6;
    g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x64));
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm2.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + MEMF(g_esi + 8);
    MEMF(g_esp + 0x3C) = g_xmm3.f[0];
    g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x68));
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    MEMF(g_esp + 0x24) = g_xmm3.f[0];
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm0.f[0];
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm2.f[0];
    g_xmm3 = g_xmm7;
    g_xmm3.f[0] = g_xmm3.f[0] + MEMF(g_esp + 0x24);
    g_xmm3.f[0] = g_xmm3.f[0] + g_xmm6.f[0];
    g_xmm6 = XMM_SCALAR(MEMF(0x44BF94));
    g_xmm6.f[0] = g_xmm6.f[0] / MEMF(g_esp + 0x2C);
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm6.f[0];
    g_xmm6 = g_xmm3;
    g_xmm6.f[0] = g_xmm6.f[0] * g_xmm0.f[0];
    g_xmm5 = g_xmm4;
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi));
    g_xmm7 = g_xmm3;
    g_xmm0.f[0] = g_xmm0.f[0] + g_xmm6.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x10);
    MEMF(g_esp + 0x20) = g_xmm0.f[0];
    fp_push(MEMF(g_esp + 0x20));
    fp_top() = fp_top() * MEMF(g_esp + 0x20);
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm2.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] + MEMF(g_esi + 8);
    g_xmm3.f[0] = g_xmm3.f[0] - MEMF(g_esp + 0x18);
    MEMF(g_esp + 0x28) = g_xmm3.f[0];
    fp_push(MEMF(g_esp + 0x28));
    fp_top() = fp_top() * MEMF(g_esp + 0x28);
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esi + 4));
    g_xmm1.f[0] = g_xmm1.f[0] + g_xmm7.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x14);
    MEMF(g_esp + 0x24) = g_xmm1.f[0];
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_push(MEMF(g_esp + 0x24));
    g_xmm5.f[0] = g_xmm5.f[0] + MEMF(g_esi);
    fp_top() = fp_top() * MEMF(g_esp + 0x24);
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x1C));
    MEMF(g_esp + 0x40) = g_xmm5.f[0];
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_top() = sqrt(fp_top());
    fp_top() = fp_top() * MEMF(0x452848);
    fp_top() = MEMF(0x44BF94) - fp_top();
    MEMF(g_esp + 0x2C) = (float)fp_top(); fp_pop();
    fp_push(MEMF(g_esp + 0x5C));
    fp_top() = fp_top() + MEMF(g_esp + 0x58);
    fp_top() = fp_top() + MEMF(g_esp + 0x54);
    fp_top() = sqrt(fp_top());
    fp_top() = MEMF(0x452844) / fp_top();
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = g_xmm4.f[0];
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x2C));
    if (((_sse_flags & 0x41u) != 0)) goto loc_0006AA6F;
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = MEMF(0x44C538);
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    fp_top() = fp_top() + MEMF(g_esp + 0x30);
    fp_push(MEMF(g_esp + 0x50));
    fp_top() = fp_top() * fp_st1();
    fp_top() = fp_top() + MEMF(g_esi);
    MEMF(g_esp + 0x20) = (float)fp_top(); fp_pop();
    g_eax = MEM32(g_esp + 0x20);
    fp_push(MEMF(g_esp + 0x4C));
    MEM32(g_esp + 0x34) = g_eax;
    fp_top() = fp_top() * fp_st1();
    fp_top() = fp_top() + MEMF(g_esi + 4);
    MEMF(g_esp + 0x24) = (float)fp_top(); fp_pop();
    g_ecx = MEM32(g_esp + 0x24);
    MEM32(g_esp + 0x38) = g_ecx;
    fp_top() = fp_top() * MEMF(g_esp + 0x60);
    fp_top() = fp_top() + MEMF(g_esi + 8);
    MEMF(g_esp + 0x28) = (float)fp_top(); fp_pop();
    g_edx = MEM32(g_esp + 0x28);
    MEM32(g_esp + 0x3C) = g_edx;
    if (((_sse_flags & 0x41u) != 0)) goto loc_0006AB86;
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x34));
    g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x3C));
    g_xmm2.f[0] = g_xmm2.f[0] - MEMF(g_esp + 0x48);
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x38));
    g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x44);
    g_xmm0.f[0] = g_xmm0.f[0] - g_xmm5.f[0];
    MEMF(g_esp + 0x10) = g_xmm0.f[0];
    fp_push(MEMF(g_esp + 0x10));
    fp_top() = fp_top() * MEMF(g_esp + 0x10);
    MEMF(g_esp + 0x18) = g_xmm2.f[0];
    fp_push(MEMF(g_esp + 0x18));
    MEMF(g_esp + 0x14) = g_xmm1.f[0];
    fp_top() = fp_top() * MEMF(g_esp + 0x18);
    g_eax = MEM32(0x51C4DC);
    { uint32_t _icall_esp = g_esp;
    PUSH32(g_esp, 0);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_push(MEMF(g_esp + 0x18));
    PUSH32(g_esp, 0x44DC04);
    fp_top() = fp_top() * MEMF(g_esp + 0x1C);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_top() = sqrt(fp_top());
    fp_top() = MEMF(0x44BF94) / fp_top();
    MEMF(g_esp + 0x38) = (float)fp_top(); fp_pop();
    g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x38));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
    g_xmm3 = XMM_SCALAR(MEMF(0x452840));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
    MEMF(g_esp + 0x1C) = g_xmm1.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
    MEMF(g_esp + 0x20) = g_xmm2.f[0];
    g_ecx = MEM32(g_eax + 0x6C);
    g_edx = MEM32(g_ecx);
    { uint32_t _icall_target = MEM32(g_edx + 0x14); PUSH32(g_esp, 0x0006AA5Fu); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
    }
    g_esi = g_esp + 0x10;
    PUSH32(g_esp, 0);
    PUSH32(g_esp, g_esi);
    g_esi = g_esp + 0x48;
    goto loc_0006AB71;
loc_0006AA6F: ;
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = MEMF(0x44C538);
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    fp_top() = fp_top() + MEMF(g_esp + 0x1C);
    fp_push(MEMF(g_esp + 0x50));
    fp_top() = fp_top() * fp_st1();
    fp_top() = fp_top() + MEMF(g_esi);
    MEMF(g_esp + 0x20) = (float)fp_top(); fp_pop();
    g_eax = MEM32(g_esp + 0x20);
    fp_push(MEMF(g_esp + 0x4C));
    MEM32(g_esp + 0x40) = g_eax;
    fp_top() = fp_top() * fp_st1();
    fp_top() = fp_top() + MEMF(g_esi + 4);
    MEMF(g_esp + 0x24) = (float)fp_top(); fp_pop();
    g_ecx = MEM32(g_esp + 0x24);
    MEM32(g_esp + 0x44) = g_ecx;
    fp_top() = fp_top() * MEMF(g_esp + 0x60);
    fp_top() = fp_top() + MEMF(g_esi + 8);
    MEMF(g_esp + 0x28) = (float)fp_top(); fp_pop();
    g_edx = MEM32(g_esp + 0x28);
    MEM32(g_esp + 0x48) = g_edx;
    if (((_sse_flags & 0x41u) != 0)) goto loc_0006AB86;
    g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x48));
    g_xmm2.f[0] = g_xmm2.f[0] - MEMF(g_esp + 0x3C);
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x44));
    g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x38);
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x40));
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x34);
    MEMF(g_esp + 0x18) = g_xmm2.f[0];
    fp_push(MEMF(g_esp + 0x18));
    fp_top() = fp_top() * MEMF(g_esp + 0x18);
    MEMF(g_esp + 0x14) = g_xmm1.f[0];
    fp_push(MEMF(g_esp + 0x14));
    MEMF(g_esp + 0x10) = g_xmm0.f[0];
    fp_top() = fp_top() * MEMF(g_esp + 0x14);
    g_eax = MEM32(0x51C4DC);
    { uint32_t _icall_esp = g_esp;
    PUSH32(g_esp, 0);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_push(MEMF(g_esp + 0x14));
    PUSH32(g_esp, 0x44DC04);
    fp_top() = fp_top() * MEMF(g_esp + 0x18);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_top() = sqrt(fp_top());
    fp_top() = MEMF(0x44BF94) / fp_top();
    MEMF(g_esp + 0x38) = (float)fp_top(); fp_pop();
    g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x38));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
    g_xmm3 = XMM_SCALAR(MEMF(0x452840));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
    MEMF(g_esp + 0x1C) = g_xmm1.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
    MEMF(g_esp + 0x20) = g_xmm2.f[0];
    g_ecx = MEM32(g_eax + 0x6C);
    g_edx = MEM32(g_ecx);
    { uint32_t _icall_target = MEM32(g_edx + 0x14); PUSH32(g_esp, 0x0006AB66u); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
    }
    g_esi = g_esp + 0x10;
    PUSH32(g_esp, 0);
    PUSH32(g_esp, g_esi);
    g_esi = g_esp + 0x3C;
loc_0006AB71: ;
    g_ecx = MEM32(0x51C4DC);
    g_ecx = MEM32(g_ecx + 0x6C);
    g_edx = MEM32(g_ecx);
    { uint32_t _icall_esp = g_esp;
    PUSH32(g_esp, g_esi);
    PUSH32(g_esp, 0x452838);
    PUSH32(g_esp, g_eax);
    { uint32_t _icall_target = MEM32(g_edx + 0x24); PUSH32(g_esp, 0x0006AB86u); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
    }
loc_0006AB86: ;
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_esp += 20; return;
    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}
#undef TRACER_CALL
#undef TRACER_VCALL
