/**
 * Xbox Static Recompilation - Runtime Type Definitions
 *
 * Type definitions and helper macros used by mechanically translated
 * x86 -> C code. Each original x86 function is translated to a C
 * function that uses these types and macros.
 *
 * This is a reusable template for ANY Xbox game. Game-specific
 * customization should go in separate headers.
 *
 * Memory model:
 *   Xbox data sections are mapped to their original VAs via
 *   CreateFileMapping + MapViewOfFileEx (see xbox_memory.h).
 *   Recompiled code accesses globals via pointer casts, e.g.:
 *     *(uint32_t*)0x003B2360
 *
 * Register model:
 *   Volatile registers (eax, ecx, edx, esp) are global variables,
 *   matching real x86 behavior where these registers are shared
 *   across all code. This enables correct argument passing via the
 *   simulated stack and return value communication via eax.
 *
 *   Callee-saved registers (ebx, esi, edi) are also global because
 *   callers pass implicit parameters through them (e.g. 'this' via
 *   esi in thiscall). The callee-save contract is enforced by
 *   PUSH32/POP32 instructions in the generated code, not by C local
 *   variable scoping.
 *
 *   ebp is NOT global - it stays local in each function because many
 *   FPO (Frame Pointer Omission) functions use it as scratch without
 *   save/restore. For SEH functions, g_seh_ebp bridges the gap.
 *
 * Calling convention:
 *   All translated functions are void(void). Arguments are passed
 *   on the simulated Xbox stack (via push instructions before call).
 *   Return values are communicated through g_eax.
 *   The call instruction pushes the real guest return address (the VA of
 *   the instruction after the call); ret discards it with esp += 4.
 *   The value is never used to transfer control -- control flow is C
 *   call/return -- but it must be correct because guest code reads it
 *   (__SEH_prolog's scope table, _alloca probes, "mov eax, [esp]").
 */

#ifndef RECOMP_TYPES_H
#define RECOMP_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* math.h is load-bearing, and its absence was invisible.
 *
 * The lifter emits sqrt() and fabs() for fsqrt/fabs -- 146 of them in one of
 * Halo's nine chunks alone -- and now sin/cos/tan/atan2/log2/exp2/fmod for the
 * x87 transcendentals. With no declaration in scope, C89 implicit declaration
 * makes every one of them return `int`: the caller reads EAX instead of XMM0 and
 * gets garbage, then converts that garbage to double. Vector normalisation is
 * 1/sqrt(x), so this corrupts every matrix the title builds.
 *
 * Nothing reported it because generated code is compiled with /w (see the game
 * CMakeLists) -- MSVC's C4013 was emitted and discarded. Same failure as the
 * missing stdlib.h in kernel_bridge.c, in a hotter path. */
#include <math.h>

/* MSVC's __forceinline -> gcc/clang equivalent on POSIX. */
#if !defined(_MSC_VER) && !defined(__forceinline)
#define __forceinline inline __attribute__((always_inline))
#endif

/* MSVC's __debugbreak() intrinsic -> gcc/clang equivalent.
 * The auto-generated code emits __debugbreak for x86 INT 3 instructions. */
#if !defined(_MSC_VER) && !defined(__debugbreak)
#define __debugbreak() __builtin_trap()
#endif

/* ================================================================
 * Memory offset
 * ================================================================ */

/**
 * Memory offset from Xbox VA to actual mapped address.
 * When Xbox memory is mapped at the original address (0x00010000),
 * this is 0 and the MEM macros are simple identity casts.
 * When mapped elsewhere, this adjusts all memory accesses.
 *
 * Set once during memory initialization, then read-only.
 */
extern ptrdiff_t g_xbox_mem_offset;

/* ================================================================
 * Global registers
 * ================================================================ */

/**
 * Volatile x86 registers (caller-saved):
 *   eax - return values, general accumulator
 *   ecx - 'this' pointer for thiscall, loop counter
 *   edx - high dword of multiply/divide, general
 *   esp - stack pointer (initialized to top of Xbox stack)
 *
 * Callee-saved x86 registers (also global):
 *   ebx, esi, edi - global because callers pass implicit parameters
 *   through them. The callee-save contract is enforced by generated
 *   PUSH32/POP32 instructions.
 *
 * NOT global: ebp - stays local in each function because FPO
 * functions use it as scratch. For SEH, g_seh_ebp bridges the gap.
 */
/* Per-thread register state.
 *
 * These started as plain globals, which works exactly as long as one thread
 * runs recompiled code. Halo is the first title to create real workers (its
 * cache/file loader), and the runtime papered over that by running every worker
 * synchronously inside PsCreateSystemThreadEx -- so a worker that blocks
 * waiting for work never returns and startup deadlocks.
 *
 * On hardware each thread has its own register set, so model it that way.
 * Thread-local costs an indirection per access; a deadlock costs the title. */
#if defined(_MSC_VER)
#  define RECOMP_TLS __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#  define RECOMP_TLS __thread
#else
#  define RECOMP_TLS _Thread_local
#endif

extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_esp;
extern RECOMP_TLS uint32_t g_ebx, g_esi, g_edi;

/**
 * Return a monotonic Xbox-compatible time-stamp counter.  Original hardware
 * increments RDTSC at the fixed 733.333333 MHz CPU frequency; the runtime
 * scales the host performance counter to that rate so XDK timing conversions
 * remain valid on every host CPU.
 */
uint64_t recomp_read_tsc(void);

/* x87 stack. Per-thread for the same reason the integer registers are:
 * arguments are passed in st(0)/st(1) across call boundaries. */
extern RECOMP_TLS double g_fp_stack[8];
extern RECOMP_TLS int g_fp_top;

/**
 * SEH frame pointer bridge.
 *
 * __SEH_prolog sets up ebp for the caller, but since ebp is a local
 * variable in each function, the caller can't see the prolog's change.
 * The prolog writes g_seh_ebp, and the caller reads it after the call.
 * Similarly, __SEH_epilog reads g_seh_ebp at entry and writes it at exit.
 */
extern RECOMP_TLS uint32_t g_seh_ebp;
extern RECOMP_TLS uint32_t g_ebp;

/* x87 control and status. Thread-local for the same reason the x87 stack
   above is: one guest routine can lift to several C functions, so a compare
   and the FNSTSW that reads it can land in different bodies, and the control
   word has to survive a call. (g_fp_stack/g_fp_top are declared above.) */
extern RECOMP_TLS uint16_t g_fp_control_word;
extern RECOMP_TLS int g_fp_cmp;
/* FRNDINT uses guest RC, never the host process rounding mode. The software
 * x87 stack stores doubles; this does not emulate 80-bit precision/exceptions. */
