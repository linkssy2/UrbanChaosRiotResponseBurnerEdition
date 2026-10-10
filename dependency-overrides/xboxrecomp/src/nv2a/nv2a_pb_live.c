/*
 * Live NV2A push-buffer decoder.
 *
 * The packet/control-flow decoding mirrors the NV2A DMA pusher implemented by
 * xemu.  Unlike the captured-menu replay helper, this decoder preserves its
 * GET, active-method, and CALL/RETURN state across frames and follows jumps in
 * the title's circular push buffer.
 */

#include "nv2a_pb_live.h"

#include "nv2a_pgraph_d3d11.h"
#include "nv2a_regs.h"
#include "nv2a_light_ab_phase.h"
#include "nv2a_frame_cost.h"
#include "nv2a_deep_trace.h"
#include "nv2a_causal_trace.h"
NvDeepTrace nv_deep_trace;
#include "nv2a_frame_cost_burst.h"
#include "../d3d/d3d8_xbox.h"
#include "nv2a_command_probe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PB_PACKET_MASK          0xE0030003u
#define PB_METHOD_INC           0x00000000u
#define PB_METHOD_NON_INC       0x40000000u
#define PB_OLD_JUMP_MASK        0xE0000003u
#define PB_OLD_JUMP             0x20000000u
#define PB_RETURN               0x00020000u
#define PB_MAX_WORDS_PER_FRAME  (4u * 1024u * 1024u)
#define PB_MAX_OWNED_SLICES     16u
#define PB_METHOD_SLOTS         (0x2000u / 4u)
#include "nv2a_state_block.h"
#include "index_admission_census.h"
#include "index_nonforward_batch.h"
#include "index_nonforward_ab.h"

enum NvPbDecodeResult { NV_PB_ERROR=0, NV_PB_COMPLETE=1, NV_PB_YIELD=2 };
enum NvPbDecodeFailure {
    NV_PB_FAILURE_NONE=0, NV_PB_FAILURE_ADDRESS, NV_PB_FAILURE_NESTED_CALL,
    NV_PB_FAILURE_RETURN, NV_PB_FAILURE_RENDERER,
    NV_PB_FAILURE_OWNED_LIMIT
};

typedef struct LivePushState {
    int command_block_enabled;
    int zero_run_enabled;
    int lean_nop_enabled;
    int packet_cursor_enabled;
    uint32_t get;
    uint32_t method;
    uint32_t method_count;
    uint32_t subchannel;
    uint32_t return_address;
    int method_incrementing;
    int subroutine_active;
    int stream_ready;
    int initialized;
    int failed;
    unsigned failure_reason;
    uint32_t failure_get, failure_put;
    uint64_t decoder_yields;
    int trace_enabled;
    int histogram_enabled;
    int index_batch_enabled;
    int index_scalar_fast_route;
    int index_nonforward_batch;
    int constant_batch_enabled;
    int inline_batch_enabled;
    int inline_ab_enabled;
    NvLightAbPhase inline_ab_phase;
    uint64_t inline_batch_calls,inline_batch_words;
    int constant_ab_enabled;
    NvLightAbPhase constant_ab_phase;
    uint64_t constant_batch_calls,constant_batch_words;
    uint8_t *mapped_guest_memory;
    size_t mapped_guest_memory_size;

    uint64_t frames;
    uint64_t packets;
    uint64_t methods;
    uint64_t jumps;
    uint64_t calls;
    uint64_t returns;
    uint64_t invalid_words;
    uint64_t begin_count;
    uint64_t end_count;
    uint64_t array_element16_count;
    uint64_t array_element32_count;
    uint64_t draw_arrays_count;
    uint64_t inline_array_count;
    uint64_t clear_surface_count;
    uint32_t geometry_logs;
    uint32_t in_batch_logs;
    uint32_t batch_number;
    int in_begin_end;
    LONGLONG timing_frequency;
    LONGLONG timing_last_entry;
    uint64_t timing_gap_ticks;
    uint64_t timing_decode_ticks;
    uint64_t timing_flush_ticks;
    uint64_t timing_present_ticks;
    uint64_t timing_gap_max;
    uint64_t timing_decode_max;
    uint64_t timing_flush_max;
    uint64_t timing_present_max;
    uint32_t timing_gap_samples;
    uint32_t timing_stage_samples;
    uint64_t timing_gaps[120];
    uint64_t post_last, post_gaps[120];
    unsigned post_count, post_reports, post_spikes;
    uint64_t timing_early_ticks, timing_early_calls, timing_empty_calls;
    uint64_t method_histogram[8][PB_METHOD_SLOTS];
    uint64_t nop_zero,nop_nonzero,nop_operations[32];
    /* Header shapes only, not accepted draws or published payload evidence.
     * Buckets 0..32 are exact lengths; 33 means a larger length. */
    uint64_t constant_packets[2][34],constant_crossing,constant_partial;
    uint32_t object_bindings[8];
    /* Keep the bounded diagnostic tables away from frequently read decoder
     * fields. Disabled/exhausted paths inspect only the active flag. */
    IndexAdmissionCensus index_admission;
    IndexNonforwardAB index_nonforward_ab;
} LivePushState;

