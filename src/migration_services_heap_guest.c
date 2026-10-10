#include "migration_services_heap_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int enabled(void) {
    static int flag = -1;
    if (flag < 0) { const char *p = getenv("UCRR_NATIVE_SERVICES_HEAP"); flag = p && !strcmp(p, "1"); }
    return flag;
}
static int span(uint32_t p, unsigned n) {
    return p >= 0x10000 && p < 0x80000000u && (uint64_t)p + n <= g_xbox_total_ram;
}
static int apart(uint32_t p, unsigned n, uint32_t q, unsigned m) {
    return (uint64_t)p + n <= q || (uint64_t)q + m <= p;
}

/* The original prefix has already entered SEH and acquired the heap lock.
 * Rejecting here resumes its untouched suffix while it still owns the lock. */
static void frame_leave(uint32_t frame, unsigned pc) {
    PUSH32(g_esp, pc); MEM32(0) = MEM32(frame - 16);
    POP32(g_esp, g_ecx); POP32(g_esp, g_edi); POP32(g_esp, g_esi); POP32(g_esp, g_ebx);
    g_esp = frame; POP32(g_esp, g_seh_ebp);
    MEM32(g_esp - 4) = g_ecx;
    g_esp += 16;
}

/* Allocate the tail of the exact-size bin, preserving the title's guest heap.
 * This is an actual unlink/bitmap/header implementation, with no translated
 * allocator, host malloc, private pool or shadow ownership table. */
static void allocate_exact(uint32_t f, uint32_t heap, uint32_t block, unsigned units, unsigned size) {
    uint32_t head = heap + 0x180 + units * 8;
    MEM32(f - 44) = head; MEM32(f - 52) = block;
    unsigned header_flags = MEM8(block + 5); MEM8(f - 25) = (uint8_t)header_flags;
    uint32_t next = MEM32(block + 8), prev = MEM32(block + 12);
    MEM32(f - 288) = next; MEM32(f - 316) = prev;
    MEM32(prev) = next; MEM32(next + 4) = prev;
    g_edi = prev;
    if (next == prev) {
        unsigned byte = units >> 3, mask = 1u << (units & 7);
        MEM32(f - 144) = byte; MEM32(f - 240) = mask;
        MEM32(f - 76) = heap + 0x160 + byte; MEM8(heap + 0x160 + byte) ^= (uint8_t)mask;
        g_edi = heap + 0x160 + byte;
    }
    MEM32(heap + 0x30) -= units; MEM32(f - 56) = block;
    g_edx = (header_flags & 16) | 1; MEM8(block + 5) = (uint8_t)g_edx;
    MEM8(block + 6) = (uint8_t)(units * 16 - size); MEM8(block + 7) = 0;
    g_esi = block + 16; MEM32(f - 60) = g_esi;
    g_eax = block; g_ecx = units * 16 - size;
    if (MEM8(f - 26)) {
        uint32_t saved = g_esp; PUSH32(g_esp, MEM32(g_ebx + 0x580));
        uint32_t target = MEM32(0x44b3ec); PUSH32(g_esp, 0x84163); RECOMP_ICALL_SAFE(target, saved);
        MEM8(f - 26) = 0;
    }
    if (MEM8(f + 12) & 8) { g_edx = MEM32(f + 16); memset((void*)XBOX_PTR(g_esi), 0, g_edx); g_edi = g_esi + g_edx; g_ecx = 0; g_eax = 0; }
    MEM32(f - 4) = 0xffffffffu; MEM32(g_esp - 4) = 0x84288; g_recomp_zf = 1;
    MEM32(f - 92) = g_esi; g_eax = MEM32(f + 16); MEM32(f - 88) = g_eax; MEM32(f - 84) = 0;
    g_eax = MEM32(MEM32(0x20) + 0x250);
    if (g_eax) {
        g_ecx = f - 92; uint32_t saved = g_esp; PUSH32(g_esp, g_ecx); PUSH32(g_esp, 10);
        uint32_t target = MEM32(g_eax + 20); PUSH32(g_esp, 0x842b2); RECOMP_ICALL_SAFE(target, saved);
    }
    g_eax = g_esi; frame_leave(f, 0x842b9);
}

/* Free a busy small block when its neighbors cannot coalesce. Reproduce the
 * normal 82973 frame as well: that helper publishes g_ebp even after return. */