static inline double RECOMP_X87_ROUND(double value) {
    if (!isfinite(value) || value == 0.0) return value;
    switch ((g_fp_control_word >> 10) & 3u) {
    case 1: return floor(value);
    case 2: return ceil(value);
    case 3: return trunc(value);
    default: {
        double magnitude = fabs(value);
        if (magnitude >= 4503599627370496.0) return value;
        double integer = floor(magnitude), fraction = magnitude - integer;
        if (fraction > 0.5 || (fraction == 0.5 && fmod(integer, 2.0) != 0.0))
            integer += 1.0;
        return copysign(integer, value);
    }
    }
}
/* Last recoverable guest zero flag, published at RET for CRT helpers that
 * intentionally communicate a classification result through EFLAGS. */
extern RECOMP_TLS int g_recomp_zf;

/* Result of an x87 compare, in the shape the status word wants:
 *   -1 less, 0 equal, 1 greater, 2 unordered (either operand is NaN).
 * The unordered case is not a curiosity: `fucompp` of a value with itself
 * followed by `test ah, 0x44; jp` is how this era's CRT asks "is this a NaN",
 * and collapsing it to "equal" answers no every time. */
#define RECOMP_FCMP(a, b)     (((a) != (a) || (b) != (b)) ? 2 : (a) < (b) ? -1 : (a) > (b) ? 1 : 0)

/* ================================================================
 * ICALL trace ring buffer (for debugging indirect calls)
 * ================================================================ */

/** Size of the ring buffer (must be power of 2). */
#define ICALL_TRACE_SIZE 16

/** Ring buffer of recent indirect call target VAs. */
extern RECOMP_TLS volatile uint32_t g_icall_trace[ICALL_TRACE_SIZE];

/** Current write index into the ring buffer. */
extern RECOMP_TLS volatile uint32_t g_icall_trace_idx;

/** Total count of indirect calls executed. */
extern RECOMP_TLS volatile uint64_t g_icall_count;

/**
 * Called when an indirect call target cannot be resolved.
 * Implement this in your game-specific code to log diagnostics.
 * The va parameter is the Xbox VA that failed to resolve.
 */
void recomp_icall_fail_log(uint32_t va);

/* Indirect-branch target feedback. The ring buffer above is crash forensics --
 * 16 entries, overwritten constantly. This is a durable, deduplicated record of
 * every target the title ever reached, for feeding back into the next codegen
 * run (tools/recomp/icall_feedback.py).
 *
 * The header is pulled in only when the feature is on, so a default build needs
 * neither the file nor src/kernel on its include path. Disabled,
 * RECOMP_ICALL_OBSERVE discards its arguments without expanding them, so the
 * RECOMP_ICALL_SEEN_* constants need not exist either. */
#ifdef RECOMP_ICALL_FEEDBACK
#include "recomp_icall_feedback.h"
#else
#define RECOMP_ICALL_OBSERVE(va, flags) ((void)0)
#endif

/**
 * Function entry trace, emitted only for addresses passed to
 * tools.recomp --trace-functions. Bring-up is largely "which of these
 * init calls does it not come back from", and answering that by
 * overriding a function loses the body you were trying to observe.
 */
void recomp_trace_enter(const char *name, uint32_t va);
#define RECOMP_TRACE_ENTER(name, va) recomp_trace_enter((name), (va))
void recomp_trace_exit(const char *name, uint32_t va);
#define RECOMP_TRACE_EXIT(name, va) recomp_trace_exit((name), (va))
void recomp_trace_esp(const char *name, const char *tag);
#define RECOMP_TRACE_ESP(name, tag) recomp_trace_esp((name), (tag))
void recomp_trace_mode_reject(
    uint32_t width, uint32_t height, uint32_t format, uint32_t flags,
    uint32_t requested_mask, uint32_t table, uint32_t table_entry,
    uint32_t entry_bits, uint32_t entry_width, uint32_t entry_height,
    uint32_t entry_extra);
void recomp_trace_av_selector(
    const char *stage, uint32_t capabilities, uint32_t index,
    uint32_t cursor, uint32_t selected);
void recomp_ctor_cursor_trace(uint32_t target, uint32_t before, uint32_t after);
void recomp_parser_trace(const char *stage);
void recomp_smartptr_trace(uint32_t original_arg, uint32_t adjusted,
                           uint32_t vtable, uint32_t target);
void recomp_dsound_trace(const char *stage, uint32_t value, uint32_t object);
void recomp_stream_copy_trace(const char *stage, uint32_t object,
                              uint32_t destination, uint32_t requested,
                              uint32_t available, uint32_t result);
void recomp_loader_count_trace(const char *stage, uint32_t stream,
                               uint32_t count, uint32_t remaining);
void recomp_loader_fields_trace(uint32_t stream, uint32_t total,
                                uint32_t first, uint32_t second,
                                uint32_t third, uint32_t fourth);
void recomp_resource_lookup_trace(const char *stage, uint32_t path,
                                  uint32_t target, uint32_t value);
void recomp_tree_lookup_trace(const char *stage, uint32_t query,
                              uint32_t node);
void recomp_tree_compare_trace(uint32_t query, uint32_t node,
                               int32_t result);
void recomp_archive_index_trace(const char *stage, uint32_t path,
                                uint32_t tree, uint32_t count,
                                uint32_t name, uint32_t value);
void recomp_tree_insert_trace(const char *stage, uint32_t tree,
                              uint32_t item, uint32_t first,
                              uint32_t second);
void recomp_tree_loop_trace(uint32_t tree, uint32_t key, uint32_t node,
                            uint32_t parent, uint32_t steps);
void recomp_archive_valid_trace(uint32_t path, uint32_t index,
                                uint32_t reader, uint32_t entry, uint32_t vtable,
                                uint32_t target, uint32_t result);
void recomp_archive_fetch_trace(const char *stage, uint32_t path,
                                uint32_t index, uint32_t object,
                                uint32_t vtable, uint32_t target,
                                uint32_t result);
void recomp_archive_name_trace(const char *stage, uint32_t query,
                               uint32_t candidate, int32_t result,
                               int32_t index, int32_t cached);
void recomp_object_fields_trace(const char *stage, uint32_t object);


/* ================================================================
 * Memory access helpers
 * ================================================================ */

/**
 * Translate an Xbox VA to an actual pointer.
 * Mask to 32-bit first: Xbox addresses are 32-bit and arithmetic
 * in the recompiled code can overflow. Without the mask, a 64-bit
 * uintptr_t cast preserves the overflow bits, landing us 4GB+ past
 * our mapping and causing access violations.
 */
/**
 * Convert the Xbox's write-combined physical-RAM alias back to the base
 * 64 MB mapping.  The XDK D3D lock helpers return pointers in the
 * 0xF0000000-0xFCFFFFFF range (for example 0xF17D9400), where the low
 * 26 address bits select physical RAM.  Mapping every alias all the way to
 * 4 GB is fragile on Windows, so normalize this hardware alias explicitly.
 * 0xFD000000 and above are real GPU/kernel/device apertures and must retain
 * their original addresses.
 */