static LivePushState g_live;
void nv2a_pb_live_get_failure(NvPbFailureInfo *out)
{
    if(!out)return;
    out->reason=g_live.failure_reason;out->get=g_live.failure_get;
    out->put=g_live.failure_put;out->internal_yields=g_live.decoder_yields;
}
NvFrameCost nv_frame_cost_shared;
#define frame_cost nv_frame_cost_shared
static NvFrameCostBurst frame_cost_burst;
static volatile LONG frame_cost_mark_requested;
void nv2a_pb_live_mark_slowdown(void){
#if UCRR_PLAY_ONLY
    return;
#endif
InterlockedExchange(&frame_cost_mark_requested,1);}
static uint64_t frame_cost_clock(void){LARGE_INTEGER t;QueryPerformanceCounter(&t);return (uint64_t)t.QuadPart;}
static void index_nonforward_ab_present(void){
    IndexNonforwardAB *s=&g_live.index_nonforward_ab;
    unsigned event=index_nonforward_ab_step(s,g_live.frames,
        s->started||pgraph_d3d11_constant_ab_ready());
    if(!event)return;
    LARGE_INTEGER now={0};PgraphD3D11Stats stats;
    int clock_ok=QueryPerformanceCounter(&now)&&now.QuadPart>0;
    pgraph_d3d11_get_stats(&stats);
    uint64_t qpc=clock_ok?(uint64_t)now.QuadPart:0;
    event=index_nonforward_ab_record(s,event,qpc,clock_ok,stats.draw_calls,
        g_live.methods,g_live.array_element16_count,g_live.array_element32_count);
    g_live.index_nonforward_batch=(int)index_nonforward_ab_mode(s);
    fprintf(stderr,"[INDEX-NONFORWARD-AB] schema=1 state=%s phase=%u frame=%llu mode=%u qpc=%llu qpf=%llu draws=%llu methods=%llu element16=%llu element32=%llu boundaries=600 warmup=3600\n",
        event==INAB_WARMUP?"warmup":event==INAB_PHASE?"phase":event==INAB_DONE?"done":"invalid",
        s->phase,(unsigned long long)g_live.frames,(unsigned)g_live.index_nonforward_batch,
        (unsigned long long)qpc,(unsigned long long)s->frequency,(unsigned long long)stats.draw_calls,
        (unsigned long long)g_live.methods,(unsigned long long)g_live.array_element16_count,
        (unsigned long long)g_live.array_element32_count);
}
extern void pgraph_d3d11_packet_probe(void);
static void frame_cost_boundary(void){
#if UCRR_PLAY_ONLY
    return;
#endif

    nv_command_probe_boundary((unsigned)g_live.frames,g_live.methods);
    if(frame_cost_burst.enabled&&InterlockedExchange(&frame_cost_mark_requested,0))frame_cost_burst.pending=1;
    if(nv_frame_cost_burst_begin(&frame_cost_burst,&frame_cost,g_live.frames)){
        pgraph_d3d11_packet_probe();
        fprintf(stderr,"[FRAME-COST-MARK] ms=%llu frame=%llu burst=%u next_samples=16 spacing=4 thread=%lu\n",(unsigned long long)GetTickCount64(),(unsigned long long)g_live.frames,frame_cost_burst.requests,GetCurrentThreadId());
    }
    if(!frame_cost.enabled)return;
    if(nv_frame_cost_boundary(&frame_cost,g_live.frames,pgraph_d3d11_constant_ab_ready())){
        LARGE_INTEGER f;QueryPerformanceFrequency(&f);double ms=1000.0/f.QuadPart;
fprintf(stderr,"[FRAME-COST] frame=%llu sample=%u total_ms=%.6f commands_ms=%.6f reports_ms=%.6f flush_ms=%.6f present_ms=%.6f unassigned_ms=%.6f roots=%u omitted=%u errors=%u index_packet_ms=%.6f constant_packet_ms=%.6f submit_ms=%.6f texture_ms=%.6f scopes=%u scope_omitted=%u clocks=%u lighting_ms=%.6f upload_ms=%.6f host_draw_ms=%.6f cpu_immediate_ms=%.6f cpu_gpu_first_ms=%.6f cpu_indexed_ms=%.6f cpu_inline_ms=%.6f cpu_other_ms=%.6f software_method_ms=%.6f schema=5 (exclusive sampled host elapsed; commands and submit are residuals; not GPU execution or production FPS)\n",
            (unsigned long long)g_live.frames,frame_cost.samples,frame_cost.total*ms,
            frame_cost.ticks[NV_COST_COMMANDS]*ms,frame_cost.ticks[NV_COST_REPORTS]*ms,
            frame_cost.ticks[NV_COST_FLUSH]*ms,frame_cost.ticks[NV_COST_PRESENT]*ms,
            frame_cost.unassigned*ms,frame_cost.roots,frame_cost.omitted,frame_cost.errors,
            frame_cost.ticks[NV_COST_INDEX_PACKET]*ms,frame_cost.ticks[NV_COST_CONSTANT_PACKET]*ms,
            frame_cost.ticks[NV_COST_SUBMIT]*ms,frame_cost.ticks[NV_COST_TEXTURE]*ms,
            frame_cost.scopes,frame_cost.scope_omitted,frame_cost.clocks,
            frame_cost.ticks[NV_COST_LIGHTING]*ms,frame_cost.ticks[NV_COST_UPLOAD]*ms,
            frame_cost.ticks[NV_COST_HOST_DRAW]*ms,
            frame_cost.ticks[NV_COST_CPU_IMMEDIATE]*ms,frame_cost.ticks[NV_COST_CPU_GPU_FIRST]*ms,
            frame_cost.ticks[NV_COST_CPU_INDEXED]*ms,frame_cost.ticks[NV_COST_CPU_INLINE]*ms,
            frame_cost.ticks[NV_COST_CPU_OTHER]*ms,frame_cost.ticks[NV_COST_SOFTWARE_METHOD]*ms);
        fprintf(stderr,"[FRAME-COST-TIME] ms=%llu frame=%llu burst=%u\n",(unsigned long long)GetTickCount64(),(unsigned long long)g_live.frames,frame_cost_burst.requests);
        nv_frame_cost_burst_end(&frame_cost_burst,&frame_cost,g_live.frames);
        nv_deep_report();
    }
    if(frame_cost.active&&!nv_deep_trace.active)nv_deep_start(frame_cost_burst.requests,(unsigned)g_live.frames);
}

static int compare_frame_ticks(const void *left, const void *right)
{
    uint64_t a = *(const uint64_t *)left, b = *(const uint64_t *)right;
    return (a > b) - (a < b);
}

/* API-return cadence, not a display scanout timestamp. Uses the existing
 * timer samples, separate from pre-wait entry cadence. No guest state reads. */
static void trace_post_present(uint64_t entry, uint64_t before, uint64_t after)
{
    static int monitor=-1;
    if(monitor<0)monitor=getenv("UCRR_PACING_MONITOR")!=NULL;
    if (!g_live.timing_frequency || (!monitor && g_live.post_reports >= 128)) return;
    const double ms = 1000.0 / g_live.timing_frequency;
    if (g_live.post_last && after >= g_live.post_last) {
        uint64_t gap = after - g_live.post_last;
        g_live.post_gaps[g_live.post_count++] = gap;
        if (gap * ms > 25.0 && g_live.post_spikes < 64) {
            fprintf(stderr,"[POST-PRESENT-SPIKE] frame=%llu gap_ms=%.3f guest_work_ms=%.3f drain_flush_ms=%.3f present_wait_ms=%.3f\n",
                (unsigned long long)g_live.frames, gap*ms,
                (entry-g_live.post_last)*ms,(before-entry)*ms,(after-before)*ms);
            ++g_live.post_spikes;
        }
        if (g_live.post_count == 120) {
            unsigned over20=0,over33=0;
            uint64_t total_ticks=0;
            qsort(g_live.post_gaps,120,sizeof(uint64_t),compare_frame_ticks);
            for(unsigned i=0;i<120;++i) {
                total_ticks+=g_live.post_gaps[i];
                over20 += g_live.post_gaps[i]*ms > 20.0;
                over33 += g_live.post_gaps[i]*ms > 1000.0/30.0;
            }
            if(monitor)fprintf(stderr,"[PACING-MONITOR] samples=120 mean_ms=%.4f api_hz=%.3f (PresentFrame returns; not unique rendered frames or scanout)\n",
                total_ticks*ms/120.0,total_ticks?120000.0/(total_ticks*ms):0.0);
            fprintf(stderr,"[POST-PRESENT] samples=120 p50_ms=%.3f p95_ms=%.3f p99_ms=%.3f max_ms=%.3f over20=%u over33=%u report=%u/128 (API returns, not scanout)\n",
                g_live.post_gaps[59]*ms,g_live.post_gaps[113]*ms,g_live.post_gaps[118]*ms,g_live.post_gaps[119]*ms,
                over20,over33,++g_live.post_reports);
            g_live.post_count=0;
        }
    }
    g_live.post_last=after;
}

static int guest_dword_valid(size_t guest_memory_size, uint32_t address)
{
    return guest_memory_size >= sizeof(uint32_t) && (address & 3u) == 0 &&
           (size_t)address <= guest_memory_size - sizeof(uint32_t);
}

static uint32_t guest_read32(const uint8_t *guest_memory, uint32_t address)
{
    uint32_t value;
    memcpy(&value, guest_memory + address, sizeof(value));
    return value;
}

static uint32_t sanitize_target(uint32_t word)
{
    /* Xbox physical addresses are 28-bit; the high nibble may contain an
     * uncached/write-combined alias selected by the D3D runtime. */
    return (word & 0x0FFFFFFCu);
}

/* Release-visible failure evidence, not per-word timing or output. Preserve the
 * first offending location and the decoder continuation; never claim PUT was
 * consumed after a malformed stream or exhausted ownership guard. */
static int nv_pb_decode_fail(unsigned reason,uint32_t get,uint32_t put)
{
    if(!g_live.failure_reason){g_live.failure_reason=reason;
        g_live.failure_get=get;g_live.failure_put=put;}
    g_live.failed=1;
    return NV_PB_ERROR;
}

/* Keep the aggregate diagnostic token entirely outside the ordinary decoder.
 * A conditional begin/end in that function still produced 48-byte stack
 * copies on the disabled branch in the MSVC production executable. */
