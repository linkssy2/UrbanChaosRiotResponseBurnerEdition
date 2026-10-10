/* Opt-in transaction. Return0 is side-effect-free and requires the cursor
 * to retain the original scalar method path. No callbacks or GET publication
 * inside admitted spans. Counts below8 avoid the measured small-span overhead.
 * All old class/ownership/reporting guards precede writes and payload reads. */
static int nv_program_transaction_disjoint(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&(x+an<=y||y+bn<=x);
}
int pgraph_d3d11_program_transaction(int sub,uint32_t method,const uint8_t*payload,unsigned count){
    if(!payload||!count||count>32||(method&3)||method<0xB00||method>=0xB80||
       count>(0xB80-method)/4||!g_pg.initialized||!ordered_gpu_enabled()||
       sub<0||sub>=8||object_classes[sub]!=0x97||raw_draw_batch.count||gameplay_batch.count||
       nv_deep_trace.active||(g_pg.in_draw&&g_pg.indexed_draw&&gpu_vertices_active&&g_pg.inline_count))return 0;
    for(unsigned i=0;i<count;++i)if(!g_pg.missing_method_reported[method/4+i])return 0;
    unsigned component=((method-0xB00)/4)&3u;
    if(count<8 || g_pg.program_load>=136 || count>(136-g_pg.program_load)*4-component ||
       !nv_program_transaction_disjoint(payload,4u*count,&g_pg,sizeof(g_pg)) ||
       !nv_program_transaction_disjoint(payload,4u*count,&attribute_cache,sizeof(attribute_cache)) ||
       !nv_program_transaction_disjoint(payload,4u*count,&nv_vp_prepared_cache,sizeof(nv_vp_prepared_cache)) ||
       !nv_program_transaction_disjoint(payload,4u*count,&nv_cpu_vertex_probe,sizeof(nv_cpu_vertex_probe)))return 0;
    nv_attribute_cache_advance(&attribute_cache,count);
    vertex_fetch_plan.valid=0;
    g_pg.stats.methods_handled+=count;g_pg.stats.methods_ignored+=count;
    uint32_t *destination=&g_pg.vertex_program[g_pg.program_load][component];
    if(memcmp(destination,payload,4u*count)){
        nv_vp_decode_invalidate();
        nv_cpu_vertex_probe_boundary();
    }
    memcpy(&g_pg.trace_methods[method/4],payload,4u*count);
    memcpy(destination,payload,4u*count);
    g_pg.program_admission.valid=0;raw_snapshot_dirty|=1;
    g_pg.program_load+=(component+count)/4;
    return 1;
}
