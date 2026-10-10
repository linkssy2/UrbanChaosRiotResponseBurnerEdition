#ifndef NV2A_CAPTURE_PROBE_H
#define NV2A_CAPTURE_PROBE_H
#include <stdio.h>
#include <stdlib.h>
#include "nv2a_vertex_snapshot.h"
/* Single decoder owner; metadata only, no guest reads or hot-path clocks.
 * Cumulative bounded dictionary. Overflow is explicit, never merged into an
 * existing shape. Counts describe captures, including subsequently rejected
 * draws, NOT submitted/rendered vertices or frame-rate improvements. */
typedef struct NvCaptureShape {
    unsigned count,stride,width[16],step[16];
    uint64_t plans,first_plans,scalar_vertices,batch_vertices;
} NvCaptureShape;
static struct {
    unsigned count,current;
    uint64_t overflow_plans,overflow_scalar,overflow_batch;
    NvCaptureShape shape[16];
} nv_capture_probe;
static int nv_capture_probe_option=-1;
static int nv_capture_probe_enabled(void){
    if(nv_capture_probe_option<0){const char*v=getenv("UCRR_CAPTURE_SHAPES");nv_capture_probe_option=v&&!strcmp(v,"1");}
    return nv_capture_probe_option;
}
static void nv_capture_probe_plan(const NvVertexSnapshotCapturePlan*p,int first){
    if(!nv_capture_probe_enabled())return;
    nv_capture_probe.current=16;
    if(!p->valid||p->count>16){++nv_capture_probe.overflow_plans;return;}
    NvCaptureShape key={0};key.count=p->count;key.stride=p->stride;
    for(unsigned i=0;i<p->count;++i){key.width[i]=p->entry[i].bytes;key.step[i]=p->entry[i].stride;}
    unsigned slot=0;
    for(;slot<nv_capture_probe.count;++slot){NvCaptureShape*s=&nv_capture_probe.shape[slot];
        if(s->count==key.count&&s->stride==key.stride&&!memcmp(s->width,key.width,sizeof(key.width))&&!memcmp(s->step,key.step,sizeof(key.step)))break;
    }
    if(slot==16){++nv_capture_probe.overflow_plans;return;}
    if(slot==nv_capture_probe.count){nv_capture_probe.shape[slot]=key;++nv_capture_probe.count;}
    nv_capture_probe.current=slot;
    ++nv_capture_probe.shape[slot].plans;
    nv_capture_probe.shape[slot].first_plans+=!!first;
}
static void nv_capture_probe_vertices(unsigned count,int batch){
    if(!nv_capture_probe_enabled()||!count)return;
    if(nv_capture_probe.current>=nv_capture_probe.count){
        if(batch)nv_capture_probe.overflow_batch+=count;else nv_capture_probe.overflow_scalar+=count;
    }else{NvCaptureShape*s=&nv_capture_probe.shape[nv_capture_probe.current];
        if(batch)s->batch_vertices+=count;else s->scalar_vertices+=count;
    }
}
static void nv_capture_probe_report(unsigned frame){
    if(!nv_capture_probe_enabled())return;
    fprintf(stderr,"[CAPTURE-SHAPES] cumulative frame=%u rows=%u overflow_plans=%llu overflow_scalar=%llu overflow_batch=%llu\n",frame,nv_capture_probe.count,
        (unsigned long long)nv_capture_probe.overflow_plans,(unsigned long long)nv_capture_probe.overflow_scalar,(unsigned long long)nv_capture_probe.overflow_batch);
    for(unsigned i=0;i<nv_capture_probe.count;++i){NvCaptureShape*s=&nv_capture_probe.shape[i];
        fprintf(stderr,"[CAPTURE-SHAPE] frame=%u slot=%u stride=%u count=%u plans=%llu first_plans=%llu scalar=%llu batch=%llu widths=",frame,i,s->stride,s->count,
            (unsigned long long)s->plans,(unsigned long long)s->first_plans,(unsigned long long)s->scalar_vertices,(unsigned long long)s->batch_vertices);
        for(unsigned a=0;a<s->count;++a)fprintf(stderr,"%s%u",a?",":"",s->width[a]);
        fprintf(stderr," steps=");for(unsigned a=0;a<s->count;++a)fprintf(stderr,"%s%u",a?",":"",s->step[a]);
        fprintf(stderr,"\n");
    }
}
#endif