static __declspec(noinline) int nv_dispatch_traced_scalar(uint32_t word){
    NvDeepToken token=nv_deep_begin(0,g_live.method,1,g_live.get-4u,g_live.subchannel);
    int handled=0;
    if(g_live.index_scalar_fast_route&&
       (g_live.method==NV097_ARRAY_ELEMENT16||g_live.method==NV097_ARRAY_ELEMENT32))
        handled=pgraph_d3d11_try_index_scalar((int)g_live.subchannel,g_live.method,word);
    if(!handled)handled=pgraph_d3d11_method((int)g_live.subchannel,g_live.method,word);
    nv_deep_end(token,handled);
    return handled;
}

static __declspec(noinline) int nv_dispatch_probed_scalar(uint32_t word){
    unsigned method=g_live.method,get=g_live.get-4u;
    nv_command_probe_shape(method,word,g_live.method_incrementing,g_live.method_count,get);
    uint64_t start=nv_command_probe_begin(method,g_live.subchannel);
    int handled=0;
    if(nv_deep_trace.active)handled=nv_dispatch_traced_scalar(word);
    else {
        if(g_live.index_scalar_fast_route&&(method==NV097_ARRAY_ELEMENT16||method==NV097_ARRAY_ELEMENT32))
            handled=pgraph_d3d11_try_index_scalar((int)g_live.subchannel,method,word);
        if(!handled)handled=pgraph_d3d11_method((int)g_live.subchannel,method,word);
    }
    nv_command_probe_end(start,method,get,word);return handled;
}

void nv2a_pb_live_init(void)
{
    const char *trace;

    memset(&g_live, 0, sizeof(g_live));
    index_admission_init(&g_live.index_admission,getenv("UCRR_INDEX_ADMISSION"));
    trace=getenv("UCRR_INDEX_SCALAR_FAST_ROUTE");
    g_live.index_scalar_fast_route=trace&&!strcmp(trace,"1");
    trace=getenv("UCRR_INDEX_NONFORWARD_BATCH");
    g_live.index_nonforward_batch=trace&&!strcmp(trace,"1");
#if !UCRR_PLAY_ONLY
    trace=getenv("UCRR_FRAME_COST");
    nv_frame_cost_init(&frame_cost,trace&&!strcmp(trace,"1"),frame_cost_clock);
    trace=getenv("UCRR_MARK_FRAME_COST");
    memset(&frame_cost_burst,0,sizeof(frame_cost_burst));frame_cost_burst.enabled=trace&&!strcmp(trace,"1");
    if(frame_cost_burst.enabled)frame_cost.enabled=0;
    memset(&nv_deep_trace,0,sizeof(nv_deep_trace));
    trace=getenv("UCRR_DEEP_TRACE");nv_deep_trace.enabled=frame_cost_burst.enabled&&trace&&!strcmp(trace,"1");
    nv_command_probe_init();
    nv_deep_trace.clock=frame_cost_clock;
    {LARGE_INTEGER f;QueryPerformanceFrequency(&f);nv_deep_trace.tick_ms=f.QuadPart?1000.0/f.QuadPart:0;}
    trace = getenv("UCRR_PB_TRACE");
    g_live.trace_enabled = trace && trace[0] && strcmp(trace, "0") != 0;
    trace = getenv("UCRR_PB_HISTOGRAM");
    g_live.histogram_enabled = g_live.trace_enabled || (trace && !strcmp(trace,"1"));
#endif
    g_live.index_batch_enabled = getenv("UCRR_INDEX_BATCH") != NULL;
    trace=getenv("UCRR_COMMAND_BLOCK");g_live.command_block_enabled=trace&&!strcmp(trace,"1");
    trace=getenv("UCRR_ZERO_RUN");g_live.zero_run_enabled=trace&&!strcmp(trace,"1");
    trace=getenv("UCRR_LEAN_NOP");g_live.lean_nop_enabled=trace&&!strcmp(trace,"1");
    trace=getenv("UCRR_PACKET_CURSOR");g_live.packet_cursor_enabled=trace&&!strcmp(trace,"1");
#if !UCRR_PLAY_ONLY
    index_nonforward_ab_init(&g_live.index_nonforward_ab,getenv("UCRR_INDEX_NONFORWARD_AB"),
        g_live.index_batch_enabled,g_live.index_nonforward_batch);
    if(g_live.index_nonforward_ab.enabled){
        LARGE_INTEGER frequency;
        if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)
            g_live.index_nonforward_ab.frequency=(uint64_t)frequency.QuadPart;
    }
#endif
    trace=getenv("UCRR_CONSTANT_BATCH");
    g_live.constant_batch_enabled=trace&&!strcmp(trace,"1");
    trace=getenv("UCRR_INLINE_BATCH");
    g_live.inline_batch_enabled=trace&&!strcmp(trace,"1");
#if !UCRR_PLAY_ONLY
    trace=getenv("UCRR_INLINE_AB");
    g_live.inline_ab_enabled=trace&&!strcmp(trace,"1");
    if(g_live.inline_ab_enabled)g_live.inline_batch_enabled=1;
    trace=getenv("UCRR_CONSTANT_AB");
    g_live.constant_ab_enabled=trace&&!strcmp(trace,"1");
    if(g_live.constant_ab_enabled)g_live.constant_batch_enabled=1;
#endif
    pgraph_d3d11_init();
    g_live.initialized = 1;
    fprintf(stderr, "[PB-LIVE] Live Xbox push-buffer decoding enabled\n");
}

