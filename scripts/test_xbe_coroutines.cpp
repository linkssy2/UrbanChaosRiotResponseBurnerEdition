#include "../common/xbe_trial_coroutines.cpp"
#define CHECK(expr) do { if(!(expr)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#expr);std::exit(1);} } while(0)
static X86Context test_regs{};
static void callback(uint32_t);
void x86_icall(X86Context&,uint8_t*,uint32_t target){callback(target);}
static int stages[3];
static unsigned destructor_calls;
X86_FUNC(__imp__sub_001FBA40) { ++destructor_calls; g_esp += 4; }
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
    uint64_t saved_mmx[8]; memcpy(saved_mmx, uc_regs->mm, sizeof(saved_mmx));
    uint32_t sp = g_esp, saved_esi = g_esi, seh = MEM32(0);
    g_ecx = ctx(i);
    PUSH32(g_esp, 0x12345678);
    sub_001FB790(*uc_regs,uc_base);
    CHECK(g_esp == sp && g_esi == saved_esi && MEM32(0) == seh);
    CHECK(memcmp(saved_mmx, uc_regs->mm, sizeof(saved_mmx)) == 0);
    CHECK(MEM8(ctx(i) + 0x2D) == 0);
    return g_eax;
}
static void yield_step(unsigned i) {
    uint32_t sp = g_esp;
    g_ecx = wrapper(i);
    PUSH32(g_esp, 0x87654321);
    sub_001FB930(*uc_regs,uc_base);
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
    for (unsigned r = 0; r < 8; ++r) uc_regs->mm[r] = ((uint64_t)marker << 32) | r;
    stages[i] = 1;
    yield_step(i);
    CHECK(MEM32(g_esp) == marker && g_esi == marker && MEM32(0) == marker);
    for (unsigned r = 0; r < 8; ++r) CHECK(uc_regs->mm[r] == (((uint64_t)marker << 32) | r));
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
    uc_base=(uint8_t*)memory;uc_regs=&test_regs;
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
        sub_001FBA40(*uc_regs,uc_base);
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
