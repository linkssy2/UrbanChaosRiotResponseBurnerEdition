#ifndef INDEX_ADMISSION_CENSUS_H
#define INDEX_ADMISSION_CENSUS_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Decoder-owned diagnostic only. No clocks, guest pointers or payload reads.
 * The owner calls boundary only after a real outer presentation boundary. */
#define INDEX_ADMISSION_WINDOW 120u
#define INDEX_ADMISSION_GAP 360u
#define INDEX_ADMISSION_SAMPLES 16u
#define INDEX_ADMISSION_BINS 14u
enum IndexAdmissionReason {
    IA_ARRAYS, IA_DISABLED, IA_TRACE, IA_INCREMENTING, IA_PACKET_ONE,
    IA_REMAINING_ONE, IA_PUT_NONFORWARD, IA_PUT_UNALIGNED, IA_GET_INVALID,
    IA_PUBLISHED_ONE, IA_RAM_ONE, IA_BUDGET_ONE, IA_RENDERER_DECLINED,
    IA_UNEXPLAINED, IA_REASONS
};
typedef struct IndexAdmissionKind {
    uint64_t headers[2][INDEX_ADMISSION_BINS];
    uint64_t header_available[2][INDEX_ADMISSION_BINS];
    uint64_t declared_words,available_words,partial_headers,nonforward_headers;
    uint64_t scalar,batched,consumed,append_requests,failed_scalar;
    uint64_t batch_calls,offered_words,declined_calls,declined_words;
    uint64_t reasons[IA_REASONS],scalar_available[INDEX_ADMISSION_BINS];
    uint64_t partial_scalar,tail_after_batch,unknown_header,other_header;
} IndexAdmissionKind;
typedef struct IndexAdmissionCensus {
    unsigned enabled,started,active,exhausted,boundaries,saturated,errors,sample,gap;
    uint64_t start_frame,last_frame;
    uint32_t carry_method,carry_remaining;
    unsigned carry_incrementing,carry_batching,carry_trace,carry_fast,carry_available;
    unsigned packet_known,packet_length,packet_had_batch;
    uint32_t packet_method;
    IndexAdmissionKind kind[3]; /* ARRAY_ELEMENT16, ARRAY_ELEMENT32, DRAW_ARRAYS */
} IndexAdmissionCensus;

static unsigned index_admission_kind(uint32_t method){
    return method==0x1800u?0u:method==0x1808u?1u:method==0x1810u?2u:3u;
}
/* 0,1,2,3,4,5..8,9..16,...,513..1024,1025..2047. */
static unsigned index_admission_bin(unsigned n){
    if(n<=4)return n;
    unsigned edge=8,bin=5;
    while(n>edge&&bin+1<INDEX_ADMISSION_BINS){edge*=2;++bin;}
    return bin;
}
static void index_admission_add(IndexAdmissionCensus*c,uint64_t *value,uint64_t n){
    if(UINT64_MAX-*value<n){*value=UINT64_MAX;c->saturated=1;}
    else *value+=n;
}
static void index_admission_init(IndexAdmissionCensus*c,const char *option){
    memset(c,0,sizeof(*c));c->enabled=option&&!strcmp(option,"1");
}
static void index_admission_arm(IndexAdmissionCensus*c,uint64_t frame,uint32_t method,unsigned remaining){
    memset(c->kind,0,sizeof(c->kind));
    c->active=1;c->started=1;c->start_frame=c->last_frame=frame;
    c->boundaries=c->gap=c->saturated=c->errors=0;
    c->packet_known=c->packet_length=c->packet_had_batch=0;c->packet_method=0;
    c->carry_incrementing=c->carry_batching=c->carry_trace=c->carry_fast=c->carry_available=0;
    c->carry_method=method;c->carry_remaining=remaining;++c->sample;
}
/* Return 1=armed, 2=complete, 3=discontinuous/incomplete; duplicates are inert.
 * Unsigned subtraction also admits the normal UINT64_MAX -> 0 boundary wrap. */
static unsigned index_admission_boundary(IndexAdmissionCensus*c,uint64_t frame,
                                         int ready,uint32_t method,unsigned remaining){
    if(!c->enabled||c->exhausted)return 0;
    if(!c->started){
        if(!ready)return 0;
        index_admission_arm(c,frame,method,remaining);return 1;
    }
    uint64_t step=frame-c->last_frame;
    if(!step)return 0;
    c->last_frame=frame;
    if(step!=1){++c->errors;c->active=0;c->exhausted=1;return 3;}
    if(!c->active){
        if(++c->gap==INDEX_ADMISSION_GAP){index_admission_arm(c,frame,method,remaining);return 1;}
        return 0;
    }
    if(++c->boundaries==INDEX_ADMISSION_WINDOW){
        c->active=0;if(c->sample==INDEX_ADMISSION_SAMPLES)c->exhausted=1;return 2;
    }
    return 0;
}
/* Available is the published forward prefix, capped by count and RAM. Invalid
 * or wrapped publication is unknown/nonforward, represented separately. */