static int decode_until_put_reference(uint8_t *guest_memory, size_t guest_memory_size,
                            uint32_t put, uint32_t initial_words)
{
    uint32_t words = initial_words;
    static int array_state_batch=-1;
    if(array_state_batch<0){const char*s=getenv("UCRR_CPU_STATE_PACKET");array_state_batch=s&&!strcmp(s,"1");}
    int constant_batch_now=g_live.constant_batch_enabled;
    int inline_batch_now=g_live.inline_batch_enabled;
    if(g_live.inline_ab_enabled && (g_live.inline_ab_phase.started||pgraph_d3d11_constant_ab_ready())){
        int changed=nv_light_ab_step(&g_live.inline_ab_phase,(uint32_t)g_live.frames);
        inline_batch_now=nv_light_ab_wide(&g_live.inline_ab_phase);
        if(changed)fprintf(stderr,"[INLINE-AB] frame=%llu phase=%u mode=%s exhausted=%u\n",
            (unsigned long long)g_live.frames,g_live.inline_ab_phase.phase,
            inline_batch_now?"batch":"scalar",g_live.inline_ab_phase.phase>=12);
    }
    /* Diagnostic choice fixed for the whole synchronous drain. The cue delays
     * phases past movie-only rendering; it does not certify a gameplay scene. */
    if(g_live.constant_ab_enabled && (g_live.constant_ab_phase.started||pgraph_d3d11_constant_ab_ready())){
        int changed=nv_light_ab_step(&g_live.constant_ab_phase,(uint32_t)g_live.frames);
        constant_batch_now=nv_light_ab_wide(&g_live.constant_ab_phase);
        if(changed)fprintf(stderr,"[CONSTANT-AB] frame=%llu phase=%u mode=%s exhausted=%u\n",
            (unsigned long long)g_live.frames,g_live.constant_ab_phase.phase,
            constant_batch_now?"batch":"scalar",g_live.constant_ab_phase.phase>=12);
    }

    /* A submitted header does not imply its payload has been published. Keep
     * method_count for the next drain, but never read past the producer PUT. */
    while (g_live.get != put &&
           words < PB_MAX_WORDS_PER_FRAME) {
        uint32_t word;

        if(g_live.zero_run_enabled&&g_live.method==NV097_NO_OPERATION&&
           !g_live.method_incrementing&&g_live.method_count>=4&&!g_live.trace_enabled&&
           !g_live.histogram_enabled&&!nv_deep_trace.active&&!frame_cost.active&&
           !nv_command_probe.active&&!g_live.index_admission.active&&
           put>g_live.get&&!(put&3u)&&guest_dword_valid(guest_memory_size,g_live.get)){
            unsigned count=g_live.method_count,available=(put-g_live.get)/4;
            size_t room=(guest_memory_size-g_live.get)/4;
            if(count>available)count=available;if(count>room)count=(unsigned)room;
            if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
            unsigned n=pgraph_d3d11_zero_run((int)g_live.subchannel,guest_memory+g_live.get,count);
            if(n){g_live.get+=4*n;words+=n;g_live.method_count-=n;g_live.methods+=n;continue;}
        }

        /* Fresh payload every execution; cached plans contain host metadata
         * only. Full state-only packets stop before any side-effect command.
         * Diagnostics and partial/nonforward packets keep the old decoder. */
        if(g_live.command_block_enabled&&!g_live.method_count&&!g_live.trace_enabled&&
           !g_live.histogram_enabled&&!nv_deep_trace.active&&!frame_cost.active&&
           !g_live.index_admission.active&&put>g_live.get&&!(put&3u)&&
           guest_dword_valid(guest_memory_size,g_live.get)){
            unsigned count=(put-g_live.get)/4u;
            size_t room=(guest_memory_size-g_live.get)/4u;
            if(count>room)count=(unsigned)room;
            if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
            NvStateBlockResult block=pgraph_d3d11_state_block(guest_memory+g_live.get,count);
            if(block.words){g_live.get+=4*block.words;words+=block.words;
                g_live.packets+=block.packets;g_live.methods+=block.methods;
                g_live.method=block.method;g_live.subchannel=block.subchannel;
                g_live.method_incrementing=block.incrementing;continue;}
        }

        if(array_state_batch&&!g_live.trace_enabled&&g_live.method_incrementing&&g_live.method_count>1&&
           ((g_live.method>=NV097_SET_VERTEX_DATA_ARRAY_FORMAT&&g_live.method<NV097_SET_VERTEX_DATA_ARRAY_FORMAT+64)||
            (g_live.method>=NV097_SET_VERTEX_DATA_ARRAY_OFFSET&&g_live.method<NV097_SET_VERTEX_DATA_ARRAY_OFFSET+64))&&
           put>g_live.get&&!(put&3u)&&guest_dword_valid(guest_memory_size,g_live.get)){
            unsigned base=g_live.method>=NV097_SET_VERTEX_DATA_ARRAY_FORMAT?NV097_SET_VERTEX_DATA_ARRAY_FORMAT:NV097_SET_VERTEX_DATA_ARRAY_OFFSET;
            unsigned count=g_live.method_count,available=(put-g_live.get)/4u;
            size_t room=(guest_memory_size-g_live.get)/4u;
            if(count>available)count=available;
            if(count>room)count=(unsigned)room;
            if(count>(base+64-g_live.method)/4)count=(base+64-g_live.method)/4;
            if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
            int accepted=0;
            if(count>1){NvDeepToken t;const unsigned traced=nv_deep_trace.active;
                if(traced)t=nv_deep_begin(1,g_live.method,count,g_live.get,g_live.subchannel);
                accepted=pgraph_d3d11_array_state_batch((int)g_live.subchannel,g_live.method,guest_memory+g_live.get,count);
                if(traced)nv_deep_end(t,accepted);}
            if(accepted){
                if(g_live.histogram_enabled)for(unsigned i=0;i<count;++i)++g_live.method_histogram[g_live.subchannel][g_live.method/4+i];
                g_live.get+=4*count;words+=count;g_live.method_count-=count;g_live.method+=4*count;g_live.methods+=count;
                continue;
            }
        }

        /* Constant spans remain owned by this drain. Clamp before reading any
         * payload and fall back without consumption when the renderer declines. */
        if(constant_batch_now&&!g_live.trace_enabled&&g_live.method_incrementing&&
           g_live.method_count>1&&g_live.method>=NV097_SET_TRANSFORM_CONSTANT&&
           g_live.method<NV097_SET_TRANSFORM_CONSTANT+0x80u&&put>g_live.get&&
           !(put&3u)&&guest_dword_valid(guest_memory_size,g_live.get)){
            uint32_t count=g_live.method_count;
            uint32_t available=(put-g_live.get)/4u;
            size_t in_ram=(guest_memory_size-g_live.get)/4u;
            uint32_t in_range=(NV097_SET_TRANSFORM_CONSTANT+0x80u-g_live.method)/4u;
            if(count>available)count=available;
            if(count>in_ram)count=(uint32_t)in_ram;
            if(count>in_range)count=in_range;
            if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
            int accepted=0;
            if(count>1){unsigned scope=nv_frame_cost_push(&frame_cost,NV_COST_CONSTANT_PACKET);
                NvDeepToken t;const unsigned traced=nv_deep_trace.active;
                if(traced)t=nv_deep_begin(2,g_live.method,count,g_live.get,g_live.subchannel);
                accepted=pgraph_d3d11_constant_batch((int)g_live.subchannel,g_live.method,guest_memory+g_live.get,count);
                if(traced)nv_deep_end(t,accepted);
                nv_frame_cost_pop(&frame_cost,scope);}
            if(accepted){
                if(g_live.histogram_enabled){
                    ++g_live.constant_batch_calls;g_live.constant_batch_words+=count;
                    for(uint32_t i=0;i<count;++i)++g_live.method_histogram[g_live.subchannel][g_live.method/4u+i];
                }
                g_live.get+=4u*count;words+=count;g_live.method_count-=count;
                g_live.method+=4u*count;g_live.methods+=count;
                {static unsigned notice;if(!notice++){fprintf(stderr,"[CONSTANT-BATCH] active words=%u\n",count);}}
                continue;
            }
        }

        /* Raw inline words have no intervening per-word callbacks. Never cross
         * published PUT, guest bounds, packet extent or decoder word budget.
         * Verbose tracing retains scalar delivery; a decline consumes nothing. */
        if(inline_batch_now&&!g_live.trace_enabled&&!g_live.method_incrementing&&
           g_live.method_count>1&&g_live.method==NV097_INLINE_ARRAY&&put>g_live.get&&
           !(put&3u)&&guest_dword_valid(guest_memory_size,g_live.get)){
            uint32_t count=g_live.method_count,available=(put-g_live.get)/4u;
            size_t in_ram=(guest_memory_size-g_live.get)/4u;
            if(count>available)count=available;
            if(count>in_ram)count=(uint32_t)in_ram;
            if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
            int accepted=0;
            if(count>1){NvDeepToken t;const unsigned traced=nv_deep_trace.active;
                if(traced)t=nv_deep_begin(4,g_live.method,count,g_live.get,g_live.subchannel);
                accepted=pgraph_d3d11_inline_batch((int)g_live.subchannel,guest_memory+g_live.get,count);
                if(traced)nv_deep_end(t,accepted);}
            if(accepted){
                g_live.get+=4u*count;words+=count;g_live.method_count-=count;
                g_live.methods+=count;g_live.inline_array_count+=count;
                if(g_live.histogram_enabled){++g_live.inline_batch_calls;g_live.inline_batch_words+=count;
                    g_live.method_histogram[g_live.subchannel][NV097_INLINE_ARRAY/4]+=count;}
                {static unsigned notice;if(!notice++)fprintf(stderr,"[INLINE-BATCH-ACTIVE] words=%u\n",count);}
                continue;
            }
        }

        /* Ordinary forward published spans, or opt-in scalar-equivalent active
         * payload segments above a lower PUT reached through CALL/jump. Neither
         * path crosses a packet, RAM, PUT, word budget, or uint32 wrap. Appends
         * have no guest callback/reentry; existing batch progress bookkeeping is
         * deferred, so this is NOT a general reentrant method-batching contract.
         * Trace runs and unaligned PUT stay scalar. */
        if(g_live.index_batch_enabled&&!g_live.trace_enabled&&
           !g_live.method_incrementing&&g_live.method_count>1&&
           (g_live.method==NV097_ARRAY_ELEMENT16||g_live.method==NV097_ARRAY_ELEMENT32)&&
           (put>g_live.get||(g_live.index_nonforward_batch&&g_live.get>put))&&
           !(put&3u)&&guest_dword_valid(guest_memory_size,g_live.get)){
            uint32_t count=g_live.method_count;
            uint32_t available=put>g_live.get?(put-g_live.get)/4u:
                index_nonforward_words(g_live.get,put,guest_memory_size,count,PB_MAX_WORDS_PER_FRAME-words);
            size_t in_ram=(guest_memory_size-g_live.get)/4;
            if(count>available)count=available;
            if(count>in_ram)count=(uint32_t)in_ram;
            if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
            int accepted=0;
            if(count>1){unsigned scope=nv_frame_cost_push(&frame_cost,NV_COST_INDEX_PACKET);
                NvDeepToken t;const unsigned traced=nv_deep_trace.active;
                if(traced)t=nv_deep_begin(3,g_live.method,count,g_live.get,g_live.subchannel);
                accepted=pgraph_d3d11_index_batch((int)g_live.subchannel,g_live.method,guest_memory+g_live.get,count);
                if(traced)nv_deep_end(t,accepted);
                nv_frame_cost_pop(&frame_cost,scope);}
            if(g_live.index_admission.active&&count>1)
                index_admission_batch(&g_live.index_admission,g_live.method,count,accepted);
            if(accepted){
                g_live.get+=4*count;words+=count;g_live.method_count-=count;
                g_live.methods+=count;
                if(g_live.method==NV097_ARRAY_ELEMENT16)g_live.array_element16_count+=count;
                else g_live.array_element32_count+=count;
                if(g_live.histogram_enabled && g_live.subchannel<8 && g_live.method<0x2000u)
                    g_live.method_histogram[g_live.subchannel][g_live.method/4u]+=count;
                /* This batch route requires verbose tracing OFF. The separate
                 * histogram option counts words without disabling batching. */
                {static unsigned samples;if(samples++<8)fprintf(stderr,
                    "[INDEX-BATCH] method=%04X words=%u remaining=%u get=%08X\n",
                    g_live.method,count,g_live.method_count,g_live.get);}
                continue;
            }
        }

        if (!guest_dword_valid(guest_memory_size, g_live.get)) {
            fprintf(stderr,
                    "[PB-LIVE] Invalid GET address 0x%08X while PUT=0x%08X\n",
                    g_live.get, put);
            return nv_pb_decode_fail(NV_PB_FAILURE_ADDRESS,g_live.get,put);
        }

        word = guest_read32(guest_memory, g_live.get);
        g_live.get += 4;
        ++words;

        if (g_live.method_count != 0) {
            /* Retain the original word read/GET advancement and packet budget.
             * Only zero NOPs bypass general accounting/dispatch. In particular,
             * nonzero software callbacks retain their reentrant scalar path. */
            if(g_live.lean_nop_enabled && g_live.method==NV097_NO_OPERATION && !word &&
               !g_live.trace_enabled && !g_live.histogram_enabled &&
               !nv_command_probe.active && !nv_deep_trace.active && !frame_cost.active &&
               !g_live.index_admission.active &&
               pgraph_d3d11_try_zero_scalar((int)g_live.subchannel)){
                ++g_live.methods;--g_live.method_count;
                if(g_live.method_incrementing)g_live.method+=4;
                continue;
            }
            if(g_live.histogram_enabled && g_live.method==NV097_NO_OPERATION){
                if(!word)++g_live.nop_zero;
                else {++g_live.nop_nonzero;++g_live.nop_operations[word&31u];}
            }
            if (g_live.method == NV097_SET_BEGIN_END) {
                if (word == NV097_SET_BEGIN_END_OP_END) {
                    ++g_live.end_count;
                } else {
                    ++g_live.begin_count;
                    ++g_live.batch_number;
                    g_live.in_begin_end = 1;
                }
            } else if (g_live.method == NV097_ARRAY_ELEMENT16) {
                ++g_live.array_element16_count;
            } else if (g_live.method == NV097_ARRAY_ELEMENT32) {
                ++g_live.array_element32_count;
            } else if (g_live.method == NV097_DRAW_ARRAYS) {
                ++g_live.draw_arrays_count;
            } else if (g_live.method == NV097_INLINE_ARRAY) {
                ++g_live.inline_array_count;
            } else if (g_live.method == NV097_CLEAR_SURFACE) {
                ++g_live.clear_surface_count;
            }

            if ((g_live.method == NV097_SET_BEGIN_END ||
                 g_live.method == NV097_ARRAY_ELEMENT16 ||
                 g_live.method == NV097_ARRAY_ELEMENT32 ||
                 g_live.method == NV097_DRAW_ARRAYS ||
                 g_live.method == NV097_INLINE_ARRAY ||
                 g_live.method == NV097_CLEAR_SURFACE) &&
                g_live.geometry_logs < 48 && g_live.trace_enabled) {
                fprintf(stderr,
                        "[PB-LIVE] Geometry s%u method=%04X param=%08X\n",
                        g_live.subchannel, g_live.method, word);
                ++g_live.geometry_logs;
            }

            if (g_live.trace_enabled && g_live.in_begin_end &&
                g_live.method != NV097_SET_BEGIN_END &&
                g_live.in_batch_logs < 256) {
                fprintf(stderr,
                        "[PB-LIVE] Batch %u s%u method=%04X param=%08X\n",
                        g_live.batch_number, g_live.subchannel,
                        g_live.method, word);
                ++g_live.in_batch_logs;
            }

            int handled=0;
            if(nv_command_probe.active)handled=nv_dispatch_probed_scalar(word);
            else if(nv_deep_trace.active)handled=nv_dispatch_traced_scalar(word);
            else {
                if(g_live.index_scalar_fast_route&&
                   (g_live.method==NV097_ARRAY_ELEMENT16||g_live.method==NV097_ARRAY_ELEMENT32))
                    handled=pgraph_d3d11_try_index_scalar((int)g_live.subchannel,g_live.method,word);
                if(!handled)handled=pgraph_d3d11_method((int)g_live.subchannel,g_live.method,word);
            }
            if(g_live.index_admission.active)
                index_admission_scalar(&g_live.index_admission,g_live.method,word,
                    g_live.method_count,g_live.method_incrementing,g_live.get-4u,put,
                    guest_memory_size,PB_MAX_WORDS_PER_FRAME-(words-1u),
                    g_live.index_batch_enabled,g_live.trace_enabled,handled);
            if(handled<0){g_live.get-=4;return nv_pb_decode_fail(NV_PB_FAILURE_RENDERER,g_live.get,put);}
            if (g_live.method == NV097_SET_BEGIN_END &&
                word == NV097_SET_BEGIN_END_OP_END) {
                g_live.in_begin_end = 0;
            }
            if (g_live.subchannel < 8 && g_live.method < 0x2000u) {
                /* Collect only for explicit histogram/trace diagnostics.
                 * Leave disabled for ordinary performance measurements. */
                if(g_live.histogram_enabled)
                    ++g_live.method_histogram[g_live.subchannel][g_live.method / 4u];
                if (g_live.method == 0) {
                    g_live.object_bindings[g_live.subchannel] = word;
                }
            }
            ++g_live.methods;
            --g_live.method_count;
            if (g_live.method_incrementing) {
                g_live.method += 4;
            }
            continue;
        }

        if ((word & PB_OLD_JUMP_MASK) == PB_OLD_JUMP) {
            g_live.get = sanitize_target(word);
            ++g_live.jumps;
            continue;
        }
        if ((word & 3u) == 1u) {
            g_live.get = sanitize_target(word);
            ++g_live.jumps;
            continue;
        }
        if ((word & 3u) == 2u) {
            if (g_live.subroutine_active) {
                fprintf(stderr,
                        "[PB-LIVE] Nested CALL at 0x%08X is invalid\n",
                        g_live.get - 4);
                return nv_pb_decode_fail(NV_PB_FAILURE_NESTED_CALL,g_live.get-4,put);
            }
            g_live.return_address = g_live.get;
            g_live.subroutine_active = 1;
            g_live.get = sanitize_target(word);
            ++g_live.calls;
            continue;
        }
        if (word == PB_RETURN) {
            if (!g_live.subroutine_active) {
                fprintf(stderr,
                        "[PB-LIVE] RETURN without CALL at 0x%08X\n",
                        g_live.get - 4);
                return nv_pb_decode_fail(NV_PB_FAILURE_RETURN,g_live.get-4,put);
            }
            g_live.get = g_live.return_address;
            g_live.subroutine_active = 0;
            ++g_live.returns;
            continue;
        }
        if ((word & PB_PACKET_MASK) == PB_METHOD_INC ||
            (word & PB_PACKET_MASK) == PB_METHOD_NON_INC) {
            g_live.method = word & 0x1FFCu;
            g_live.subchannel = (word >> 13) & 7u;
            g_live.method_count = (word >> 18) & 0x7FFu;
            g_live.method_incrementing =
                ((word & PB_PACKET_MASK) == PB_METHOD_INC);
            if(g_live.index_admission.active)
                index_admission_header(&g_live.index_admission,g_live.method,g_live.method_count,
                    g_live.method_incrementing,g_live.get,put,guest_memory_size);
            if(g_live.histogram_enabled && g_live.method>=NV097_SET_TRANSFORM_CONSTANT &&
               g_live.method<NV097_SET_TRANSFORM_CONSTANT+0x80u){
                unsigned n=g_live.method_count;
                ++g_live.constant_packets[!!g_live.method_incrementing][n<=32?n:33];
                if(g_live.method_incrementing && n>(NV097_SET_TRANSFORM_CONSTANT+0x80u-g_live.method)/4u)
                    ++g_live.constant_crossing;
                if(put<g_live.get || (put&3u) || n>(put-g_live.get)/4u)
                    ++g_live.constant_partial;
            }
            ++g_live.packets;
            continue;
        }

        ++g_live.invalid_words;
        if (g_live.invalid_words <= 16) {
            fprintf(stderr,
                    "[PB-LIVE] Reserved command 0x%08X at 0x%08X; skipping\n",
                    word, g_live.get - 4);
        }
    }

    return g_live.get==put ? NV_PB_COMPLETE : NV_PB_YIELD;
}