static void release_isolated(uint32_t f, uint32_t heap, uint32_t block, unsigned units) {
    MEM32(f - 32) = units;
    PUSH32(g_esp, 0); PUSH32(g_esp, f - 32); PUSH32(g_esp, block); PUSH32(g_esp, heap); PUSH32(g_esp, 0x84364);
    PUSH32(g_esp, g_ebp); g_ebp = g_esp;
    PUSH32(g_esp, g_ebx); PUSH32(g_esp, g_esi); PUSH32(g_esp, g_edi);
    g_esp += 36; /* three preserved registers, frame, RET and four arguments */
    g_eax = block; MEM32(f - 48) = block;
    MEM8(block + 5) &= 16;
    uint32_t head = heap + 0x180 + units * 8; g_edx = head; MEM32(f - 52) = head;
    if (MEM32(head) == head) {
        unsigned byte = units >> 3, mask = 1u << (units & 7);
        MEM32(f - 56) = byte; MEM32(f - 60) = mask;
        MEM8(heap + 0x160 + byte) |= (uint8_t)mask;
    }
    MEM32(f - 64) = head; uint32_t prev = MEM32(head + 4); MEM32(f - 68) = prev;
    MEM32(block + 8) = head; MEM32(block + 12) = prev;
    MEM32(prev) = block + 8; MEM32(head + 4) = block + 8;
    MEM32(heap + 0x30) += units;
    g_eax = units; g_ecx = block + 8; g_edi = prev;
    MEM32(f - 4) = 0xffffffffu; PUSH32(g_esp, 0x844ab);
    if (MEM8(f - 25)) {
        uint32_t saved = g_esp; PUSH32(g_esp, MEM32(g_ebx + 0x580));
        uint32_t target = MEM32(0x44b3ec); PUSH32(g_esp, 0x844cb); RECOMP_ICALL_SAFE(target, saved);
        g_recomp_zf = 0;
    } else g_recomp_zf = 1;
    g_esp += 4; SET_LO8(g_eax, MEM8(f - 26)); frame_leave(f, 0x844b3);
}

int uc_migration_services_heap(unsigned entry, unsigned f) {
    if (!enabled() || (entry != 0x83b5d && entry != 0x842d8)) return 0;
    uint32_t s = f + 4;
    if ((s & 3) || s < 0x10200 || !span(s - 512, 532) || !span(0x51c178, 1) || MEM8(0x51c178) ||
        g_seh_ebp != f || g_esp != f - (entry == 0x83b5d ? 412 : 120)) return 0;
    uint32_t heap = g_esi, value = MEM32(f + 16), thread = MEM32(0x20);
    if (g_ebx != heap) return 0;
    if ((heap & 15) || !span(heap, 0x584) || !apart(heap, 0x584, s - 512, 532) ||
        !span(thread, 0x254) || !apart(thread, 0x254, s - 512, 532) || !apart(heap, 0x584, thread, 0x254) || MEM32(thread + 0x250)) return 0;
    /* ponytail: split, grow, large and coalescing paths remain original;
     * widen only with complete callback/metadata/ABI qualification. */
    uint32_t block, head; unsigned units;
    if (entry == 0x83b5d) {
        if (value > 2000) return 0;
        units = ((value ? value : 1) + 31) >> 4; head = heap + 0x180 + units * 8;
        if (g_edi != units || MEM32(f - 40) != units || MEM32(f - 32) != units * 16) return 0;
        if (MEM32(head) == head) return 0;
        block = MEM32(head + 4) - 8;
    } else {
        if (value < 16) return 0;
        block = value - 16; if (!span(block, 16)) return 0;
        if (g_edi != block) return 0;
        units = MEM16(block); if (units < 2 || units >= 128) return 0;
        head = heap + 0x180 + units * 8;
    }
    if ((block & 15) || !span(block, units * 16) || !apart(block, units * 16, heap, 0x584) ||
        !apart(block, units * 16, s - 512, 532) || !apart(block, units * 16, thread, 0x254) || MEM16(block) != units) return 0;
    if (entry == 0x83b5d) {
        uint32_t next = MEM32(block + 8), prev = MEM32(block + 12);
        if ((MEM8(block + 5) & 9) || next != head || !span(prev, 8) || !apart(prev, 8, s - 512, 532) ||
            !apart(prev, 8, block, units * 16) || !apart(prev, 8, thread, 0x254) ||
            (prev != head && !apart(prev, 8, heap, 0x584)) || MEM32(prev) != block + 8 || MEM32(head + 4) != block + 8 ||
            !(MEM8(heap + 0x160 + (units >> 3)) & (1u << (units & 7))) || MEM32(heap + 0x30) < units) return 0;
        allocate_exact(f, heap, block, units, value);
    } else {
        if (!(MEM8(block + 5) & 1) || (MEM8(block + 5) & 8)) return 0;
        uint32_t prev = block - MEM16(block + 2) * 16u, next = block + units * 16;
        if (prev != block && (!span(prev, 16) || !apart(prev, 16, s - 512, 532) || !(MEM8(prev + 5) & 1))) return 0;
        if (!(MEM8(block + 5) & 16) && (!span(next, 16) || !apart(next, 16, s - 512, 532) || !(MEM8(next + 5) & 1))) return 0;
        uint32_t tail = MEM32(head + 4);
        if (!span(tail, 8) || !apart(tail, 8, s - 512, 532) || !apart(tail, 8, block, units * 16) ||
            !apart(tail, 8, thread, 0x254) || (tail != head && !apart(tail, 8, heap, 0x584)) || MEM32(tail) != head ||
            ((MEM32(head) == head) != (tail == head))) return 0;
        release_isolated(f, heap, block, units);
    }
    return 1;
}
