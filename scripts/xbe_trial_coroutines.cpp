#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include "cpu/x86_ops.h"
static thread_local X86Context* uc_regs;
static thread_local uint8_t* uc_base;
#define MEM8(a) (*(uc_base+uint32_t(a)))
#define MEM32(a) (*(x86_u32u*)(uc_base+uint32_t(a)))
#define XBOX_PTR(a) (uc_base+uint32_t(a))
#define PUSH32(sp,v) do { uint32_t value=(v); (sp)-=4; MEM32(sp)=value; } while(0)
#define RECOMP_ICALL_SAFE(target,saved) x86_icall(*uc_regs,uc_base,target)
#define g_eax (uc_regs->eax)
#define g_ecx (uc_regs->ecx)
#define g_edx (uc_regs->edx)
#define g_ebx (uc_regs->ebx)
#define g_esi (uc_regs->esi)
#define g_edi (uc_regs->edi)
#define g_esp (uc_regs->esp)
#define g_ebp (uc_regs->ebp)
struct UCRR_COROUTINE_REGS : X86Context { uint32_t exception_list; };

typedef struct UCRR_COROUTINE UCRR_COROUTINE;

typedef struct UCRR_COROUTINE_FRAME {
    void *parent_fiber;
    UCRR_COROUTINE *parent_coroutine;
    UCRR_COROUTINE_REGS parent_regs;
} UCRR_COROUTINE_FRAME;

struct UCRR_COROUTINE {
    uint32_t context_va;
    uint32_t guest_stack_base;
    void *fiber;
    uint8_t *stack_copy;
    uint32_t stack_offset;
    int stack_valid;
    int finished;
    int last_yield;
    UCRR_COROUTINE_REGS regs;
    UCRR_COROUTINE_FRAME *parent_frame;
    UCRR_COROUTINE *next;
};

static thread_local void *g_ucrr_thread_fiber;
static thread_local UCRR_COROUTINE *g_ucrr_coroutines;
static thread_local UCRR_COROUTINE *g_ucrr_active_coroutine;
static thread_local unsigned g_ucrr_coroutine_warnings;

static void ucrr_coroutine_regs_save(UCRR_COROUTINE_REGS* r){
    static_cast<X86Context&>(*r)=*uc_regs;
    r->exception_list=MEM32(uc_regs->fs_base);
}
static void ucrr_coroutine_regs_load(const UCRR_COROUTINE_REGS* r){
    *uc_regs=static_cast<const X86Context&>(*r);
    MEM32(uc_regs->fs_base)=r->exception_list;
}
static void ucrr_coroutine_stack_save(UCRR_COROUTINE *coroutine)
{
    if (coroutine && coroutine->stack_copy && coroutine->guest_stack_base) {
        uint32_t offset = g_esp - coroutine->guest_stack_base;
        if (offset > 0x10000u) {
            fprintf(stderr, "[UCRR-COROUTINE] Stack outside context %08X: %08X\n",
                    coroutine->context_va, g_esp);
            abort();
        }
        coroutine->stack_offset = offset;
        memcpy(coroutine->stack_copy,
               (const void *)XBOX_PTR(coroutine->guest_stack_base + offset),
               0x10000u - offset);
        coroutine->stack_valid = 1;
    }
}

static void ucrr_coroutine_stack_load(const UCRR_COROUTINE *coroutine)
{
    if (coroutine && coroutine->stack_copy && coroutine->guest_stack_base &&
        coroutine->stack_valid) {
        memcpy((void *)XBOX_PTR(coroutine->guest_stack_base + coroutine->stack_offset),
               coroutine->stack_copy, 0x10000u - coroutine->stack_offset);
    }
}

static UCRR_COROUTINE *ucrr_coroutine_find(uint32_t context_va)
{
    UCRR_COROUTINE *coroutine;
    for (coroutine = g_ucrr_coroutines; coroutine;
         coroutine = coroutine->next) {
        if (coroutine->context_va == context_va) {
            if (coroutine == g_ucrr_active_coroutine || coroutine->parent_frame) {
                fprintf(stderr, "[UCRR-COROUTINE] Destroying running context %08X\n",
                        context_va);
                abort();
            }
            return coroutine;
        }
    }
    return NULL;
}