#include "nv2a_mixed_probe.h"
#include "nv2a_packet_cursor.h"

static int decode_until_put(uint8_t *memory,size_t size,uint32_t put)
{
    if(g_live.failed)return NV_PB_ERROR;
    if(g_live.packet_cursor_enabled && nv_packet_cursor_quiet() && !(put&3u))
        return nv_packet_cursor_decode(memory,size,put);
    NV_MIXED_EVENT(12,0,((uint64_t)g_live.get<<32)|put);
    return decode_until_put_reference(memory,size,put,0);
}

/* Slice boundaries are an internal fairness/safety budget, not guest ownership
 * boundaries. Finish the published stream synchronously before returning to a
 * guest producer that may reuse mutable buffers or patch return jumps. No GET
 * publication, report poll or presentation is inserted between these slices. */
static int decode_owned_until_put(uint8_t *memory,size_t size,uint32_t put)
{
    for(unsigned slice=0;slice<PB_MAX_OWNED_SLICES;++slice){
        int result=decode_until_put(memory,size,put);
        if(result!=NV_PB_YIELD)return result;
        ++g_live.decoder_yields;
    }
    return nv_pb_decode_fail(NV_PB_FAILURE_OWNED_LIMIT,g_live.get,put);
}

static void print_method_profile(void)
{
    struct MethodRank {
        uint64_t count;
        uint32_t method;
        uint32_t subchannel;
    } top[20];

    memset(top, 0, sizeof(top));
    for (uint32_t subchannel = 0; subchannel < 8; ++subchannel) {
        for (uint32_t slot = 0; slot < PB_METHOD_SLOTS; ++slot) {
            const uint64_t count =
                g_live.method_histogram[subchannel][slot];
            if (count <= top[19].count) {
                continue;
            }
            for (unsigned rank = 0; rank < 20; ++rank) {
                if (count > top[rank].count) {
                    memmove(&top[rank + 1], &top[rank],
                            (19u - rank) * sizeof(top[0]));
                    top[rank].count = count;
                    top[rank].method = slot * 4u;
                    top[rank].subchannel = subchannel;
                    break;
                }
            }
        }
    }

    fprintf(stderr, "[PB-LIVE] Top methods:");
    for (unsigned rank = 0; rank < 20 && top[rank].count != 0; ++rank) {
        fprintf(stderr, " s%u:%04X=%llu",
                top[rank].subchannel, top[rank].method,
                (unsigned long long)top[rank].count);
    }
    fputc('\n', stderr);
    fprintf(stderr,"[PB-NOP] cumulative zero=%llu nonzero=%llu operations=",
        (unsigned long long)g_live.nop_zero,(unsigned long long)g_live.nop_nonzero);
    for(unsigned i=0;i<32;++i)if(g_live.nop_operations[i])fprintf(stderr," %u:%llu",i,(unsigned long long)g_live.nop_operations[i]);
    fputc('\n',stderr);
    fprintf(stderr,"[PB-CONSTANT-PACKETS] cumulative crossing=%llu partial_at_header=%llu lengths=",
        (unsigned long long)g_live.constant_crossing,(unsigned long long)g_live.constant_partial);
    for(unsigned inc=0;inc<2;++inc)for(unsigned n=0;n<34;++n)
        if(g_live.constant_packets[inc][n])fprintf(stderr," %s:%u=%llu",inc?"inc":"fixed",n,
            (unsigned long long)g_live.constant_packets[inc][n]);
    fputc('\n',stderr);
    fprintf(stderr,"[PB-CONSTANT-BATCH] cumulative calls=%llu words=%llu\n",
        (unsigned long long)g_live.constant_batch_calls,(unsigned long long)g_live.constant_batch_words);
    fprintf(stderr,"[PB-INLINE-BATCH] cumulative calls=%llu words=%llu\n",
        (unsigned long long)g_live.inline_batch_calls,(unsigned long long)g_live.inline_batch_words);
    fprintf(stderr,
            "[PB-LIVE] Current objects: %08X %08X %08X %08X "
            "%08X %08X %08X %08X\n",
            g_live.object_bindings[0], g_live.object_bindings[1],
            g_live.object_bindings[2], g_live.object_bindings[3],
            g_live.object_bindings[4], g_live.object_bindings[5],
            g_live.object_bindings[6], g_live.object_bindings[7]);
    fprintf(stderr,
            "[PB-LIVE] Geometry totals: begin=%llu end=%llu "
            "elem16=%llu elem32=%llu arrays=%llu inline=%llu clears=%llu\n",
            (unsigned long long)g_live.begin_count,
            (unsigned long long)g_live.end_count,
            (unsigned long long)g_live.array_element16_count,
            (unsigned long long)g_live.array_element32_count,
            (unsigned long long)g_live.draw_arrays_count,
            (unsigned long long)g_live.inline_array_count,
            (unsigned long long)g_live.clear_surface_count);
}

