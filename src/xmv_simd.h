/* Integer-only SSE2 implementations of existing MMX helpers. Low 64 bits
 * retain the guest result; no hardware MMX/x87 state or guest memory changes.
 * Enabled only for the translation unit containing the XMV decoder. */
#pragma once
#include "recomp_types.h"
#include <emmintrin.h>
#define XMV_BINARY(name, op) \
static __forceinline uint64_t xmv_##name(uint64_t a,uint64_t b) { \
 return (uint64_t)_mm_cvtsi128_si64(op(_mm_cvtsi64_si128((int64_t)a),_mm_cvtsi64_si128((int64_t)b))); }
XMV_BINARY(addw,_mm_add_epi16)
XMV_BINARY(addd,_mm_add_epi32)
XMV_BINARY(subw,_mm_sub_epi16)
XMV_BINARY(subd,_mm_sub_epi32)
XMV_BINARY(avgb,_mm_avg_epu8)
XMV_BINARY(mullw,_mm_mullo_epi16)
XMV_BINARY(mulhw,_mm_mulhi_epi16)
XMV_BINARY(maddwd,_mm_madd_epi16)
XMV_BINARY(unpacklbw,_mm_unpacklo_epi8)
XMV_BINARY(unpacklwd,_mm_unpacklo_epi16)
XMV_BINARY(sraw,_mm_sra_epi16)
XMV_BINARY(srad,_mm_sra_epi32)
XMV_BINARY(sllw,_mm_sll_epi16)
XMV_BINARY(srlw,_mm_srl_epi16)
XMV_BINARY(slld,_mm_sll_epi32)
XMV_BINARY(srld,_mm_srl_epi32)
#undef XMV_BINARY
static __forceinline uint64_t xmv_unpackhbw(uint64_t a,uint64_t b) {
 return xmv_unpacklbw(a>>32,b>>32);
}
static __forceinline uint64_t xmv_unpackhwd(uint64_t a,uint64_t b) {
 return xmv_unpacklwd(a>>32,b>>32);
}
static __forceinline uint64_t xmv_packuswb(uint64_t a,uint64_t b) {
 __m128i pair=_mm_unpacklo_epi64(_mm_cvtsi64_si128((int64_t)a),_mm_cvtsi64_si128((int64_t)b));
 return (uint64_t)_mm_cvtsi128_si64(_mm_packus_epi16(pair,_mm_setzero_si128()));
}
static __forceinline uint64_t xmv_packssdw(uint64_t a,uint64_t b) {
 __m128i pair=_mm_unpacklo_epi64(_mm_cvtsi64_si128((int64_t)a),_mm_cvtsi64_si128((int64_t)b));
 return (uint64_t)_mm_cvtsi128_si64(_mm_packs_epi32(pair,_mm_setzero_si128()));
}
#define MMX_PADDW xmv_addw
#define MMX_PADDD xmv_addd
#define MMX_PSUBW xmv_subw
#define MMX_PSUBD xmv_subd
#define MMX_PAVGB xmv_avgb
#define MMX_PACKSSDW xmv_packssdw
#define MMX_PSLLD xmv_slld
#define MMX_PSRLD xmv_srld
#define MMX_PMULLW xmv_mullw
#define MMX_PMULHW xmv_mulhw
#define MMX_PMADDWD xmv_maddwd
#define MMX_PUNPCKLBW xmv_unpacklbw
#define MMX_PUNPCKHBW xmv_unpackhbw
#define MMX_PUNPCKLWD xmv_unpacklwd
#define MMX_PUNPCKHWD xmv_unpackhwd
#define MMX_PACKUSWB xmv_packuswb
#define MMX_PSRAW xmv_sraw
#define MMX_PSRAD xmv_srad
#define MMX_PSLLW xmv_sllw
#define MMX_PSRLW xmv_srlw