static void *ucrr_coroutine_prepare_thread(void)
{
    if (!g_ucrr_thread_fiber) {
        g_ucrr_thread_fiber = ConvertThreadToFiberEx(NULL,FIBER_FLAG_FLOAT_SWITCH);
        if (!g_ucrr_thread_fiber && GetLastError() == ERROR_ALREADY_FIBER) {
            g_ucrr_thread_fiber = GetCurrentFiber();
        }
        if (!g_ucrr_thread_fiber && g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] ConvertThreadToFiber failed: %lu\n",
                    (unsigned long)GetLastError());
        }
    }
    return g_ucrr_thread_fiber;
}

static void ucrr_coroutine_switch_to_parent(UCRR_COROUTINE *coroutine,
                                             int value)
{
    UCRR_COROUTINE_FRAME *frame = coroutine->parent_frame;

    coroutine->last_yield = value;
    ucrr_coroutine_regs_save(&coroutine->regs);
    ucrr_coroutine_stack_save(coroutine);

    if (!frame || !frame->parent_fiber) {
        if (g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] Context %08X has no parent fiber\n",
                    coroutine->context_va);
        }
        return;
    }

    if (frame->parent_coroutine) {
        ucrr_coroutine_stack_load(frame->parent_coroutine);
    }
    ucrr_coroutine_regs_load(&frame->parent_regs);
    g_ucrr_active_coroutine = frame->parent_coroutine;
    SwitchToFiber(frame->parent_fiber);
}

static VOID CALLBACK ucrr_coroutine_entry(void *argument)
{
    UCRR_COROUTINE *coroutine = (UCRR_COROUTINE *)argument;
    uint32_t object;
    uint32_t vtable;
    uint32_t target;
    uint32_t saved_esp;
    uint32_t return_va;

    g_ucrr_active_coroutine = coroutine;
    object = MEM32(coroutine->context_va + 0x28u);
    if (!object) {
        if (g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] Context %08X has no callback object\n",
                    coroutine->context_va);
        }
    } else {
        vtable = MEM32(object);
        if (MEM8(coroutine->context_va + 0x2Cu)) {
            target = MEM32(vtable + 0x5Cu);
            return_va = 0x001FB826u;
        } else {
            target = MEM32(vtable + 0x58u);
            return_va = 0x001FB83Du;
        }

        MEM8(coroutine->context_va + 0x2Du) = 1;
        g_eax = object;
        g_ecx = object;
        g_edx = vtable;
        saved_esp = g_esp;
        PUSH32(g_esp, return_va);
        RECOMP_ICALL_SAFE(target, saved_esp);
    }

    coroutine->finished = 1;
    ucrr_coroutine_switch_to_parent(coroutine, 2);

    /* A completed coroutine is normally deleted by 0x001FBA40 without being
     * resumed.  Keep a defensive yield loop so an unexpected extra resume can
     * never return from the Win32 fiber entry point and terminate the thread. */
    for (;;) {
        ucrr_coroutine_switch_to_parent(coroutine, 2);
    }
}

static UCRR_COROUTINE *ucrr_coroutine_create(
    uint32_t context_va, const UCRR_COROUTINE_REGS *parent_regs)
{
    UCRR_COROUTINE *coroutine;

    if (!ucrr_coroutine_prepare_thread()) {
        return NULL;
    }

    coroutine = (UCRR_COROUTINE *)calloc(1, sizeof(*coroutine));
    if (!coroutine) {
        return NULL;
    }
    coroutine->stack_copy = (uint8_t *)malloc(0x10000u);
    if (!coroutine->stack_copy) {
        free(coroutine);
        return NULL;
    }

    coroutine->context_va = context_va;
    coroutine->guest_stack_base = MEM32(0x51C3DCu);
    coroutine->regs = *parent_regs;
    coroutine->regs.eax = context_va;
    coroutine->regs.ebx = context_va;
    coroutine->regs.ecx = context_va;
    if (coroutine->guest_stack_base) {
        coroutine->regs.esp = coroutine->guest_stack_base + 0xFFF4u;
    }

    coroutine->fiber = CreateFiberEx(0,0,FIBER_FLAG_FLOAT_SWITCH,ucrr_coroutine_entry,coroutine);
    if (!coroutine->fiber || !coroutine->guest_stack_base) {
        if (coroutine->fiber) {
            DeleteFiber(coroutine->fiber);
        }
        free(coroutine->stack_copy);
        free(coroutine);
        return NULL;
    }

    coroutine->next = g_ucrr_coroutines;
    g_ucrr_coroutines = coroutine;
#ifndef UC_BURNER_RELEASE // UC_RELEASE_ENVIRONMENT
    if(getenv("UCRR_COROUTINE_TRACE"))fprintf(stderr,
            "[UCRR-COROUTINE] Created context=%08X guest_stack=%08X\n",
            context_va, coroutine->guest_stack_base);
#endif // UC_RELEASE_ENVIRONMENT
    return coroutine;
}