/* A caller may ignore a drain's return value and immediately patch/reuse its
 * command buffer. A malformed stream therefore cannot safely resume guest
 * execution. Keep the project's existing abort-on-broken-ownership convention;
 * the first-failure record above remains inspectable. Renderer rejection keeps
 * its existing poll/return policy and is not reclassified as a parse error. */
static void nv_pb_stop_on_parse_failure(void)
{
    if(g_live.failure_reason&&g_live.failure_reason!=NV_PB_FAILURE_RENDERER){
        fprintf(stderr,"[PB-LIVE-FAIL] reason=%u get=%08X put=%08X yields=%llu; refusing guest buffer reuse\n",
            g_live.failure_reason,g_live.failure_get,g_live.failure_put,
            (unsigned long long)g_live.decoder_yields);
        abort();
    }
}

static int nv2a_pb_live_run_body(uint8_t *guest_memory, size_t guest_memory_size,
                       uint32_t device_pointer_va, int present)
{
    uint32_t device_va;
    uint32_t base;
    uint32_t end;
    uint32_t put;
    LARGE_INTEGER timing_entry;
    LARGE_INTEGER timing_after_decode;
    LARGE_INTEGER timing_after_flush;
    LARGE_INTEGER timing_after_present;
#if UCRR_PLAY_ONLY
    const int timing_option=0,timing_enabled=0,coarse_timing=0;
#else
    static int timing_option=-1;
    if(timing_option<0)timing_option=getenv("UCRR_TIMING_TRACE")!=NULL;
    static int coarse_timing=-1;
    if(coarse_timing<0)coarse_timing=getenv("UCRR_COARSE_TIMING")!=NULL;
    int timing_enabled = present && timing_option;
#endif
    LARGE_INTEGER early_entry;

    if (!g_live.initialized || !guest_memory || guest_memory_size < 4 ||
        !guest_dword_valid(guest_memory_size, device_pointer_va)) {
        return 0;
    }

    if (timing_enabled) {
        QueryPerformanceCounter(&timing_entry);
        if (g_live.timing_frequency == 0) {
            LARGE_INTEGER frequency;
            QueryPerformanceFrequency(&frequency);
            g_live.timing_frequency = frequency.QuadPart;
        }
        if (g_live.timing_last_entry != 0) {
            uint64_t gap = (uint64_t)(timing_entry.QuadPart -
                                      g_live.timing_last_entry);
            g_live.timing_gap_ticks += gap;
            if (gap > g_live.timing_gap_max) g_live.timing_gap_max = gap;
            if (g_live.timing_gap_samples < 120u)
                g_live.timing_gaps[g_live.timing_gap_samples] = gap;
            ++g_live.timing_gap_samples;
        }
        g_live.timing_last_entry = timing_entry.QuadPart;
    }

    /* Empty progress polls do not consume vertex/attribute data. Defer their
     * redundant cache reset until commands resume, but always install a new
     * mapping and preserve presentation/failed-stream completion semantics. */
    int reset_guest_caches = present || g_live.failed ||
        g_live.mapped_guest_memory != guest_memory ||
        g_live.mapped_guest_memory_size != guest_memory_size;
    if (reset_guest_caches) {
        pgraph_d3d11_set_guest_memory(guest_memory, guest_memory_size);
        g_live.mapped_guest_memory = guest_memory;
        g_live.mapped_guest_memory_size = guest_memory_size;
    }
    if(g_live.failed){unsigned previous=pgraph_d3d11_query_scope(NV_WAIT_OTHER);pgraph_d3d11_poll_reports();pgraph_d3d11_query_scope(previous);return 0;}

    device_va = guest_read32(guest_memory, device_pointer_va);
    if (!guest_dword_valid(guest_memory_size, device_va) ||
        (size_t)device_va + 0x30u >= guest_memory_size) {
        return 0;
    }

    put = guest_read32(guest_memory, device_va + 0x00u);
    base = guest_read32(guest_memory, device_va + 0x24u);
    end = guest_read32(guest_memory, device_va + 0x28u);
    if (!guest_dword_valid(guest_memory_size, base) ||
        end <= base || end > guest_memory_size ||
        put < base || put > end || (put & 3u) != 0) {
        return 0;
    }

    if (!g_live.stream_ready) {
        /* The D3D runtime reserves the first dword as its circular-buffer jump
         * slot. Starting there lets the normal control-flow decoder select the
         * active region, including initialization commands emitted before the
         * first vertical blank. */
        g_live.get = base;
        g_live.stream_ready = 1;
        fprintf(stderr,
                "[PB-LIVE] D3DDevice=0x%08X buffer=0x%08X..0x%08X "
                "initial PUT=0x%08X\n",
                device_va, base, end, put);
    }

    /* Progress waits may call this millions of times with no new commands.
     * Count those polls without two clock queries per iteration. They still
     * execute the normal report/semaphore completion path below. */
    int measure_early=!present && timing_option && !coarse_timing && g_live.get!=put;
    if (!reset_guest_caches && g_live.get != put)
        pgraph_d3d11_set_guest_memory(guest_memory, guest_memory_size);
    if(measure_early)QueryPerformanceCounter(&early_entry);
    /* Empty progress polls stay unclocked and explicitly unassigned. */
    unsigned cost_token=(present||g_live.get!=put)?nv_frame_cost_enter(&frame_cost):0;
    if (decode_owned_until_put(guest_memory, guest_memory_size, put)==NV_PB_ERROR) {
        nv_pb_stop_on_parse_failure();
        if(g_live.failed){unsigned previous=pgraph_d3d11_query_scope(NV_WAIT_OTHER);nv_frame_cost_stage(&frame_cost,cost_token,NV_COST_REPORTS);pgraph_d3d11_poll_reports();pgraph_d3d11_query_scope(previous);nv_frame_cost_leave(&frame_cost,cost_token);return 0;}
        return 0;
    }

    /* END methods already submit complete draws. A query wait must consume
     * work without inventing a frame boundary or presenting a partial scene. */
    pgraph_d3d11_publish_dma_get(g_live.get);
    nv_frame_cost_stage(&frame_cost,cost_token,NV_COST_REPORTS);
    pgraph_d3d11_poll_reports();
    if (!present) {
        nv_frame_cost_leave(&frame_cost,cost_token);
        if(measure_early){
            LARGE_INTEGER now;QueryPerformanceCounter(&now);
            g_live.timing_early_ticks+=(uint64_t)(now.QuadPart-early_entry.QuadPart);
            ++g_live.timing_early_calls;
        }else if(timing_option&&!coarse_timing)++g_live.timing_empty_calls;
        return 1;
    }
    if (timing_enabled) QueryPerformanceCounter(&timing_after_decode);

    nv_frame_cost_stage(&frame_cost,cost_token,NV_COST_FLUSH);
    pgraph_d3d11_flush();
    if (timing_enabled) QueryPerformanceCounter(&timing_after_flush);
    nv_frame_cost_stage(&frame_cost,cost_token,NV_COST_PRESENT);
    d3d8_PresentFrame();
    nv_frame_cost_leave(&frame_cost,cost_token);
    if (timing_enabled) {
        uint64_t decode_ticks;
        uint64_t flush_ticks;
        uint64_t present_ticks;
        QueryPerformanceCounter(&timing_after_present);
        trace_post_present(timing_entry.QuadPart,timing_after_flush.QuadPart,timing_after_present.QuadPart);
        decode_ticks = (uint64_t)(timing_after_decode.QuadPart -
                                  timing_entry.QuadPart);
        flush_ticks = (uint64_t)(timing_after_flush.QuadPart -
                                 timing_after_decode.QuadPart);
        present_ticks = (uint64_t)(timing_after_present.QuadPart -
                                   timing_after_flush.QuadPart);
        g_live.timing_decode_ticks += decode_ticks;
        g_live.timing_flush_ticks += flush_ticks;
        g_live.timing_present_ticks += present_ticks;
        if (decode_ticks > g_live.timing_decode_max)
            g_live.timing_decode_max = decode_ticks;
        if (flush_ticks > g_live.timing_flush_max)
            g_live.timing_flush_max = flush_ticks;
        if (present_ticks > g_live.timing_present_max)
            g_live.timing_present_max = present_ticks;
        ++g_live.timing_stage_samples;
    }
    ++g_live.frames;
    if(g_live.index_nonforward_ab.enabled&&!g_live.index_nonforward_ab.exhausted)
        index_nonforward_ab_present();
    frame_cost_boundary();
    if(g_live.index_admission.enabled&&!g_live.index_admission.exhausted){
        unsigned event=index_admission_boundary(&g_live.index_admission,g_live.frames,
            g_live.index_admission.started||pgraph_d3d11_constant_ab_ready(),
            g_live.method,g_live.method_count);
        if(event==1){
            g_live.index_admission.carry_incrementing=!!g_live.method_incrementing;
            g_live.index_admission.carry_batching=!!g_live.index_batch_enabled;
            g_live.index_admission.carry_trace=!!g_live.trace_enabled;
            g_live.index_admission.carry_fast=!!g_live.index_scalar_fast_route;
            g_live.index_admission.carry_available=index_admission_available(
                g_live.get,put,guest_memory_size,g_live.method_count);
        }
        if(event)index_admission_print(&g_live.index_admission,
            event==1?"armed":event==2?"complete":"discontinuous");
    }

    if (timing_enabled && (g_live.frames % 120u) == 0u &&
        g_live.timing_frequency != 0) {
        fprintf(stderr,"[FRAME-WINDOW] ms=%llu frame=%llu samples=%u\n",(unsigned long long)GetTickCount64(),(unsigned long long)g_live.frames,g_live.timing_gap_samples);
        const double tick_ms = 1000.0 / (double)g_live.timing_frequency;
        const double gap_avg = g_live.timing_gap_samples
            ? (double)g_live.timing_gap_ticks * tick_ms /
                (double)g_live.timing_gap_samples : 0.0;
        const double stage_divisor = g_live.timing_stage_samples
            ? (double)g_live.timing_stage_samples : 1.0;
        uint32_t n = g_live.timing_gap_samples;
        uint32_t over20 = 0, over33 = 0;
        if (n > 120u) n = 120u;
        qsort(g_live.timing_gaps, n, sizeof(uint64_t), compare_frame_ticks);
        for (uint32_t i = 0; i < n; ++i) {
            double ms = g_live.timing_gaps[i] * tick_ms;
            over20 += ms > 20.0;
            over33 += ms > (1000.0 / 30.0);
        }
        if (n) fprintf(stderr,
            "[FRAME-PACING] samples=%u p50-ms=%.3f p95-ms=%.3f p99-ms=%.3f over20=%u over33=%u (pre-wait entry intervals, not presentation or simulation FPS)\n",
            n, g_live.timing_gaps[(n - 1) / 2] * tick_ms,
            g_live.timing_gaps[(95u * n + 99u) / 100u - 1u] * tick_ms,
            g_live.timing_gaps[(99u * n + 99u) / 100u - 1u] * tick_ms,
            over20, over33);
        fprintf(stderr,
                "[FRAME-STAGES] avg-ms total=%.2f decode=%.2f flush=%.2f "
                "present=%.2f other=%.2f max-ms total=%.2f decode=%.2f "
                "flush=%.2f present=%.2f\n",
                gap_avg,
                (double)g_live.timing_decode_ticks * tick_ms / stage_divisor,
                (double)g_live.timing_flush_ticks * tick_ms / stage_divisor,
                (double)g_live.timing_present_ticks * tick_ms / stage_divisor,
                gap_avg - ((double)(g_live.timing_decode_ticks +
                    g_live.timing_flush_ticks + g_live.timing_present_ticks) *
                    tick_ms / stage_divisor),
                (double)g_live.timing_gap_max * tick_ms,
                (double)g_live.timing_decode_max * tick_ms,
                (double)g_live.timing_flush_max * tick_ms,
                (double)g_live.timing_present_max * tick_ms);
        if(!coarse_timing)fprintf(stderr,"[EARLY-RENDER] window_ms=%.3f work_drains=%llu untimed_empty_drains=%llu (new-command decode/submission only; empty query-poll cost remains in other)\n",
            g_live.timing_early_ticks*tick_ms,(unsigned long long)g_live.timing_early_calls,(unsigned long long)g_live.timing_empty_calls);
        g_live.timing_early_ticks=g_live.timing_early_calls=g_live.timing_empty_calls=0;
        g_live.timing_gap_ticks = 0;
        g_live.timing_decode_ticks = 0;
        g_live.timing_flush_ticks = 0;
        g_live.timing_present_ticks = 0;
        g_live.timing_gap_max = 0;
        g_live.timing_decode_max = 0;
        g_live.timing_flush_max = 0;
        g_live.timing_present_max = 0;
        g_live.timing_gap_samples = 0;
        g_live.timing_stage_samples = 0;
    }

    if (g_live.frames <= 5 || (g_live.frames % 300u) == 0) {
#if !defined(UCRR_LEAN_RUNTIME) || UCRR_LEAN_RUNTIME != 1
        PgraphD3D11Stats stats;
        pgraph_d3d11_get_stats(&stats);
        fprintf(stderr,
                "[PB-LIVE] Frame %llu: GET/PUT=%08X packets=%llu "
                "methods=%llu jumps=%llu calls=%llu returns=%llu "
                "invalid=%llu draws=%u clears=%u\n",
                (unsigned long long)g_live.frames, put,
                (unsigned long long)g_live.packets,
                (unsigned long long)g_live.methods,
                (unsigned long long)g_live.jumps,
                (unsigned long long)g_live.calls,
                (unsigned long long)g_live.returns,
                (unsigned long long)g_live.invalid_words,
                stats.draw_calls, stats.clears);
#endif
        if (g_live.histogram_enabled &&
            (g_live.frames == 5 || (g_live.frames % 300u) == 0)) {
            print_method_profile();
        }
    }

    return 1;
}

