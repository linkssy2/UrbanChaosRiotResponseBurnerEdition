#include "hybrid_expanding_smoke_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* Existing entity, expansion, source and emitter allocations remain live.
 * Expansion and lifecycle use structured native control and scalar movement.
 * Exact virtual CPU projection remains at original service boundaries and in
 * the alias-sensitive clipper/matrix setup. No performance claim is made.
 * World ray, registry,
 * manager, allocation and base-entity services preserve their public calls.
 * All failure is before the first effect; completing callbacks inherit the
 * same valid allocation/lifetime contract as the original routines. */
extern void sub_00011000(void);
extern void sub_0004F390(void);
extern void sub_0004F930(void);
extern void sub_0004FA00(void);
extern void sub_00055E80(void);
extern void sub_000DA5E0(void);
extern void sub_000F9130(void);
extern void sub_00134680(void);
extern void sub_001A8420(void);
extern void sub_001A8590(void);
extern void sub_001A8650(void);
extern void sub_001A86D0(void);
extern void sub_001A8930(void);
extern void sub_001A8B20(void);
extern void sub_001A8C50(void);
extern void sub_001A9140(void);
extern void sub_001A9440(void);
extern void sub_001A94C0(void);
extern void sub_001D52F0(void);
extern void sub_001F7DA0(void);
extern void sub_001F7E00(void);
extern void sub_001F7EA0(void);
extern void sub_001F8FC0(void);
extern void sub_001F9010(void);
extern void sub_00208C50(void);
extern void sub_0020A140(void);
extern void sub_0020A620(void);
extern void sub_0020BDC0(void);
extern void sub_00214230(void);
extern void sub_002D3A10(void);
extern void sub_002D57FB(void);
extern void sub_002D5CCD(void);
static int smoke_enabled(void){static int value=-1;if(value<0){const char*v=getenv("UCRR_NATIVE_EXPANDING_SMOKE");value=v&&!strcmp(v,"1");}return value;}
static int smoke_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int smoke_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static void smoke_001a8420(void)
{
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)
    g_esp = g_esp - 0xC;
    PUSH32(g_esp, g_esi);
    g_esi = MEM32(g_esp + 0x18);
    PUSH32(g_esp, g_edi);
    g_edi = MEM32(g_esp + 0x20);
    PUSH32(g_esp, 0);
    PUSH32(g_esp, 0x80);
    PUSH32(g_esp, 0xFFFF);
    PUSH32(g_esp, 1);
    PUSH32(g_esp, g_edi);
    PUSH32(g_esp, g_esi);
    PUSH32(g_esp, 0x001A8442u); sub_00214230();
    g_esp = g_esp + 0x18;
    if(g_eax){
    g_xmm3 = XMM_SCALAR(MEMF(g_eax));
    g_xmm4 = XMM_SCALAR(MEMF(g_eax + 4));
    g_xmm5 = XMM_SCALAR(MEMF(g_eax + 8));
    g_xmm0 = g_xmm3;
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esi);
    MEMF(g_esp + 8) = g_xmm0.f[0];
    g_xmm0 = g_xmm4;
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esi + 4);
    MEMF(g_esp + 0xC) = g_xmm0.f[0];
    g_xmm0 = g_xmm5;
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esi + 8);
    MEMF(g_esp + 0x10) = g_xmm0.f[0];
    fp_push(MEMF(g_esp + 0x10));
    fp_top() = fp_top() * MEMF(g_esp + 0x10);
    fp_push(MEMF(g_esp + 0xC));
    fp_top() = fp_top() * MEMF(g_esp + 0xC);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_push(MEMF(g_esp + 8));
    fp_top() = fp_top() * MEMF(g_esp + 8);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_top() = sqrt(fp_top());
    fp_push(MEMF(0x44E088));
    { double _t = fp_top(); fp_top() = fp_st1(); fp_st1() = _t; }
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); fp_pop();
    fp_pop();
    if(g_fp_cmp>0){
    g_xmm2 = XMM_SCALAR(MEMF(g_edi + 8));
    g_xmm2.f[0] = g_xmm2.f[0] - MEMF(g_esi + 8);
    g_xmm1 = XMM_SCALAR(MEMF(g_edi + 4));
    g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esi + 4);
    g_xmm0 = XMM_SCALAR(MEMF(g_edi));
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esi);
    MEMF(g_esp + 0x10) = g_xmm2.f[0];
    fp_push(MEMF(g_esp + 0x10));
    fp_top() = fp_top() * MEMF(g_esp + 0x10);
    MEMF(g_esp + 0xC) = g_xmm1.f[0];
    fp_push(MEMF(g_esp + 0xC));
    MEMF(g_esp + 8) = g_xmm0.f[0];
    fp_top() = fp_top() * MEMF(g_esp + 0xC);
    g_eax = MEM32(g_esp + 0x18);
    POP32(g_esp, g_edi);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_push(MEMF(g_esp + 4));
    POP32(g_esp, g_esi);
    fp_top() = fp_top() * MEMF(g_esp);
    fp_st1() = fp_st1() + fp_top(); fp_pop();
    fp_top() = sqrt(fp_top());
    fp_top() = MEMF(0x44BF94) / fp_top();
    MEMF(g_esp + 0x18) = (float)fp_top(); fp_pop();
    g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x18));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm6.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm6.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm6.f[0];
    g_xmm6 = XMM_SCALAR(MEMF(0x44E088));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm6.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm6.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm6.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] - g_xmm0.f[0];
    g_xmm4.f[0] = g_xmm4.f[0] - g_xmm1.f[0];
    g_xmm5.f[0] = g_xmm5.f[0] - g_xmm2.f[0];
    MEMF(g_eax) = g_xmm3.f[0];
    MEMF(g_eax + 4) = g_xmm4.f[0];
    MEMF(g_eax + 8) = g_xmm5.f[0];
    g_esp = g_esp + 0xC;
    g_recomp_zf = !!((g_esp == 0));
    g_esp += 16; return;
    }else{
    g_ecx = MEM32(g_esi);
    g_eax = MEM32(g_esp + 0x18);
    MEM32(g_eax) = g_ecx;
    g_edx = MEM32(g_esi + 4);
    MEM32(g_eax + 4) = g_edx;
    g_ecx = MEM32(g_esi + 8);
    POP32(g_esp, g_edi);
    MEM32(g_eax + 8) = g_ecx;
    POP32(g_esp, g_esi);
    g_esp = g_esp + 0xC;
    g_recomp_zf = !!((g_esp == 0));
    g_esp += 16; return;
    }
    }else{
    g_edx = MEM32(g_edi);
    g_eax = MEM32(g_esp + 0x18);
    MEM32(g_eax) = g_edx;
    g_ecx = MEM32(g_edi + 4);
    MEM32(g_eax + 4) = g_ecx;
    g_edx = MEM32(g_edi + 8);
    POP32(g_esp, g_edi);
    MEM32(g_eax + 8) = g_edx;
    POP32(g_esp, g_esi);
    g_esp = g_esp + 0xC;
    g_recomp_zf = !!((g_esp == 0));
    g_esp += 16; return;
    }
    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/* Seed and ray-clip one authored corner. Keep sequential input copies:
 * the supplied center may overlap the record being initialized. */
