#ifndef NV2A_MIXED_PROBE_H
#define NV2A_MIXED_PROBE_H
#ifdef UCRR_MIXED_COMMAND_PROBE
/* Optional extension to the bounded dependency recorder. No guest reads here.
 * Opaque state blocks are NOT complete shape identities or replay keys.
 * Payloads passed here are the exact local word already read by the decoder. */
static int nv_mixed_probe_enabled=-1;
static void nv_mixed_probe(unsigned kind,uint64_t id,uint64_t value){
    if(nv_mixed_probe_enabled<0){const char*s=getenv("UCRR_MIXED_COMMAND_PROBE");
        nv_mixed_probe_enabled=s&&!strcmp(s,"1");}
    if(nv_mixed_probe_enabled)pgraph_d3d11_dependency_event(kind,id,value);
}
static uint64_t nv_mixed_method(unsigned method,unsigned sub,unsigned inc,unsigned count){
    return (uint64_t)method|((uint64_t)sub<<32)|((uint64_t)inc<<40)|((uint64_t)count<<41);
}
#define NV_MIXED_EVENT(kind,id,value) nv_mixed_probe(kind,id,value)
#define NV_MIXED_METHOD(m,s,i,n) nv_mixed_method(m,s,i,n)
#endif
#endif
