#ifndef UCRR_SOLVER_COST_PROBE_H
#define UCRR_SOLVER_COST_PROBE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UC_SOLVER_CLOCK
#include <windows.h>
static uint64_t uc_solver_clock(void){LARGE_INTEGER q;QueryPerformanceCounter(&q);return (uint64_t)q.QuadPart;}
static uint64_t uc_solver_frequency(void){LARGE_INTEGER q;QueryPerformanceFrequency(&q);return (uint64_t)q.QuadPart;}
#define UC_SOLVER_CLOCK uc_solver_clock
#define UC_SOLVER_FREQUENCY uc_solver_frequency
#endif
#ifndef UC_SOLVER_TLS
#define UC_SOLVER_TLS __declspec(thread)
#endif
typedef struct UcSolverCost {
 int initialized,enabled;
 unsigned depth,reports,calls;
 uint64_t frequency,start,first,total,sites[3],matches[3],errors;
} UcSolverCost;
static UC_SOLVER_TLS UcSolverCost uc_solver_cost;
static int uc_solver_active(void){return uc_solver_cost.enabled&&uc_solver_cost.reports<128;}
static void uc_solver_begin(void){
 UcSolverCost*s=&uc_solver_cost;
 if(!s->initialized){const char*v=getenv("UCRR_SOLVER_COST");s->initialized=1;s->enabled=v&&!strcmp(v,"1");if(s->enabled)s->frequency=UC_SOLVER_FREQUENCY();}
 if(!uc_solver_active())return;
 if(s->depth++==0){s->start=UC_SOLVER_CLOCK();if(!s->calls)s->first=s->start;}
}
/* Receives values already read by the original dispatch; no new guest read. */
static void uc_solver_dispatch(unsigned site,uint32_t index,uint32_t target){
 static const uint32_t expected[3][17]={
 {0x349AC9,0x3482D0,0x3488F5,0x348320,0x3492E2,0x349905,0x349604,0x348E8E,0x348B59,0x3490F8,0x3491EE,0x349750,0x349406,0x3494D7,0x349AAF,0x349A71,0x349A96},
 {0x34A106,0x349AF1,0x34A0FB,0x34A032,0x34A0A1,0x349D9A,0x349F22},
 {0x34A315,0x34A122,0x34A1CD,0x34A18A,0x34A219,0x34A25F,0x34A2A9,0x34A30A}};
 static const unsigned count[]={17,7,8};
 UcSolverCost*s=&uc_solver_cost;
 if(!uc_solver_active()||!s->depth)return;
 if(site>=3){++s->errors;return;}
 ++s->sites[site];if(index<count[site]&&target==expected[site][index])++s->matches[site];
}
static void uc_solver_end(void){
 UcSolverCost*s=&uc_solver_cost;if(!uc_solver_active())return;
 if(!s->depth){++s->errors;return;}if(--s->depth)return;
 uint64_t end=UC_SOLVER_CLOCK();if(end<s->start||!s->frequency){++s->errors;return;}
 s->total+=end-s->start;
 if(++s->calls<120)return;
 /* Reporting happens after the measured invocation. Wall includes inter-call
  * gaps; total is inclusive root-call time, not exclusive CPU or frame cost. */
 fprintf(stderr,"[SOLVER-COST] report=%u calls=%u total_ms=%.6f wall_ms=%.6f site0=%llu match0=%llu site1=%llu match1=%llu site2=%llu match2=%llu errors=%llu\n",
  ++s->reports,s->calls,1000.0*s->total/s->frequency,1000.0*(end-s->first)/s->frequency,
  (unsigned long long)s->sites[0],(unsigned long long)s->matches[0],
  (unsigned long long)s->sites[1],(unsigned long long)s->matches[1],
  (unsigned long long)s->sites[2],(unsigned long long)s->matches[2],(unsigned long long)s->errors);
 s->calls=0;s->total=0;memset(s->sites,0,sizeof(s->sites));memset(s->matches,0,sizeof(s->matches));
}
#endif
