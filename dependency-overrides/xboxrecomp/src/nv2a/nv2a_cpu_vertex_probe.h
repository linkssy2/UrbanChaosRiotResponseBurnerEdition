#ifndef NV2A_CPU_VERTEX_PROBE_H
#define NV2A_CPU_VERTEX_PROBE_H
#include <stdio.h>
#include <stdlib.h>
#include "nv2a_vertex_eval.h"
#include "nv2a_vertex_prepared.h"
#ifdef UCRR_BOUNDED_VERTEX
#include "bounded_specialized_eval.h"
#endif
#ifdef UCRR_INLINE_VERTEX
#include "inline_specialized_eval.h"
#endif
#ifndef NV_CPU_VERTEX_COST_BEGIN
#define NV_CPU_VERTEX_COST_BEGIN(route) 0u
#define NV_CPU_VERTEX_COST_END(token) ((void)(token))
#endif
/* Single renderer owner. Counter probe adds no clocks, guest reads, per-vertex
 * scans or cached results. Optional cost hooks belong to the separate bounded
 * frame ledger. Counters are cumulative and not time attribution. */
/* Bounded diagnostic corpus: the observed high-reuse inline span only. Copies
 * owned instruction words at the 64th inline call, never caches evaluation.
 * Dedup compares full words and start, not a hash or program length alone. */
static struct {
 unsigned count,reported,start[8];
 uint32_t words[8][17][4];
 uint64_t attempts,duplicates,overflow;
} nv_cpu_vertex_corpus;
static void nv_cpu_vertex_corpus_capture(const uint32_t program[136][4],unsigned start){
 if(!program||start>136-17)return;
 ++nv_cpu_vertex_corpus.attempts;
 for(unsigned i=0;i<nv_cpu_vertex_corpus.count;++i)
  if(nv_cpu_vertex_corpus.start[i]==start&&!memcmp(nv_cpu_vertex_corpus.words[i],program+start,sizeof(nv_cpu_vertex_corpus.words[i]))){
   ++nv_cpu_vertex_corpus.duplicates;return;
  }
 if(nv_cpu_vertex_corpus.count==8){++nv_cpu_vertex_corpus.overflow;return;}
 unsigned i=nv_cpu_vertex_corpus.count++;
 nv_cpu_vertex_corpus.start[i]=start;
 memcpy(nv_cpu_vertex_corpus.words[i],program+start,sizeof(nv_cpu_vertex_corpus.words[i]));
}
static void nv_cpu_vertex_corpus_report(FILE*stream,unsigned frame){
 for(unsigned i=nv_cpu_vertex_corpus.reported;i<nv_cpu_vertex_corpus.count;++i){
  fprintf(stream,"[CPU-VERTEX-CORPUS] frame=%u id=%u start=%u span=17 route=inline_array threshold=64\n",frame,i,nv_cpu_vertex_corpus.start[i]);
  for(unsigned pc=0;pc<17;++pc){const uint32_t*w=nv_cpu_vertex_corpus.words[i][pc];
   fprintf(stream,"[CPU-VERTEX-CORPUS-WORD] id=%u pc=%u words=%08X,%08X,%08X,%08X\n",i,nv_cpu_vertex_corpus.start[i]+pc,w[0],w[1],w[2],w[3]);
  }
 }
 nv_cpu_vertex_corpus.reported=nv_cpu_vertex_corpus.count;
 fprintf(stream,"[CPU-VERTEX-CORPUS-STATS] frame=%u count=%u attempts=%llu duplicates=%llu overflow=%llu\n",frame,nv_cpu_vertex_corpus.count,
  (unsigned long long)nv_cpu_vertex_corpus.attempts,(unsigned long long)nv_cpu_vertex_corpus.duplicates,(unsigned long long)nv_cpu_vertex_corpus.overflow);
}
static struct {
 uint64_t calls[5],accepted[5],starts[5][137];
 uint64_t pending,pending_route[5],runs[10],run_calls[10],route_calls[10][5];
 unsigned pending_span;
 uint64_t span_scans,span_runs[138],span_calls[138][5],span_reused[138][5];
} nv_cpu_vertex_probe;
static int nv_cpu_vertex_probe_option=-1;
static int nv_cpu_vertex_probe_enabled(void){
 if(nv_cpu_vertex_probe_option<0){const char*v=getenv("UCRR_CPU_VERTEX_TRACE");nv_cpu_vertex_probe_option=v&&!strcmp(v,"1");}
 return nv_cpu_vertex_probe_option;
}
/* Owned program storage only. Span through first END, not executed instruction
 * count: unsupported instructions may reject sooner. 0 invalid start,137 no END. */
static unsigned nv_cpu_vertex_span(const uint32_t program[136][4],unsigned start){
 if(start>=136)return 0;
 for(unsigned pc=start;pc<136;++pc)if(program[pc][3]&1)return pc-start+1;
 return 137;
}
/* End a maximal observed interval of unchanged uploaded words and start slot.
 * Reuploading identical bits does not end it. Constants never end it. This
 * measures sequential reuse, not reuse after restoring an older program. */