static void smoke_001a8590(void){
 g_eax=MEM32(g_esp+4);g_edx=MEM32(g_eax);g_esp-=0x18;PUSH32(g_esp,g_esi);g_esi=g_ecx;
 float step=MEMF(g_esi+28);step=step*MEMF(0x44c798);MEM32(g_ecx)=g_edx;
 g_edx=MEM32(g_eax+4);MEM32(g_ecx+4)=g_edx;g_eax=MEM32(g_eax+8);MEM32(g_ecx+8)=g_eax;
 g_eax=MEM32(g_esi+24);float x=MEMF(g_esi),y=MEMF(g_esi+4),z=MEMF(g_esi+8);
 if(g_eax&1)x=x+step;if(g_eax&2)x=x-step;
 if(g_eax&4){float rise=step*MEMF(0x44d744);rise=rise+y;y=rise;g_xmm4=XMM_SCALAR(rise);}
 if(g_eax&8)y=y-step;if(g_eax&16)z=z+step;if(g_eax&32)z=z-step;
 g_xmm0=XMM_SCALAR(step);g_xmm1=XMM_SCALAR(z);g_xmm2=XMM_SCALAR(y);g_xmm3=XMM_SCALAR(x);
 g_ecx=g_esp+4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_edx=g_esp+0x18;PUSH32(g_esp,g_edx);g_ecx=g_esi;
 MEMF(g_esp+0x10)=x;MEMF(g_esp+0x14)=y;MEMF(g_esp+0x18)=z;PUSH32(g_esp,0x1a862a);sub_001A8420();
 g_ecx=MEM32(g_eax);g_esi+=12;MEM32(g_esi)=g_ecx;g_edx=MEM32(g_eax+4);MEM32(g_esi+4)=g_edx;g_eax=MEM32(g_eax+8);MEM32(g_esi+8)=g_eax;
 POP32(g_esp,g_esi);g_esp+=0x18;g_recomp_zf=g_esp==0;g_esp+=8;
}

static void smoke_001a8650(void){
 /* The final ECX assignment and interleaved loads/stores are observable when
  * source/output overlap; this is deliberately not memcpy or memmove. */
 g_edx=MEM32(g_ecx+0x15c);g_eax=MEM32(g_esp+4);MEM32(g_eax)=g_edx;
 for(unsigned i=1;i<11;i++){g_edx=MEM32(g_ecx+0x15c+4*i);MEM32(g_eax+4*i)=g_edx;}
 g_ecx=MEM32(g_ecx+0x188);MEM32(g_eax+44)=g_ecx;g_esp+=8;
}

/* Advance the eight authored corner records. Arithmetic stays scalar and
 * source-ordered: candidate travel is clamped against the remaining ray span,
 * then positive travel decays velocity. The original bounds service remains
 * the only call inside the loop. Publish its exact ABI state at that boundary
 * instead of routing every intermediate through the virtual SIMD registers. */
static unsigned smoke_compare(float lhs,float rhs){
 double a=lhs,b=rhs;return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
}
static void smoke_001a86d0(void){
 uint32_t inherited_frame=g_seh_ebp;
 g_esp-=0x58;PUSH32(g_esp,g_ebx);PUSH32(g_esp,inherited_frame);g_ebx=g_ecx;
 SET_LO8(g_eax,MEM8(g_ebx+0x28));PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(LO8(g_eax)){
  g_esi=g_ebx+0x2c;g_edi=g_ebx+0x15c;g_ecx=12;
  memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;
 }
 for(unsigned corner=0;corner<8;corner++){
  uint32_t record=g_ebx+0x5c+corner*32, directions=MEM32(record+24);
  float step=MEMF(record+28),x=MEMF(record),z=MEMF(record+8);
  step=step*MEMF(g_esp+0x6c);float y=MEMF(record+4);
  MEMF(g_esp+0x20)=x;MEMF(g_esp+0x28)=z;MEMF(g_esp+0x10)=step;
  if(directions&1){x=x+step;MEMF(g_esp+0x20)=x;}
  if(directions&2){x=x-step;MEMF(g_esp+0x20)=x;}
  if(directions&4){float rise=step*MEMF(0x44d744);rise=rise+y;y=rise;}
  if(directions&8)y=y-step;
  if(directions&16){z=z+step;MEMF(g_esp+0x28)=z;}
  if(directions&32){z=z-step;MEMF(g_esp+0x28)=z;}
  float dz=z-MEMF(record+8),dx=x-MEMF(record);
  float remaining_y=MEMF(record+16);remaining_y=remaining_y-MEMF(record+4);
  float remaining_z=MEMF(record+20);remaining_z=remaining_z-MEMF(record+8);
  float remaining_x=MEMF(record+12);remaining_x=remaining_x-MEMF(record);
  float traveled=dz*dz,dy=y-MEMF(record+4),y_square=dy*dy,x_square=dx*dx;
  traveled=traveled+y_square;traveled=traveled+x_square;
  float remaining=remaining_z*remaining_z,remaining_y_square=remaining_y*remaining_y;
  remaining=remaining+remaining_y_square;float remaining_x_square=remaining_x*remaining_x;remaining=remaining+remaining_x_square;
  float selected_z;
  if(!(smoke_compare(traveled,remaining)&0x41u)){
   float value=MEMF(record+12);MEMF(g_esp+0x14)=value;
   value=MEMF(record+16);MEMF(g_esp+0x18)=value;selected_z=MEMF(record+20);
  }else{
   float value=MEMF(g_esp+0x20);MEMF(g_esp+0x14)=value;
   selected_z=MEMF(g_esp+0x28);MEMF(g_esp+0x18)=y;
  }
  g_edx=MEM32(g_esp+0x14);g_ecx=record;MEM32(g_ecx)=g_edx;
  g_edx=MEM32(g_esp+0x18);MEMF(g_esp+0x1c)=selected_z;
  float decay=MEMF(g_esp+0x10);unsigned moving=smoke_compare(decay,0.f);
  MEM32(g_ecx+4)=g_edx;g_edx=MEM32(g_esp+0x1c);MEM32(g_ecx+8)=g_edx;
  float final_speed=remaining_x_square;
  if(!(moving&0x41u)){final_speed=MEMF(record+28);decay=decay*MEMF(0x44d76c);final_speed=final_speed-decay;MEMF(record+28)=final_speed;}
  g_eax=record;
  g_xmm0=XMM_SCALAR(decay);g_xmm1=XMM_SCALAR(final_speed);g_xmm2=XMM_SCALAR(y_square);g_xmm3=XMM_ZERO();
  g_xmm4=XMM_SCALAR(remaining_y);g_xmm5=XMM_SCALAR(remaining_z);g_xmm6=XMM_SCALAR(y);g_xmm7=XMM_SCALAR(traveled);
  int first_unmerged=0;
  if(!corner){SET_LO8(g_ecx,MEM8(g_ebx+0x28));first_unmerged=!LO8(g_ecx);}
  if(first_unmerged){
   g_xmm0=XMM_SCALAR(MEMF(g_ebx+0x5c));g_xmm1=XMM_SCALAR(MEMF(g_ebx+0x60));g_xmm2=XMM_SCALAR(MEMF(g_ebx+0x64));
   g_ecx=12;g_esi=g_esp+0x38;g_edi=g_ebx+0x15c;
   /* Nine point coordinates then three zero extents, in original store order. */
   for(unsigned i=0;i<3;i++){MEMF(g_esp+0x38+12*i)=g_xmm0.f[0];MEMF(g_esp+0x3c+12*i)=g_xmm1.f[0];MEMF(g_esp+0x40+12*i)=g_xmm2.f[0];}
   MEMF(g_esp+0x5c)=g_xmm3.f[0];MEMF(g_esp+0x60)=g_xmm3.f[0];MEMF(g_esp+0x64)=g_xmm3.f[0];
   memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;
  }else{
   g_xmm0=XMM_SCALAR(MEMF(g_eax));MEMF(g_esp+0x2c)=g_xmm0.f[0];
   g_xmm0=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+0x30)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+8));
   g_eax=g_esp+0x2c;PUSH32(g_esp,g_eax);g_ecx=g_ebx+0x15c;MEMF(g_esp+0x38)=g_xmm0.f[0];
   PUSH32(g_esp,0x1a8912);sub_002D3A10();
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,inherited_frame);POP32(g_esp,g_ebx);g_esp+=0x58;g_recomp_zf=g_esp==0;g_esp+=8;
}