#if defined(UCRR_GUEST_ADDRESS_SPLIT)
/* Keep the common ordinary-address path off the mask/select dependency chain.
 * No memory is accessed here. The rare high-address path retains the exact
 * alias interval, mask and device-aperture behavior of the original helper. */
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
static uint32_t recomp_normalize_xbox_alias(uint32_t va)
{
    if (va >= 0xF0000000u && va < 0xFD000000u) return va & 0x07FFFFFFu;
    return va;
}
#endif
static inline uint32_t recomp_normalize_xbox_va(uint32_t va)
{
#if defined(UCRR_GUEST_ADDRESS_SPLIT)
    if (va < 0xF0000000u) return va;
    return recomp_normalize_xbox_alias(va);
#else
    if (va >= 0xF0000000u && va < 0xFD000000u) {
        /* Preserve the upper physical bit for 128 MB diagnostic/devkit RAM.
         * Retail's 64 MB mirror mapping still provides its wrap semantics. */
        return va & 0x07FFFFFFu;
    }
    return va;
#endif
}

#define XBOX_PTR(addr) ((uintptr_t)recomp_normalize_xbox_va((uint32_t)(addr)) + g_xbox_mem_offset)

/** Read/write N bytes at a flat Xbox memory address. */
#define MEM8(addr)   (*(volatile uint8_t  *)XBOX_PTR(addr))
#define MEM16(addr)  (*(volatile uint16_t *)XBOX_PTR(addr))
#define MEM32(addr)  (*(volatile uint32_t *)XBOX_PTR(addr))
#ifndef RECOMP_MEMORY_READ_POLL
extern void xbox_NotifyMemoryReadPoll(uint32_t address);
#define RECOMP_MEMORY_READ_POLL(addr) xbox_NotifyMemoryReadPoll((uint32_t)(addr))
#endif
#ifndef RECOMP_PFB_FLUSH_WRITE
extern void xbox_NotifyGpuProgressRead(uint32_t address);
#define RECOMP_GPU_PROGRESS_READ(addr) xbox_NotifyGpuProgressRead((uint32_t)(addr))
extern void xbox_NotifyPfbFlushWrite(uint32_t address);
#define RECOMP_PFB_FLUSH_WRITE(addr) xbox_NotifyPfbFlushWrite((uint32_t)(addr))
#endif
#define MEM64(addr)  (*(volatile uint64_t *)XBOX_PTR(addr))

/** Signed memory reads. */
#define SMEM8(addr)  (*(volatile int8_t   *)XBOX_PTR(addr))
#define SMEM16(addr) (*(volatile int16_t  *)XBOX_PTR(addr))
#define SMEM32(addr) (*(volatile int32_t  *)XBOX_PTR(addr))
#define SMEM64(addr) (*(volatile int64_t  *)XBOX_PTR(addr))

/** Float/double memory access. */
#define MEMF(addr)   (*(volatile float    *)XBOX_PTR(addr))
#define MEMD(addr)   (*(volatile double   *)XBOX_PTR(addr))

/* ================================================================
 * MMX register helpers
 *
 * MMX values are represented by the generated code as uint64_t.  The
 * helpers below reinterpret that value as the packed byte/word/dword lanes
 * used by the Xbox video codecs.  Keeping the operations here, rather than
 * expanding them into generated C, makes the lifter output both readable
 * and bit-exact about wraparound, saturation, and variable shift counts.
 * ================================================================ */

typedef union RecompMmx {
    uint64_t u64;
    int64_t  i64;
    uint8_t  u8[8];
    int8_t   i8[8];
    uint16_t u16[4];
    int16_t  i16[4];
    uint32_t u32[2];
    int32_t  i32[2];
} RecompMmx;

static inline uint64_t MMX_MEM(uint32_t addr) {
    RecompMmx r;
    r.u32[0] = MEM32(addr);
    r.u32[1] = MEM32(addr + 4);
    return r.u64;
}

static inline void MMX_STORE(uint32_t addr, uint64_t value) {
    RecompMmx v; v.u64 = value;
    MEM32(addr) = v.u32[0];
    MEM32(addr + 4) = v.u32[1];
}

#define RECOMP_MMX_BINARY(name, count, member, expr)                       \
    static inline uint64_t name(uint64_t av, uint64_t bv) {                \
        RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;                  \
        for (i = 0; i < (count); ++i) r.member[i] = (expr);                 \
        return r.u64;                                                       \
    }

RECOMP_MMX_BINARY(MMX_PADDB, 8, u8,  (uint8_t)(a.u8[i] + b.u8[i]))
RECOMP_MMX_BINARY(MMX_PADDW, 4, u16, (uint16_t)(a.u16[i] + b.u16[i]))
RECOMP_MMX_BINARY(MMX_PADDD, 2, u32, a.u32[i] + b.u32[i])
RECOMP_MMX_BINARY(MMX_PSUBB, 8, u8,  (uint8_t)(a.u8[i] - b.u8[i]))
RECOMP_MMX_BINARY(MMX_PSUBW, 4, u16, (uint16_t)(a.u16[i] - b.u16[i]))
RECOMP_MMX_BINARY(MMX_PSUBD, 2, u32, a.u32[i] - b.u32[i])
RECOMP_MMX_BINARY(MMX_PMULLW, 4, u16,
                  (uint16_t)((uint32_t)a.u16[i] * (uint32_t)b.u16[i]))
RECOMP_MMX_BINARY(MMX_PMULHW, 4, u16,
                  (uint16_t)(((int32_t)a.i16[i] * (int32_t)b.i16[i]) >> 16))
RECOMP_MMX_BINARY(MMX_PMULHUW, 4, u16,
                  (uint16_t)(((uint32_t)a.u16[i] * (uint32_t)b.u16[i]) >> 16))
RECOMP_MMX_BINARY(MMX_PCMPEQB, 8, u8,
                  a.u8[i] == b.u8[i] ? UINT8_MAX : 0)
RECOMP_MMX_BINARY(MMX_PCMPEQW, 4, u16,
                  a.u16[i] == b.u16[i] ? UINT16_MAX : 0)
RECOMP_MMX_BINARY(MMX_PCMPEQD, 2, u32,
                  a.u32[i] == b.u32[i] ? UINT32_MAX : 0)
RECOMP_MMX_BINARY(MMX_PCMPGTB, 8, u8,
                  a.i8[i] > b.i8[i] ? UINT8_MAX : 0)
RECOMP_MMX_BINARY(MMX_PCMPGTW, 4, u16,
                  a.i16[i] > b.i16[i] ? UINT16_MAX : 0)
RECOMP_MMX_BINARY(MMX_PCMPGTD, 2, u32,
                  a.i32[i] > b.i32[i] ? UINT32_MAX : 0)