static void nv_cpu_vertex_probe_boundary(void){
 if(!nv_cpu_vertex_probe_enabled()||!nv_cpu_vertex_probe.pending)return;
 uint64_t n=nv_cpu_vertex_probe.pending;unsigned bucket=0;
 while(n>1&&bucket<9){n>>=1;++bucket;}
 ++nv_cpu_vertex_probe.runs[bucket];
 nv_cpu_vertex_probe.run_calls[bucket]+=nv_cpu_vertex_probe.pending;
 unsigned span=nv_cpu_vertex_probe.pending_span;
 ++nv_cpu_vertex_probe.span_runs[span];
 for(unsigned r=0;r<5;++r){
  nv_cpu_vertex_probe.route_calls[bucket][r]+=nv_cpu_vertex_probe.pending_route[r];
  nv_cpu_vertex_probe.span_calls[span][r]+=nv_cpu_vertex_probe.pending_route[r];
  if(nv_cpu_vertex_probe.pending>=64)nv_cpu_vertex_probe.span_reused[span][r]+=nv_cpu_vertex_probe.pending_route[r];
  nv_cpu_vertex_probe.pending_route[r]=0;
 }
 nv_cpu_vertex_probe.pending=0;
}
static void nv_cpu_vertex_probe_change(uint32_t old_value,uint32_t new_value){
 if(old_value!=new_value)nv_cpu_vertex_probe_boundary();
}
static int nv_cpu_vertex_eval(unsigned route,const uint32_t program[136][4],unsigned start,
 const float constants[192][4],const float input[16][4],float output[16][4],unsigned written[16]){
 unsigned cost_token=NV_CPU_VERTEX_COST_BEGIN(route);
#ifdef UCRR_BOUNDED_VERTEX
 int result=nv2a_vertex_eval_specialized(program,start,constants,input,output,written);
#elif defined(UCRR_INLINE_VERTEX)
 int result=route==3?inline_specialized_eval(program,start,constants,input,output,written):
  nv2a_vertex_eval(program,start,constants,input,output,written);
#else
 int result=nv_vp_decode_eval(route,program,start,constants,input,output,written);
#endif
 NV_CPU_VERTEX_COST_END(cost_token);
 if(nv_cpu_vertex_probe_enabled()){
  unsigned r=route<4?route:4,s=start<136?start:136;
  ++nv_cpu_vertex_probe.calls[r];nv_cpu_vertex_probe.accepted[r]+=!!result;
  ++nv_cpu_vertex_probe.starts[r][s];
  if(!nv_cpu_vertex_probe.pending){nv_cpu_vertex_probe.pending_span=nv_cpu_vertex_span(program,start);++nv_cpu_vertex_probe.span_scans;}
  ++nv_cpu_vertex_probe.pending;++nv_cpu_vertex_probe.pending_route[r];
  if(result&&r==3&&nv_cpu_vertex_probe.pending_span==17&&nv_cpu_vertex_probe.pending_route[3]==64)
   nv_cpu_vertex_corpus_capture(program,start);
 }
 return result;
}
static void nv_cpu_vertex_probe_report(unsigned frame){
 if(!nv_cpu_vertex_probe_enabled())return;
 nv_cpu_vertex_corpus_report(stderr,frame);
 static const char*names[]={"immediate","indexed_gpu_first","indexed_cpu","inline_array","unknown"};
 for(unsigned r=0;r<5;++r){
  fprintf(stderr,"[CPU-VERTEX-ROUTE] cumulative frame=%u route=%s calls=%llu accepted=%llu starts=",frame,names[r],
   (unsigned long long)nv_cpu_vertex_probe.calls[r],(unsigned long long)nv_cpu_vertex_probe.accepted[r]);
  for(unsigned s=0;s<137;++s)if(nv_cpu_vertex_probe.starts[r][s])fprintf(stderr,"%u:%llu,",s,(unsigned long long)nv_cpu_vertex_probe.starts[r][s]);
  fprintf(stderr,"\n");
 }
 fprintf(stderr,"[CPU-VERTEX-REUSE] cumulative frame=%u pending=%llu buckets=log2-clamped-9\n",frame,(unsigned long long)nv_cpu_vertex_probe.pending);
 for(unsigned b=0;b<10;++b)if(nv_cpu_vertex_probe.runs[b]){
  fprintf(stderr,"[CPU-VERTEX-REUSE-BIN] frame=%u min=%u runs=%llu calls=%llu routes=",frame,1u<<b,
   (unsigned long long)nv_cpu_vertex_probe.runs[b],(unsigned long long)nv_cpu_vertex_probe.run_calls[b]);
  for(unsigned r=0;r<5;++r)fprintf(stderr,"%s%llu",r?",":"",(unsigned long long)nv_cpu_vertex_probe.route_calls[b][r]);
  fprintf(stderr,"\n");
 }
 fprintf(stderr,"[CPU-VERTEX-SPANS] cumulative frame=%u scans=%llu pending_span=%u\n",frame,(unsigned long long)nv_cpu_vertex_probe.span_scans,nv_cpu_vertex_probe.pending_span);
 for(unsigned s=0;s<138;++s)if(nv_cpu_vertex_probe.span_runs[s]){
  fprintf(stderr,"[CPU-VERTEX-SPAN] frame=%u span=%u runs=%llu routes=",frame,s,(unsigned long long)nv_cpu_vertex_probe.span_runs[s]);
  for(unsigned r=0;r<5;++r)fprintf(stderr,"%s%llu",r?",":"",(unsigned long long)nv_cpu_vertex_probe.span_calls[s][r]);
  fprintf(stderr," reuse_ge64_routes=");
  for(unsigned r=0;r<5;++r)fprintf(stderr,"%s%llu",r?",":"",(unsigned long long)nv_cpu_vertex_probe.span_reused[s][r]);
  fprintf(stderr,"\n");
 }
}
#endif
