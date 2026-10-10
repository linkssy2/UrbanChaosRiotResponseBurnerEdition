#if UCRR_PLAY_ONLY
#define NV_PACKET_DIAGNOSTIC(...) ((void)0)
#else
#define NV_PACKET_DIAGNOSTIC(...) do { __VA_ARGS__; } while(0)
#endif
/* Contiguous index-only packet: no guest callbacks or state changes inside.
 * First-vertex validation stays scalar. On capture failure, resume scalar at
 * that exact index so owned staging expansion and rejection remain unchanged.
 * Capture still checks revisions, each source index and destination capacity.
 * Disabled under vertex profiling to preserve its existing counter semantics. */
static uint64_t packed_packet_admitted,packed_packet_first,packed_packet_remainder;
static uint64_t extreme_packet_calls,extreme_packet_indices,extreme_packet_partial;
static int packed_progress_option=-1;
static void packed_progress_publish(unsigned pending){
    if(!pending)return;
    NV_PACKET_DIAGNOSTIC(profile_gpu_vertices+=pending);g_pg.inline_count+=pending*INLINE_VERT_DWORDS;
    g_pg.indexed_draw=1;
}
static int packed_packet_append(uint32_t method,const uint8_t*payload,unsigned count){
    static int profile=-1,fast=-1;
    if(profile<0)profile=getenv("UCRR_RENDER_PROFILE")!=NULL;
    if(fast<0)fast=getenv("UCRR_PACKED_FAST_ENTRY")!=NULL;
    if(packed_progress_option<0){const char*s=getenv("UCRR_PACKED_PROGRESS");packed_progress_option=s&&!strcmp(s,"1");}
    unsigned v=g_pg.inline_count/INLINE_VERT_DWORDS;
    unsigned per_word=method==NV097_ARRAY_ELEMENT16?2u:1u;
    if(profile||!fast||!g_pg.in_draw||g_pg.indexed_failed||
       v>=MAX_INLINE_VERTS||count>(MAX_INLINE_VERTS-v)/per_word)return 0;
    float zmax=u2f(g_pg.trace_methods[NV097_SET_CLIP_MAX/4]);
    if(v&&(!packed_draw_valid||(g_pg.transform_execution_mode&3)!=2||!(zmax>0)||!isfinite(zmax)))return 0;
    NV_PACKET_DIAGNOSTIC(++packed_packet_admitted);
    unsigned pending=0;
    int scalar=0,span=v&&packed_span_ready();
    unsigned span_options=span?packed_span_options():0;
    for(unsigned i=0;i<count;++i){
        unsigned first_half=0;
#ifdef NV_EXTREME_PACKET_AVAILABLE
        if(!scalar&&span&&packed_progress_option&&nv_extreme_packet_enabled()){
            packed_progress_publish(pending);pending=0;
            unsigned done=packed_extreme_span(method,payload+4u*i,count-i,v,span_options);
            NV_PACKET_DIAGNOSTIC(++extreme_packet_calls);NV_PACKET_DIAGNOSTIC(extreme_packet_indices+=done);
            NV_PACKET_DIAGNOSTIC(extreme_packet_partial+=done!=(count-i)*per_word);
            v+=done;packed_progress_publish(done);
            unsigned whole=done/per_word;
            g_pg.stats.methods_handled+=whole;
            if(whole){uint32_t last;memcpy(&last,payload+4u*(i+whole-1),4);g_pg.trace_methods[method/4]=last;}
            i+=whole;
            if(i==count)return 1;
            first_half=done%per_word;
            scalar=1; /* exact failed half-word, never restart a captured index */
        }
#endif
        uint32_t word;memcpy(&word,payload+4u*i,4);
        ++g_pg.stats.methods_handled;g_pg.trace_methods[method/4]=word;
        for(unsigned j=first_half;j<per_word;++j){
            uint32_t index=per_word==2?(word>>(j*16))&65535u:word;
            if(!v&&!scalar){
                NV_PACKET_DIAGNOSTIC(++packed_packet_first);
                /* Scalar first vertex establishes capture layout and validates
                 * CPU/GPU semantics. Never restart or duplicate this word. */
                append_indexed_vertex(index);
                v=g_pg.inline_count/INLINE_VERT_DWORDS;
                zmax=u2f(g_pg.trace_methods[NV097_SET_CLIP_MAX/4]);
                scalar=!v||!g_pg.in_draw||g_pg.indexed_failed||!packed_draw_valid||
                    (g_pg.transform_execution_mode&3)!=2||!(zmax>0)||!isfinite(zmax);
                if(!scalar&&count*per_word>1)NV_PACKET_DIAGNOSTIC(++packed_packet_remainder);
#ifdef NV_RAW_FIRST_PACKET_AVAILABLE
                if(!scalar&&!i&&raw_capture_first_packet(method,payload,count)){
                    /* First vertex and first DWORD already committed above. */
                    packed_progress_publish(count*per_word-1);
                    g_pg.stats.methods_handled+=count-1;
                    memcpy(&g_pg.trace_methods[method/4],payload+4u*(count-1),4);
                    return 1;
                }
#endif
                span=!scalar&&packed_span_ready();
                if(span)span_options=packed_span_options();
                continue;
            }
            if(!scalar&&(span?packed_span_capture_options(index,v,span_options):packed_capture_vertex(index,v))){
                /* Span capture uses explicit v, never these published counters.
                 * No callbacks in the span; publish before scalar fallback and
                 * packet return. Non-span capture keeps the original updates. */
                if(span&&packed_progress_option)++pending;
                else packed_progress_publish(1);
                ++v;
            }else{packed_progress_publish(pending);pending=0;scalar=1;append_indexed_vertex(index);}
        }
    }
    packed_progress_publish(pending);return 1;
}

#undef NV_PACKET_DIAGNOSTIC
