/**
 * Urban Chaos: Riot Response - Recompiled Game Entry Point
 *
 * This is the Windows executable that hosts the recompiled game code.
 * It performs the following initialization sequence:
 *
 * 1. Load the original XBE file from disk
 * 2. Initialize the Xbox memory layout (map data sections to original VAs)
 * 3. Initialize the Xbox kernel replacement layer
 * 4. Initialize the kernel bridge (thunk table in Xbox memory)
 * 5. Set up game file paths for I/O redirection
 * 6. Initialize the stack pointer
 * 7. Install VEH crash handler for diagnostics
 * 8. Call the game's original entry point (recompiled)
 *
 * Customize this file for your game:
 *   - Keep the XBE entry point synchronized with config/game.json
 *   - Keep the local XBE and asset paths under ignored game_files/
 *   - Add any CRT global pre-initialization your game needs
 *   - Customize the VEH handler for game-specific crash diagnosis
 *
 * XBE Details (fill in from xbe_parser output):
 *   Title:       Urban Chaos - Riot Response
 *   Title ID:    0x534300F7
 *   Base addr:   0x00010000
 *   Entry point: 0x00082027
 *   Code size:   3,787,500 bytes (.text)
 *   Sections:    17
 *   Kernel imports: 146
 */

#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "host_key_latch.h"
#include "frame_capture_hotkey.h"
#if defined(UCRR_CUSTOM_LIGHTING)
#include "custom_lighting_guest.h"
extern void uc_custom_lighting_host_init(void);
#endif
#if defined(UCRR_LIGHT_RESTORATION)
#include "light_restoration.h"
#endif
#if defined(UCRR_PARTICLE_RESTORATION)
#include "particle_restoration.h"
#endif
#if defined(UCRR_DECAL_RESTORATION)
#include "decal_restoration.h"
#endif
#if defined(UCRR_GLOW_REMOVAL)
#include "glow_removal.h"
#endif
#if defined(UCRR_DECAL_REMOVAL)
#include "decal_removal.h"
#endif
#if defined(UCRR_SPECULAR_REMOVAL)
#include "specular_removal.h"
#endif
#if defined(UCRR_LIGHT_REMOVAL)
#include "light_removal.h"
#endif
#if defined(UCRR_SHADOW_REMOVAL)
#include "shadow_removal.h"
#endif
#if defined(UCRR_FEATURE_REMOVAL)
#include "feature_removal.h"
#endif
#if defined(UCRR_PALETTE_AFFINE_CANDIDATE)
#include "palette_affine_runtime.h"
#endif
#include "nv2a/nv2a_pgraph_d3d11.h"
#define UCRR_FIXUP_WRITE_NOTIFY pgraph_d3d11_notify_ram_write
#include "gpu_software_fixups.h"
#include "apu/apu.h"
#include <stdbool.h>
extern MCPXAPUState *g_apu_state;
extern bool apu_hook_handle_mmio(PCONTEXT,uintptr_t,uint32_t,int);
static int g_audio_bridge;
static int g_direct_vp_stores;
static __declspec(thread) MCPXAPUState *g_guest_list_owner;
int ucrr_take_voice_idle(unsigned handle){
    return g_audio_bridge?mcpx_apu_take_voice_idle(g_apu_state,handle):0;
}
void ucrr_audio_list_begin(void){
    g_guest_list_owner=g_audio_bridge?g_apu_state:NULL;
    mcpx_apu_guest_list_begin(g_guest_list_owner);
}
void ucrr_audio_list_end(void){
    mcpx_apu_guest_list_end(g_guest_list_owner);g_guest_list_owner=NULL;
}

/* xboxrecomp runtime headers */
#include <xbox/xboxrecomp.h>
#include "kernel_ordinal_remap.h"
extern ptrdiff_t g_xbox_mem_offset;

/* Byte-verified MOV site 0042B221. Preserve one ordered device call per store,
 * including its existing mutex, register synchronization and worker wakeup. */
extern void recomp_audio_control_trace(uint32_t address,uint32_t value);
void recomp_mmio_store32(uint32_t address, uint32_t value)
{
#if !UCRR_PLAY_ONLY
    recomp_audio_control_trace(address,value);
#endif
    if (g_direct_vp_stores && g_audio_bridge && g_apu_state &&
        address >= 0xFE820000u && address <= 0xFE82FFFCu && !(address & 3u)) {
        mcpx_apu_mmio_write(g_apu_state, address - 0xFE800000u, value, 4);
    } else {
        *(volatile uint32_t *)((uintptr_t)g_xbox_mem_offset + address) = value;
    }
}

/*
 * If xboxrecomp.h is not an umbrella header in your setup, include
 * the individual headers directly:
 *
 * #include "kernel.h"
 * #include "xbox_memory_layout.h"
 * #include "d3d8_xbox.h"
 * #include "dsound_xbox.h"
 * #include "xinput_xbox.h"
 */

/* ── Global register state (defined in xbox_memory_layout.c) ── */

/* RECOMP_TLS is not optional here. The runtime defines these thread-local, and
 * a plain `extern` referencing a __declspec(thread) variable does not resolve
 * to the calling thread's copy -- it resolves to the image's TLS template. The
 * host side then writes g_esp somewhere the generated code never reads, so the
 * guest starts with every register at zero and faults immediately, having
 * apparently ignored the setup that visibly ran. */
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_esp;
extern RECOMP_TLS uint32_t g_ebx, g_esi, g_edi;
extern RECOMP_TLS uint32_t g_seh_ebp, g_ebp;
/* x87 and SSE state. Global for the same reason the volatile GPRs are: one
 * guest routine can lift to several C functions, so a value written in one
 * body is read in the next. Defined in xbox_memory_layout.c like the rest of
 * the register file. */
extern RECOMP_TLS double g_fp_stack[8];
extern RECOMP_TLS int g_fp_top;
extern RECOMP_TLS uint16_t g_fp_control_word;
extern RECOMP_TLS int g_fp_cmp;
extern RECOMP_TLS RecompXmm g_xmm0, g_xmm1, g_xmm2, g_xmm3;
extern RECOMP_TLS RecompXmm g_xmm4, g_xmm5, g_xmm6, g_xmm7;
extern ptrdiff_t g_xbox_mem_offset;
extern RECOMP_TLS volatile uint32_t g_icall_trace[16];
extern RECOMP_TLS volatile uint32_t g_icall_trace_idx;
extern RECOMP_TLS volatile uint64_t g_icall_count;

/* ── XBE Constants ─────────────────────────────────────────── */

/*
 * TODO: Set these from your xbe_parser output.
 * Run: py -3 -m tools.xbe_parser game/default.xbe
 */
#define UCRR_ENTRY_POINT   0x00082027u
#define UCRR_XBE_PATH      "game_files\\Default.xbe"
#define UCRR_GAME_DIR      "game_files"

/* ── Forward declarations ──────────────────────────────────── */

static BOOL load_xbe(const char *path, void **out_data, size_t *out_size);
static void initialize_crash_symbols(void);
static void start_opt_in_watchdog(void);
static void start_tls_watch(void);
static int g_watch_heap_list;
static uint32_t g_watch_heap_address=0x01000180u;
void ucrr_start_mission_watchdog(void)
{
    static LONG bin_watch_started;
    if(getenv("UCRR_WATCH_HEAP_BIN") && InterlockedCompareExchange(&bin_watch_started,1,0)==0) start_tls_watch();
#if UCRR_PLAY_ONLY
    return;
#endif

    static LONG started;
    static int enabled=-1;
    if(enabled<0) enabled=getenv("UCRR_WATCHDOG_MISSION")!=NULL;
    if(enabled && InterlockedCompareExchange(&started,1,0)==0) {
        if(getenv("UCRR_WATCH_HEAP_LIST")) start_tls_watch();
        start_opt_in_watchdog();
    }
}
static void print_native_frame(HANDLE process, DWORD64 address, unsigned index);
static BOOL initialize_host_graphics(HINSTANCE instance, int show_command);

