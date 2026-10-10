#ifndef NV_STATE_PROGRAM_H
#define NV_STATE_PROGRAM_H
#include "nv2a_cpu_compact2.h"
/* A guarded straight-line program of state writes. No guest values or final
 * GPU commands are cached. All header/class/publication guards run before the
 * first write, so a failed guard can fall back without partial side effects.
 * Header checks cover mutation without requiring unproven RAM write tracking. */
typedef struct NvStateProgramOp {
    uint32_t *trace;void *destination;int *enabled;
    unsigned short word;unsigned char boolean,ignored;
    unsigned short trace_run;
} NvStateProgramOp;
typedef struct NvStateProgram {
    unsigned flags,valid,preserve,ignored,operations,classes;
    NvStateBlockResult result;
    uint32_t header[32];unsigned short word[32];unsigned char sub[32];
    unsigned compact_mask,ignored_count,array_count,array_blocked;
    unsigned short ignored_op[256],array_op[32];
    NvStateProgramOp op[256];
} NvStateProgram;
/* Programs contain host destinations and relative payload offsets, never guest
 * pointers. Reuse across producer buffer addresses, but guard every header and
 * routing class before writing anything. Two ways retain common tail variants. */
static NvStateProgram nv_state_programs[32][2];
/* Rejection-only directory: full mutable-header/routing guards still execute
 * before any program use. No guest payload or resource certificate lives here. */
typedef struct NvStateDirectory { unsigned valid,header,flags,words; } NvStateDirectory;
static NvStateDirectory nv_state_directory[32][2];
static int nv_state_directory_option=-1;
static int nv_state_prefix_option=-1;
static uint64_t nv_state_prefix_blocks,nv_state_prefix_stores;
static int nv_state_prefix_enabled(void){
    if(nv_state_prefix_option<0){const char*s=getenv("UCRR_STATE_EQUAL_PREFIX");nv_state_prefix_option=s&&!strcmp(s,"1");}
    return nv_state_prefix_option;
}
static int nv_state_directory_enabled(void){
    if(nv_state_directory_option<0){const char*s=getenv("UCRR_STATE_DIRECTORY");nv_state_directory_option=s&&!strcmp(s,"1");}
    return nv_state_directory_option;
}
/* Decoder-owned scratch: unsuccessful admission must not evict a hot program.
 * Compilation has no callbacks/reentry, like the state-plan cache above. */