static unsigned index_admission_available(uint32_t get,uint32_t put,size_t bytes,
                                          unsigned remaining){
    if((get&3u)||(put&3u)||put<get||bytes<4||(size_t)get>bytes-4)return 0;
    uint64_t n=(put-get)/4u,ram=(bytes-get)/4u;
    if(n>ram)n=ram;if(n>remaining)n=remaining;return (unsigned)n;
}
static void index_admission_header(IndexAdmissionCensus*c,uint32_t method,unsigned n,
                                   int incrementing,uint32_t get,uint32_t put,size_t bytes){
    if(!c->active)return;
    c->packet_known=1;c->packet_method=method;c->packet_length=n;c->packet_had_batch=0;
    unsigned k=index_admission_kind(method);if(k>=3)return;
    unsigned available=index_admission_available(get,put,bytes,n);
    IndexAdmissionKind*r=&c->kind[k];
    index_admission_add(c,&r->headers[!!incrementing][index_admission_bin(n)],1);
    index_admission_add(c,&r->header_available[!!incrementing][index_admission_bin(available)],1);
    index_admission_add(c,&r->declared_words,n);index_admission_add(c,&r->available_words,available);
    if(available<n)index_admission_add(c,&r->partial_headers,1);
    if(put<get||(put&3u))index_admission_add(c,&r->nonforward_headers,1);
}
static void index_admission_batch(IndexAdmissionCensus*c,uint32_t method,unsigned n,int accepted){
    if(!c->active)return;
    unsigned k=index_admission_kind(method);if(k>=2){++c->errors;return;}
    IndexAdmissionKind*r=&c->kind[k];
    index_admission_add(c,&r->batch_calls,1);index_admission_add(c,&r->offered_words,n);
    if(!accepted){index_admission_add(c,&r->declined_calls,1);index_admission_add(c,&r->declined_words,n);return;}
    index_admission_add(c,&r->batched,n);index_admission_add(c,&r->consumed,n);
    index_admission_add(c,&r->append_requests,(uint64_t)n*(k?1u:2u));c->packet_had_batch=1;
    if(!c->packet_known)index_admission_add(c,&r->unknown_header,n);
    else if(c->packet_method!=method)index_admission_add(c,&r->other_header,n);
}
static void index_admission_scalar(IndexAdmissionCensus*c,uint32_t method,uint32_t word,
    unsigned remaining,int incrementing,uint32_t get,uint32_t put,size_t bytes,
    unsigned budget,int batching,int trace,int handled){
    if(!c->active)return;
    unsigned k=index_admission_kind(method);if(k>=3)return;
    IndexAdmissionKind*r=&c->kind[k];
    if(handled<0){index_admission_add(c,&r->failed_scalar,1);return;}
    unsigned available=index_admission_available(get,put,bytes,remaining),reason;
    if(k==2)reason=IA_ARRAYS;
    else if(!batching)reason=IA_DISABLED;
    else if(trace)reason=IA_TRACE;
    else if(incrementing)reason=IA_INCREMENTING;
    else if(remaining<=1)reason=c->packet_known&&c->packet_length==1?IA_PACKET_ONE:IA_REMAINING_ONE;
    else if(put<=get)reason=IA_PUT_NONFORWARD;
    else if(put&3u)reason=IA_PUT_UNALIGNED;
    else if((get&3u)||bytes<4||(size_t)get>bytes-4)reason=IA_GET_INVALID;
    else if((put-get)/4u<=1)reason=IA_PUBLISHED_ONE;
    else if((bytes-get)/4u<=1)reason=IA_RAM_ONE;
    else if(budget<=1)reason=IA_BUDGET_ONE;
    /* Reaching scalar with all admission conditions satisfied means the real
     * renderer batch call declined. No additional renderer invocation/probe. */
    else reason=IA_RENDERER_DECLINED;
    index_admission_add(c,&r->scalar,1);index_admission_add(c,&r->consumed,1);
    index_admission_add(c,&r->reasons[reason],1);
    index_admission_add(c,&r->scalar_available[index_admission_bin(available)],1);
    index_admission_add(c,&r->append_requests,k==0?2u:k==1?1u:(word>>24)+1u);
    if(available<remaining)index_admission_add(c,&r->partial_scalar,1);
    if(remaining==1&&c->packet_had_batch)index_admission_add(c,&r->tail_after_batch,1);
    if(!c->packet_known)index_admission_add(c,&r->unknown_header,1);
    else if(c->packet_method!=method)index_admission_add(c,&r->other_header,1);
}
static unsigned index_admission_valid(const IndexAdmissionCensus*c){
    if(c->saturated||c->errors)return 0;
    for(unsigned k=0;k<3;++k){
        const IndexAdmissionKind*r=&c->kind[k];uint64_t reasons=0,availability=0;
        for(unsigned n=0;n<IA_REASONS;++n)reasons+=r->reasons[n];
        for(unsigned n=0;n<INDEX_ADMISSION_BINS;++n)availability+=r->scalar_available[n];
        if(reasons!=r->scalar||availability!=r->scalar||r->consumed!=r->scalar+r->batched||
           r->declined_calls>r->batch_calls||r->offered_words!=r->batched+r->declined_words)return 0;
    }return 1;
}
static void index_admission_print(const IndexAdmissionCensus*c,const char *state){
    static const char *kinds[]={"element16","element32","draw_arrays"};
    static const char *bins[]={"0","1","2","3","4","5-8","9-16","17-32","33-64","65-128","129-256","257-512","513-1024","1025-2047"};
    static const char *reasons[]={"draw_arrays","batch_disabled","trace","incrementing","single_word_packet","remaining_one","put_nonforward","put_unaligned","get_invalid","published_one","ram_one","budget_one","renderer_declined","unexplained"};
    fprintf(stderr,"[INDEX-ADMISSION] schema=1 state=%s sample=%u max_samples=%u start=%llu end=%llu boundaries=%u target=%u gap=%u exhausted=%u valid=%u saturated=%u errors=%u carry_method=%04X carry_remaining=%u carry_incrementing=%u carry_batching=%u carry_trace=%u carry_fast=%u carry_available=%u (decoder words; readiness is not visual scene proof; not timing or rendered vertices)\n",
        state,c->sample,INDEX_ADMISSION_SAMPLES,(unsigned long long)c->start_frame,(unsigned long long)c->last_frame,c->boundaries,
        INDEX_ADMISSION_WINDOW,INDEX_ADMISSION_GAP,c->exhausted,index_admission_valid(c),c->saturated,c->errors,c->carry_method,c->carry_remaining,
        c->carry_incrementing,c->carry_batching,c->carry_trace,c->carry_fast,c->carry_available);
    if(!strcmp(state,"armed"))return;
    for(unsigned k=0;k<3;++k){const IndexAdmissionKind*r=&c->kind[k];
        fprintf(stderr,"[INDEX-ADMISSION-KIND] kind=%s consumed=%llu scalar=%llu batched=%llu append_requests=%llu failed_scalar=%llu batch_calls=%llu offered=%llu declined_calls=%llu declined_words=%llu declared=%llu available_at_header=%llu partial_headers=%llu nonforward_headers=%llu partial_scalar=%llu tail_after_batch=%llu unknown_header_words=%llu other_header_words=%llu reasons=",
            kinds[k],(unsigned long long)r->consumed,(unsigned long long)r->scalar,(unsigned long long)r->batched,
            (unsigned long long)r->append_requests,(unsigned long long)r->failed_scalar,(unsigned long long)r->batch_calls,
            (unsigned long long)r->offered_words,(unsigned long long)r->declined_calls,(unsigned long long)r->declined_words,
            (unsigned long long)r->declared_words,(unsigned long long)r->available_words,(unsigned long long)r->partial_headers,
            (unsigned long long)r->nonforward_headers,(unsigned long long)r->partial_scalar,(unsigned long long)r->tail_after_batch,
            (unsigned long long)r->unknown_header,(unsigned long long)r->other_header);
        for(unsigned n=0;n<IA_REASONS;++n)if(r->reasons[n])fprintf(stderr," %s:%llu",reasons[n],(unsigned long long)r->reasons[n]);
        fputc('\n',stderr);
        for(unsigned inc=0;inc<2;++inc){
            fprintf(stderr,"[INDEX-ADMISSION-HEADERS] kind=%s incrementing=%u lengths=",kinds[k],inc);
            for(unsigned n=0;n<INDEX_ADMISSION_BINS;++n)if(r->headers[inc][n])fprintf(stderr," %s:%llu",bins[n],(unsigned long long)r->headers[inc][n]);
            fprintf(stderr," available=");
            for(unsigned n=0;n<INDEX_ADMISSION_BINS;++n)if(r->header_available[inc][n])fprintf(stderr," %s:%llu",bins[n],(unsigned long long)r->header_available[inc][n]);
            fputc('\n',stderr);
        }
        fprintf(stderr,"[INDEX-ADMISSION-SCALAR-AVAILABLE] kind=%s bins=",kinds[k]);
        for(unsigned n=0;n<INDEX_ADMISSION_BINS;++n)if(r->scalar_available[n])fprintf(stderr," %s:%llu",bins[n],(unsigned long long)r->scalar_available[n]);
        fputc('\n',stderr);
    }
    fprintf(stderr,"[INDEX-ADMISSION-END] schema=1 state=%s sample=%u valid=%u\n",state,c->sample,index_admission_valid(c));
}
#endif
