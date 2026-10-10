#ifndef UC_CERTIFICATE_READS_H
#define UC_CERTIFICATE_READS_H
#include <windows.h>
#include <intrin.h>
#ifndef UCRR_CERTIFICATE_READS
#define UCRR_CERTIFICATE_READS 0
#endif
#if UCRR_CERTIFICATE_READS
#if !defined(_M_X64) || !defined(_MSC_VER)
#error Certificate read experiment requires the reviewed Windows x64 compiler contract
#endif
/* Only read-only certificate observations use this scope. Keep publication,
 * fault handlers, arming, disable and all writes on their original interlocked
 * paths. Aligned x64 cached-RAM loads are atomic and ordered load-to-load.
 * Full entry/exit barriers preserve ordering against preceding copy/data reads
 * and surrounding writes; acquire alone would not provide that bracketing.
 * Explicit compiler barriers also avoid relying on /volatile:ms defaults.
 * This is not a coherent snapshot and does not extend certificate lifetime. */
static __forceinline void uc_certificate_fence(void){
    _ReadWriteBarrier();MemoryBarrier();_ReadWriteBarrier();
}
static __forceinline LONG uc_certificate_read32(const volatile LONG *p){
    _ReadWriteBarrier();LONG value=ReadAcquire(p);_ReadWriteBarrier();return value;
}
static __forceinline LONG64 uc_certificate_read64(const volatile LONG64 *p){
    _ReadWriteBarrier();LONG64 value=ReadAcquire64(p);_ReadWriteBarrier();return value;
}
#endif
#endif