static void smoke_001a8930(void){
 SET_LO8(g_eax,MEM8(g_ecx+0x28));int first=!LO8(g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(first){
  g_esi=MEM32(g_esp+12);MEM8(g_ecx+0x28)=1;g_edi=g_ecx+0x2c;g_ecx=12;
  memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;
 }else{
  g_edi=MEM32(g_esp+12);g_esi=g_ecx+0x2c;g_eax=g_edi+12;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x1a894b);sub_002D3A10();
  PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x1a8953);sub_002D3A10();
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=first;g_esp+=8;
}

static void smoke_001a8970(void)
{
    uint32_t frame = g_ebp;
    frame = g_ebp;
    frame = g_seh_ebp;
    g_eax = MEM32(0x51C398);
    { uint32_t _icall_esp = g_esp;
    PUSH32(g_esp, g_esi);
    g_esi = g_ecx;
    g_ecx = MEM32(g_eax + 0xC);
    g_eax = MEM32(g_esi + 0x54);
    g_edx = MEM32(g_ecx);
    PUSH32(g_esp, 0x47788C);
    PUSH32(g_esp, g_eax);
    { uint32_t _icall_target = MEM32(g_edx + 8); PUSH32(g_esp, 0x001A8989u); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
    }
    g_eax = MEM32(g_esi + 0x54);
    g_ecx = MEM32(0x51C4DC);
    PUSH32(g_esp, g_eax);
    PUSH32(g_esp, 0x001A8998u); sub_00055E80();
    g_ecx = g_eax;
    PUSH32(g_esp, 0x001A899Fu); sub_0004F930();
    g_ecx = MEM32(g_esi + 0x84);
    PUSH32(g_esp, g_ecx);
    PUSH32(g_esp, 0x001A89ABu); sub_002D5CCD();
    g_esp = g_esp + 4;
    g_ecx = g_esi;
    POP32(g_esp, g_esi);
    g_seh_ebp = frame; sub_0020A620(); return;
}

static void smoke_001a8a20(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x1a8a28);sub_000DA5E0();
 if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1a8a35);sub_002D5CCD();g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;
}

static void smoke_001a8a40(void){
 g_eax=MEM32(g_ecx+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1a8a49);sub_001F8FC0();g_esp+=4;
 if(g_eax&&!(MEM8(g_eax+0x5c)&0x20)){SET_LO8(g_eax,0);g_recomp_zf=1;}
 else SET_LO8(g_eax,1);
 g_esp+=4;
}

static void smoke_001a8a60(void)
{
    uint32_t frame = g_ebp;
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    g_esp = g_esp - 0x5C;
    PUSH32(g_esp, g_esi);
    PUSH32(g_esp, 0x1F4B10);
    PUSH32(g_esp, 4);
    PUSH32(g_esp, 0x10);
    g_eax = g_esp + 0x2C;
    PUSH32(g_esp, g_eax);
    g_esi = g_ecx;
    g_ebp = frame;
    PUSH32(g_esp, 0x001A8A7Fu); sub_00011000();
    g_xmm0 = XMM_MEM(0x51E110);
    XMM_STORE(g_esp + 0x20, g_xmm0);
    g_xmm0 = XMM_MEM(0x51E120);
    XMM_STORE(g_esp + 0x30, g_xmm0);
    g_xmm0 = XMM_MEM(0x51E130);
    XMM_STORE(g_esp + 0x40, g_xmm0);
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 8));
    MEMF(g_esp + 0x10) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 0xC));
    MEMF(g_esp + 0x14) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 0x10));
    g_esi = MEM32(frame + 8);
    PUSH32(g_esp, 0x1F4B10);
    MEMF(g_esp + 0x1C) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44BF94));
    PUSH32(g_esp, 4);
    MEMF(g_esp + 0x24) = g_xmm0.f[0];
    g_xmm0 = XMM_MEM(g_esp + 0x18);
    PUSH32(g_esp, 0x10);
    PUSH32(g_esp, g_esi);
    XMM_STORE(g_esp + 0x60, g_xmm0);
    g_ebp = frame;
    PUSH32(g_esp, 0x001A8AEEu); sub_00011000();
    g_xmm0 = XMM_MEM(g_esp + 0x20);
    XMM_STORE(g_esi, g_xmm0);
    g_xmm0 = XMM_MEM(g_esp + 0x30);
    XMM_STORE(g_esi + 0x10, g_xmm0);
    g_xmm0 = XMM_MEM(g_esp + 0x40);
    XMM_STORE(g_esi + 0x20, g_xmm0);
    g_xmm0 = XMM_MEM(g_esp + 0x50);
    XMM_STORE(g_esi + 0x30, g_xmm0);
    g_eax = g_esi;
    POP32(g_esp, g_esi);
    g_esp = frame;
    POP32(g_esp, frame);
    g_recomp_zf = !!((g_esp == 0));
    g_esp += 12; return;
}

static void smoke_001a8b20(void){
 static const uint32_t direction[8]={0x15,0x16,0x19,0x1a,0x25,0x26,0x29,0x2a};
 static const uint32_t returns[8]={0x1a8b5a,0x1a8b77,0x1a8b97,0x1a8bb7,0x1a8bd7,0x1a8bf7,0x1a8c17,0x1a8c37};
 g_eax=MEM32(g_esp+4);g_edx=MEM32(g_eax);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_edi=g_esi+0x10;g_ecx=g_edi;
 MEM32(g_ecx)=g_edx;g_edx=MEM32(g_eax+4);MEM32(g_ecx+4)=g_edx;g_eax=MEM32(g_eax+8);MEM32(g_ecx+8)=g_eax;
 for(unsigned i=0;i<8;i++){
  g_ecx=g_esi+0x5c+i*0x20;MEM32(g_ecx+0x18)=direction[i];g_xmm0=XMM_SCALAR(MEMF(0x477874));PUSH32(g_esp,g_edi);MEMF(g_ecx+0x1c)=g_xmm0.f[0];PUSH32(g_esp,returns[i]);sub_001A8590();
 }
 PUSH32(g_esp,0x3f800000);g_ecx=g_esi;PUSH32(g_esp,0x1a8c43);sub_001A86D0();POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}

/* Expand the existing bounds, publish descriptor extents and scale emission
 * by their volume. Keep intermediate stores ordered for admitted aliases. */