RECOMP_MMX_BINARY(MMX_PAVGB, 8, u8,
                  (uint8_t)(((uint16_t)a.u8[i] + b.u8[i] + 1u) >> 1))
RECOMP_MMX_BINARY(MMX_PMINSW, 4, i16,
                  a.i16[i] < b.i16[i] ? a.i16[i] : b.i16[i])
RECOMP_MMX_BINARY(MMX_PMAXSW, 4, i16,
                  a.i16[i] > b.i16[i] ? a.i16[i] : b.i16[i])

#undef RECOMP_MMX_BINARY

static inline uint64_t MMX_PMADDWD(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 2; ++i) {
        int64_t sum = (int64_t)a.i16[i * 2] * b.i16[i * 2]
                    + (int64_t)a.i16[i * 2 + 1] * b.i16[i * 2 + 1];
        r.u32[i] = (uint32_t)sum;
    }
    return r.u64;
}

static inline uint64_t MMX_PAND(uint64_t a, uint64_t b)  { return a & b; }
static inline uint64_t MMX_PANDN(uint64_t a, uint64_t b) { return (~a) & b; }
static inline uint64_t MMX_POR(uint64_t a, uint64_t b)   { return a | b; }
static inline uint64_t MMX_PXOR(uint64_t a, uint64_t b)  { return a ^ b; }

static inline uint64_t MMX_PSLLW(uint64_t value, uint64_t count) {
    RecompMmx a, r; int i; a.u64 = value;
    if (count > 15) return 0;
    for (i = 0; i < 4; ++i) r.u16[i] = (uint16_t)(a.u16[i] << count);
    return r.u64;
}
static inline uint64_t MMX_PSLLD(uint64_t value, uint64_t count) {
    RecompMmx a, r; int i; a.u64 = value;
    if (count > 31) return 0;
    for (i = 0; i < 2; ++i) r.u32[i] = a.u32[i] << count;
    return r.u64;
}
static inline uint64_t MMX_PSLLQ(uint64_t value, uint64_t count) {
    return count > 63 ? 0 : value << count;
}
static inline uint64_t MMX_PSRLW(uint64_t value, uint64_t count) {
    RecompMmx a, r; int i; a.u64 = value;
    if (count > 15) return 0;
    for (i = 0; i < 4; ++i) r.u16[i] = (uint16_t)(a.u16[i] >> count);
    return r.u64;
}
static inline uint64_t MMX_PSRLD(uint64_t value, uint64_t count) {
    RecompMmx a, r; int i; a.u64 = value;
    if (count > 31) return 0;
    for (i = 0; i < 2; ++i) r.u32[i] = a.u32[i] >> count;
    return r.u64;
}
static inline uint64_t MMX_PSRLQ(uint64_t value, uint64_t count) {
    return count > 63 ? 0 : value >> count;
}
static inline uint64_t MMX_PSRAW(uint64_t value, uint64_t count) {
    RecompMmx a, r; int i; a.u64 = value;
    if (count > 15) count = 15;
    for (i = 0; i < 4; ++i) r.i16[i] = (int16_t)(a.i16[i] >> count);
    return r.u64;
}
static inline uint64_t MMX_PSRAD(uint64_t value, uint64_t count) {
    RecompMmx a, r; int i; a.u64 = value;
    if (count > 31) count = 31;
    for (i = 0; i < 2; ++i) r.i32[i] = a.i32[i] >> count;
    return r.u64;
}

static inline uint64_t MMX_PSHUFW(uint64_t value, uint32_t imm) {
    RecompMmx a, r; int i; a.u64 = value;
    for (i = 0; i < 4; ++i) r.u16[i] = a.u16[(imm >> (i * 2)) & 3u];
    return r.u64;
}

static inline uint64_t MMX_PUNPCKLBW(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 4; ++i) { r.u8[i * 2] = a.u8[i]; r.u8[i * 2 + 1] = b.u8[i]; }
    return r.u64;
}
static inline uint64_t MMX_PUNPCKHBW(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 4; ++i) { r.u8[i * 2] = a.u8[i + 4]; r.u8[i * 2 + 1] = b.u8[i + 4]; }
    return r.u64;
}
static inline uint64_t MMX_PUNPCKLWD(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 2; ++i) { r.u16[i * 2] = a.u16[i]; r.u16[i * 2 + 1] = b.u16[i]; }
    return r.u64;
}
static inline uint64_t MMX_PUNPCKHWD(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 2; ++i) { r.u16[i * 2] = a.u16[i + 2]; r.u16[i * 2 + 1] = b.u16[i + 2]; }
    return r.u64;
}
static inline uint64_t MMX_PUNPCKLDQ(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; a.u64 = av; b.u64 = bv;
    r.u32[0] = a.u32[0]; r.u32[1] = b.u32[0]; return r.u64;
}
static inline uint64_t MMX_PUNPCKHDQ(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; a.u64 = av; b.u64 = bv;
    r.u32[0] = a.u32[1]; r.u32[1] = b.u32[1]; return r.u64;
}

static inline int8_t recomp_sat_i8(int32_t value) {
    return (int8_t)(value < -128 ? -128 : value > 127 ? 127 : value);
}
static inline int16_t recomp_sat_i16(int64_t value) {
    return (int16_t)(value < -32768 ? -32768 : value > 32767 ? 32767 : value);
}
static inline uint8_t recomp_sat_u8(int32_t value) {
    return (uint8_t)(value < 0 ? 0 : value > 255 ? 255 : value);
}
static inline uint64_t MMX_PACKSSWB(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 4; ++i) { r.i8[i] = recomp_sat_i8(a.i16[i]); r.i8[i + 4] = recomp_sat_i8(b.i16[i]); }
    return r.u64;
}
static inline uint64_t MMX_PACKSSDW(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 2; ++i) { r.i16[i] = recomp_sat_i16(a.i32[i]); r.i16[i + 2] = recomp_sat_i16(b.i32[i]); }
    return r.u64;
}
static inline uint64_t MMX_PACKUSWB(uint64_t av, uint64_t bv) {
    RecompMmx a, b, r; int i; a.u64 = av; b.u64 = bv;
    for (i = 0; i < 4; ++i) { r.u8[i] = recomp_sat_u8(a.i16[i]); r.u8[i + 4] = recomp_sat_u8(b.i16[i]); }
    return r.u64;
}
static inline uint32_t MMX_PMOVMSKB(uint64_t value) {
    RecompMmx a; uint32_t r = 0; int i; a.u64 = value;
    for (i = 0; i < 8; ++i) r |= ((uint32_t)a.u8[i] >> 7) << i;
    return r;
}