static NvStateProgram nv_state_program_pending;
static unsigned char nv_state_program_victim[32];
static uint64_t nv_state_program_hits,nv_state_program_builds,nv_state_program_declines,nv_state_program_methods;
static int nv_state_program_option=-1;
static void nv_state_program_publish(NvStateProgram *dst,const NvStateProgram *src){
    /* The unused operation tail is never read: guards and execution are bounded
     * by operations. Preserve the fixed guards/result, copy only live stores.
     * In particular, a compacted 224-write block need not copy 256 operations. */
    memcpy(dst,src,offsetof(NvStateProgram,op)+src->operations*sizeof(src->op[0]));
}
static int nv_state_program_enabled(void){
    if(nv_state_program_option<0){const char*s=getenv("UCRR_STATE_PROGRAM");nv_state_program_option=s&&!strcmp(s,"1");}
    return nv_state_program_option;
}
static int nv_state_program_guard(const NvStateProgram*p,const uint8_t*source,unsigned words,unsigned flags){
    if(!p->valid||p->flags!=flags||p->result.words>words)return 0;
    for(unsigned sub=0,mask=p->classes;mask;sub++,mask>>=1)
        if((mask&1)&&object_classes[sub]!=0x97)return 0;
    for(unsigned i=0;i<p->result.packets;i++){
        uint32_t h;memcpy(&h,source+4*p->word[i],4);
        if(h!=p->header[i])return 0;
    }
    if(p->ignored){
        if((p->compact_mask&NV_C2_STATE_GUARDS)&&NV_CPU_COMPACT2_ON(NV_C2_STATE_GUARDS)){
            for(unsigned i=0;i<p->ignored_count;i++){
                const NvStateProgramOp*o=&p->op[p->ignored_op[i]];
                if(!g_pg.missing_method_reported[o->trace-g_pg.trace_methods])return 0;
            }
        }else for(unsigned i=0;i<p->operations;i++){
            const NvStateProgramOp*o=&p->op[i];
            if(o->ignored&&!g_pg.missing_method_reported[o->trace-g_pg.trace_methods])return 0;
        }
    }
    return 1;
}
static int nv_state_program_compile_first(NvStateProgram*p,const uint8_t*source,unsigned words,unsigned flags,NvStatePlan*first){
    p->valid=0;p->flags=flags;p->preserve=1;p->ignored=0;p->operations=0;p->classes=0;
    p->compact_mask=NV_CPU_COMPACT2_MASK();p->ignored_count=p->array_count=p->array_blocked=0;
    memset(&p->result,0,sizeof(p->result));
    while(words-p->result.words>=2&&p->result.packets<32){
        unsigned at=p->result.words;uint32_t h;memcpy(&h,source+4*at,4);
        NvStatePlan*plan=(!at&&first)?first:nv_state_plan(h);
        /* Constant cursor/program uploads and all side effects stay on their
         * existing path. This prototype optimizes only proven state writes. */
        if(!plan->count||plan->kind||plan->count>=words-at||object_classes[plan->sub]!=0x97)break;
        int accepted=1;
        if(plan->ignored)for(unsigned i=0;i<plan->count;i++)if(plan->write[i].ignored&&
            !g_pg.missing_method_reported[plan->write[i].method/4]){accepted=0;break;}
        if(!accepted)break;
        unsigned packet=p->result.packets;
        p->header[packet]=h;p->word[packet]=(unsigned short)at;p->sub[packet]=(unsigned char)plan->sub;
        p->classes|=1u<<plan->sub;
        for(unsigned i=0;i<plan->count;i++){
            NvStateWrite*w=&plan->write[i];NvStateProgramOp*o=&p->op[p->result.methods+i];
            o->trace=&g_pg.trace_methods[w->method/4];o->destination=w->destination;o->enabled=w->enabled;
            o->word=(unsigned short)(at+1+i);o->boolean=(unsigned char)w->boolean;o->ignored=(unsigned char)w->ignored;
            o->trace_run=1;
        }
        p->preserve&=plan->preserve;p->ignored+=plan->ignored;
        p->result.words+=plan->count+1;p->result.methods+=plan->count;++p->result.packets;
        p->result.subchannel=plan->sub;p->result.incrementing=plan->inc;
        p->result.method=(h&0x1ffc)+(plan->inc?4*plan->count:0);
    }
    /* Tiny singles already have a cheap exact-header plan. Larger packets
     * benefit from compiled stores and nonincrementing write compaction too. */
    p->valid=p->result.packets>=2||p->result.methods>=4;
    if(p->valid){
        /* No admitted operation reads state or calls out. Within this guarded
         * block only the final write to each method can be observed. Compact
         * in place, keeping last-occurrence order. Trace destinations are a
         * unique method identity; related destination/enabled writes travel
         * together. Counts/epoch/preserve still describe ALL original words. */
        uint32_t seen[64]={0};unsigned keep[256],count=0;
        for(unsigned i=p->result.methods;i>0;){
            --i;unsigned m=(unsigned)(p->op[i].trace-g_pg.trace_methods);
            uint32_t bit=1u<<(m&31);
            if(!(seen[m>>5]&bit)){seen[m>>5]|=bit;keep[count++]=i;}
        }
        for(unsigned i=0;i<count;i++)p->op[i]=p->op[keep[count-1-i]];
        p->operations=count;
        if(p->compact_mask&NV_C2_STATE_GUARDS)for(unsigned i=0;i<count;i++){
            const NvStateProgramOp*o=&p->op[i];
            unsigned method=(unsigned)(o->trace-g_pg.trace_methods)*4;
            if(o->ignored)p->ignored_op[p->ignored_count++]=(unsigned short)i;
            if(!nv_cpu_metadata_method(method)){
                if(nv_bundle_array_method(method)&&o->destination&&p->array_count<32)
                    p->array_op[p->array_count++]=(unsigned short)i;
                else p->array_blocked=1;
            }
        }
        if(p->compact_mask&NV_C2_STATE_RUNS)for(unsigned i=0;i<count;){
            unsigned end=i+1;NvStateProgramOp*o=&p->op[i];
            /* Only callback-free trace stores to adjacent, distinct host words.
             * Compaction preserved last-occurrence order and source offsets;
             * guest payload never aliases this private host state. */
            if(!o->destination&&!o->enabled&&!o->boolean)
                while(end<count){const NvStateProgramOp*n=&p->op[end];
                    if(n->destination||n->enabled||n->boolean||
                       n->trace!=o->trace+(end-i)||n->word!=o->word+(end-i))break;
                    ++end;
                }
            o->trace_run=(unsigned short)(end-i);i=end;
        }
    }
    return p->valid;
}
static int nv_state_program_compile(NvStateProgram*p,const uint8_t*source,unsigned words,unsigned flags){
    return nv_state_program_compile_first(p,source,words,flags,NULL);
}
static NvStateBlockResult nv_state_program_run_first(const uint8_t*source,unsigned words,NvStatePlan**handoff){
    NvStateBlockResult none={0};unsigned flags=nv_state_flags();uint32_t header;
    if(words<4)return none;
    memcpy(&header,source,4);
    unsigned set=((header>>2)^(header>>8)^(header>>18)^flags)&31;
    NvStateProgram*p=NULL;int directory=nv_state_directory_enabled();
    for(unsigned way=0;way<2;way++){
      const NvStateDirectory*d=&nv_state_directory[set][way];
      if(directory&&(!d->valid||d->header!=header||d->flags!=flags||d->words>words))continue;
      if(nv_state_program_guard(&nv_state_programs[set][way],source,words,flags)){
        p=&nv_state_programs[set][way];nv_state_program_victim[set]=(unsigned char)(way^1);break;
      }
    }
    if(p)++nv_state_program_hits;
    else {
        int front=nv_bundle_state_enabled();
        /* Reject an ineligible first packet without clearing a program or
         * scanning its payload. Do not memoize publication/routing failures:
         * both can change on the next drain. The plan caches interpretation
         * only and is revalidated by the compiler before any state writes. */
        NvStatePlan*first=nv_state_plan(header);
        if(front&&handoff)*handoff=first;
        if(!first->count||first->kind||first->count>=words||object_classes[first->sub]!=0x97){
            ++nv_state_program_declines;return none;
        }
        unsigned way=!nv_state_programs[set][0].valid?0:!nv_state_programs[set][1].valid?1:nv_state_program_victim[set];
        p=&nv_state_programs[set][way];
        if(nv_state_program_compile_first(&nv_state_program_pending,source,words,flags,front?first:NULL)){
            nv_state_program_publish(p,&nv_state_program_pending);
            nv_state_directory[set][way]=(NvStateDirectory){1,p->header[0],p->flags,p->result.words};
            ++nv_state_program_builds;nv_state_program_victim[set]=(unsigned char)(way^1);
        }else {++nv_state_program_declines;return none;}
    }
    nv_state_program_methods+=p->result.methods;
    /* All routing/publication/first-diagnostic guards above precede this test.
     * Compacted operations already retain only the final pure write to each
     * method. Equal state is not a certificate for guest resource contents;
     * the renderer still validates those before the next retained append. */
    unsigned preserve=p->preserve;
    if(!preserve&&vertex_fetch_plan.valid&&nv_bundle_arrays_enabled()&&nv_cpu_metadata_enabled()){
        if((p->compact_mask&NV_C2_STATE_GUARDS)&&NV_CPU_COMPACT2_ON(NV_C2_STATE_GUARDS)){
            preserve=!p->array_blocked;
            for(unsigned i=0;preserve&&i<p->array_count;i++){
                const NvStateProgramOp*o=&p->op[p->array_op[i]];uint32_t value,current;
                memcpy(&value,source+4*o->word,4);memcpy(&current,o->destination,4);
                if(value!=current)preserve=0;
            }
        }else {preserve=1;for(unsigned i=0;i<p->operations;i++){
            const NvStateProgramOp*o=&p->op[i];unsigned method=(unsigned)(o->trace-g_pg.trace_methods)*4;
            if(!nv_cpu_metadata_method(method)){
                uint32_t value,current;memcpy(&value,source+4*o->word,4);
                if(!nv_bundle_array_method(method)||!o->destination){preserve=0;break;}
                memcpy(&current,o->destination,4);if(value!=current){preserve=0;break;}
            }
        }
        }
    }
    unsigned equal=0,first_changed=0;
    if(nv_equal_state_batch_enabled()&&NV_STATE_BATCH_PENDING()){
        equal=1;for(unsigned i=0;i<p->operations;i++){
            const NvStateProgramOp*o=&p->op[i];uint32_t value;memcpy(&value,source+4*o->word,4);
            if(!nv_state_value_equal(o->trace,o->destination,o->enabled,o->boolean,value)){equal=0;first_changed=i;break;}
        }
    }
    if(!equal){
     gameplay_batch_flush();
     unsigned start=0;
     /* Sparse late changes only. Flush is a callout, so revalidate the prefix
      * after it. Guest payload must not alias private state being updated. */
     if(p->operations>=16&&first_changed>=p->operations-p->operations/4&&
        nv_state_prefix_enabled()&&
        nv_cpu_compact2_disjoint(source,4u*p->result.words,&g_pg,sizeof(g_pg))){
         for(;start<first_changed;start++){
             const NvStateProgramOp*o=&p->op[start];uint32_t value;
             memcpy(&value,source+4*o->word,4);
             if(!nv_state_value_equal(o->trace,o->destination,o->enabled,o->boolean,value))break;
         }
         ++nv_state_prefix_blocks;nv_state_prefix_stores+=start;
     }
     for(unsigned i=start;i<p->operations;i++){
        const NvStateProgramOp*o=&p->op[i];
        if((p->compact_mask&NV_C2_STATE_RUNS)&&NV_CPU_COMPACT2_ON(NV_C2_STATE_RUNS)&&o->trace_run>1&&
           nv_cpu_compact2_disjoint(o->trace,4u*o->trace_run,source+4*o->word,4u*o->trace_run)){
            nv_tx_dirty_range((unsigned)(o->trace-g_pg.trace_methods)*4,o->trace_run,1);
            memcpy(o->trace,source+4*o->word,4u*o->trace_run);i+=o->trace_run-1;continue;
        }
        uint32_t value;memcpy(&value,source+4*o->word,4);
        nv_tx_dirty_range((unsigned)(o->trace-g_pg.trace_methods)*4,1,1);
        *o->trace=value;if(o->enabled)*o->enabled=(value>>30)&1;
        if(o->boolean)value=!!value;
        if(o->destination)memcpy(o->destination,&value,4);
     }
    }
    uint32_t next=attribute_cache.epoch+p->result.methods;
    if(next<attribute_cache.epoch){memset(attribute_cache.tags,0,sizeof(attribute_cache.tags));++next;}
    attribute_cache.epoch=next;
    if(!preserve||!nv_cpu_metadata_enabled())vertex_fetch_plan.valid=0;
    g_pg.stats.methods_handled+=p->result.methods;g_pg.stats.methods_ignored+=p->ignored;
    return p->result;
}
static NvStateBlockResult nv_state_program_run(const uint8_t*source,unsigned words){return nv_state_program_run_first(source,words,NULL);}
static void nv_state_program_report(void){
    if(nv_state_prefix_enabled())fprintf(stderr,"[STATE-PREFIX] blocks=%llu omitted-stores=%llu\n",(unsigned long long)nv_state_prefix_blocks,(unsigned long long)nv_state_prefix_stores);
    if(nv_state_program_enabled())fprintf(stderr,"[STATE-PROGRAM] hits=%llu builds=%llu declines=%llu methods=%llu (lifetime coverage, not FPS)\n",
        (unsigned long long)nv_state_program_hits,(unsigned long long)nv_state_program_builds,
        (unsigned long long)nv_state_program_declines,(unsigned long long)nv_state_program_methods);
}
#endif