static void smoke_001a8c50(void){
 g_esp-=12;PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x1a8c5b);sub_001F9010();
 PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;PUSH32(g_esp,0x1a8c66);sub_001A86D0();
 float scale=MEMF(0x44c530),x=MEMF(g_esi+0x180);g_eax=MEM32(g_esi+12);x=x*scale;MEMF(g_esp+4)=x;
 float y=MEMF(g_esi+0x184);g_ecx=MEM32(g_esp+4);y=y*scale;MEMF(g_esp+8)=y;
 float z=MEMF(g_esi+0x188);g_edx=MEM32(g_esp+8);z=z*scale;g_eax+=4;MEM32(g_eax)=g_ecx;MEMF(g_esp+12)=z;
 g_ecx=MEM32(g_esp+12);MEM32(g_eax+4)=g_edx;MEM32(g_eax+8)=g_ecx;
 y=MEMF(g_esi+0x184);z=MEMF(g_esi+0x188);x=MEMF(g_esi+0x180);g_eax=MEM32(g_esi+8);
 z=z*scale;y=y*scale;x=x*scale;float minimum=MEMF(g_eax+12);float volume=z*y;volume=volume*x;volume=volume*MEMF(0x4778a8);
 if(smoke_compare(volume,minimum)&0x41u)volume=minimum;MEMF(g_eax+4)=volume;
 float center=MEMF(g_esi+0x174);g_edx=MEM32(g_esi+4);MEMF(g_esp+4)=center;center=MEMF(g_esi+0x178);
 g_eax=MEM32(g_esp+4);MEMF(g_esp+8)=center;center=MEMF(g_esi+0x17c);g_ecx=MEM32(g_esp+8);g_edx+=8;
 MEM32(g_edx)=g_eax;MEMF(g_esp+12)=center;g_eax=MEM32(g_esp+12);MEM32(g_edx+4)=g_ecx;MEM32(g_edx+8)=g_eax;
 g_xmm0=XMM_SCALAR(center);g_xmm1=XMM_SCALAR(minimum);g_xmm2=XMM_SCALAR(x);g_xmm3=XMM_SCALAR(y);
 POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;
}

/* Follow the source, merge settled smoke, then retire after source loss.
 * These are the authored decisions; registry, ray, group and bounds services
 * retain their original order and can change live owner state between phases. */
