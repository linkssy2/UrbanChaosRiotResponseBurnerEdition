#ifndef NV_LIGHT_COMPARE_PROBE_H
#define NV_LIGHT_COMPARE_PROBE_H
#ifndef NV_LIGHT_COMPARE_BYTES
#define NV_LIGHT_COMPARE_BYTES(a,b,n) memcmp(a,b,n)
#endif
/* Single decoder owner. Optional bounded diagnostics; never skip a comparison.
 * Counts are per report window. Timing samples every127th comparison per key,
 * capped at256 samples/key/window. A periodic sample is not an unbiased census. */
#ifndef NV_LIGHT_CLOCK
static uint64_t nv_light_clock(void){LARGE_INTEGER t;QueryPerformanceCounter(&t);return (uint64_t)t.QuadPart;}
#define NV_LIGHT_CLOCK() nv_light_clock()
#endif
typedef struct NvLightCompareRow {
 uint32_t offset,format,signs;size_t bytes;
 uint64_t calls,samples,ticks,clock_ticks,changes;
} NvLightCompareRow;
typedef struct NvLightCompareProbe {int enabled;unsigned reports;uint64_t omitted;NvLightCompareRow rows[16];} NvLightCompareProbe;
static NvLightCompareProbe light_compare_probe;
static int nv_light_compare(const void*a,const void*b,size_t n,uint32_t offset,uint32_t format,uint32_t signs){
 NvLightCompareProbe*p=&light_compare_probe;
 if(!p->enabled||p->reports>=64)return NV_LIGHT_COMPARE_BYTES(a,b,n);
 NvLightCompareRow*r=NULL;
 for(unsigned i=0;i<16;++i){NvLightCompareRow*q=&p->rows[i];
  if(!q->calls||(q->offset==offset&&q->format==format&&q->signs==signs&&q->bytes==n)){r=q;break;}}
 if(!r){++p->omitted;return NV_LIGHT_COMPARE_BYTES(a,b,n);}
 r->offset=offset;r->format=format;r->signs=signs;r->bytes=n;
 int sample=(r->calls++%127==0)&&r->samples<256,result;
 if(sample){
  uint64_t t0=NV_LIGHT_CLOCK();result=NV_LIGHT_COMPARE_BYTES(a,b,n);uint64_t t1=NV_LIGHT_CLOCK();
  uint64_t c0=NV_LIGHT_CLOCK(),c1=NV_LIGHT_CLOCK();
  r->ticks+=t1-t0;r->clock_ticks+=c1-c0;++r->samples;
 }else result=NV_LIGHT_COMPARE_BYTES(a,b,n);
 if(result)++r->changes;return result;
}
static void nv_light_compare_report(unsigned frame){
 NvLightCompareProbe*p=&light_compare_probe;if(!p->enabled||p->reports>=64||!p->rows[0].calls)return;
 LARGE_INTEGER f;QueryPerformanceFrequency(&f);
 for(unsigned i=0;i<16;++i){NvLightCompareRow*r=&p->rows[i];if(!r->calls)continue;
  fprintf(stderr,"[LIGHT-COMPARE-TIME] frame=%u report=%u offset=%08X format=%08X signs=%X bytes=%zu calls=%llu samples=%llu ticks=%llu clock_ticks=%llu frequency=%lld changes=%llu omitted=%llu\n",frame,p->reports,r->offset,r->format,r->signs,r->bytes,(unsigned long long)r->calls,(unsigned long long)r->samples,(unsigned long long)r->ticks,(unsigned long long)r->clock_ticks,f.QuadPart,(unsigned long long)r->changes,(unsigned long long)p->omitted);
 }
 ++p->reports;memset(p->rows,0,sizeof(p->rows));p->omitted=0;
}
#endif
