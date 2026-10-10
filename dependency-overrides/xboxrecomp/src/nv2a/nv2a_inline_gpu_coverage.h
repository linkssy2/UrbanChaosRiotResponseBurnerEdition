#ifndef NV2A_INLINE_GPU_COVERAGE_H
#define NV2A_INLINE_GPU_COVERAGE_H
#include <stdint.h>
/* One presentation/renderer thread, one bounded capture. No clocks or I/O at
 * draw sites. TLS admission prevents another thread from racing these counters. */
enum {
 UC_INLINE_ATTEMPTS, UC_INLINE_ADMITTED, UC_INLINE_VERTICES,
 UC_INLINE_GPU_DRAWS, UC_INLINE_GPU_VERTICES, UC_INLINE_UPLOAD_FAILURES,
 UC_INLINE_CPU_FALLBACKS, UC_INLINE_DRAW_FAILURES,
 UC_INLINE_INDEXED_FIRST_GPU, UC_INLINE_INDEXED_FIRST_CPU,
 UC_INLINE_INDEXED_FALLBACK_CPU, UC_INLINE_PACKED_FIRST_GPU,
 UC_INLINE_IMMEDIATE_CAPTURED,UC_INLINE_IMMEDIATE_MATERIALIZED,UC_INLINE_IMMEDIATE_DRAWS,UC_INLINE_IMMEDIATE_VERTICES,UC_INLINE_IMMEDIATE_FAILED,UC_INLINE_EFFECT_MEMO_ATTEMPTS,UC_INLINE_EFFECT_MEMO_HITS,UC_INLINE_EFFECT_CULL_QUADS,UC_INLINE_EFFECT_CULLED_QUADS,UC_INLINE_COMPACT_DRAWS,UC_INLINE_COMPACT_PARTICLES,UC_INLINE_COUNT
};
#ifdef UC_INLINE_GPU_COVERAGE_DEFINE
uint64_t uc_inline_gpu_counts[UC_INLINE_COUNT];
unsigned uc_inline_gpu_overflow;
__declspec(thread) int uc_inline_gpu_capture_thread;
#else
extern uint64_t uc_inline_gpu_counts[UC_INLINE_COUNT];
extern unsigned uc_inline_gpu_overflow;
extern __declspec(thread) int uc_inline_gpu_capture_thread;
#endif
#if UCRR_PLAY_ONLY
#define UC_INLINE_GPU_ADD(NAME, VALUE) ((void)0)
#else
#define UC_INLINE_GPU_ADD(NAME, VALUE) do { \
 if(uc_inline_gpu_capture_thread) { \
  uint64_t uc_n=(uint64_t)(VALUE); \
  if(uc_inline_gpu_counts[UC_INLINE_##NAME]>UINT64_MAX-uc_n)uc_inline_gpu_overflow=1; \
  else uc_inline_gpu_counts[UC_INLINE_##NAME]+=uc_n; \
 } \
} while(0)
#endif /* !UCRR_PLAY_ONLY */
#endif