/* ================================================================
 * SSE / XMM register state
 *
 * XMM is 128 bits of architectural state, not a scalar float. Modelling
 * it as a `float` made movaps/movups transfer 4 of 16 bytes and silently
 * drop the upper three lanes, and left the packed arithmetic with no
 * representation at all.
 *
 * The registers are global for the same reason the volatile GPRs are:
 * one guest routine can lift to several C functions, so a value produced
 * in one body and read in the next has to outlive the body that wrote it.
 * A function-local declaration would also shadow these, and the local
 * starts zeroed -- a returned float would silently read as 0.0.
 *
 * The helpers are lane-wise C rather than host intrinsics: the guest
 * semantics stay explicit (MINPS returning src on unordered, CMPNEQPS
 * being the unordered form) and the header stays portable.
 * ================================================================ */

#ifndef RECOMP_XMM_DEFINED
#define RECOMP_XMM_DEFINED
typedef union RecompXmm {
    float    f[4];
    double   d[2];
    uint32_t u[4];
    int32_t  i[4];
    uint64_t q[2];
} RecompXmm;
#endif

static inline RecompXmm MMX_CVTPI2PS(RecompXmm dst, uint64_t value) {
    RecompMmx v; v.u64 = value;
    dst.f[0] = (float)v.i32[0]; dst.f[1] = (float)v.i32[1];
    return dst;
}

/* Adapted from XboxRecomp 9f064b8ab167: scalar SSE respects host MXCSR
 * and returns integer indefinite without an undefined out-of-range C cast.
 * Guest MXCSR load/save and unmasked exceptions remain separate contracts. */
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
static inline int32_t MMX_CVT_F2I(float value) {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    return _mm_cvtss_si32(_mm_set_ss(value));
#else
    double rounded = nearbyint((double)value);
    if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0))
        return INT32_MIN;
    return (int32_t)rounded;
#endif
}
static inline uint64_t MMX_CVTPS2PI(RecompXmm value) {
    RecompMmx r;
    r.i32[0] = MMX_CVT_F2I(value.f[0]);
    r.i32[1] = MMX_CVT_F2I(value.f[1]);
    return r.u64;
}

extern RECOMP_TLS RecompXmm g_xmm0, g_xmm1, g_xmm2, g_xmm3;
extern RECOMP_TLS RecompXmm g_xmm4, g_xmm5, g_xmm6, g_xmm7;
/* Preserve MMX across guest calls. Raw x87/MMX aliasing remains unmodeled. */
extern RECOMP_TLS uint64_t g_mmx[8];
#define mm0 g_mmx[0]
#define mm1 g_mmx[1]
#define mm2 g_mmx[2]
#define mm3 g_mmx[3]
#define mm4 g_mmx[4]
#define mm5 g_mmx[5]
#define mm6 g_mmx[6]
#define mm7 g_mmx[7]

/* -- construction -- */

static inline RecompXmm XMM_ZERO(void) {
    RecompXmm r; r.q[0] = 0; r.q[1] = 0; return r;
}

/** movss from memory: lane 0 set, upper lanes zeroed. */
static inline RecompXmm XMM_SCALAR(float v) {
    RecompXmm r = XMM_ZERO(); r.f[0] = v; return r;
}

/** movsd from memory: low double set, high double zeroed. */
static inline RecompXmm XMM_SCALAR_DOUBLE(double v) {
    RecompXmm r = XMM_ZERO(); r.d[0] = v; return r;
}

/** movd: 32 raw bits into lane 0, upper lanes zeroed. */
static inline RecompXmm XMM_SCALAR_BITS(uint32_t bits) {
    RecompXmm r = XMM_ZERO(); r.u[0] = bits; return r;
}

/* -- guest memory --
 * Addresses are guest VAs, so they go through MEM32 like every other
 * access. Done lane-wise, which is also unaligned-safe for movups. */

static inline RecompXmm XMM_MEM(uint32_t addr) {
    RecompXmm r;
    r.u[0] = MEM32(addr);      r.u[1] = MEM32(addr + 4);
    r.u[2] = MEM32(addr + 8);  r.u[3] = MEM32(addr + 12);
    return r;
}

static inline void XMM_STORE(uint32_t addr, RecompXmm v) {
    MEM32(addr)      = v.u[0]; MEM32(addr + 4)  = v.u[1];
    MEM32(addr + 8)  = v.u[2]; MEM32(addr + 12) = v.u[3];
}

/* movlps/movhps move 8 bytes into or out of one half, leaving the
 * other half alone. */
#define XMM_LOAD_LOW(dst, addr)   recomp_xmm_load_half(&(dst), (addr), 0)
#define XMM_LOAD_HIGH(dst, addr)  recomp_xmm_load_half(&(dst), (addr), 1)
#define XMM_STORE_LOW(addr, src)  recomp_xmm_store_half((addr), (src), 0)
#define XMM_STORE_HIGH(addr, src) recomp_xmm_store_half((addr), (src), 1)

static inline void recomp_xmm_load_half(RecompXmm *dst, uint32_t addr,
                                        int high) {
    dst->u[high * 2]     = MEM32(addr);
    dst->u[high * 2 + 1] = MEM32(addr + 4);
}

static inline void recomp_xmm_store_half(uint32_t addr, RecompXmm src,
                                         int high) {
    MEM32(addr)     = src.u[high * 2];
    MEM32(addr + 4) = src.u[high * 2 + 1];
}

/** movlhps: dst high = src low. */
static inline RecompXmm XMM_MOVE_LOW_TO_HIGH(RecompXmm a, RecompXmm b) {
    RecompXmm r; r.q[0] = a.q[0]; r.q[1] = b.q[0]; return r;
}

/** movhlps: dst low = src high. */
static inline RecompXmm XMM_MOVE_HIGH_TO_LOW(RecompXmm a, RecompXmm b) {
    RecompXmm r; r.q[0] = b.q[1]; r.q[1] = a.q[1]; return r;
}

/* -- packed arithmetic -- */

#define RECOMP_XMM_LANEWISE(name, expr)                                   \
    static inline RecompXmm name(RecompXmm a, RecompXmm b) {              \
        RecompXmm r; int i;                                               \
        for (i = 0; i < 4; ++i) { (void)a; (void)b; r.f[i] = (expr); }    \
        return r;                                                         \
    }

RECOMP_XMM_LANEWISE(XMM_ADD, a.f[i] + b.f[i])
RECOMP_XMM_LANEWISE(XMM_SUB, a.f[i] - b.f[i])
RECOMP_XMM_LANEWISE(XMM_MUL, a.f[i] * b.f[i])
RECOMP_XMM_LANEWISE(XMM_DIV, a.f[i] / b.f[i])
/* MINPS/MAXPS return the second operand when the lanes are unordered or
 * equal -- that is the hardware's tie-break, not a C fmin/fmax. */
RECOMP_XMM_LANEWISE(XMM_MIN, (a.f[i] < b.f[i]) ? a.f[i] : b.f[i])
RECOMP_XMM_LANEWISE(XMM_MAX, (a.f[i] > b.f[i]) ? a.f[i] : b.f[i])

