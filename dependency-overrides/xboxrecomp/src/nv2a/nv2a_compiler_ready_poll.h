#ifndef NV_COMPILER_READY_POLL_H
#define NV_COMPILER_READY_POLL_H
#include <windows.h>
#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

/* A negative hint only: payload/slot consumption still uses the original
 * interlocked ready read and 3->4 job claim. Aligned LONG reads are atomic on
 * Windows x64; the barrier also pins the load under /volatile:iso. A completion
 * published after this read is picked up by a later draw, as before. Other
 * compilers/architectures retain the established interlocked path. */
static __inline LONG nv_compiler_ready_hint(volatile LONG *ready) {
#if defined(_MSC_VER) && defined(_M_X64) && !defined(NV_READY_FORCE_INTERLOCKED)
    LONG value = *ready;
    _ReadWriteBarrier();
    return value;
#else
    return InterlockedCompareExchange(ready, 0, 0);
#endif
}
#endif
