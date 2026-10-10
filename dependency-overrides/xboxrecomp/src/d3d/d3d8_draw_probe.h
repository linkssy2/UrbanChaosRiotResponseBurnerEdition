#ifndef D3D8_DRAW_PROBE_H
#define D3D8_DRAW_PROBE_H
/* Default-off sampled host draws. No flush, synchronous wait, batching changes,
 * or inference of particle ownership. Results are elapsed spans, not GPU busy %. */
#define DP_SLOTS 8
#define DP_SAMPLES 16
#define DP_ROWS 32768
typedef struct DrawProbeSample { ID3D11Query *a,*b; unsigned kind,count,blend,src,dst,depth,write,ordinal; } DrawProbeSample;
typedef struct DrawProbeSlot { ID3D11Query *clock; DrawProbeSample sample[DP_SAMPLES]; unsigned state,n,frame; } DrawProbeSlot;
typedef struct DrawProbeRow { unsigned frame,kind,count,blend,src,dst,depth,write,ordinal; double ms; } DrawProbeRow;
static struct { int initialized,enabled,active,current,ending; unsigned frame,ordinal,rows,skipped,invalid,errors,overflow;
    DrawProbeSlot slot[DP_SLOTS]; DrawProbeRow row[DP_ROWS]; wchar_t path[MAX_PATH]; } dp;
static void dp_init(ID3D11Device *dev){
    if(dp.initialized)return;dp.initialized=1;dp.current=-1;
    const char *v=getenv("UCRR_HOST_DRAW_PROBE");if(!v||strcmp(v,"1"))return;
    D3D11_QUERY_DESC desc={D3D11_QUERY_TIMESTAMP_DISJOINT,0};
    for(unsigned i=0;i<DP_SLOTS;++i){
        desc.Query=D3D11_QUERY_TIMESTAMP_DISJOINT;
        if(FAILED(ID3D11Device_CreateQuery(dev,&desc,&dp.slot[i].clock)))goto fail;
        desc.Query=D3D11_QUERY_TIMESTAMP;
        for(unsigned j=0;j<DP_SAMPLES;++j)
            if(FAILED(ID3D11Device_CreateQuery(dev,&desc,&dp.slot[i].sample[j].a))||
               FAILED(ID3D11Device_CreateQuery(dev,&desc,&dp.slot[i].sample[j].b)))goto fail;
    }
    dp.enabled=1;return;
fail:++dp.errors;
}
static void dp_dump(void){
    FILE *f=_wfopen(dp.path,L"w");if(!f)return;
    fprintf(f,"# sampled elapsed host draws; skipped=%u invalid=%u errors=%u overflow=%u\n",dp.skipped,dp.invalid,dp.errors,dp.overflow);
    fprintf(f,"frame,kind,elements,blend,src,dst,depth,depth_write,ordinal,gpu_ms\n");
    for(unsigned i=0;i<dp.rows;++i){DrawProbeRow*r=&dp.row[i];fprintf(f,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%.9f\n",r->frame,r->kind,r->count,r->blend,r->src,r->dst,r->depth,r->write,r->ordinal,r->ms);}
    fclose(f);
}
static void dp_boundary(int start,const wchar_t *path){
    if(!dp.enabled)return;
    if(start){dp.active=1;dp.frame=0;dp.rows=0;swprintf(dp.path,MAX_PATH,L"%ls.host-draw.csv",path);}
    else {dp.active=0;dp.ending=1;}
}
static void dp_frame(ID3D11Device *dev,ID3D11DeviceContext *ctx){
    dp_init(dev);if(!dp.enabled)return;
    if(dp.current>=0){DrawProbeSlot*s=&dp.slot[dp.current];ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)s->clock);s->state=2;dp.current=-1;}
    ++dp.frame;dp.ordinal=0;unsigned pending=0;
    for(unsigned i=0;i<DP_SLOTS;++i){DrawProbeSlot*s=&dp.slot[i];if(s->state!=2)continue;
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d={0};
        HRESULT hr=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)s->clock,&d,sizeof(d),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE){++pending;continue;}
        if(FAILED(hr)){++dp.errors;s->state=3;continue;}
        if(d.Disjoint||!d.Frequency){dp.invalid+=s->n;s->state=0;continue;}
        UINT64 a[DP_SAMPLES],b[DP_SAMPLES];int ready=1;
        for(unsigned j=0;j<s->n;++j){
            HRESULT ha=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)s->sample[j].a,&a[j],sizeof(a[j]),D3D11_ASYNC_GETDATA_DONOTFLUSH);
            HRESULT hb=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)s->sample[j].b,&b[j],sizeof(b[j]),D3D11_ASYNC_GETDATA_DONOTFLUSH);
            if(FAILED(ha)||FAILED(hb)){++dp.errors;ready=-1;break;}
            if(ha!=S_OK||hb!=S_OK){ready=0;break;}
        }
        if(!ready){++pending;continue;}if(ready<0){s->state=3;continue;}
        for(unsigned j=0;j<s->n;++j){DrawProbeSample*x=&s->sample[j];if(b[j]<a[j]){++dp.invalid;continue;}
            if(dp.rows==DP_ROWS){++dp.overflow;continue;}
            dp.row[dp.rows++]=(DrawProbeRow){s->frame,x->kind,x->count,x->blend,x->src,x->dst,x->depth,x->write,x->ordinal,(b[j]-a[j])*1000.0/d.Frequency};
        }s->state=0;
    }
    if(dp.ending&&!pending){dp_dump();dp.ending=0;}
    if(!dp.active)return;
    for(unsigned i=0;i<DP_SLOTS;++i)if(!dp.slot[i].state){DrawProbeSlot*s=&dp.slot[i];s->state=1;s->n=0;s->frame=dp.frame;dp.current=(int)i;ID3D11DeviceContext_Begin(ctx,(ID3D11Asynchronous*)s->clock);return;}
    ++dp.skipped;
}
static int dp_begin(ID3D11DeviceContext*ctx,unsigned kind,unsigned count,const DWORD*rs){
    if(!dp.active||dp.current<0)return -1;
    unsigned ordinal=dp.ordinal++;
    /* Rotate the sampling phase each frame; never split or combine draws. */
    if((ordinal+dp.frame)%64u)return -1;
    DrawProbeSlot*s=&dp.slot[dp.current];if(s->n==DP_SAMPLES){++dp.skipped;return -1;}
    unsigned n=s->n++;DrawProbeSample*x=&s->sample[n];x->kind=kind;x->count=count;x->ordinal=ordinal;
    x->blend=rs[D3DRS_ALPHABLENDENABLE];x->src=rs[D3DRS_SRCBLEND];x->dst=rs[D3DRS_DESTBLEND];x->depth=rs[D3DRS_ZENABLE];x->write=rs[D3DRS_ZWRITEENABLE];
    ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)x->a);return (int)n;
}
static void dp_end(ID3D11DeviceContext*ctx,int token){if(token>=0)ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)dp.slot[dp.current].sample[token].b);}
static void dp_shutdown(void){for(unsigned i=0;i<DP_SLOTS;++i){DrawProbeSlot*s=&dp.slot[i];if(s->clock)ID3D11Query_Release(s->clock);for(unsigned j=0;j<DP_SAMPLES;++j){if(s->sample[j].a)ID3D11Query_Release(s->sample[j].a);if(s->sample[j].b)ID3D11Query_Release(s->sample[j].b);}}memset(&dp,0,sizeof(dp));}
#endif
