/* Synthetic guest callbacks only: no ROM, renderer, audio, or game entry point. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "recomp_types.h"

ptrdiff_t g_xbox_mem_offset;
RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_esp, g_ebx, g_esi, g_edi;
RECOMP_TLS uint32_t g_ebp, g_seh_ebp;
RECOMP_TLS RecompXmm g_xmm0, g_xmm1, g_xmm2, g_xmm3;
RECOMP_TLS RecompXmm g_xmm4, g_xmm5, g_xmm6, g_xmm7;
RECOMP_TLS uint64_t g_mmx[8];
RECOMP_TLS double g_fp_stack[8];
RECOMP_TLS int g_fp_top, g_fp_cmp, g_recomp_zf;
RECOMP_TLS uint16_t g_fp_control_word;
#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr); exit(1); } } while (0)
static void callback(uint32_t target);
#undef RECOMP_ICALL_SAFE
#define RECOMP_ICALL_SAFE(target, saved) callback(target)
#include "coroutine_under_test.inc"

static int stages[3];
static unsigned destructor_calls;
void sub_001FBA40_gen(void) { ++destructor_calls; g_esp += 4; }
static uint32_t ctx(unsigned i) { return 0x1000 + i * 0x100; }
static uint32_t wrapper(unsigned i) { return 0x4000 + i * 4; }
static void setup(unsigned i) {
    uint32_t object = 0x2000 + i * 0x100;
    uint32_t vt = 0x3000 + i * 0x100;
    MEM32(wrapper(i)) = ctx(i);
    MEM32(ctx(i) + 0x28) = object;
    MEM32(object) = vt;
    MEM32(vt + 0x58) = i;
    MEM32(vt + 0x5C) = i;
}
static int step(unsigned i) {
    uint64_t saved_mmx[8]; memcpy(saved_mmx, g_mmx, sizeof(saved_mmx));
    uint32_t sp = g_esp, saved_esi = g_esi, seh = MEM32(0);
    g_ecx = ctx(i);
    PUSH32(g_esp, 0x12345678);
    sub_001FB790();
    CHECK(g_esp == sp && g_esi == saved_esi && MEM32(0) == seh);
    CHECK(memcmp(saved_mmx, g_mmx, sizeof(saved_mmx)) == 0);
    CHECK(MEM8(ctx(i) + 0x2D) == 0);
    return g_eax;
}
static void yield_step(unsigned i) {
    uint32_t sp = g_esp;
    g_ecx = wrapper(i);
    PUSH32(g_esp, 0x87654321);
    sub_001FB930();
    CHECK(g_esp == sp);
    CHECK(MEM8(ctx(i) + 0x2D) == 1);
}
static void callback(uint32_t i) {
    uint32_t marker = 0xABCDE000 + i;
    CHECK(i < 3 && MEM8(ctx(i) + 0x2D) == 1);
    g_esp -= 32;
    MEM32(g_esp) = marker;
    g_esi = marker;
    MEM32(0) = marker;
    for (unsigned r = 0; r < 8; ++r) g_mmx[r] = ((uint64_t)marker << 32) | r;
    stages[i] = 1;
    yield_step(i);
    CHECK(MEM32(g_esp) == marker && g_esi == marker && MEM32(0) == marker);
    for (unsigned r = 0; r < 8; ++r) CHECK(g_mmx[r] == (((uint64_t)marker << 32) | r));
    if (i == 0) {
        CHECK(step(1) == 0); /* B must yield to A, not the root fiber. */
        CHECK(MEM32(g_esp) == marker);
        yield_step(i);
        CHECK(step(1) == 1);
        CHECK(MEM32(g_esp) == marker);
    }
    stages[i] = 2;
    g_esp += 36;
}
int main(void) {
    void *memory = calloc(1, 0x800000);
    CHECK(memory != NULL);
    g_xbox_mem_offset = (ptrdiff_t)memory;
    MEM32(0x51C3DC) = 0x600000;
    g_esp = 0x700000;
    g_esi = 0x55555555;
    MEM32(0) = 0xFFFFFFFF;
    for (unsigned i = 0; i < 3; ++i) setup(i);
    CHECK(step(0) == 0);
    CHECK(step(2) == 0); /* Interleaved contexts share the guest stack. */
    CHECK(step(0) == 0);
    CHECK(stages[0] == 1 && stages[1] == 1);
    CHECK(step(2) == 1);
    CHECK(step(0) == 1);
    CHECK(step(0) == 1);
    for (unsigned i = 0; i < 3; ++i) {
        CHECK(stages[i] == 2);
        CHECK(ucrr_coroutine_find(ctx(i))->stack_offset > 0xFF00);
        g_ecx = wrapper(i);
        PUSH32(g_esp, 0x12345678);
        sub_001FBA40();
        CHECK(ucrr_coroutine_find(ctx(i)) == NULL);
    }
    CHECK(destructor_calls == 3 && g_esp == 0x700000);
    CHECK(step(2) == 0); /* Reused guest address must get a fresh host context. */
    CHECK(step(2) == 1);
    ucrr_coroutine_destroy(ctx(2));
    CHECK(g_ucrr_coroutines == NULL);
    puts("PASS: nested/interleaved yield, stack/register/SEH restore, active flag, completion, cleanup, reuse, bounded copies");
    free(memory);
    return 0;
}