static void smoke_001a8d50(void){
#define fp_push(v) do{double value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;}while(0)
#define fp_pop() (g_fp_top=(g_fp_top+1u)&7u)
#define fp_top() g_fp_stack[g_fp_top]
#define fp_st1() g_fp_stack[(g_fp_top+1u)&7u]
g_esp = g_esp - 0x54;
PUSH32(g_esp, g_ebx);
PUSH32(g_esp, g_esi);
g_esi = g_ecx;
g_eax = MEM32(g_esi + 0x58);
PUSH32(g_esp, g_edi);
int source_present=0;
if(g_eax!=0xffffffffu){
PUSH32(g_esp, g_eax);
PUSH32(g_esp, 0x001A8D6Au); sub_001F8FC0();
g_esp = g_esp + 4;
source_present=g_eax!=0;
}
if(source_present){
PUSH32(g_esp, 0x001A8D7Au); sub_001F9010();
fp_top() = fp_top() + MEMF(g_esi + 0xB8);
MEMF(g_esi + 0xB8) = (float)fp_top();
fp_push(MEMF(0x44C530));
g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); fp_pop();
fp_pop();
if(g_fp_cmp>0){
g_eax = MEM32(g_esi + 0x58);
PUSH32(g_esp, g_eax);
PUSH32(g_esp, 0x001A8D9Fu); sub_001F8FC0();
g_esp = g_esp + 4;
if(g_eax){
g_ecx = MEM32(g_esi + 0x84);
g_xmm2 = XMM_SCALAR(MEMF(g_ecx + 0x18));
g_xmm2.f[0] = g_xmm2.f[0] - MEMF(g_eax + 0x48);
g_xmm1 = XMM_SCALAR(MEMF(g_ecx + 0x14));
g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_eax + 0x44);
g_xmm0 = XMM_SCALAR(MEMF(g_ecx + 0x10));
g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_eax + 0x40);
g_eax = g_eax + 0x40;
MEMF(g_esp + 0x20) = g_xmm2.f[0];
fp_push(MEMF(g_esp + 0x20));
fp_top() = fp_top() * MEMF(g_esp + 0x20);
MEMF(g_esp + 0x1C) = g_xmm1.f[0];
fp_push(MEMF(g_esp + 0x1C));
MEMF(g_esp + 0x18) = g_xmm0.f[0];
fp_top() = fp_top() * MEMF(g_esp + 0x1C);
fp_st1() = fp_st1() + fp_top(); fp_pop();
fp_push(MEMF(g_esp + 0x18));
fp_top() = fp_top() * MEMF(g_esp + 0x18);
fp_st1() = fp_st1() + fp_top(); fp_pop();
fp_top() = sqrt(fp_top());
fp_push(MEMF(0x44C798));
{ double _t = fp_top(); fp_top() = fp_st1(); fp_st1() = _t; }
g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); fp_pop();
fp_pop();
if(g_fp_cmp>0){
PUSH32(g_esp, g_eax);
PUSH32(g_esp, 0x001A8E19u); sub_001A8B20();
g_xmm0 = XMM_ZERO();
MEMF(g_esi + 0xB8) = g_xmm0.f[0];
}
}
}else{
SET_LO8(g_eax, MEM8(g_esi + 0xBC));
g_ebx = 0;
if(LO8(g_eax)==LO8(g_ebx)){
g_eax = MEM32(0x51C398);
g_ecx = MEM32(g_eax + 0xC);
g_eax = MEM32(g_esi + 0x54);
g_edx = MEM32(g_ecx);
{ uint32_t _icall_esp = g_esp;
PUSH32(g_esp, 0x47788C);
PUSH32(g_esp, g_eax);
{ uint32_t _icall_target = MEM32(g_edx + 4); PUSH32(g_esp, 0x001A8E4Bu); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
}
MEM8(g_esi + 0xBC) = 1;
}
PUSH32(g_esp, 0x47788C);
g_ecx = g_esp + 0x10;
MEM32(g_esp + 0x10) = g_ebx;
PUSH32(g_esp, 0x001A8E64u); sub_001F7EA0();
g_ecx = g_esp + 0xC;
PUSH32(g_esp, 0x001A8E6Du); sub_001F7DA0();
if(g_eax)do{
g_edi = MEM32(g_esi + 0x54);
PUSH32(g_esp, g_ebx);
g_ecx = g_esp + 0x10;
PUSH32(g_esp, 0x001A8E82u); sub_001F7E00();
if(g_edi!=MEM32(g_eax)){
PUSH32(g_esp, g_ebx);
g_ecx = g_esp + 0x10;
PUSH32(g_esp, 0x001A8E94u); sub_001F7E00();
g_ecx = MEM32(g_eax);
PUSH32(g_esp, g_ecx);
PUSH32(g_esp, 0x001A8E9Cu); sub_001F8FC0();
g_esp = g_esp + 4;
g_edi = g_eax;
g_ecx = MEM32(g_edi + 0x84);
g_edx = g_esp + 0x30;
PUSH32(g_esp, g_edx);
PUSH32(g_esp, 0x001A8EB1u); sub_001A8650();
int overlaps=1;
for(unsigned axis=0;axis<3&&overlaps;axis++){
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30+4*axis));
 overlaps=!!(smoke_compare(g_xmm0.f[0],MEMF(g_esi+0x94+4*axis))&0x41u);
 if(overlaps){g_xmm0=XMM_SCALAR(MEMF(g_esi+0x88+4*axis));overlaps=!!(smoke_compare(g_xmm0.f[0],MEMF(g_esp+0x3c+4*axis))&0x41u);}
}
if(overlaps){
g_ecx = MEM32(g_esi + 0x84);
g_eax = g_esp + 0x30;
PUSH32(g_esp, g_eax);
PUSH32(g_esp, 0x001A8F1Bu); sub_001A8930();
PUSH32(g_esp, 1);
g_ecx = g_edi;
PUSH32(g_esp, 0x001A8F24u); sub_0020A140();
}
}
g_ecx = g_esp + 0xC;
g_ebx++;
PUSH32(g_esp, 0x001A8F2Eu); sub_001F7DA0();
}while(g_ebx<g_eax);
}
/* Refresh emission and publish the expanded world bounds. */
g_ecx = MEM32(g_esi + 0x84);
PUSH32(g_esp, 0x001A8F41u); sub_001A8C50();
g_ecx = g_esp + 0x30;
PUSH32(g_esp, g_ecx);
g_ecx = MEM32(g_esi + 0x84);
PUSH32(g_esp, 0x001A8F51u); sub_001A8650();
g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x30));
g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x34));
g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x38));
g_xmm3 = XMM_SCALAR(MEMF(0x44C1F8));
g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x3C));
g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x40));
g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x44));
g_xmm0.f[0] = g_xmm0.f[0] - g_xmm4.f[0];
g_xmm1.f[0] = g_xmm1.f[0] - g_xmm5.f[0];
g_xmm2.f[0] = g_xmm2.f[0] - g_xmm6.f[0];
g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
g_xmm3 = g_xmm0;
g_xmm3.f[0] = g_xmm3.f[0] + g_xmm4.f[0];
g_xmm4 = g_xmm1;
g_xmm4.f[0] = g_xmm4.f[0] + g_xmm5.f[0];
g_xmm5 = g_xmm2;
g_xmm5.f[0] = g_xmm5.f[0] + g_xmm6.f[0];
g_xmm6 = XMM_SCALAR(MEMF(0x44C47C));
g_xmm0.f[0] = g_xmm0.f[0] * g_xmm6.f[0];
g_xmm1.f[0] = g_xmm1.f[0] * g_xmm6.f[0];
g_xmm2.f[0] = g_xmm2.f[0] * g_xmm6.f[0];
g_edx = g_esp + 0x24;
MEMF(g_esp + 0xC) = g_xmm0.f[0];
MEMF(g_esp + 0x10) = g_xmm1.f[0];
MEMF(g_esp + 0x14) = g_xmm2.f[0];
g_edi = g_esi + 0x88;
g_xmm0.f[0] = g_xmm0.f[0] + g_xmm3.f[0];
g_xmm1.f[0] = g_xmm1.f[0] + g_xmm4.f[0];
g_xmm2.f[0] = g_xmm2.f[0] + g_xmm5.f[0];
PUSH32(g_esp, g_edx);
g_ecx = g_edi;
MEMF(g_esp + 0x1C) = g_xmm3.f[0];
MEMF(g_esp + 0x20) = g_xmm4.f[0];
MEMF(g_esp + 0x24) = g_xmm5.f[0];
MEMF(g_esp + 0x28) = g_xmm0.f[0];
MEMF(g_esp + 0x2C) = g_xmm1.f[0];
MEMF(g_esp + 0x30) = g_xmm2.f[0];
PUSH32(g_esp, 0x001A9012u); sub_002D3A10();
g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0xC));
g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x18));
g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x10));
g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x14));
g_xmm0.f[0] = g_xmm0.f[0] - g_xmm1.f[0];
MEMF(g_esp + 0x24) = g_xmm0.f[0];
g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x1C));
g_xmm0.f[0] = g_xmm0.f[0] - g_xmm2.f[0];
MEMF(g_esp + 0x28) = g_xmm0.f[0];
g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x20));
g_eax = g_esp + 0x24;
g_xmm0.f[0] = g_xmm0.f[0] - g_xmm3.f[0];
PUSH32(g_esp, g_eax);
g_ecx = g_edi;
MEMF(g_esp + 0x30) = g_xmm0.f[0];
PUSH32(g_esp, 0x001A9060u); sub_002D3A10();
g_ecx = MEM32(g_edi);
g_esp = g_esp - 0x30;
g_eax = g_esp;
MEM32(g_eax) = g_ecx;
g_edx = MEM32(g_edi + 4);
MEM32(g_eax + 4) = g_edx;
g_ecx = MEM32(g_edi + 8);
MEM32(g_eax + 8) = g_ecx;
g_edx = MEM32(g_edi + 0xC);
MEM32(g_eax + 0xC) = g_edx;
g_ecx = MEM32(g_edi + 0x10);
MEM32(g_eax + 0x10) = g_ecx;
g_edx = MEM32(g_edi + 0x14);
MEM32(g_eax + 0x14) = g_edx;
g_ecx = MEM32(g_edi + 0x18);
MEM32(g_eax + 0x18) = g_ecx;
g_edx = MEM32(g_edi + 0x1C);
MEM32(g_esp + 0x44) = g_esp;
MEM32(g_eax + 0x1C) = g_edx;
g_ecx = MEM32(g_edi + 0x20);
MEM32(g_eax + 0x20) = g_ecx;
g_edx = MEM32(g_edi + 0x24);
MEM32(g_eax + 0x24) = g_edx;
g_ecx = MEM32(g_edi + 0x28);
MEM32(g_eax + 0x28) = g_ecx;
g_edx = MEM32(g_edi + 0x2C);
MEM32(g_eax + 0x2C) = g_edx;
g_esi = MEM32(g_esi + 0x54);
g_ecx = MEM32(0x51C4DC);
PUSH32(g_esp, g_esi);
PUSH32(g_esp, 0x001A90BEu); sub_00055E80();
g_ecx = g_eax;
PUSH32(g_esp, 0x001A90C5u); sub_0004F390();
}else{
if(!(MEM8(g_esi+0x5c)&0x20)){
PUSH32(g_esp, 0x20);
g_ecx = g_esi;
PUSH32(g_esp, 0x001A90DBu); sub_0020A140();
g_xmm0 = XMM_SCALAR(MEMF(0x477878));
MEMF(g_esi + 0x80) = g_xmm0.f[0];
g_xmm0 = XMM_ZERO();
MEMF(g_esi + 0xB8) = g_xmm0.f[0];
g_eax = MEM32(0x51C398);
g_ecx = MEM32(g_eax + 0xC);
g_eax = MEM32(g_esi + 0x54);
g_edx = MEM32(g_ecx);
{ uint32_t _icall_esp = g_esp;
PUSH32(g_esp, 0x47788C);
PUSH32(g_esp, g_eax);
{ uint32_t _icall_target = MEM32(g_edx + 8); PUSH32(g_esp, 0x001A910Cu); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
}
}
PUSH32(g_esp, 0x001A9111u); sub_001F9010();
fp_top() = MEMF(g_esi + 0x80) - fp_top();
MEMF(g_esi + 0x80) = (float)fp_top();
fp_push(MEMF(0x44C174));
g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); fp_pop();
fp_pop();
if(g_fp_cmp>=0){
g_edx = MEM32(g_esi);
g_ecx = g_esi;
{ uint32_t _icall_esp = g_esp;
{ uint32_t _icall_target = MEM32(g_edx + 0x24); PUSH32(g_esp, 0x001A9130u); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
}
}
}
POP32(g_esp, g_edi);
POP32(g_esp, g_esi);
POP32(g_esp, g_ebx);
g_esp = g_esp + 0x54;
g_recomp_zf = !!((g_esp == 0));
g_esp += 4; return;
#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st1
}