static HWND g_host_window = NULL;
static IDirect3DDevice8 *g_host_d3d_device = NULL;

/* Recompiled entry point (generated by recomp pipeline) */
extern void xbe_entry_point(void);

/* ── VEH crash handler ─────────────────────────────────────── */

/*
 * Vectored Exception Handler for crash diagnostics.
 *
 * When the recompiled game hits an access violation, this handler prints
 * the faulting address, all Xbox register values, and a native stack trace.
 * This is your primary debugging tool during bring-up.
 *
 * Customize this for your game:
 *   - Add game-specific address checks (GPU register probes, etc.)
 *   - Add dumps of game-specific globals (heap handles, state flags)
 *   - Add SEH simulation if your game uses __try/__except
 */
/* Crash-only bounded reads: no guest writes, and no recursive faults when a
 * corrupt link leaves mapped RAM. This is not a consistent cross-thread dump. */
static void dump_heap_crash(void)
{
    if (!getenv("UCRR_HEAP_CRASH_TRACE")) return;
    uint32_t node=0x01000180u;
    for(unsigned i=0;i<256;++i) {
        uint32_t words[6]; SIZE_T got=0;
        if(node<8 || node>g_xbox_total_ram-16 || (node&3) ||
           !ReadProcessMemory(GetCurrentProcess(),(void *)((uintptr_t)g_xbox_mem_offset+node-8),words,sizeof(words),&got) || got!=sizeof(words)) {
            fprintf(stderr,"[HEAP-CRASH] invalid node=%08X step=%u\n",node,i); break;
        }
        fprintf(stderr,"[HEAP-CRASH] node=%08X header=%08X,%08X next=%08X prev=%08X tail=%08X,%08X\n",node,words[0],words[1],words[2],words[3],words[4],words[5]);
        node=words[2]; if(node==0x01000180u) break;
    }
    const uint32_t regs[]={0xFE801324u,0xFE801334u,0xFE80115Cu};
    for(unsigned i=0;i<3;++i) {
        uint32_t value=0; SIZE_T got=0;
        if(ReadProcessMemory(GetCurrentProcess(),(void *)((uintptr_t)g_xbox_mem_offset+regs[i]),&value,4,&got)&&got==4)
            fprintf(stderr,"[HEAP-CRASH-APU] reg=%08X value=%08X\n",regs[i],value);
    }
}
static LONG CALLBACK veh_handler(PEXCEPTION_POINTERS ep)
{
    if(g_watch_heap_list && ep->ExceptionRecord->ExceptionCode==EXCEPTION_SINGLE_STEP &&
       ep->ContextRecord->Dr0==(DWORD64)g_xbox_mem_offset+g_watch_heap_address &&
       (ep->ContextRecord->Dr6&3)) {
        struct HeapWrite {DWORD64 rip;uint32_t next,prev,esp;};
        static struct HeapWrite recent[8];
        static unsigned count;
        uint32_t *head=(uint32_t *)((uintptr_t)g_xbox_mem_offset+g_watch_heap_address);
        recent[count%8]=(struct HeapWrite){ep->ContextRecord->Rip,head[0],head[1],g_esp};
        ++count;
        if(head[0]<0x10000u || head[0]>g_xbox_total_ram-8 || (head[0]&7) ||
           head[1]<0x10000u || head[1]>g_xbox_total_ram-8 || (head[1]&7) || count>=1000000){
            ep->ContextRecord->Dr7=0;
            (fprintf)(stderr,"[HEAP-LIST-WRITE] writes=%u next=%08X prev=%08X eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X edi=%08X ebp=%08X\n",count,head[0],head[1],g_eax,g_ebx,g_ecx,g_edx,g_esi,g_edi,g_ebp);
            for(unsigned i=count>8?count-8:0;i<count;++i){
                struct HeapWrite *w=&recent[i%8];
                (fprintf)(stderr,"[HEAP-LIST-HISTORY] next=%08X prev=%08X esp=%08X\n",w->next,w->prev,w->esp);
                (fprintf)(stderr,"[HEAP-LIST-WRITER] host_rva=%llX\n",(unsigned long long)(w->rip-(uintptr_t)GetModuleHandleA(NULL)));
            }
        }
        ep->ContextRecord->Dr6=0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
#if !UCRR_PLAY_ONLY
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP &&
        ep->ContextRecord->Dr0 == (DWORD64)g_xbox_mem_offset + 4 &&
        (ep->ContextRecord->Dr6 & 1)) {
        static unsigned hits;
        char storage[sizeof(SYMBOL_INFO)+MAX_SYM_NAME] = {0};
        SYMBOL_INFO *symbol = (SYMBOL_INFO *)storage;
        DWORD64 displacement = 0;
        symbol->SizeOfStruct=sizeof(*symbol); symbol->MaxNameLen=MAX_SYM_NAME;
        SymFromAddr(GetCurrentProcess(),ep->ContextRecord->Rip,&displacement,symbol);
        fprintf(stderr,"[TLS-WRITE] hit=%u value=%08X rip=%llX %s+%llX esp=%08X\n",
            ++hits, *(uint32_t *)((uintptr_t)g_xbox_mem_offset+4),
            (unsigned long long)ep->ContextRecord->Rip, symbol->Name,
            (unsigned long long)displacement,g_esp);
        if(hits<=3) {
            IMAGEHLP_LINE64 line={0}; DWORD line_displacement=0;
            line.SizeOfStruct=sizeof(line);
            if(SymGetLineFromAddr64(GetCurrentProcess(),ep->ContextRecord->Rip,&line_displacement,&line))
                fprintf(stderr,"[TLS-WRITE-SOURCE] %s:%lu\n",line.FileName,line.LineNumber);
            fprintf(stderr,"[TLS-WRITE-REGS] eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X edi=%08X ebp=%08X\n",
                g_eax,g_ebx,g_ecx,g_edx,g_esi,g_edi,g_ebp);
            uint32_t values[3]={g_esi,g_edi,g_ebp};
            for(unsigned j=0;j<3;++j) if(values[j]>=0x10000 && values[j]<g_xbox_total_ram-32) {
                uint32_t *p=(uint32_t *)((uintptr_t)g_xbox_mem_offset+values[j]);
                fprintf(stderr,"[TLS-WRITE-MEM] %08X: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    values[j],p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
            }
        }
        ep->ContextRecord->Dr6=0;
        if(hits>=16) ep->ContextRecord->Dr7=0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
#endif
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        uintptr_t fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
        uintptr_t audio_base=(uintptr_t)g_xbox_mem_offset+0xFE820000u;
        if(g_audio_bridge && fault_addr>=audio_base && fault_addr<audio_base+0x10000u &&
           ep->ExceptionRecord->ExceptionInformation[0]==1 &&
           apu_hook_handle_mmio(ep->ContextRecord,fault_addr,
               0xFE820000u+(uint32_t)(fault_addr-audio_base),1))
            return EXCEPTION_CONTINUE_EXECUTION;

        /*
         * GPU register probe at 0xFD000000 range.
         * Some games probe NV2A registers directly. On real hardware this
         * returns GPU state; here we just skip the instruction.
         * TODO: Implement mini x86-64 decoder for instruction skipping,
         * or connect to the xbox_nv2a library for proper handling.
         */
        if (fault_addr >= 0xFD000000 && fault_addr < 0xFE000000) {
            return EXCEPTION_CONTINUE_SEARCH;
        }

#if UCRR_PLAY_ONLY
        /* Failure-only record: no frame logging or symbol loading in play builds. */
        {char report[320];DWORD written;int n=snprintf(report,sizeof report,
            "[FATAL-AV] host_rva=%llX address=%llX operation=%llu guest_eax=%08X ecx=%08X edx=%08X esp=%08X\n",
            (unsigned long long)(ep->ContextRecord->Rip-(uintptr_t)GetModuleHandleA(NULL)),
            (unsigned long long)fault_addr,(unsigned long long)ep->ExceptionRecord->ExceptionInformation[0],g_eax,g_ecx,g_edx,g_esp);
         if(n>0&&n<sizeof report)WriteFile(GetStdHandle(STD_ERROR_HANDLE),report,(DWORD)n,&written,NULL);}
        return EXCEPTION_CONTINUE_SEARCH;
#endif
        fprintf(stderr, "[CRASH] Access violation at RIP=0x%llX, fault addr=0x%llX (%s)\n",
            (unsigned long long)ep->ContextRecord->Rip,
            (unsigned long long)fault_addr,
            ep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read");
        dump_heap_crash();
        {
            uintptr_t image_base = (uintptr_t)GetModuleHandleA(NULL);
            fprintf(stderr, "  Host image: base=0x%llX RIP RVA=0x%llX\n",
                (unsigned long long)image_base,
                (unsigned long long)(ep->ContextRecord->Rip - image_base));
        }
        {
            HMODULE fault_module = NULL;
            char module_path[MAX_PATH];
            if (GetModuleHandleExA(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    (LPCSTR)(uintptr_t)ep->ContextRecord->Rip,
                    &fault_module) &&
                GetModuleFileNameA(fault_module, module_path,
                                   (DWORD)sizeof(module_path))) {
                fprintf(stderr, "  Fault module: %s+0x%llX\n", module_path,
                    (unsigned long long)(ep->ContextRecord->Rip -
                                         (uintptr_t)fault_module));
            }
        }
        {
            HANDLE process = GetCurrentProcess();
            DWORD64 symbol_displacement = 0;
            DWORD line_displacement = 0;
            char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
            SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbol_buffer;
            IMAGEHLP_LINE64 line = {0};

            memset(symbol_buffer, 0, sizeof(symbol_buffer));
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            if (SymFromAddr(process, ep->ContextRecord->Rip,
                            &symbol_displacement, symbol)) {
                fprintf(stderr, "  Host symbol: %s+0x%llX\n", symbol->Name,
                    (unsigned long long)symbol_displacement);
            }
            if (SymGetLineFromAddr64(process, ep->ContextRecord->Rip,
                                     &line_displacement, &line)) {
                fprintf(stderr, "  Host source: %s:%lu (+0x%lX)\n",
                    line.FileName, line.LineNumber, line_displacement);
            }
        }
        fprintf(stderr, "  Xbox regs: eax=0x%08X ecx=0x%08X edx=0x%08X esp=0x%08X\n",
            g_eax, g_ecx, g_edx, g_esp);
        fprintf(stderr, "  Xbox regs: ebx=0x%08X esi=0x%08X edi=0x%08X\n",
            g_ebx, g_esi, g_edi);
        fprintf(stderr, "  Xbox VA of fault: 0x%08X\n",
            (uint32_t)(fault_addr - (uintptr_t)g_xbox_mem_offset));
        fprintf(stderr,
            "  Guest stack: %08X %08X %08X %08X %08X %08X\n",
            *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp),
            *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp + 4),
            *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp + 8),
            *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp + 12),
            *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp + 16),
            *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp + 20));
        fprintf(stderr, "  Recent indirect calls (%llu total):\n",
            (unsigned long long)g_icall_count);
        for (int i = 0; i < 16; ++i) {
            const uint32_t index = (g_icall_trace_idx - 16u + (uint32_t)i) & 15u;
            if (g_icall_trace[index]) {
                fprintf(stderr, "    0x%08X\n", g_icall_trace[index]);
            }
        }

        /* The first allocator failure during bring-up occurred inside the XDK
         * heap's 128 small-block free-list table. Report its invariants without
         * changing guest memory so a zeroed table can be distinguished from a
         * damaged individual bucket. */
        if (g_esi >= 0x00010000u && g_esi < 0x04000000u && g_edi < 128u) {
            uint32_t zero_heads = 0;
            uint32_t empty_heads = 0;
            uint32_t linked_heads = 0;
            uint32_t head_va = g_esi + 0x180u + g_edi * 8u;
            uint32_t *head = (uint32_t *)((uintptr_t)g_xbox_mem_offset + head_va);
            char head_states[129];

            for (uint32_t i = 0; i < 128u; ++i) {
                uint32_t current_va = g_esi + 0x180u + i * 8u;
                uint32_t *current = (uint32_t *)((uintptr_t)g_xbox_mem_offset + current_va);
                if (current[0] == 0 && current[1] == 0) {
                    ++zero_heads;
                    head_states[i] = 'Z';
                } else if (current[0] == current_va && current[1] == current_va) {
                    ++empty_heads;
                    head_states[i] = 'E';
                } else {
                    ++linked_heads;
                    head_states[i] = 'L';
                }
            }
            head_states[128] = '\0';

            fprintf(stderr,
                "  XDK heap: base=0x%08X bucket=%u head=0x%08X "
                "flink=0x%08X blink=0x%08X\n",
                g_esi, g_edi, head_va, head[0], head[1]);
            fprintf(stderr,
                "  XDK heap heads: zero=%u empty/self=%u linked=%u; "
                "bitmap[0..3]=%08X %08X %08X %08X\n",
                zero_heads, empty_heads, linked_heads,
                *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esi + 0x160u),
                *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esi + 0x164u),
                *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esi + 0x168u),
                *(uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esi + 0x16Cu));
            fprintf(stderr,
                "  XDK heap head states (0..127; E=empty, L=linked, Z=zero):\n"
                "    %.*s\n    %s\n",
                64, head_states, head_states + 64);
        }

        /*
         * TODO: Add game-specific diagnostics here. Examples:
         *
         * Dump CRT heap handle:
         *   uint32_t heap = *(uint32_t *)((uint8_t *)g_xbox_mem_offset + HEAP_HANDLE_VA);
         *   fprintf(stderr, "  CRT heap handle: 0x%08X\n", heap);
         *
         * Dump game state:
         *   uint32_t state = *(uint32_t *)((uint8_t *)g_xbox_mem_offset + GAME_STATE_VA);
         *   fprintf(stderr, "  Game state: %u\n", state);
         */

        /* Print return addresses that fall inside this executable.  The old
         * bring-up filter assumed the non-ASLR 0x140000000 image range, which
         * silently discarded every useful frame once the linker randomized
         * the image base. */
        {
            HANDLE process = GetCurrentProcess();
            uintptr_t image_base = (uintptr_t)GetModuleHandleA(NULL);
            IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)image_base;
            IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(
                image_base + (uintptr_t)dos->e_lfanew);
            uintptr_t image_end = image_base + nt->OptionalHeader.SizeOfImage;
            uintptr_t *sp = (uintptr_t *)ep->ContextRecord->Rsp;
            unsigned found = 0;
            fprintf(stderr, "  Native executable return addresses:\n");
            for (int i = 0; i < 192 && found < 16; ++i) {
                if (sp[i] >= image_base && sp[i] < image_end) {
                    fprintf(stderr, "    stack[%d] ", i);
                    print_native_frame(process, (DWORD64)sp[i], found++);
                }
            }
        }
        fflush(stderr);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