#define RECOMP_XMM_BITWISE(name, expr)                                    \
    static inline RecompXmm name(RecompXmm a, RecompXmm b) {              \
        RecompXmm r; int i;                                               \
        for (i = 0; i < 4; ++i) { (void)a; (void)b; r.u[i] = (expr); }    \
        return r;                                                         \
    }

RECOMP_XMM_BITWISE(XMM_AND,  a.u[i] & b.u[i])
RECOMP_XMM_BITWISE(XMM_OR,   a.u[i] | b.u[i])
RECOMP_XMM_BITWISE(XMM_XOR,  a.u[i] ^ b.u[i])
/* ANDNPS is ~dst & src, not dst & ~src. */
RECOMP_XMM_BITWISE(XMM_ANDN, (~a.u[i]) & b.u[i])

/* Compares produce an all-ones or all-zero mask per lane. EQ/LT/LE are
 * the ordered forms (false when either lane is NaN); NEQ is the
 * unordered form, so it is true when a lane is NaN. */
RECOMP_XMM_BITWISE(XMM_CMP_EQ,  (a.f[i] == b.f[i]) ? 0xFFFFFFFFu : 0u)
RECOMP_XMM_BITWISE(XMM_CMP_LT,  (a.f[i] <  b.f[i]) ? 0xFFFFFFFFu : 0u)
RECOMP_XMM_BITWISE(XMM_CMP_LE,  (a.f[i] <= b.f[i]) ? 0xFFFFFFFFu : 0u)
RECOMP_XMM_BITWISE(XMM_CMP_NEQ, (a.f[i] == b.f[i]) ? 0u : 0xFFFFFFFFu)

/** movmskps: the four lane sign bits, packed into the low nibble. */
static inline uint32_t XMM_MOVEMASK(RecompXmm a) {
    return ((a.u[0] >> 31) & 1u) | (((a.u[1] >> 31) & 1u) << 1)
         | (((a.u[2] >> 31) & 1u) << 2) | (((a.u[3] >> 31) & 1u) << 3);
}

/** shufps: lanes 0-1 selected out of `a`, lanes 2-3 out of `b`. */
static inline RecompXmm XMM_SHUFFLE(RecompXmm a, RecompXmm b, uint32_t imm) {
    RecompXmm r;
    r.u[0] = a.u[(imm >> 0) & 3u]; r.u[1] = a.u[(imm >> 2) & 3u];
    r.u[2] = b.u[(imm >> 4) & 3u]; r.u[3] = b.u[(imm >> 6) & 3u];
    return r;
}

/** unpcklps / unpckhps: interleave the low or high halves. */
static inline RecompXmm XMM_UNPACK_LOW(RecompXmm a, RecompXmm b) {
    RecompXmm r;
    r.u[0] = a.u[0]; r.u[1] = b.u[0]; r.u[2] = a.u[1]; r.u[3] = b.u[1];
    return r;
}

static inline RecompXmm XMM_UNPACK_HIGH(RecompXmm a, RecompXmm b) {
    RecompXmm r;
    r.u[0] = a.u[2]; r.u[1] = b.u[2]; r.u[2] = a.u[3]; r.u[3] = b.u[3];
    return r;
}

/* ================================================================
 * Flag computation helpers
 *
 * These macros compute x86 flags for conditional branches.
 * Used by the lifter's pattern-matching output:
 *   cmp a, b; jcc target  ->  if (COND(a, b)) goto target;
 * ================================================================ */

/* Unsigned comparison conditions (from CMP a, b -> a - b) */
#define CMP_EQ(a, b)  ((uint32_t)(a) == (uint32_t)(b))
#define CMP_NE(a, b)  ((uint32_t)(a) != (uint32_t)(b))
#define CMP_B(a, b)   ((uint32_t)(a) <  (uint32_t)(b))   /* below (CF=1) */
#define CMP_AE(a, b)  ((uint32_t)(a) >= (uint32_t)(b))   /* above or equal */
#define CMP_BE(a, b)  ((uint32_t)(a) <= (uint32_t)(b))   /* below or equal */
#define CMP_A(a, b)   ((uint32_t)(a) >  (uint32_t)(b))   /* above */

/* Signed comparison conditions */
/* x86 evaluates the signed conditions at the operand width, not at 32 bits.
   The generated code passes LO8/HI8/LO16 sub-register reads straight in, and
   those zero-extend, so recover the width and sign-extend before comparing. */
#define RECOMP_FLAG_WIDTH(a, b) (sizeof(a) < sizeof(b) ? sizeof(a) : sizeof(b))
#define RECOMP_SIGNED(value, width) \
    ((width) == 1u ? (int32_t)(int8_t)(uint8_t)(uint32_t)(value) \
     : (width) == 2u ? (int32_t)(int16_t)(uint16_t)(uint32_t)(value) \
     : (int32_t)(uint32_t)(value))
#define CMP_L(a, b)   (RECOMP_SIGNED(a, RECOMP_FLAG_WIDTH(a, b)) <  \
                       RECOMP_SIGNED(b, RECOMP_FLAG_WIDTH(a, b)))  /* less */
#define CMP_GE(a, b)  (RECOMP_SIGNED(a, RECOMP_FLAG_WIDTH(a, b)) >= \
                       RECOMP_SIGNED(b, RECOMP_FLAG_WIDTH(a, b)))  /* >= */
#define CMP_LE(a, b)  (RECOMP_SIGNED(a, RECOMP_FLAG_WIDTH(a, b)) <= \
                       RECOMP_SIGNED(b, RECOMP_FLAG_WIDTH(a, b)))  /* <= */
#define CMP_G(a, b)   (RECOMP_SIGNED(a, RECOMP_FLAG_WIDTH(a, b)) >  \
                       RECOMP_SIGNED(b, RECOMP_FLAG_WIDTH(a, b)))  /* > */

/* TEST-based conditions (AND without storing result) */
#define TEST_Z(a, b)  (((uint32_t)(a) & (uint32_t)(b)) == 0)  /* ZF=1 */
#define TEST_NZ(a, b) (((uint32_t)(a) & (uint32_t)(b)) != 0)  /* ZF=0 */
#define TEST_S(a, b)  (RECOMP_SIGNED((uint32_t)(a) & (uint32_t)(b), \
                                     RECOMP_FLAG_WIDTH(a, b)) < 0)  /* SF=1 */

/* ================================================================
 * Arithmetic with carry/overflow detection
 * ================================================================ */

/** Add with carry flag. Returns result, sets *cf. */
static inline uint32_t ADD32_CF(uint32_t a, uint32_t b, int *cf) {
    uint32_t r = a + b;
    *cf = (r < a);
    return r;
}