static void smoke_001a9140(void)
{
    uint32_t frame = g_ebp;
    uint32_t _sse_flags = 0x02u;
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    g_esp = g_esp - 0x174;
    g_xmm0 = XMM_ZERO();
    g_eax = MEM32(frame + 8);
    PUSH32(g_esp, g_ebx);
    g_ebx = 0;
    PUSH32(g_esp, g_esi);
    g_esi = g_ecx;
    MEM32(g_esi) = g_eax;
    /* Eight corner records start with zero position, no direction and zero
     * speed. The traced endpoint bytes remain untouched until ray setup. */
    for(unsigned i=0;i<8;i++){
        uint32_t corner=g_esi+0x5c+32*i;
        MEMF(corner)=g_xmm0.f[0];MEMF(corner+4)=g_xmm0.f[0];MEMF(corner+8)=g_xmm0.f[0];
        MEM32(corner+24)=g_ebx;MEMF(corner+28)=g_xmm0.f[0];g_eax=corner;
    }
    PUSH32(g_esp, g_edi);
    g_edi = MEM32(frame + 0x10);
    g_ecx = MEM32(g_edi);
    MEM32(g_esi + 0x15C) = g_ecx;
    g_edx = MEM32(g_edi + 4);
    MEM32(g_esi + 0x160) = g_edx;
    g_eax = MEM32(g_edi + 8);
    MEM32(g_esi + 0x164) = g_eax;
    g_ecx = MEM32(g_edi);
    MEM32(g_esi + 0x168) = g_ecx;
    g_edx = MEM32(g_edi + 4);
    MEM32(g_esi + 0x16C) = g_edx;
    g_eax = MEM32(g_edi + 8);
    MEM32(g_esi + 0x170) = g_eax;
    g_ecx = MEM32(g_edi);
    MEM32(g_esi + 0x174) = g_ecx;
    g_edx = MEM32(g_edi + 4);
    MEM32(g_esi + 0x178) = g_edx;
    g_eax = MEM32(g_edi + 8);
    MEM32(g_esi + 0x17C) = g_eax;
    MEMF(g_esi + 0x180) = g_xmm0.f[0];
    MEMF(g_esi + 0x184) = g_xmm0.f[0];
    MEMF(g_esi + 0x188) = g_xmm0.f[0];
    MEM8(g_esi + 0x28) = LO8(g_ebx);
    PUSH32(g_esp, g_edi);
    g_ecx = g_esi;
    g_ebp = frame;
    PUSH32(g_esp, 0x001A929Fu); sub_001A8B20();
    g_ecx = MEM32(0x51C4DC);
    g_ebp = frame;
    PUSH32(g_esp, 0x001A92AAu); sub_00208C50();
    g_ecx = MEM32(frame + 0xC);
    g_edx = MEM32(g_eax);
    { uint32_t _icall_esp = g_esp;
    PUSH32(g_esp, g_ecx);
    g_ecx = g_esp + 0x24;
    PUSH32(g_esp, g_ecx);
    g_ecx = g_eax;
    { uint32_t _icall_target = MEM32(g_edx + 0x1C); PUSH32(g_esp, 0x001A92BAu); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
    }
    PUSH32(g_esp, 0x10);
    g_ebp = frame;
    PUSH32(g_esp, 0x001A92C1u); sub_002D57FB();
    g_esp = g_esp + 4;
    if(g_eax!=g_ebx){
    g_xmm0 = XMM_SCALAR(MEMF(0x44C798));
    MEMF(g_eax + 4) = g_xmm0.f[0];
    g_xmm0 = XMM_ZERO();
    MEMF(g_eax + 8) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(frame + 0x14));
    MEM32(g_eax) = 0x477894;
    MEMF(g_eax + 0xC) = g_xmm0.f[0];
    }else g_eax=0;
    PUSH32(g_esp, 0x14);
    MEM32(g_esi + 8) = g_eax;
    g_ebp = frame;
    PUSH32(g_esp, 0x001A92FBu); sub_002D57FB();
    g_esp = g_esp + 4;
    if(g_eax!=g_ebx){
    g_edx = MEM32(frame + 8);
    g_xmm0 = XMM_ZERO();
    MEM32(g_eax) = 0x4778AC;
    MEM32(g_eax + 4) = g_edx;
    MEMF(g_eax + 8) = g_xmm0.f[0];
    MEMF(g_eax + 0xC) = g_xmm0.f[0];
    MEMF(g_eax + 0x10) = g_xmm0.f[0];
    }else g_eax=0;
    MEM32(g_esi + 4) = g_eax;
    g_ecx = MEM32(g_edi);
    g_eax = g_eax + 8;
    MEM32(g_eax) = g_ecx;
    g_edx = MEM32(g_edi + 4);
    MEM32(g_eax + 4) = g_edx;
    g_ecx = MEM32(g_edi + 8);
    PUSH32(g_esp, 0x160);
    MEM32(g_eax + 8) = g_ecx;
    g_ebp = frame;
    PUSH32(g_esp, 0x001A9344u); sub_002D57FB();
    g_esp = g_esp + 4;
    if(g_eax!=g_ebx){
    g_edx = g_esp + 0x20;
    PUSH32(g_esp, g_edx);
    g_ecx = g_eax;
    g_ebp = frame;
    PUSH32(g_esp, 0x001A9357u); sub_00134680();
    }else g_eax=0;
    g_xmm0 = XMM_SCALAR(MEMF(0x44C530));
    MEM32(g_esi + 0xC) = g_eax;
    MEM8(g_eax + 0x110) = 1;
    g_xmm1 = XMM_SCALAR(MEMF(g_esi + 0x180));
    g_eax = MEM32(g_esi + 0xC);
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
    MEMF(g_esp + 0x14) = g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esi + 0x184));
    g_ecx = MEM32(g_esp + 0x14);
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
    MEMF(g_esp + 0x18) = g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esi + 0x188));
    g_edx = MEM32(g_esp + 0x18);
    g_eax = g_eax + 4;
    MEM32(g_eax) = g_ecx;
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
    MEMF(g_esp + 0x1C) = g_xmm1.f[0];
    g_ecx = MEM32(g_esp + 0x1C);
    MEM32(g_eax + 4) = g_edx;
    MEM32(g_eax + 8) = g_ecx;
    g_ecx = MEM32(0x51C4DC);
    g_ebp = frame;
    PUSH32(g_esp, 0x001A93C8u); sub_00208C50();
    g_ecx = MEM32(g_esi + 0xC);
    g_edx = MEM32(g_eax);
    { uint32_t _icall_esp = g_esp;
    PUSH32(g_esp, g_ecx);
    g_ecx = MEM32(g_esi + 4);
    PUSH32(g_esp, g_ecx);
    g_ecx = MEM32(g_esi + 8);
    PUSH32(g_esp, g_ecx);
    g_ecx = g_eax;
    { uint32_t _icall_target = MEM32(g_edx + 0x18); PUSH32(g_esp, 0x001A93DBu); RECOMP_ICALL_SAFE(_icall_target, _icall_esp); }
    }
    g_xmm3 = XMM_SCALAR(MEMF(0x44C530));
    g_xmm2 = XMM_SCALAR(MEMF(g_esi + 0x184));
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 0x188));
    g_xmm1 = XMM_SCALAR(MEMF(g_esi + 0x180));
    g_eax = MEM32(g_esi + 8);
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm2.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * MEMF(0x4778A8);
    g_xmm1 = XMM_SCALAR(MEMF(g_eax + 0xC));
    { double _sse_a = g_xmm0.f[0];
      double _sse_b = g_xmm1.f[0];
      _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
          _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
    }
    if(_sse_flags & 0x41u)g_xmm0=g_xmm1;
    POP32(g_esp, g_edi);
    MEMF(g_eax + 4) = g_xmm0.f[0];
    g_eax = g_esi;
    POP32(g_esp, g_esi);
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_recomp_zf = !!(((_sse_flags & 0x40u) != 0));
    g_esp += 20; return;
}

