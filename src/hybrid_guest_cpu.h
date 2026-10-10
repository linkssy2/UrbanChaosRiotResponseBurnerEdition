#ifndef UC_HYBRID_GUEST_CPU_H
#define UC_HYBRID_GUEST_CPU_H
#include "recomp_types.h"
#include <immintrin.h>
#include <string.h>
typedef struct {
    uint32_t r[9]; RecompXmm x[8]; uint64_t mm[8]; double fp[8];
    int top,cmp,zf; uint16_t cw; unsigned csr;
} HybridCPU;
static void save_cpu(HybridCPU*s) {
    uint32_t r[]={g_eax,g_ecx,g_edx,g_esp,g_ebx,g_esi,g_edi,g_ebp,g_seh_ebp};
    RecompXmm x[]={g_xmm0,g_xmm1,g_xmm2,g_xmm3,g_xmm4,g_xmm5,g_xmm6,g_xmm7};
    memcpy(s->r,r,sizeof(r));memcpy(s->x,x,sizeof(x));memcpy(s->mm,g_mmx,sizeof(s->mm));
    memcpy(s->fp,g_fp_stack,sizeof(s->fp));s->top=g_fp_top;s->cmp=g_fp_cmp;
    s->zf=g_recomp_zf;s->cw=g_fp_control_word;s->csr=_mm_getcsr();
}
static void restore_cpu(const HybridCPU*s) {
    g_eax=s->r[0];g_ecx=s->r[1];g_edx=s->r[2];g_esp=s->r[3];g_ebx=s->r[4];
    g_esi=s->r[5];g_edi=s->r[6];g_ebp=s->r[7];g_seh_ebp=s->r[8];
    g_xmm0=s->x[0];g_xmm1=s->x[1];g_xmm2=s->x[2];g_xmm3=s->x[3];
    g_xmm4=s->x[4];g_xmm5=s->x[5];g_xmm6=s->x[6];g_xmm7=s->x[7];
    memcpy(g_mmx,s->mm,sizeof(s->mm));memcpy(g_fp_stack,s->fp,sizeof(s->fp));
    g_fp_top=s->top;g_fp_cmp=s->cmp;g_recomp_zf=s->zf;g_fp_control_word=s->cw;_mm_setcsr(s->csr);
}
#endif
