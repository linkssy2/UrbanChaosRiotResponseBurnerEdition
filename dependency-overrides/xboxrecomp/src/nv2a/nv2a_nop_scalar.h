/* Narrow all-or-decline entry. No callback or resource write is skipped.
 * Pending raw draws survive only under the explicit zero-NOP batch policy;
 * packed gameplay draws and unsupported ownership use the general handler.
 * The caller proves method==NO_OPERATION and param==0. */
int pgraph_d3d11_try_zero_scalar(int subchannel)
{
    if(!g_pg.initialized || subchannel<0 || subchannel>=8 ||
       (raw_draw_batch.count&&!raw_draw_nop_batch_enabled()) || gameplay_batch.count ||
       !ordered_gpu_enabled() || object_classes[subchannel]!=0x97 ||
       !nv_cpu_metadata_preserve(NV097_NO_OPERATION,0))return 0;
    raw_draw_batch_note_zero();
    nv_attribute_cache_invalidate(&attribute_cache);
    ++g_pg.stats.methods_handled;
    g_pg.trace_methods[NV097_NO_OPERATION/4]=0;
    return 1;
}
