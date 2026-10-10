#ifdef UCRR_PROGRAM_PACKET
/* Incrementing program words only. Decoder validates the guest source extent;
 * it cannot alias private host state. No callback, draw or query inside a span.
 * All declines precede writes and payload reads. Keep first-use diagnostics and
 * pending draw barriers on the scalar path. */
int pgraph_d3d11_program_batch(int sub,uint32_t method,const uint8_t*payload,unsigned count){
    if(!payload||!count||count>32||(method&3)||method<0xB00||method>=0xB80||
       count>(0xB80-method)/4||!g_pg.initialized||!ordered_gpu_enabled()||
       sub<0||sub>=8||object_classes[sub]!=0x97||raw_draw_batch.count||gameplay_batch.count||
       nv_deep_trace.active||(g_pg.in_draw&&g_pg.indexed_draw&&gpu_vertices_active&&g_pg.inline_count))return 0;
    for(unsigned i=0;i<count;++i)if(!g_pg.missing_method_reported[method/4+i])return 0;
    nv_attribute_cache_advance(&attribute_cache,count);
    vertex_fetch_plan.valid=0;
    g_pg.stats.methods_handled+=count;g_pg.stats.methods_ignored+=count;
    for(unsigned i=0;i<count;++i){
        uint32_t param;memcpy(&param,payload+4*i,4);
        unsigned current=method+4*i,c=((current-0xB00)/4)&3;
        g_pg.trace_methods[current/4]=param;
        if(g_pg.program_load<136){
            nv_vp_decode_change(g_pg.vertex_program[g_pg.program_load][c],param);
            nv_cpu_vertex_probe_change(g_pg.vertex_program[g_pg.program_load][c],param);
            nv_program_admission_write(&g_pg.program_admission,&g_pg.vertex_program[g_pg.program_load][c],param);
            raw_snapshot_dirty|=1;
        }
        if(c==3)++g_pg.program_load;
    }
    return 1;
}
#endif