static int nv2a_pb_live_run(uint8_t *memory,size_t size,uint32_t device_pointer_va,int present){
    /* GET here is the previous consumer position, not a guest CPU instruction. */
    nv_causal_set_frame((uint32_t)g_live.frames);
    uint64_t causal=nv_causal_begin(NV_CAUSAL_DRAIN,__LINE__,g_live.get,(unsigned)present);
    uint64_t token=pgraph_d3d11_dependency_event(1,0,((uint64_t)g_live.get<<32)|(unsigned)present);
    int result=nv2a_pb_live_run_body(memory,size,device_pointer_va,present);
    pgraph_d3d11_dependency_event(2,token,((uint64_t)g_live.get<<32)|(unsigned)result);
    nv_causal_end(causal,((uint64_t)g_live.get<<32)|(unsigned)result);
    nv_causal_set_frame((uint32_t)g_live.frames);
    nv_causal_flush();
    return result;
}

int nv2a_pb_live_frame(uint8_t *memory, size_t size, uint32_t device_pointer_va)
{
    unsigned previous=pgraph_d3d11_query_scope(NV_WAIT_FRAME);
    int result=nv2a_pb_live_run(memory, size, device_pointer_va, 1);
    pgraph_d3d11_query_scope(previous);return result;
}

int nv2a_pb_live_drain_reason(uint8_t *memory,size_t size,uint32_t device_pointer_va,unsigned reason)
{
    unsigned previous=pgraph_d3d11_query_scope(reason);
    int result=nv2a_pb_live_run(memory,size,device_pointer_va,0);
    pgraph_d3d11_query_scope(previous);return result;
}

int nv2a_pb_live_drain(uint8_t *memory, size_t size, uint32_t device_pointer_va)
{
    return nv2a_pb_live_run(memory, size, device_pointer_va, 0);
}

void nv2a_pb_live_shutdown(void)
{
    if (!g_live.initialized) {
        return;
    }
    if(g_live.index_admission.active){
        g_live.index_admission.active=0;g_live.index_admission.exhausted=1;
        index_admission_print(&g_live.index_admission,"shutdown_incomplete");
    }
    pgraph_d3d11_shutdown();
    memset(&g_live, 0, sizeof(g_live));
}
