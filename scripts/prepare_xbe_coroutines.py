"""Adapt the established title coroutine bridge to XboxXBERecomp's register ABI."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
AREA=ROOT/'build/xbe-full-integration-20261014'

def prepare():
    old=(ROOT/'src/recomp_manual.c').read_text()
    start=old.index('typedef struct UCRR_COROUTINE_REGS')
    end=old.index('/* Resource/render callback',start)
    body=old[start:end]
    body=re.sub(r'typedef struct UCRR_COROUTINE_REGS \{.*?\} UCRR_COROUTINE_REGS;',
        'struct UCRR_COROUTINE_REGS : X86Context { uint32_t exception_list; };',body,flags=re.S)
    a=body.index('static void ucrr_coroutine_regs_save(')
    b=body.index('static void ucrr_coroutine_stack_save(',a)
    body=body[:a]+'''static void ucrr_coroutine_regs_save(UCRR_COROUTINE_REGS* r){
    static_cast<X86Context&>(*r)=*uc_regs;
    r->exception_list=MEM32(uc_regs->fs_base);
}
static void ucrr_coroutine_regs_load(const UCRR_COROUTINE_REGS* r){
    *uc_regs=static_cast<const X86Context&>(*r);
    MEM32(uc_regs->fs_base)=r->exception_list;
}
'''+body[b:]
    body=body.replace('RECOMP_TLS','thread_local')
    body=body.replace('ConvertThreadToFiber(NULL)','ConvertThreadToFiberEx(NULL,FIBER_FLAG_FLOAT_SWITCH)')
    body=body.replace('CreateFiber(0, ucrr_coroutine_entry, coroutine)',
                      'CreateFiberEx(0,0,FIBER_FLAG_FLOAT_SWITCH,ucrr_coroutine_entry,coroutine)')
    for name in ('001FB790','001FB930','001FBA40'):
        oldsig=f'void sub_{name}(void)\n{{'
        assert body.count(oldsig)==1
        body=body.replace(oldsig,f'X86_FUNC(sub_{name})\n{{\n    uc_regs=&ctx;uc_base=base;')
    body=body.replace('extern void sub_001FBA40_gen(void);','X86_EXTERN_FUNC(__imp__sub_001FBA40);')
    body=body.replace('sub_001FBA40_gen();','__imp__sub_001FBA40(ctx,base);')
    # Unlike the legacy global-register runtime, all SIMD/flags/x87 state lives in X86Context.
    prefix='''#define NOMINMAX
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
'''
    prefix+=''.join(f'#define g_{r} (uc_regs->{r})\n' for r in ('eax','ecx','edx','ebx','esi','edi','esp','ebp'))
    (AREA/'src/common/xbe_trial_coroutines.cpp').write_text(prefix+body)
    # Reuse the existing interleaved/nested/cleanup fixture against the actual adapted bridge.
    test=(ROOT/'scripts/test_coroutines.c').read_text()
    test=test[test.index('static int stages[3];'):]
    test=test.replace('void sub_001FBA40_gen(void)', 'X86_FUNC(__imp__sub_001FBA40)')
    for name in ('001FB790','001FB930','001FBA40'):
        test=test.replace(f'sub_{name}();',f'sub_{name}(*uc_regs,uc_base);')
    test=test.replace('g_xbox_mem_offset = (ptrdiff_t)memory;','uc_base=(uint8_t*)memory;uc_regs=&test_regs;')
    test=test.replace('g_mmx','uc_regs->mm')
    (AREA/'src/compat/test_coroutines.cpp').write_text('''#include "../common/xbe_trial_coroutines.cpp"
#define CHECK(expr) do { if(!(expr)){std::fprintf(stderr,"FAIL %d: %s\\n",__LINE__,#expr);std::exit(1);} } while(0)
static X86Context test_regs{};
static void callback(uint32_t);
void x86_icall(X86Context&,uint8_t*,uint32_t target){callback(target);}
'''+test)

if __name__=='__main__':prepare()