/** Sub with carry (borrow) flag. Returns result, sets *cf. */
static inline uint32_t SUB32_CF(uint32_t a, uint32_t b, int *cf) {
    *cf = (a < b);
    return a - b;
}

/* ================================================================
 * Rotation / shift helpers
 * ================================================================ */

static inline uint32_t ROL32(uint32_t val, int n) {
    n &= 31;
    return (val << n) | (val >> (32 - n));
}

static inline uint32_t ROR32(uint32_t val, int n) {
    n &= 31;
    return (val >> n) | (val << (32 - n));
}

/* ================================================================
 * Sign/zero extension
 * ================================================================ */

#define ZX8(v)   ((uint32_t)(uint8_t)(v))
#define ZX16(v)  ((uint32_t)(uint16_t)(v))
#define SX8(v)   ((uint32_t)(int32_t)(int8_t)(v))
#define SX16(v)  ((uint32_t)(int32_t)(int16_t)(v))

/* ================================================================
 * Byte/word register access
 *
 * These macros extract or set partial registers, matching x86
 * behavior where writing AL doesn't affect bits 8-31 of EAX.
 * ================================================================ */

/** Extract low byte (al, bl, cl, dl). */
#define LO8(r)  ((uint8_t)((r) & 0xFF))
/** Extract high byte of low word (ah, bh, ch, dh). */
#define HI8(r)  ((uint8_t)(((r) >> 8) & 0xFF))
/** Extract low word (ax, bx, cx, dx). */
#define LO16(r) ((uint16_t)((r) & 0xFFFF))

/** Set low byte, preserving upper 24 bits. */
#define SET_LO8(r, v)  ((r) = ((r) & 0xFFFFFF00u) | ((uint32_t)(uint8_t)(v)))
/** Set high byte of low word, preserving other bits. */
#define SET_HI8(r, v)  ((r) = ((r) & 0xFFFF00FFu) | (((uint32_t)(uint8_t)(v)) << 8))
/** Set low word, preserving upper 16 bits. */
#define SET_LO16(r, v) ((r) = ((r) & 0xFFFF0000u) | ((uint32_t)(uint16_t)(v)))

/* ================================================================
 * Stack simulation
 *
 * For push/pop heavy prologues in the generated code.
 * ================================================================ */

/**
 * Push a 32-bit value onto the simulated stack.
 * Evaluates val BEFORE decrementing sp, matching x86 semantics
 * where push [esp+N] reads the operand before adjusting ESP.
 */
#define PUSH32(sp, val) do { \
    uint32_t _pv = (uint32_t)(val); \
    (sp) -= 4; \
    MEM32(sp) = _pv; \
} while(0)

/**
 * x86 parity flag: 1 when the low byte of the result has an EVEN number of set
 * bits (that is what PF means). Used by the x87 float-compare idiom
 * `fnstsw ax; test ah, mask; jp/jnp`, which is how all pre-SSE code branches on
 * a float comparison. Without a real parity here the branch was hardcoded and
 * every such comparison went one fixed direction.
 */
static inline int recomp_parity8(uint32_t x) {
    x &= 0xFFu; x ^= x >> 4; x ^= x >> 2; x ^= x >> 1;
    return (int)(~x & 1u);   /* 1 = even parity (PF set) */
}
#define RECOMP_PARITY8(x) recomp_parity8((uint32_t)(x))

/** Pop a 32-bit value from the simulated stack. */
#define POP32(sp, dst) do { \
    (dst) = MEM32(sp); \
    (sp) += 4; \
} while(0)

/* ================================================================
 * Byte swap (for endian conversion if needed)
 *
 * Xbox is little-endian like x86, so these are rarely needed,
 * but some games use bswap for network byte order or data parsing.
 * ================================================================ */

static inline uint32_t BSWAP32(uint32_t v) {
    return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) |
           ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000u);
}

static inline uint16_t BSWAP16(uint16_t v) {
    return (uint16_t)((v >> 8) | (v << 8));
}

/* Bit-scan helpers. Callers guard zero because x86 leaves the destination
 * undefined for a zero source while setting ZF. */
static inline uint32_t RECOMP_BSF32(uint32_t v) {
    uint32_t bit = 0;
    while ((v & 1u) == 0u) {
        v >>= 1;
        ++bit;
    }
    return bit;
}

static inline uint32_t RECOMP_BSR32(uint32_t v) {
    uint32_t bit = 31;
    while ((v & 0x80000000u) == 0u) {
        v <<= 1;
        --bit;
    }
    return bit;
}

/* ================================================================
 * Indirect call dispatch
 *
 * The dispatch system resolves Xbox virtual addresses to native
 * function pointers at runtime. Three lookup sources are checked:
 *   1. Manual overrides (hand-written reimplementations)
 *   2. Generated dispatch table (auto-recompiled functions)
 *   3. Kernel thunk bridge (Xbox kernel function replacements)
 * ================================================================ */

/**
 * Generic function pointer type for all recompiled functions.
 * All translated functions are void(void) - arguments and return
 * values are passed through global registers and the simulated stack.
 */
#ifndef RECOMP_DISPATCH_H  /* avoid conflict with recomp_dispatch.h */
typedef void (*recomp_func_t)(void);

/**
 * Look up a recompiled function by its Xbox VA.
 * Returns NULL if the VA is not in the generated dispatch table.
 */
recomp_func_t recomp_lookup(uint32_t xbox_va);

/**
 * Build the flat, directly-indexed dispatch table.
 *
 * Turns recomp_lookup from a binary search over every translated function into
 * a bounds check and one indexed load -- the C form of Microsoft's
 * `jmp [table + guest_eip*8]`. Call it once at startup, before any recompiled
 * code runs.
 *
 * Entirely optional: if it is never called, or returns 0 because the allocation
 * failed, recomp_lookup keeps using the binary search and everything still
 * works. Costs 8 bytes per byte of guest code span (see
 * recomp_dispatch_flat_bytes), allocated with calloc so the untouched middle
 * stays uncommitted.
 *
 * Returns 1 if the flat table is in use, 0 if the search is.
 */
int recomp_dispatch_init(void);

/** Bytes held by the flat table, or 0 if it was never built. */
size_t recomp_dispatch_flat_bytes(void);

/**
 * Look up a kernel thunk function by its synthetic VA.
 * Kernel thunks live at 0xFE000000+ (synthetic addresses assigned
 * during kernel bridge initialization).
 * Returns NULL if the VA is not a kernel thunk.
 */
recomp_func_t recomp_lookup_kernel(uint32_t xbox_va);

/**
 * Look up a manually overridden function by its Xbox VA.
 * Manual overrides take priority over generated code.
 * Returns NULL if no manual override exists for this VA.
 */
recomp_func_t recomp_lookup_manual(uint32_t xbox_va);
#endif