/* Allocate the effect-local expansion, retaining allocation and construction
 * frontiers and their live argument/saved-frame feedback. */
static void smoke_001a9440(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39796b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,0x18c);g_esi=g_ecx;PUSH32(g_esp,0x1a9465);sub_002D57FB();g_esp+=4;
 MEM32(g_esp+8)=g_eax;int allocated=g_eax!=0;MEM32(g_esp+0x14)=0;
 if(allocated){
  g_edx=MEM32(g_esp+0x20);g_ecx=MEM32(g_esi+0x54);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+0x20);PUSH32(g_esp,g_edx);
  g_edx=g_esp+12;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ecx=g_eax;MEM32(g_esp+0x14)=0x4778b8;PUSH32(g_esp,0x1a949a);sub_001A9140();
  POP32(g_esp,g_esi);g_ecx=MEM32(g_esp+8);MEM32(0)=g_ecx;
 }else{g_ecx=MEM32(g_esp+12);g_eax=0;POP32(g_esp,g_esi);MEM32(0)=g_ecx;}
 g_esp+=0x14;g_recomp_zf=g_esp==0;g_esp+=12;
}

static void smoke_001a94c0(void)
{
    uint32_t frame = g_ebp;
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    PUSH32(g_esp, 0xFFFFFFFFu);
    PUSH32(g_esp, 0x397988);
    g_eax = MEM32(0);
    PUSH32(g_esp, g_eax);
    MEM32(0) = g_esp;
    g_esp = g_esp - 0x58;
    PUSH32(g_esp, g_ebx);
    PUSH32(g_esp, g_esi);
    g_ebx = g_ecx;
    PUSH32(g_esp, g_edi);
    MEM32(g_esp + 0x18) = g_ebx;
    g_ebp = frame;
    PUSH32(g_esp, 0x001A94ECu); sub_0020BDC0();
    g_xmm0 = XMM_ZERO();
    MEMF(g_ebx + 0x80) = g_xmm0.f[0];
    MEMF(g_ebx + 0xB8) = g_xmm0.f[0];
    MEM32(g_ebx) = 0x4778E8;
    MEM8(g_ebx + 0xBC) = 0;
    g_xmm0 = XMM_MEM(0x51E110);
    XMM_STORE(g_ebx + 0x10, g_xmm0);
    g_xmm0 = XMM_MEM(0x51E120);
    XMM_STORE(g_ebx + 0x20, g_xmm0);
    g_xmm0 = XMM_MEM(0x51E130);
    XMM_STORE(g_ebx + 0x30, g_xmm0);
    g_xmm0 = XMM_SCALAR(MEMF(g_ebx + 0x44));
    g_xmm0.f[0] = g_xmm0.f[0] + MEMF(0x44C798);
    g_xmm1 = XMM_SCALAR(MEMF(g_ebx + 0x48));
    g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0x40));
    g_esi = g_ebx + 0x40;
    MEMF(g_esp + 0x24) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44BF94));
    MEMF(g_esp + 0x20) = g_xmm2.f[0];
    MEMF(g_esp + 0x28) = g_xmm1.f[0];
    MEMF(g_esp + 0x2C) = g_xmm0.f[0];
    g_xmm0 = XMM_MEM(g_esp + 0x20);
    PUSH32(g_esp, 0x40E00000);
    PUSH32(g_esp, g_esi);
    g_ecx = g_ebx;
    MEM32(g_esp + 0x74) = 0;
    XMM_STORE(g_esi, g_xmm0);
    g_ebp = frame;
    PUSH32(g_esp, 0x001A9584u); sub_001A9440();
    MEM32(g_ebx + 0x84) = g_eax;
    g_xmm0 = XMM_SCALAR(MEMF(g_esi));
    MEMF(g_esp + 0x30) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 4));
    MEMF(g_esp + 0x34) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 8));
    MEMF(g_esp + 0x38) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi));
    MEMF(g_esp + 0x3C) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 4));
    MEMF(g_esp + 0x40) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 8));
    MEMF(g_esp + 0x44) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi));
    g_eax = g_ebx + 0x88;
    MEMF(g_esp + 0x48) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 4));
    MEMF(g_esp + 0x4C) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 8));
    MEMF(g_esp + 0x50) = g_xmm0.f[0];
    g_xmm0 = XMM_ZERO();
    PUSH32(g_esp, 0);
    g_ecx = 0xC;
    g_esi = g_esp + 0x34;
    g_edi = g_eax;
    g_esp = g_esp - 0x30;
    MEMF(g_esp + 0x88) = g_xmm0.f[0];
    MEMF(g_esp + 0x8C) = g_xmm0.f[0];
    MEMF(g_esp + 0x90) = g_xmm0.f[0];
    memcpy((void*)XBOX_PTR(g_edi), (void*)XBOX_PTR(g_esi), g_ecx * 4);
    g_esi += g_ecx * 4; g_edi += g_ecx * 4; g_ecx = 0;
    g_edx = MEM32(g_eax);
    g_ecx = g_esp;
    MEM32(g_ecx) = g_edx;
    g_edx = MEM32(g_eax + 4);
    MEM32(g_ecx + 4) = g_edx;
    g_edx = MEM32(g_eax + 8);
    MEM32(g_ecx + 8) = g_edx;
    g_edx = MEM32(g_eax + 0xC);
    MEM32(g_ecx + 0xC) = g_edx;
    g_edx = MEM32(g_eax + 0x10);
    MEM32(g_ecx + 0x10) = g_edx;
    g_edx = MEM32(g_eax + 0x14);
    MEM32(g_ecx + 0x14) = g_edx;
    g_edx = MEM32(g_eax + 0x18);
    MEM32(g_esp + 0x50) = g_esp;
    MEM32(g_ecx + 0x18) = g_edx;
    g_edx = MEM32(g_eax + 0x1C);
    MEM32(g_ecx + 0x1C) = g_edx;
    g_edx = MEM32(g_eax + 0x20);
    MEM32(g_ecx + 0x20) = g_edx;
    g_edx = MEM32(g_eax + 0x24);
    MEM32(g_ecx + 0x24) = g_edx;
    g_edx = MEM32(g_eax + 0x28);
    MEM32(g_ecx + 0x28) = g_edx;
    g_eax = MEM32(g_eax + 0x2C);
    MEM32(g_ecx + 0x2C) = g_eax;
    g_eax = MEM32(g_ebx + 0x54);
    g_ecx = MEM32(0x51C4DC);
    PUSH32(g_esp, g_eax);
    g_ebp = frame;
    PUSH32(g_esp, 0x001A967Bu); sub_00055E80();
    g_ecx = g_eax;
    g_ebp = frame;
    PUSH32(g_esp, 0x001A9682u); sub_0004FA00();
    g_ecx = MEM32(g_esp + 0x64);
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    g_eax = g_ebx;
    MEM32(0) = g_ecx;
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_recomp_zf = !!((g_esp == 0));
    g_esp += 8; return;
}