/* ── Opt-in native stack sampler ───────────────────────────── */
#include "watchdog_snapshot.h"
static UCRR_STACK_SNAPSHOT *g_unwind_snapshot;
static BOOL CALLBACK watchdog_read_memory(HANDLE process,DWORD64 address,PVOID buffer,DWORD size,LPDWORD read)
{
    UCRR_STACK_SNAPSHOT *s=g_unwind_snapshot;
    if(s && address>=s->base && address-s->base<s->size){
        if(size>s->size-(SIZE_T)(address-s->base))return FALSE;
        memcpy(buffer,s->bytes+(SIZE_T)(address-s->base),size);*read=size;return TRUE;
    }
    /* Non-stack reads are unwind metadata/code from loaded modules. */
    MEMORY_BASIC_INFORMATION region;
    if(!VirtualQueryEx(process,(void *)(uintptr_t)address,&region,sizeof(region)) || region.Type!=MEM_IMAGE)return FALSE;
    SIZE_T received=0;
    BOOL ok=ReadProcessMemory(process,(void *)(uintptr_t)address,buffer,size,&received);
    *read=(DWORD)received;return ok;
}

typedef struct UCRR_WATCHDOG_CONTEXT {
    HANDLE target_thread;
    DWORD interval_ms;
} UCRR_WATCHDOG_CONTEXT;

