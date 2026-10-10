/* Bounded, read-only provenance for the nonrecursive original 001F0E50.
 * Its direct callees do not reenter it. Thread-local pending state prevents
 * cross-thread pairing. Bounds validation is not object-lifetime proof. */
typedef struct BlendProbe {
    unsigned inspections, finite_samples, bad_samples, witness_samples;
    int pending, node_valid;
    uint32_t object, caller, input[12], node[10], node_address;
} BlendProbe;
static RECOMP_TLS BlendProbe blend_probe;
static int blend_probe_enabled = -1;
static void animation_blend_probe(int leaving) {
    BlendProbe *p=&blend_probe;
    if(blend_probe_enabled<0)
        blend_probe_enabled=getenv("UCRR_CONSTANT_SOURCE_TRACE")!=NULL;
    if(!blend_probe_enabled)return;
    if(!leaving){
        p->pending=0;
        if(p->inspections>=65536 || (p->finite_samples>=2&&p->bad_samples>=8&&p->witness_samples>=8))return;
        ++p->inspections;
        if(!resource_readable(g_ecx,48)||!resource_readable(g_esp,4))return;
        p->object=g_ecx;p->caller=MEM32(g_esp);
        for(unsigned i=0;i<12;++i)p->input[i]=MEM32(g_ecx+i*4);
        p->node_address=p->input[11];
        p->node_valid=p->node_address&&resource_readable(p->node_address,40);
        if(p->node_valid)for(unsigned i=0;i<10;++i)p->node[i]=MEM32(p->node_address+i*4);
        p->pending=1;return;
    }
    if(!p->pending)return;
    p->pending=0;
    if(!resource_readable(p->object,28))return;
    uint32_t output[7];int bad=0,witness=0;
    for(unsigned i=0;i<7;++i){output[i]=MEM32(p->object+i*4);
        if((output[i]&0x7F800000u)==0x7F800000u)bad=1;
        if(output[i]==0xFFFFFFC3u)witness=1;}
    if(witness){if(p->witness_samples>=8)return;++p->witness_samples;}
    else if(bad){if(p->bad_samples>=8)return;++p->bad_samples;}
    else{if(p->finite_samples>=2)return;++p->finite_samples;}
    fprintf(stderr,"[ANIMATION-BLEND] inspection=%u object=%08X caller=%08X bad=%d witness=%d node=%08X node-valid=%d\n",
        p->inspections,p->object,p->caller,bad,witness,p->node_address,p->node_valid);
    for(unsigned i=0;i<12;++i)fprintf(stderr,"[ANIMATION-BLEND-IN] i=%u bits=%08X\n",i,p->input[i]);
    if(p->node_valid)for(unsigned i=0;i<10;++i)fprintf(stderr,"[ANIMATION-BLEND-NODE] i=%u bits=%08X\n",i,p->node[i]);
    for(unsigned i=0;i<7;++i)fprintf(stderr,"[ANIMATION-BLEND-OUT] i=%u bits=%08X\n",i,output[i]);
}