#ifndef RECOMP_DISPATCH_H
/* Synthetic kernel thunks occupy a small, reserved VA window. Movie playback
 * polls KeQuerySystemTime tens of millions of times per second. Sending those
 * calls through the manual override chain and the general game dispatch table
 * first wastes most of the poll budget and makes video pacing uneven. */
#define RECOMP_KERNEL_VA_BASE 0xFE000000u
#define RECOMP_KERNEL_VA_END  (RECOMP_KERNEL_VA_BASE + 378u * 4u)

static __forceinline int recomp_is_kernel_va(uint32_t va)
{
    return va >= RECOMP_KERNEL_VA_BASE && va < RECOMP_KERNEL_VA_END;
}

#include "recomp_call_cache.h"
extern int g_recomp_call_cache_enabled;
extern __declspec(thread) RecompCallCache g_recomp_call_cache;
static __forceinline recomp_func_t recomp_lookup_indirect(uint32_t va)
{
    recomp_func_t fn;

    if (recomp_is_kernel_va(va)) {
        return recomp_lookup_kernel(va);
    }

    if(g_recomp_call_cache_enabled){fn=recomp_call_cache_get(&g_recomp_call_cache,va);if(fn)return fn;}
    fn = recomp_lookup_manual(va);
    if (!fn) fn = recomp_lookup(va);
    if(fn&&g_recomp_call_cache_enabled)recomp_call_cache_put(&g_recomp_call_cache,va,fn);
    if (!fn) fn = recomp_lookup_kernel(va);
    return fn;
}

/* Kernel polling has its own counter in kernel_bridge.c. Keeping it out of the
 * crash-forensics ring avoids three volatile TLS writes on every clock poll;
 * the ring now retains the recent game/vtable calls that are more useful when
 * diagnosing a crash. */
static __forceinline void recomp_icall_trace_record(uint32_t va)
{
#if defined(UCRR_LEAN_RUNTIME) && UCRR_LEAN_RUNTIME == 1
    /* Crash forensics only: no guest state or dispatch behavior belongs to
     * this ring. Lean release omits even the volatile TLS writes; diagnostics
     * builds retain the historical ring without any runtime flag branch. */
    (void)va;
#else
    if (recomp_is_kernel_va(va)) return;
    g_icall_trace[g_icall_trace_idx & (ICALL_TRACE_SIZE - 1)] = va;
    g_icall_trace_idx++;
    g_icall_count++;
#endif
}
#endif

/**
 * RECOMP_ICALL - Indirect call through the dispatch table.
 *
 * Looks up the Xbox VA and calls the translated function.
 * Falls back to kernel bridge for kernel thunk synthetic VAs.
 * The caller must PUSH32 the guest return address before this macro.
 * If not found, pops it back off to keep the stack balanced.
 *
 * Do not reject addresses with a hard-coded upper bound here. Xbox titles
 * commonly place executable library sections after the main .text section;
 * Urban Chaos has valid code through 0x0044B25F. The generated lookup is a
 * safe binary search, so an unknown or garbage address simply falls through
 * to the normal unresolved-target path.
 */
#define RECOMP_ICALL(xbox_va) do { \
    uint32_t _va = (uint32_t)(xbox_va); \
    recomp_icall_trace_record(_va); \
    recomp_func_t _fn = recomp_lookup_indirect(_va); \
    if (_fn) { RECOMP_ICALL_OBSERVE(_va, RECOMP_ICALL_SEEN_RESOLVED); _fn(); } \
    else { RECOMP_ICALL_OBSERVE(_va, RECOMP_ICALL_SEEN_UNRESOLVED); \
           recomp_icall_fail_log(_va); g_esp += 4; g_eax = 0; } \
} while(0)

/**
 * RECOMP_ICALL_SAFE - Stack-safe indirect call.
 *
 * Restores g_esp to saved_esp (pre-argument value) on lookup failure,
 * preventing stdcall argument leaks on failed vtable calls.
 * Use this when the caller pushes arguments that the callee would
 * normally clean up (stdcall convention).
 */
#define RECOMP_ICALL_SAFE(xbox_va, saved_esp) do { \
    uint32_t _va = (uint32_t)(xbox_va); \
    recomp_icall_trace_record(_va); \
    recomp_func_t _fn = recomp_lookup_indirect(_va); \
    if (_fn) { RECOMP_ICALL_OBSERVE(_va, RECOMP_ICALL_SEEN_RESOLVED); _fn(); } \
    else { RECOMP_ICALL_OBSERVE(_va, RECOMP_ICALL_SEEN_UNRESOLVED); \
           recomp_icall_fail_log(_va); g_esp = (saved_esp); g_eax = 0; } \
} while(0)

/**
 * RECOMP_ITAIL - Indirect tail call (jmp through function pointer).
 *
 * No return address is pushed - reuses the current frame's return addr.
 * Used for tail-call optimization where the original code uses
 * jmp [reg] instead of call [reg].
 */
#define RECOMP_ITAIL(xbox_va) do { \
    uint32_t _va = (uint32_t)(xbox_va); \
    recomp_icall_trace_record(_va); \
    recomp_func_t _fn = recomp_lookup_indirect(_va); \
    if (_fn) { RECOMP_ICALL_OBSERVE(_va, RECOMP_ICALL_SEEN_RESOLVED); _fn(); } \
    else { RECOMP_ICALL_OBSERVE(_va, RECOMP_ICALL_SEEN_UNRESOLVED); \
           recomp_icall_fail_log(_va); g_esp += 4; g_eax = 0; } \
} while(0)

/* ================================================================
 * Register name aliases for generated code
 *
 * Map x86 volatile register names to global variables.
 * These #defines allow the generated code to use natural register
 * names (eax, ecx, edx, esp) which the preprocessor maps to the
 * corresponding globals (g_eax, g_ecx, g_edx, g_esp).
 *
 * Only active when RECOMP_GENERATED_CODE is defined (in generated
 * .c files) to avoid polluting hand-written code.
 * ================================================================ */

#ifdef RECOMP_GENERATED_CODE
#define eax g_eax
#define ecx g_ecx
#define edx g_edx
#define esp g_esp
#define ebx g_ebx
#define esi g_esi
#define edi g_edi
#define xmm0 g_xmm0
#define xmm1 g_xmm1
#define xmm2 g_xmm2
#define xmm3 g_xmm3
#define xmm4 g_xmm4
#define xmm5 g_xmm5
#define xmm6 g_xmm6
#define xmm7 g_xmm7
/* ebp is NOT global - it's local in each function.
 * For __SEH_prolog/epilog, use g_seh_ebp to bridge. */
#endif

/* ================================================================
 * Forward declarations for translated functions
 *
 * These are generated by the recompiler and included per-file.
 * The recomp_funcs.h header (generated) declares all translated
 * function prototypes.
 * ================================================================ */

#endif /* RECOMP_TYPES_H */