static void print_native_frame(HANDLE process, DWORD64 address, unsigned index)
{
    DWORD64 symbol_displacement = 0;
    DWORD line_displacement = 0;
    char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbol_buffer;
    IMAGEHLP_LINE64 line = {0};

    memset(symbol_buffer, 0, sizeof(symbol_buffer));
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    fprintf(stderr, "  #%u 0x%llX", index, (unsigned long long)address);
    if (SymFromAddr(process, address, &symbol_displacement, symbol)) {
        fprintf(stderr, " %s+0x%llX", symbol->Name,
                (unsigned long long)symbol_displacement);
    }
    if (SymGetLineFromAddr64(process, address, &line_displacement, &line)) {
        fprintf(stderr, " at %s:%lu", line.FileName, line.LineNumber);
    }
    fputc('\n', stderr);
}

static DWORD WINAPI watchdog_thread_main(LPVOID parameter)
{
    UCRR_WATCHDOG_CONTEXT *watchdog = (UCRR_WATCHDOG_CONTEXT *)parameter;
    HANDLE process = GetCurrentProcess();
    UCRR_STACK_SNAPSHOT snapshot;

    unsigned sample_limit=3;
    const char *sample_option=getenv("UCRR_WATCHDOG_SAMPLES");
    if(sample_option){unsigned n=(unsigned)strtoul(sample_option,NULL,10);if(n>=1&&n<=64)sample_limit=n;}
    for (unsigned sample = 1; sample <= sample_limit; ++sample) {
        CONTEXT context = {0};
        STACKFRAME64 frame = {0};

        Sleep(watchdog->interval_ms);
        if (!ucrr_capture_stack(process,watchdog->target_thread,&snapshot)) break;
        context=snapshot.context;
        g_unwind_snapshot=&snapshot;

        frame.AddrPC.Offset = context.Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Rsp;
        frame.AddrStack.Mode = AddrModeFlat;

        fprintf(stderr,
                "[WATCHDOG] sample %u after %lu ms: captured stack; target resumed; guest counters below are live\n",
                sample, (unsigned long)(sample * watchdog->interval_ms));
        {
            const uint32_t d3d_device =
                *(volatile uint32_t *)((uintptr_t)g_xbox_mem_offset +
                                       0x003F4238u);
            const uint32_t dsound_reset_count =
                *(volatile uint32_t *)((uintptr_t)g_xbox_mem_offset +
                                       0x00441C64u);

            if (d3d_device != 0 && d3d_device < 0x04000000u) {
                const volatile uint32_t *device =
                    (const volatile uint32_t *)((uintptr_t)g_xbox_mem_offset +
                                                d3d_device);
                const uint32_t consumer_va = device[0x30u / 4u];
                uint32_t consumer = 0xFFFFFFFFu;
                if ((consumer_va & 3u) == 0 && consumer_va < 0x04000000u) {
                    consumer = *(volatile uint32_t *)(
                        (uintptr_t)g_xbox_mem_offset + consumer_va);
                }
                fprintf(stderr,
                        "  Guest state: dsound resets=%u D3DDevice=%08X "
                        "producer=%08X consumer_ptr=%08X consumer=%08X\n",
                        dsound_reset_count, d3d_device, device[0x2Cu / 4u],
                        consumer_va, consumer);
                fprintf(stderr,
                        "  D3DDevice[00..3F]: "
                        "%08X %08X %08X %08X %08X %08X %08X %08X "
                        "%08X %08X %08X %08X %08X %08X %08X %08X\n",
                        device[0], device[1], device[2], device[3],
                        device[4], device[5], device[6], device[7],
                        device[8], device[9], device[10], device[11],
                        device[12], device[13], device[14], device[15]);
            } else {
                fprintf(stderr,
                        "  Guest state: dsound resets=%u D3DDevice=%08X\n",
                        dsound_reset_count, d3d_device);
            }

        }
        for (unsigned depth = 0; depth < 16 && frame.AddrPC.Offset; ++depth) {
            print_native_frame(process, frame.AddrPC.Offset, depth);
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process,
                             watchdog->target_thread, &frame, &context, watchdog_read_memory,
                             SymFunctionTableAccess64, SymGetModuleBase64,
                             NULL)) {
                break;
            }
        }
        fflush(stderr);
        g_unwind_snapshot=NULL;
    }

    CloseHandle(watchdog->target_thread);
    free(watchdog);
    return 0;
}

static void start_opt_in_watchdog(void)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    char value[32];
    DWORD length = GetEnvironmentVariableA("UCRR_WATCHDOG_MS", value,
                                           (DWORD)sizeof(value));
    unsigned long requested_ms;
    UCRR_WATCHDOG_CONTEXT *watchdog;
    HANDLE sampler;

    if (length == 0 || length >= sizeof(value)) {
        return;
    }
    requested_ms = strtoul(value, NULL, 10);
    if (requested_ms < 1000 || requested_ms > 60000) {
        fprintf(stderr,
                "[WATCHDOG] ignoring UCRR_WATCHDOG_MS=%s (valid: 1000-60000)\n",
                value);
        return;
    }

    watchdog = (UCRR_WATCHDOG_CONTEXT *)calloc(1, sizeof(*watchdog));
    if (!watchdog) {
        return;
    }
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                         GetCurrentProcess(), &watchdog->target_thread,
                         THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                             THREAD_QUERY_INFORMATION,
                         FALSE, 0)) {
        free(watchdog);
        return;
    }
    watchdog->interval_ms = (DWORD)requested_ms;
    sampler = CreateThread(NULL, 0, watchdog_thread_main, watchdog, 0, NULL);
    if (!sampler) {
        CloseHandle(watchdog->target_thread);
        free(watchdog);
        return;
    }
    CloseHandle(sampler);
    fprintf(stderr, "[WATCHDOG] enabled at %lu ms intervals (3 samples)\n",
            requested_ms);
}

/* ── Host graphics window ──────────────────────────────────── */

static UCRR_KEY_LATCH host_key_latch;
static void ucrr_host_pause_cancel(void);
static void ucrr_host_pause_event(UINT,WPARAM,LPARAM);
#include "hybrid_frontend_guest.h"
static int hf_window_input(HWND,UINT,WPARAM,LPARAM);
#include "pc_mouse_host.h"
#include "xbox_overlay.h"
#include "menu_pointer_host.h"
#include "hybrid_frontend_host.h"
/* Escape joins the original controller publication, so physical and logical
 * readers share its edge. Menu gathering must not consume a gameplay press. */