static void smoke_001a96a0(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39796b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);
 PUSH32(g_esp,0xc0);PUSH32(g_esp,0x1a96c0);sub_002D57FB();g_esp+=4;MEM32(g_esp)=g_eax;int allocated=g_eax!=0;MEM32(g_esp+12)=0;
 if(allocated){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x1a96de);sub_001A94C0();}
 else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
/* The smoke emitter has table477894, so the generic emitter gate correctly
 * declines it. Reuse its proved rate/credit algorithm under this exact class,
 * with original ceil/modf/integer conversion services and original scratch. */
extern void sub_002D72D1(void),sub_002D8121(void),sub_002D6780(void);
static void smoke_push(double x){g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=x;}
static void smoke_pop(void){g_fp_top=(g_fp_top+1u)&7u;}
#define SMOKE_FP g_fp_stack[g_fp_top]
static void smoke_emitter(uint32_t address){
 smoke_push(MEMF(g_esp+4));g_esp-=8;
 if(address==0x1a89c0){
  SMOKE_FP*=MEMF(g_ecx+4);MEMD(g_esp)=SMOKE_FP;smoke_pop();
  PUSH32(g_esp,0x1a89d2);sub_002D72D1();g_esp+=8;
  PUSH32(g_esp,0x1a89da);sub_002D6780();g_recomp_zf=g_esp==0;g_esp+=8;
 }else{
  PUSH32(g_esp,g_esi);g_esi=g_ecx;SMOKE_FP*=MEMF(g_esi+4);g_eax=g_esp+4;
  PUSH32(g_esp,g_eax);g_esp-=8;SMOKE_FP+=MEMF(g_esi+8);MEMD(g_esp)=SMOKE_FP;smoke_pop();
  PUSH32(g_esp,0x1a8a00);sub_002D8121();MEMF(g_esi+8)=(float)SMOKE_FP;smoke_pop();smoke_push(MEMD(g_esp+16));g_esp+=12;
  PUSH32(g_esp,0x1a8a0f);sub_002D6780();POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
 }
}
#undef SMOKE_FP
static void smoke_delete(uint32_t address){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;
 if(address==0x1d52d0){PUSH32(g_esp,0x1d52d8);sub_001D52F0();}
 else{PUSH32(g_esp,0x1b2708);sub_000F9130();}
 if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,address==0x1d52d0?0x1d52e5:0x1b2715);sub_002D5CCD();g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;
}
int uc_hybrid_expanding_smoke(uint32_t address){
 if(!smoke_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;unsigned bytes=0;
 switch(address){
 case 0x1a8420:case 0x1a96a0:break;
 case 0x1a8590:bytes=0x20;break;
 case 0x1a8650:case 0x1a86d0:case 0x1a8b20:case 0x1a8c50:case 0x1a9140:bytes=0x18c;break;
 case 0x1a8930:bytes=0x5c;break;
 case 0x1a8970:case 0x1a8d50:case 0x1a9440:case 0x1a94c0:case 0x1b2700:bytes=0xc0;break;
 case 0x1a89c0:case 0x1a89e0:case 0x1a8a20:bytes=0x10;break;
 case 0x1a8a40:case 0x1a8a60:case 0x1d52d0:bytes=0x14;break;
 default:return 0;
 }
 if((s&3)||s<0x12000u||!smoke_span(s-8192,8240)||!smoke_span(0x10000,0x5f0000)||MEM8(0x51c178)||(unsigned)g_fp_top>7||!smoke_apart(s-8192,8240,0x51c178,1))return 0;
 if(bytes&&(!smoke_span(p,bytes)||!smoke_apart(p,bytes,s-8192,8240)||!smoke_apart(p,bytes,0x51c178,1)))return 0;
 if((address==0x1a89c0||address==0x1a89e0)&&MEM32(p)!=0x477894)return 0;
 if(address==0x1d52d0&&MEM32(p)!=0x4778ac)return 0;
 if(address==0x1b2700&&MEM32(p)!=0x4778e8)return 0;
 if(address==0x1a8420){if(!smoke_span(MEM32(s+4),12)||!smoke_span(MEM32(s+8),12)||!smoke_span(MEM32(s+12),12)||!smoke_apart(MEM32(s+4),12,0x51c178,1))return 0;}
 if((address==0x1a8590||address==0x1a8b20)&&!smoke_span(MEM32(s+4),12))return 0;
 if(address==0x1a8650&&(!smoke_span(MEM32(s+4),48)||!smoke_apart(MEM32(s+4),48,0x51c178,1)))return 0;
 if(address==0x1a8930&&!smoke_span(MEM32(s+4),48))return 0;
 if(address==0x1a8a60&&(!smoke_span(MEM32(s+4),64)||!smoke_apart(MEM32(s+4),64,0x51c178,1)))return 0;
 if(address==0x1a9140&&!smoke_span(MEM32(s+12),12))return 0;
 if(address==0x1a9440&&!smoke_span(MEM32(s+4),12))return 0;
 switch(address){
 case 0x1a8420:smoke_001a8420();return 1;
 case 0x1a8590:smoke_001a8590();return 1;
 case 0x1a8650:smoke_001a8650();return 1;
 case 0x1a86d0:smoke_001a86d0();return 1;
 case 0x1a8930:smoke_001a8930();return 1;
 case 0x1a8970:smoke_001a8970();return 1;
 case 0x1a89c0:case 0x1a89e0:smoke_emitter(address);return 1;
 case 0x1a8a20:smoke_001a8a20();return 1;
 case 0x1a8a40:smoke_001a8a40();return 1;
 case 0x1a8a60:smoke_001a8a60();return 1;
 case 0x1a8b20:smoke_001a8b20();return 1;
 case 0x1a8c50:smoke_001a8c50();return 1;
 case 0x1a8d50:smoke_001a8d50();return 1;
 case 0x1a9140:smoke_001a9140();return 1;
 case 0x1a9440:smoke_001a9440();return 1;
 case 0x1a94c0:smoke_001a94c0();return 1;
 case 0x1a96a0:smoke_001a96a0();return 1;
 case 0x1d52d0:case 0x1b2700:smoke_delete(address);return 1;
 default:return 0;
 }
}
