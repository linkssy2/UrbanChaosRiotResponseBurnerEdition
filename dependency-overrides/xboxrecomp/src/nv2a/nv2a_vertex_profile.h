/* Included after the production route and profile counters. Sample totals are
 * explicitly not a full-work estimate: periodic sampling can alias workloads. */
static uint64_t profile_vertex_samples;
static void append_indexed_vertex(uint32_t index){
    static int enabled=-1,exact;
    if(enabled<0){
        enabled=getenv("UCRR_RENDER_PROFILE")!=NULL;
        const char *s=getenv("UCRR_RENDER_PROFILE_VERTEX_EXACT");
        exact=s&&!strcmp(s,"1");
    }
    if(!enabled){append_indexed_vertex_route(index);return;}
    uint64_t ordinal=profile_calls[0]++;
    if(!exact&&(ordinal&1023u)){append_indexed_vertex_route(index);return;}
    uint64_t start=profile_start();
    append_indexed_vertex_route(index);
    if(start){LARGE_INTEGER now;QueryPerformanceCounter(&now);
        profile_ticks[0]+=(uint64_t)now.QuadPart-start;++profile_vertex_samples;}
}