static SRWLOCK host_pause_lock=SRWLOCK_INIT;
static uint64_t host_pause_at;
static int host_pause_pending,host_pause_suppressed;
static void ucrr_host_pause_cancel(void)
{
    if(!hf_configured())return;
    AcquireSRWLockExclusive(&host_pause_lock);
    host_pause_pending=0;
    host_pause_suppressed=(GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
    ReleaseSRWLockExclusive(&host_pause_lock);
}
static void ucrr_host_pause_event(UINT message,WPARAM key,LPARAM flags)
{
#if defined(UCRR_HYBRID_PC_FRONTEND)
    if(!hf_configured())return;
    if(message==WM_KILLFOCUS||(message==WM_ACTIVATEAPP&&!key)){
        ucrr_host_pause_cancel();return;
    }
    if(key!=VK_ESCAPE)return;
    if((message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&!(flags&((1LL<<29)|(1LL<<30)))){
        if(pc_menu_active()||xbox_InputIsBlocked()||hf_native_visible()||hf_movie){ucrr_host_pause_cancel();return;}
        AcquireSRWLockExclusive(&host_pause_lock);
        if(!host_pause_suppressed){host_pause_pending=1;host_pause_at=GetTickCount64();}
        ReleaseSRWLockExclusive(&host_pause_lock);
    }else if(message==WM_KEYUP||message==WM_SYSKEYUP){
        AcquireSRWLockExclusive(&host_pause_lock);host_pause_suppressed=0;ReleaseSRWLockExclusive(&host_pause_lock);
    }
#else
    (void)message;(void)key;(void)flags;
#endif
}
int ucrr_host_pause_button(void)
{
#if defined(UCRR_HYBRID_PC_FRONTEND)
    if(!hf_configured())return 0;
    int held=(GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
    int allowed=GetForegroundWindow()==pc_mouse_window&&!pc_menu_active()&&
        !xbox_InputIsBlocked()&&!hf_native_visible()&&!hf_movie&&!(GetAsyncKeyState(VK_MENU)&0x8000);
    AcquireSRWLockExclusive(&host_pause_lock);
    if(!held)host_pause_suppressed=0;
    int down=allowed&&!host_pause_suppressed&&
        (held||(host_pause_pending&&GetTickCount64()-host_pause_at<=250));
    host_pause_pending=0;
    if(!allowed)host_pause_suppressed=held;
    ReleaseSRWLockExclusive(&host_pause_lock);
    return down;
#else
    return 0;
#endif
}

/* Command drag is rendered inside the original HUD pad. */
void ucrr_host_begin_frontend_input(void)
{
    DWORD owner = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &owner);
    ucrr_key_begin(&host_key_latch, GetTickCount64(), owner == GetCurrentProcessId());
}
void ucrr_host_end_frontend_input(void) { ucrr_key_end(&host_key_latch); }
int ucrr_host_take_menu_press(uint32_t action)
{
    DWORD owner = 0;
    unsigned binding = ucrr_menu_action_binding(action);
    if (action != 0x1D && !host_key_latch.depth) return 0;
    if (binding >= UCRR_CONTROLLER_ACTIONS || xbox_InputIsBlocked()) return 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &owner);
    int pressed = 0;
    if(hf_configured()&&pc_menu_active()&&action==0x17u)
        pressed|=ucrr_key_take(&host_key_latch,VK_ESCAPE,GetTickCount64(),owner==GetCurrentProcessId());
    for (unsigned slot = 0; slot < 2; ++slot) {
        unsigned key = xbox_KeyboardBinding(binding, slot);
        if (key) pressed |= ucrr_key_take(&host_key_latch, key, GetTickCount64(),
                                         owner == GetCurrentProcessId());
    }
    return pressed;
}

static unsigned pc_key_source(WPARAM key, LPARAM flags)
{
    if (key == VK_SHIFT) return MapVirtualKeyA((unsigned)(flags >> 16) & 255u, MAPVK_VSC_TO_VK_EX);
    if (key == VK_CONTROL) return flags & (1LL << 24) ? VK_RCONTROL : VK_LCONTROL;
    if (key == VK_MENU) return flags & (1LL << 24) ? VK_RMENU : VK_LMENU;
    return (unsigned)key;
}

static LRESULT CALLBACK host_window_proc(HWND window, UINT message,
                                         WPARAM wparam, LPARAM lparam)
{
    if(hf_window_input(window,message,wparam,lparam))return message==WM_SETCURSOR?TRUE:0;
#if defined(UCRR_FEATURE_REMOVAL)
    if (uc_feature_removal_key(message, wparam, lparam)) return 0;
#endif
#if defined(UCRR_PALETTE_AFFINE_CANDIDATE) && !UCRR_PLAY_ONLY
    if(wparam==VK_F10 && !(lparam&(1LL<<29)) &&
       (message==WM_KEYDOWN||message==WM_SYSKEYDOWN||message==WM_KEYUP||message==WM_SYSKEYUP) &&
       ucrr_palette_affine_probe_enabled){
        if((message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&!(lparam&(1LL<<30)))
            ucrr_palette_affine_probe_arm();
        return 0;
    }
#endif
#if UCRR_CAUSAL_TRACE && !UCRR_PLAY_ONLY
    if(wparam==VK_F10 && !(lparam&(1LL<<29)) &&
       (message==WM_KEYDOWN||message==WM_SYSKEYDOWN||message==WM_KEYUP||message==WM_SYSKEYUP)){
        const char*probe=getenv("UCRR_CAUSAL_TRACE");
        if(probe&&!strcmp(probe,"1")){
            extern void nv_causal_mark(void);
            if((message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&!(lparam&(1LL<<30)))nv_causal_mark();
            return 0;
        }
    }
#endif
#if defined(UCRR_MATERIAL_OPCODE_PROBE) && !UCRR_PLAY_ONLY
    if(wparam==VK_F10 && !(lparam&(1LL<<29)) &&
       (message==WM_KEYDOWN||message==WM_SYSKEYDOWN||message==WM_KEYUP||message==WM_SYSKEYUP)){
        const char*probe=getenv("UCRR_MATERIAL_OPCODE_PROBE");
        if(probe&&!strcmp(probe,"1")){
            extern void uc_material_probe_arm(void);
            if((message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&!(lparam&(1LL<<30)))uc_material_probe_arm();
            return 0;
        }
    }
#endif
#if !UCRR_PLAY_ONLY
    static int input_trace = -1;
    static unsigned key_samples;
    if (input_trace < 0) input_trace = getenv("UCRR_INPUT_TRACE") != NULL && getenv("UCRR_QUIET_PLAY") == NULL;
    if (input_trace && key_samples < 128u &&
        (message == WM_KEYDOWN || message == WM_KEYUP)) {
        ++key_samples;
        fprintf(stderr, "[INPUT-WINDOW] ms=%llu message=%04X key=%llu flags=%08llX sample=%u\n",
                (unsigned long long)GetTickCount64(), message,
                (unsigned long long)wparam, (unsigned long long)lparam, key_samples);
    }
#endif
#if !UCRR_PLAY_ONLY
    /* A diagnostic F10 must not enter DefWindowProc's modal menu path.
     * Latch at delivery so short taps survive until the next drain boundary. */
    if(wparam==VK_F10 && !(lparam&(1LL<<29)) &&
       (message==WM_KEYDOWN||message==WM_SYSKEYDOWN||message==WM_KEYUP||message==WM_SYSKEYUP)){
        const char*probe=getenv("UCRR_DEPENDENCY_PROBE");
        if(probe&&!strcmp(probe,"1")){
            if((message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&!(lparam&(1LL<<30)))
                pgraph_d3d11_dependency_mark();
            return 0;
        }
    }
#endif
    /* Windows delivers bare F10 as WM_SYSKEYDOWN. Consume both message
     * forms before default menu activation; never intercept Alt+F10. */
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
        wparam == VK_F10 && !(lparam & ((1LL << 30) | (1LL << 29))) &&
        getenv("UCRR_REMOVAL_INLINE")) {
        pgraph_d3d11_removal_toggle();
        return 0;
    }
    switch (message) {
    case WM_APP+74:return 0; /* renderer reads gesture feedback */
    case WM_INPUT:pc_mouse_raw(lparam);break;
    case WM_LBUTTONDOWN:if(!xbox_overlay_open){xbox_KeyboardNoteInput();if(pc_menu_active())pc_menu_click();else pc_mouse_capture();}break;
    case UCRR_MENU_CURSOR_MESSAGE:pc_menu_apply_capture();break;
    case WM_SETCURSOR:if(pc_mouse_captured&&LOWORD(lparam)==HTCLIENT){SetCursor(NULL);return TRUE;}break;
    case WM_ACTIVATEAPP:if(!wparam)ucrr_host_release_mouse();break;
    case WM_CAPTURECHANGED:if(pc_mouse_captured&&(HWND)lparam!=window)ucrr_host_release_mouse();break;
    case WM_KEYDOWN:
        if(uc_frame_capture_hotkey(wparam,lparam))return 0;
#if !UCRR_PLAY_ONLY
        if(wparam==VK_F9&&!(lparam&(1LL<<30))&&getenv("UCRR_EFFECT_REFS")){pgraph_d3d11_effect_toggle();return 0;}
        if(wparam==VK_F8&&!(lparam&(1LL<<30))&&(getenv("UCRR_HOSTAGE_TRACE")||getenv("UCRR_MARK_FRAME_COST"))){
            LARGE_INTEGER qpc;QueryPerformanceCounter(&qpc);
            nv2a_pb_live_mark_slowdown();
            fprintf(stderr,"[USER-MARK] ms=%llu qpc=%lld key=F8\n",(unsigned long long)GetTickCount64(),qpc.QuadPart);fflush(stderr);return 0;
        }
#endif
        if(wparam==VK_ESCAPE&&!hf_configured()){if(!(lparam&(1LL<<30))){ucrr_key_clear(&host_key_latch);xbox_overlay_show(window,!xbox_overlay_open);}return 0;}
        xbox_KeyboardSourceEvent(pc_key_source(wparam,lparam),1);
        ucrr_key_record(&host_key_latch, pc_key_source(wparam,lparam),
                        (lparam & (1LL << 30)) != 0, GetTickCount64());
        /* Keep generic Shift/Ctrl/Alt bindings compatible with older settings. */
        if (pc_key_source(wparam,lparam) != (unsigned)wparam)
            ucrr_key_record(&host_key_latch, (unsigned)wparam,
                           (lparam & (1LL << 30)) != 0, GetTickCount64());
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        xbox_KeyboardSourceEvent(pc_key_source(wparam,lparam),0);
        break;
    case WM_SYSKEYDOWN:
        if (!(lparam & (1LL << 29))) {
            xbox_KeyboardSourceEvent(pc_key_source(wparam,lparam),1);
            ucrr_key_record(&host_key_latch,pc_key_source(wparam,lparam),
                           (lparam & (1LL << 30)) != 0,GetTickCount64());
            if (pc_key_source(wparam,lparam) != (unsigned)wparam)
                ucrr_key_record(&host_key_latch,(unsigned)wparam,
                               (lparam & (1LL << 30)) != 0,GetTickCount64());
        }
        break;
    case WM_KILLFOCUS:
        pc_menu_clear();
        ucrr_key_clear(&host_key_latch);
        break;
    case WM_SIZE:
    case WM_MOVE:
        xbox_overlay_layout(window);if(pc_mouse_captured)pc_mouse_clip();break;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        ucrr_host_release_mouse();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(window, message, wparam, lparam);
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

static BOOL initialize_host_graphics(HINSTANCE instance, int show_command)
{
    static const char class_name[] = "UCRRRecompiledWindow";
    WNDCLASSEXA window_class;
    RECT window_rect = {0, 0, 640, 480};
    D3DPRESENT_PARAMETERS present;
    IDirect3D8 *d3d;
    HRESULT result;

    memset(&window_class, 0, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = host_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = class_name;
    if (!RegisterClassExA(&window_class) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        fprintf(stderr, "D3D8: RegisterClassEx failed (error %lu)\n",
                GetLastError());
        return FALSE;
    }

    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    g_host_window = CreateWindowExA(
        0, class_name,
        "Urban Chaos: Riot Response - Recompiled",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
        NULL, NULL, instance, NULL);
    if (!g_host_window) {
        fprintf(stderr, "D3D8: CreateWindowEx failed (error %lu)\n",
                GetLastError());
        return FALSE;
    }

    ShowWindow(g_host_window, show_command ? show_command : SW_SHOW);
    /* A launcher's STARTUPINFO can override the first ShowWindow call. This
     * interactive game has no headless mode: repair a hidden/minimized start
     * once, without stealing focus again during normal play. */
    if (!IsWindowVisible(g_host_window) || IsIconic(g_host_window))
        ShowWindow(g_host_window, SW_RESTORE);
    UpdateWindow(g_host_window);
    fprintf(stderr, "[HOST-WINDOW] visible=%d minimized=%d requested_show=%d\n",
            IsWindowVisible(g_host_window), IsIconic(g_host_window), show_command);

    xbox_overlay_initialize(g_host_window,instance);
    pc_mouse_initialize(g_host_window,xbox_output_window);

    memset(&present, 0, sizeof(present));
    present.BackBufferWidth = 640;
    present.BackBufferHeight = 480;
    present.BackBufferFormat = D3DFMT_X8R8G8B8;
    present.BackBufferCount = 1;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.hDeviceWindow = xbox_output_window;
    present.Windowed = TRUE;
    present.EnableAutoDepthStencil = TRUE;
    present.AutoDepthStencilFormat = D3DFMT_D24S8;

    d3d = xbox_Direct3DCreate8(0);
    result = d3d->lpVtbl->CreateDevice(
        d3d, 0, 0, xbox_output_window, 0, &present, &g_host_d3d_device);
    if (FAILED(result) || !g_host_d3d_device) {
        fprintf(stderr, "D3D8: Host device creation failed (0x%08lX)\n",
                result);
        DestroyWindow(g_host_window);
        g_host_window = NULL;
        return FALSE;
    }

    g_host_d3d_device->lpVtbl->Clear(
        g_host_d3d_device, 0, NULL, D3DCLEAR_TARGET, 0xFF000000u,
        1.0f, 0);
    hf_initialize();
    d3d8_PresentFrame();
    nv2a_pb_live_init();
    /* This adapter only patches CPU-owned fixup records and notifies writes;
     * it has no guest reentry, GPU calls or result waits. Experimental scope. */
    const char *batch_fixups=getenv("UCRR_QUERY_BATCH_FIXUPS");
    if(batch_fixups&&strcmp(batch_fixups,"1")==0)
        pgraph_d3d11_set_cpu_only_software_method_handler(ucrr_gpu_fixups);
    else pgraph_d3d11_set_tracked_software_method_handler(ucrr_gpu_fixups);
    pgraph_d3d11_set_gpu_registers((const uint8_t *)((uintptr_t)g_xbox_mem_offset+0xFD000000ull),0x01000000u);
    return TRUE;
}

/* ── WinMain ───────────────────────────────────────────────── */

static DWORD WINAPI install_tls_watch(LPVOID target)
{
    CONTEXT c={0}; c.ContextFlags=CONTEXT_DEBUG_REGISTERS;
    int armed=0;
    if(SuspendThread((HANDLE)target)!=(DWORD)-1) {
        if(GetThreadContext((HANDLE)target,&c) && !(c.Dr7 & 255)) {
            c.Dr0=(DWORD64)g_xbox_mem_offset+(g_watch_heap_list?g_watch_heap_address:4u);
            c.Dr1=g_watch_heap_list?(DWORD64)g_xbox_mem_offset+g_watch_heap_address+0x80000000u:0;
            c.Dr6=0; c.Dr7=g_watch_heap_list?0x990005:0xD0001;
            armed=!!SetThreadContext((HANDLE)target,&c);
        }
        ResumeThread((HANDLE)target);
    }
    (fprintf)(stderr,"[%s-WATCH] armed=%d\n",g_watch_heap_list?"HEAP-LIST":"TLS",armed);
    CloseHandle((HANDLE)target);
    return 0;
}

static void start_tls_watch(void)
{
    const char *bin=getenv("UCRR_WATCH_HEAP_BIN");
#if UCRR_PLAY_ONLY
    if(!bin)return;
#endif
    if(bin){
        char *end;unsigned long value=strtoul(bin,&end,10);
        if(!*bin || *end || value>127)return;
        g_watch_heap_address=0x01000180u+(uint32_t)value*8;
    }

    char value[8]; HANDLE target=NULL;
    g_watch_heap_list=bin!=NULL || getenv("UCRR_WATCH_HEAP_LIST")!=NULL;
    if(!g_watch_heap_list && !GetEnvironmentVariableA("UCRR_WATCH_TLS",value,sizeof(value)))return;
    if(!DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),GetCurrentProcess(),
        &target,THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT,FALSE,0))return;
    HANDLE worker=CreateThread(NULL,0,install_tls_watch,target,0,NULL);
    if(worker) {WaitForSingleObject(worker,5000);CloseHandle(worker);}
    else CloseHandle(target);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    void *xbe_data = NULL;
    size_t xbe_size = 0;

    (void)hPrevInstance;
    (void)lpCmdLine;

    /* Original dashboard aspect selection drives the game's own projection,
     * frustum and UI policy; never patch a shared 4/3 floating-point constant. */
    extern unsigned d3d8_GetHostAspect(void);
    extern void xbox_SetHostWidescreen(int enabled);
    xbox_SetHostWidescreen(d3d8_GetHostAspect()==1);

    /* Optional batched diagnostics: avoid a host write for each fprintf.
     * Static buffers outlive worker threads; normal exit/fatal flushes retain
     * their existing behavior. Abrupt termination can lose the final64KiB. */
    static char stdout_buffer[65536],stderr_buffer[65536];
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    if(getenv("UCRR_BUFFERED_LOGS")){
        setvbuf(stdout,stdout_buffer,_IOFBF,sizeof(stdout_buffer));
        setvbuf(stderr,stderr_buffer,_IOFBF,sizeof(stderr_buffer));
    }

    printf("=== Urban Chaos: Riot Response - Static Recompilation ===\n");
    /* Publish the optional host-only lookup table before any guest execution.
     * Manual overrides and synthetic kernel dispatch retain their precedence.
     * Allocation failure keeps the generated binary-search fallback. */
    if (getenv("UCRR_FLAT_DISPATCH")) {
        extern int recomp_dispatch_init(void);
        extern size_t recomp_dispatch_flat_bytes(void);
        int flat = recomp_dispatch_init();
        fprintf(stderr, "[DISPATCH] flat=%d bytes=%zu\n", flat,
                recomp_dispatch_flat_bytes());
    }
    {extern int g_recomp_call_cache_enabled;const char*s=getenv("UCRR_CPU_CALL_CACHE");
     g_recomp_call_cache_enabled=s&&!strcmp(s,"1");}
    {extern void uc_scene_getter_init(void);uc_scene_getter_init();}
    printf("Loading XBE...\n");

    initialize_crash_symbols();

    /* Install VEH handler (first handler in chain) */
    AddVectoredExceptionHandler(1, veh_handler);

    /* Step 1: Load XBE */
    if (!load_xbe(UCRR_XBE_PATH, &xbe_data, &xbe_size)) {
        MessageBoxA(NULL, "Failed to load default.xbe.\n"
                    "Place the game files in the 'game' subdirectory.",
                    "Urban Chaos Recompiled", MB_ICONERROR);
        return 1;
    }
    printf("XBE loaded: %zu bytes\n", xbe_size);

    /* Step 2: Initialize Xbox memory layout */
    /* Diagnostic only: distinguish downstream loader/rendering failures from
     * the incomplete reservation/physical-page accounting. Never enabled by
     * default and never evidence of retail-memory fidelity. */
    {
        char diagnostic_ram[8];
        DWORD n = GetEnvironmentVariableA("UCRR_DIAGNOSTIC_RAM_MB", diagnostic_ram,
                                         sizeof(diagnostic_ram));
        if (n == 3 && strcmp(diagnostic_ram, "128") == 0) {
            xbox_SetTotalRam(XBOX_DEVKIT_RAM);
            fprintf(stderr, "[DIAGNOSTIC] 128 MB RAM override; not retail-memory verification\n");
        }
    }
    printf("Initializing Xbox memory layout...\n");
    xbox_EnablePhysicalHeapAliases(1);
    /* XDK visibility reports require physical addresses below 16 MB. Keep
     * the heap's existing low 512 KB available without borrowing XBE/stack RAM. */
    xbox_SetHeapGeneralFloor(0x01000000u);
    if (!xbox_MemoryLayoutInit(xbe_data, xbe_size)) {
        MessageBoxA(NULL, "Failed to initialize Xbox memory layout.\n"
                    "The required virtual address range may be unavailable.",
                    "Urban Chaos Recompiled", MB_ICONERROR);
        free(xbe_data);
        return 1;
    }

    g_xbox_mem_offset = xbox_GetMemoryOffset();
#if defined(UCRR_CUSTOM_LIGHTING)
    uc_custom_lighting_guest_init();
    uc_custom_lighting_host_init();
#endif
#if defined(UCRR_LIGHT_RESTORATION)
    uc_light_restoration_init();
#endif
#if defined(UCRR_PARTICLE_RESTORATION)
    uc_particle_restoration_init();
#endif
#if defined(UCRR_DECAL_RESTORATION)
    uc_decal_restoration_init();
#endif
#if defined(UCRR_GLOW_REMOVAL)
    uc_glow_removal_init();
#endif
#if defined(UCRR_DECAL_REMOVAL)
    uc_decal_removal_init();
#endif
#if defined(UCRR_SPECULAR_REMOVAL)
    uc_specular_removal_init();
#endif
#if defined(UCRR_LIGHT_REMOVAL)
    uc_light_removal_init();
#endif
#if defined(UCRR_SHADOW_REMOVAL)
    uc_shadow_removal_init();
#endif
#if defined(UCRR_FEATURE_REMOVAL)
    uc_feature_removal_init();
#endif
#if defined(UCRR_PALETTE_AFFINE_CANDIDATE)
    ucrr_palette_affine_init(g_xbox_mem_offset);
#endif
    /* This title allocates D3D movie buffers from the low heap and releases
     * them via cached physical aliases. It does not use the pinned-pool
     * ownership model for those buffers. Enable alias matching for frees. */
    xbox_EnableHeapPhysicalAliasFree(1);
    printf("Xbox memory mapped. Offset: 0x%llX\n", (unsigned long long)g_xbox_mem_offset);

    /* The original DSOUND library resets the MCPX AC97 controller and polls
     * global-status bit 8 (codec ready) at 0xFEC00130. The device aperture is
     * currently RAM-backed, so no hardware exists to raise that bit after the
     * write to global control. Seed the stable ready state that retail Xbox
     * hardware presents after reset; without it DirectSoundCreate returns
     * 0x88780078 and leaves the title's output interface null. */
    *(uint32_t *)((uintptr_t)g_xbox_mem_offset + 0xFEC00130u) = 0x00000100u;

    /* This XDK D3D build stores the push-buffer consumer counter through
     * [D3DDevice + 0x30] in ordinary Xbox RAM, with the producer at +0x2C.
     * Legacy timer acknowledgement keeps some setup paths moving, but does
     * not consume commands. Ordered semaphore/PGRAPH ownership remains a
     * known issue (including the intermittent startup wait003E92C0). */
    xbox_RegisterSoftwarePushBufferAck(0x003F4238u, 0x2Cu, 0x30u);

    /* Step 3: Initialize Xbox kernel */
    printf("Initializing Xbox kernel replacement...\n");
    xbox_kernel_init();

    /* Step 4: Set game directory for file I/O path translation */
    {
        extern void xbox_path_init(const char *game_dir, const char *save_dir);
#if defined(UCRR_HYBRID_NATIVE_LIGHTING)
        const char *hybrid_saves=getenv("UCRR_HYBRID_SAVE_DIR");
        if(!hybrid_saves||!hybrid_saves[0]){
            MessageBoxA(NULL,"Launch this experiment through its Hybrid launcher to isolate saves.",
                "Urban Chaos Hybrid",MB_ICONERROR);return 1;
        }
        xbox_path_init(UCRR_GAME_DIR,hybrid_saves);
#else
        xbox_path_init(UCRR_GAME_DIR, NULL);
#endif
    }

    /* Keep the generated per-title mapping hook explicit. For this XBE the
     * audited result is identity (all 146 imports already use the runtime's
     * canonical Xbox ordinals), and scripts/analyze.ps1 regenerates the header
     * from the same import analysis consumed by the lifter. */
    xbox_kernel_set_ordinal_remap(g_xbox_ordinal_remap,
                                  XBOX_ORDINAL_REMAP_COUNT);

    /* Step 5: Initialize kernel bridge (thunk table in Xbox memory) */
    printf("Initializing kernel bridge...\n");
    xbox_kernel_bridge_init();

    /* The title keeps its original XDK D3D library, so it does not create a
     * native PC window itself. Create the D3D11-backed host before entering
     * the game; the vertical-blank bridge feeds it live NV2A commands. */
    printf("Initializing host graphics...\n");
    if (!initialize_host_graphics(hInstance, nCmdShow)) {
        MessageBoxA(NULL, "Failed to initialize the PC graphics device.",
                    "Urban Chaos Recompiled", MB_ICONERROR);
        xbox_kernel_shutdown();
        xbox_MemoryLayoutShutdown();
        free(xbe_data);
        return 1;
    }
#if defined(UCRR_LIGHT_REMOVAL)
    /* Overlay initialization has loaded this runtime's saved lighting choice. */
    uc_light_removal_apply_pending();
#endif

    /* Step 6: Initialize stack */
    g_direct_vp_stores = getenv("UCRR_DIRECT_VP_STORES") != NULL;
    fprintf(stderr,"[APU-STORE-ROUTE] direct_vp=%d site=0042B221\n",g_direct_vp_stores);
    if(getenv("UCRR_AUDIO_BRIDGE")) {
        g_apu_state=mcpx_apu_init_standalone((uint8_t *)xbox_GetMemoryBase(),g_xbox_total_ram);
        if(g_apu_state) {
            mcpx_apu_attach_host(g_apu_state,
                (volatile uint32_t *)((uintptr_t)g_xbox_mem_offset+0xFE800000u),xbox_NotifyRealApu);
            xbox_EnableRealApu(1);
            mcpx_apu_set_guest_list_ownership(g_apu_state,1);
            fprintf(stderr,"[AUDIO-LIST-OWNER] guest-removal; inactive voices skipped without unlinking\n");
            DWORD old_protect;
            g_audio_bridge=1;
            if(!VirtualProtect((void *)((uintptr_t)g_xbox_mem_offset+0xFE820000u),0x10000,PAGE_READONLY,&old_protect)) {
                g_audio_bridge=0;xbox_EnableRealApu(0);
                mcpx_apu_shutdown(g_apu_state);g_apu_state=NULL;
                fprintf(stderr,"[AUDIO-BRIDGE] command protection failed\n");
            } else fprintf(stderr,"[AUDIO-BRIDGE] ordered VP writes enabled; synthetic SSL completion disabled\n");
        }
    }
    g_esp = XBOX_STACK_TOP;

    /*
     * TODO: Pre-initialize CRT globals if needed.
     *
     * Many Xbox games use the MSVC CRT. The CRT's __heap_init sets up a
     * heap descriptor at a game-specific address. You may need to
     * pre-initialize __active_heap to avoid small-block heap issues:
     *
     *   uint32_t *active_heap = (uint32_t *)((uint8_t *)g_xbox_mem_offset + ACTIVE_HEAP_VA);
     *   *active_heap = 1;  // 1 = system heap (HeapAlloc), avoids SBH init
     *
     * Find ACTIVE_HEAP_VA by searching the disassembly for __heap_init
     * or by looking for the CRT's __active_heap global in the data section.
     */

    printf("\n=== Initialization complete ===\n");
    printf("Entry point: 0x%08X\n", UCRR_ENTRY_POINT);
    printf("ESP: 0x%08X\n", g_esp);

    /* Step 7: Call the recompiled entry point */
    printf("\nStarting game...\n");
    fflush(stdout);

    if(!getenv("UCRR_WATCHDOG_MISSION")) start_opt_in_watchdog();
    if(!getenv("UCRR_WATCH_HEAP_LIST") && !getenv("UCRR_WATCH_HEAP_BIN")) start_tls_watch();
    xbe_entry_point();

    printf("\nGame returned. Cleaning up...\n");

    /* Cleanup */
    if(g_apu_state) {
        mcpx_apu_shutdown(g_apu_state);g_apu_state=NULL;
        DWORD old_protect;
        VirtualProtect((void *)((uintptr_t)g_xbox_mem_offset+0xFE820000u),0x10000,PAGE_READWRITE,&old_protect);
        g_audio_bridge=0;xbox_EnableRealApu(0);
    }
    nv2a_pb_live_shutdown();
    hf_shutdown();
    if (g_host_d3d_device) {
        g_host_d3d_device->lpVtbl->Release(g_host_d3d_device);
        g_host_d3d_device = NULL;
    }
    if (g_host_window) {
        DestroyWindow(g_host_window);
        g_host_window = NULL;
    }
    xbox_kernel_shutdown();
    xbox_MemoryLayoutShutdown();
    free(xbe_data);

    return 0;
}

static void initialize_crash_symbols(void)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
        fprintf(stderr, "Warning: host debug symbols unavailable (error %lu)\n",
            GetLastError());
    }
}

/* ── XBE Loading ───────────────────────────────────────────── */

static BOOL load_xbe(const char *path, void **out_data, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open XBE: %s\n", path);
        return FALSE;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return FALSE;
    }

    void *data = malloc((size_t)size);
    if (!data) {
        fclose(f);
        return FALSE;
    }

    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        fclose(f);
        return FALSE;
    }

    fclose(f);
    *out_data = data;
    *out_size = (size_t)size;
    return TRUE;
}

/* Console entry point (for debugging -- lets you see printf output) */
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
}