static void ucrr_coroutine_destroy(uint32_t context_va)
{
    UCRR_COROUTINE **link = &g_ucrr_coroutines;
    UCRR_COROUTINE *coroutine;

    while ((coroutine = *link) != NULL) {
        if (coroutine->context_va == context_va) {
            *link = coroutine->next;
            if (coroutine != g_ucrr_active_coroutine && coroutine->fiber) {
                DeleteFiber(coroutine->fiber);
            }
            free(coroutine->stack_copy);
            free(coroutine);
            return;
        }
        link = &coroutine->next;
    }
}

/* Resume one coroutine step.  This replaces the generated body at 0x001FB790
 * and returns AL=1 only after the callback has completed, matching the
 * original comparison of the outer longjmp result with 2. */
X86_FUNC(sub_001FB790)
{
    uc_regs=&ctx;uc_base=base;
    const uint32_t context_va = g_ecx;
    UCRR_COROUTINE_FRAME frame;
    UCRR_COROUTINE *coroutine;

    memset(&frame, 0, sizeof(frame));
    ucrr_coroutine_regs_save(&frame.parent_regs);
    frame.parent_coroutine = g_ucrr_active_coroutine;
    frame.parent_fiber = ucrr_coroutine_prepare_thread()
        ? GetCurrentFiber() : NULL;

    coroutine = ucrr_coroutine_find(context_va);
    if (!coroutine) {
        coroutine = ucrr_coroutine_create(context_va, &frame.parent_regs);
    }

    if (!coroutine) {
        fprintf(stderr, "[UCRR-COROUTINE] Cannot allocate context %08X\n", context_va);
        abort(); /* Do not report a loading callback as successfully completed. */
    }
    if (coroutine->parent_frame || coroutine == g_ucrr_active_coroutine) {
        fprintf(stderr, "[UCRR-COROUTINE] Recursive resume of context %08X\n", context_va);
        abort();
    }
    if (coroutine->finished) {
        MEM8(context_va + 0x2Du) = 0;
        g_eax = 1;
        g_esp = frame.parent_regs.esp + 4u;
        return;
    }

    if (frame.parent_coroutine) {
        ucrr_coroutine_stack_save(frame.parent_coroutine);
    }
    ucrr_coroutine_stack_load(coroutine);
    coroutine->parent_frame = &frame;
    MEM8(context_va + 0x2Du) = 1;
    ucrr_coroutine_regs_load(&coroutine->regs);
    g_ucrr_active_coroutine = coroutine;
    SwitchToFiber(coroutine->fiber);

    /* The yielding fiber restored this snapshot before switching back.  Load
     * it again deliberately so the contract remains explicit and resilient to
     * future diagnostic code added between the switch and this point. */
    ucrr_coroutine_regs_load(&frame.parent_regs);
    g_ucrr_active_coroutine = frame.parent_coroutine;
    coroutine->parent_frame = NULL;
    MEM8(context_va + 0x2Du) = 0;
    g_eax = (coroutine->last_yield == 2) ? 1u : 0u;
    g_esp = frame.parent_regs.esp + 4u;
}

/* Yield from the active callback.  Advancing ESP before the host switch makes
 * the suspended guest state exactly the state following this function's RET;
 * when the fiber is resumed, returning from this C function continues at the
 * original caller without executing a longjmp call site twice. */
X86_FUNC(sub_001FB930)
{
    uc_regs=&ctx;uc_base=base;
    uint32_t context_va = g_ecx ? MEM32(g_ecx) : 0;
    UCRR_COROUTINE *coroutine = g_ucrr_active_coroutine;

    if (!coroutine || coroutine->context_va != context_va) {
        if (g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] Yield outside active context: %08X\n",
                    context_va);
        }
        g_eax = 1;
        g_esp += 4u;
        return;
    }

    MEM8(context_va + 0x2Du) = 0;
    g_eax = 1;
    g_esp += 4u;
    ucrr_coroutine_switch_to_parent(coroutine, 1);
}

X86_EXTERN_FUNC(__imp__sub_001FBA40);

X86_FUNC(sub_001FBA40)
{
    uc_regs=&ctx;uc_base=base;
    uint32_t context_va = g_ecx ? MEM32(g_ecx) : 0;
    if (context_va) {
        ucrr_coroutine_destroy(context_va);
    }
    __imp__sub_001FBA40(ctx,base);
}

