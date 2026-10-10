/* Quiet, packet-oriented alternative to the reference word interpreter.
 * Shares GET/continuation and renderer ownership; never caches guest pointers
 * across callbacks or changes report/present boundaries. Included after the
 * reference decoder so an activated diagnostic can resume the same budget. */
#include "nv2a_state_call_filter.h"
#ifndef NV_MIXED_EVENT
#define NV_MIXED_EVENT(kind,id,value) ((void)0)
#define NV_MIXED_METHOD(m,s,i,n) 0
#endif
static int nv_state_call_filter_option=-1;
static int nv_packet_cursor_quiet(void)
{
    return !g_live.trace_enabled && !g_live.histogram_enabled &&
        !nv_deep_trace.active && !nv_command_probe.active && !frame_cost.active &&
        !g_live.index_admission.active && !g_live.constant_ab_enabled &&
        !g_live.inline_ab_enabled;
}

static int nv_packet_cursor_decode(uint8_t *memory,size_t size,uint32_t put)
{
    uint32_t words=0;
#ifdef UCRR_PROGRAM_PACKET
    static int program_packet=-1;
    if(program_packet<0){const char*s=getenv("UCRR_PROGRAM_PACKET");program_packet=s&&!strcmp(s,"1");}
#endif
#if UCRR_PREPARATION_COMMAND
    static int program_transaction=-1;
    if(program_transaction<0){const char*s=getenv("UCRR_PROGRAM_TRANSACTION");program_transaction=s&&!strcmp(s,"1");}
#endif
    static int array_state_batch=-1;
    if(array_state_batch<0){const char*s=getenv("UCRR_CPU_STATE_PACKET");array_state_batch=s&&!strcmp(s,"1");}
    if(nv_state_call_filter_option<0){const char*s=getenv("UCRR_STATE_CALL_FILTER");nv_state_call_filter_option=s&&!strcmp(s,"1");}
    while(g_live.get!=put && words<PB_MAX_WORDS_PER_FRAME){
        /* Software callbacks can reenter runtime services. Recheck on return,
         * never assume an entry-time diagnostic choice lasts across reentry. */
        if(!nv_packet_cursor_quiet()){
            NV_MIXED_EVENT(12,words,((uint64_t)g_live.get<<32)|put);
            return decode_until_put_reference(memory,size,put,words);
        }
        if(!guest_dword_valid(size,g_live.get))return nv_pb_decode_fail(NV_PB_FAILURE_ADDRESS,g_live.get,put);
        if(!g_live.method_count){
            if(g_live.command_block_enabled && put>g_live.get &&
               (!nv_state_call_filter_option||nv_state_call_possible(guest_read32(memory,g_live.get)))){
                unsigned count=(put-g_live.get)/4;
                size_t room=(size-g_live.get)/4;
                if(count>room)count=(unsigned)room;
                if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
                NvStateBlockResult b=pgraph_d3d11_state_block(memory+g_live.get,count);
                if(b.words){
                    NV_MIXED_EVENT(7,((uint64_t)b.packets<<32)|b.methods,((uint64_t)g_live.get<<32)|b.words);
                    g_live.get+=4*b.words;words+=b.words;
                    g_live.packets+=b.packets;g_live.methods+=b.methods;
                    g_live.method=b.method;g_live.subchannel=b.subchannel;
                    g_live.method_incrementing=b.incrementing;continue;}
            }
            uint32_t word=guest_read32(memory,g_live.get);g_live.get+=4;++words;
            NV_MIXED_EVENT(6,0,((uint64_t)(g_live.get-4)<<32)|word);
            if((word&PB_OLD_JUMP_MASK)==PB_OLD_JUMP || (word&3u)==1u){
                g_live.get=sanitize_target(word);++g_live.jumps;continue;
            }
            if((word&3u)==2u){
                if(g_live.subroutine_active)return nv_pb_decode_fail(NV_PB_FAILURE_NESTED_CALL,g_live.get-4,put);
                g_live.return_address=g_live.get;g_live.subroutine_active=1;
                g_live.get=sanitize_target(word);++g_live.calls;continue;
            }
            if(word==PB_RETURN){
                if(!g_live.subroutine_active)return nv_pb_decode_fail(NV_PB_FAILURE_RETURN,g_live.get-4,put);
                g_live.get=g_live.return_address;g_live.subroutine_active=0;
                ++g_live.returns;continue;
            }
            unsigned kind=word&PB_PACKET_MASK;
            if(kind==PB_METHOD_INC || kind==PB_METHOD_NON_INC){
                g_live.method=word&0x1ffcu;g_live.subchannel=(word>>13)&7;
                g_live.method_count=(word>>18)&2047;
                g_live.method_incrementing=kind==PB_METHOD_INC;
                ++g_live.packets;continue;
            }
            ++g_live.invalid_words;continue;
        }

        /* This is the same bounded ascending segment the scalar loop visits.
         * GET above PUT is possible inside CALL targets; do not mistake the
         * lower PUT for the extent of that payload, or cross uint32 wrap. */
        unsigned count=g_live.method_count;
        size_t room=(size-g_live.get)/4;
        if(count>room)count=(unsigned)room;
        if(put>g_live.get && count>(put-g_live.get)/4)count=(put-g_live.get)/4;
        unsigned nowrap=(UINT32_MAX-g_live.get)/4;
        if(count>nowrap)count=nowrap;
        if(count>PB_MAX_WORDS_PER_FRAME-words)count=PB_MAX_WORDS_PER_FRAME-words;
        /* The wrapping final word is left to the reference interpreter. */
        if(!count){NV_MIXED_EVENT(12,words,((uint64_t)g_live.get<<32)|put);return decode_until_put_reference(memory,size,put,words);}
        unsigned method=g_live.method,n=0;
        int sub=(int)g_live.subchannel,forward=put>g_live.get;
        const uint8_t *payload=memory+g_live.get;
        if(!g_live.method_incrementing){
            switch(method){
            case NV097_NO_OPERATION:
                if(count>=4)n=pgraph_d3d11_zero_run(sub,payload,count);
                break;
            case NV097_INLINE_ARRAY:
                if(forward && g_live.inline_batch_enabled && count>1 &&
                   pgraph_d3d11_inline_batch(sub,payload,count))n=count;
                break;
            case NV097_ARRAY_ELEMENT16:case NV097_ARRAY_ELEMENT32:
                if((forward||g_live.index_nonforward_batch) && g_live.index_batch_enabled && count>1 &&
                   pgraph_d3d11_index_batch(sub,method,payload,count))n=count;
                break;
            }
        }else if(forward && count>1){
            if(array_state_batch &&
               ((method>=NV097_SET_VERTEX_DATA_ARRAY_FORMAT&&method<NV097_SET_VERTEX_DATA_ARRAY_FORMAT+64)||
                (method>=NV097_SET_VERTEX_DATA_ARRAY_OFFSET&&method<NV097_SET_VERTEX_DATA_ARRAY_OFFSET+64))){
                unsigned base=method>=NV097_SET_VERTEX_DATA_ARRAY_FORMAT?NV097_SET_VERTEX_DATA_ARRAY_FORMAT:NV097_SET_VERTEX_DATA_ARRAY_OFFSET;
                unsigned span=(base+64-method)/4;if(span>count)span=count;
                if(span>1 && pgraph_d3d11_array_state_batch(sub,method,payload,span))n=span;
            }else if(g_live.constant_batch_enabled && method>=NV097_SET_TRANSFORM_CONSTANT &&
                     method<NV097_SET_TRANSFORM_CONSTANT+0x80){
                unsigned span=(NV097_SET_TRANSFORM_CONSTANT+0x80-method)/4;if(span>count)span=count;
                if(span>1 && pgraph_d3d11_constant_batch(sub,method,payload,span))n=span;
            }
        }
#if UCRR_PREPARATION_COMMAND
        if(!n&&program_transaction&&g_live.method_incrementing&&count>=8&&method>=0xB00&&method<0xB80){
            unsigned span=(0xB80-method)/4;if(span>count)span=count;
            if(span>=8&&pgraph_d3d11_program_transaction(sub,method,payload,span))n=span;
        }
#endif
#ifdef UCRR_PROGRAM_PACKET
        if(!n&&program_packet&&g_live.method_incrementing&&count>1&&method>=0xB00&&method<0xB80){
            unsigned span=(0xB80-method)/4;if(span>count)span=count;
            if(span>1&&pgraph_d3d11_program_batch(sub,method,payload,span))n=span;
        }
#endif
        if(n){
            NV_MIXED_EVENT(8,NV_MIXED_METHOD(method,g_live.subchannel,g_live.method_incrementing,g_live.method_count),((uint64_t)g_live.get<<32)|n);
            g_live.get+=4*n;words+=n;g_live.method_count-=n;g_live.methods+=n;
            if(g_live.method_incrementing)g_live.method+=4*n;
            if(method==NV097_INLINE_ARRAY)g_live.inline_array_count+=n;
            else if(method==NV097_ARRAY_ELEMENT16)g_live.array_element16_count+=n;
            else if(method==NV097_ARRAY_ELEMENT32)g_live.array_element32_count+=n;
            continue;
        }

        /* One route selection for a fixed-method scalar remainder. State is
         * published before each callback exactly as in the old interpreter.
         * Incrementing packets reclassify on the next word; side-effect/bind
         * boundaries reclassify too. Never pre-read payload after a callback. */
        do {
            uint32_t word=guest_read32(memory,g_live.get);
            g_live.get+=4;++words;
            unsigned m=g_live.method;
            NV_MIXED_EVENT(9,NV_MIXED_METHOD(m,g_live.subchannel,g_live.method_incrementing,g_live.method_count),((uint64_t)(g_live.get-4)<<32)|word);
            switch(m){
            case NV097_SET_BEGIN_END:
                if(word==NV097_SET_BEGIN_END_OP_END)++g_live.end_count;
                else{++g_live.begin_count;++g_live.batch_number;g_live.in_begin_end=1;}
                break;
            case NV097_ARRAY_ELEMENT16:++g_live.array_element16_count;break;
            case NV097_ARRAY_ELEMENT32:++g_live.array_element32_count;break;
            case NV097_DRAW_ARRAYS:++g_live.draw_arrays_count;break;
            case NV097_INLINE_ARRAY:++g_live.inline_array_count;break;
            case NV097_CLEAR_SURFACE:++g_live.clear_surface_count;break;
            }
            int handled=0;
            if(m==NV097_NO_OPERATION && !word)
                handled=pgraph_d3d11_try_zero_scalar((int)g_live.subchannel);
            else if(g_live.index_scalar_fast_route && (m==NV097_ARRAY_ELEMENT16||m==NV097_ARRAY_ELEMENT32))
                handled=pgraph_d3d11_try_index_scalar((int)g_live.subchannel,m,word);
            if(!handled)handled=pgraph_d3d11_method((int)g_live.subchannel,m,word);
            NV_MIXED_EVENT(10,NV_MIXED_METHOD(g_live.method,g_live.subchannel,g_live.method_incrementing,g_live.method_count),((uint64_t)g_live.get<<32)|(uint32_t)handled);
            if(handled<0){g_live.get-=4;return nv_pb_decode_fail(NV_PB_FAILURE_RENDERER,g_live.get,put);}
            if(g_live.method==NV097_SET_BEGIN_END && word==NV097_SET_BEGIN_END_OP_END)g_live.in_begin_end=0;
            if(g_live.subchannel<8 && g_live.method==0)g_live.object_bindings[g_live.subchannel]=word;
            ++g_live.methods;--g_live.method_count;
            if(g_live.method_incrementing){g_live.method+=4;break;}
            /* Zero NOP fallback may have flushed a batch, now permitting a
             * bulk run. Nonzero NOP may mutate command memory or decoder state. */
            if(m==NV097_NO_OPERATION || m==0 || m==NV097_SET_BEGIN_END)break;
        }while(--count && g_live.method_count && g_live.get!=put);
    }
    return g_live.get==put ? NV_PB_COMPLETE : NV_PB_YIELD;
}
