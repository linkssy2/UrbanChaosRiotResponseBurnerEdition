#ifndef NV2A_QUERY_SUBMISSION_H
#define NV2A_QUERY_SUBMISSION_H
#include "nv2a_pgraph_d3d11.h"
/* A reusable-buffer ownership drain must still decode/copy all commands before
 * guest mutation. Only its opportunistic query submission may be deferred.
 * Explicit progress, publication, frame, failure and capacity paths submit.
 * Unknown callbacks must retain the conservative submission contract. */
static int nv_query_submit_on_poll(int reuse_batch,int cpu_only,unsigned reason){
    return !(reuse_batch && cpu_only &&
        (reason==NV_WAIT_REUSE_BEFORE || reason==NV_WAIT_REUSE_AFTER));
}
/* Call ONLY after a real Flush on the single context that owns both queues.
 * The initiating queue marks itself. Cover only the other queue's ended queries;
 * active reports and future End calls are deliberately not covered. This never
 * changes completion, data, ordering, or query ownership. Returns newly covered
 * queries, not a count of flushes actually saved. origin: 0 report, 1 semaphore. */
static unsigned nv_query_cover_other(NvReports*r,NvSemaphores*s,unsigned origin){
    unsigned covered=0;
    if(origin==0){
        for(unsigned i=0;i<s->count;++i){NvSemaphoreItem*q=&s->items[(s->head+i)%NV_SEMAPHORE_QUEUE];
            if(!q->submitted){q->submitted=1;++covered;}}
    }else if(origin==1){
        for(unsigned i=0;i<r->count;++i){NvReportItem*q=&r->queue[(r->head+i)%NV_REPORT_QUEUE];
            if(q->kind==0&&!q->submitted){q->submitted=1;++covered;}}
    }
    return covered;
}
#endif
