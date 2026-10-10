#ifndef NV_COMMAND_PROBE_H
#define NV_COMMAND_PROBE_H
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Decoder-owner only. Counts residual scalar dispatch without changing batch
 * admission. Sampled elapsed time includes descendants; never sum with ETW
 * exclusive CPU time. Two frames per external mark, at most four marks/run. */
typedef struct NvCommandBin {uint64_t calls,ticks,max;unsigned samples,get,param,submask;} NvCommandBin;
typedef struct NvNopShape {uint64_t zero_incrementing,zero_repeated,zero_tail,nonzero;
 uint64_t operations[32];unsigned max_remaining,first_nonzero_get,first_nonzero_param;} NvNopShape;
static struct {HANDLE event;FILE*out;unsigned active,frames,burst,random,timed,omitted;
 uint64_t begin,methods;NvCommandBin bin[2048];NvNopShape nop;} nv_command_probe;
/* Called only inside the already-active probe helper. No guest reads, clocks,
 * allocation or I/O. 'tail' is NOT necessarily a one-word packet: it may be
 * the last word of a repeated packet. Nonzero values must remain callbacks. */
static void nv_command_probe_shape(unsigned method,unsigned param,unsigned incrementing,unsigned remaining,unsigned get){
 if(method!=0x100)return;
 NvNopShape*n=&nv_command_probe.nop;
 if(param){if(!n->nonzero){n->first_nonzero_get=get;n->first_nonzero_param=param;}
  ++n->nonzero;++n->operations[param&31u];}
 else if(incrementing)++n->zero_incrementing;
 else if(remaining>1)++n->zero_repeated;
 else ++n->zero_tail;
 if(remaining>n->max_remaining)n->max_remaining=remaining;
}
static uint64_t nv_command_clock(void){LARGE_INTEGER q;QueryPerformanceCounter(&q);return q.QuadPart;}
static void nv_command_probe_init(void){
 const char*s=getenv("UCRR_COMMAND_PROBE");if(!s||strcmp(s,"1"))return;
 char name[96],path[128];snprintf(name,sizeof(name),"Local\\UCRR-Command-%lu",GetCurrentProcessId());
 snprintf(path,sizeof(path),"command-probe-%lu-%llu.tsv",GetCurrentProcessId(),(unsigned long long)GetTickCount64());
 nv_command_probe.out=fopen(path,"wx");if(!nv_command_probe.out)return;
 nv_command_probe.event=CreateEventA(NULL,FALSE,FALSE,name);
 if(!nv_command_probe.event){fclose(nv_command_probe.out);nv_command_probe.out=NULL;return;}
 LARGE_INTEGER f;QueryPerformanceFrequency(&f);
 fprintf(nv_command_probe.out,"SCHEMA\t2\tqpc_frequency=%llu\tscalar-only; calls exact, time sampled inclusive; batches unchanged\n",(unsigned long long)f.QuadPart);fflush(nv_command_probe.out);
 nv_command_probe.random=0x1926abcd;
}
static void nv_command_probe_boundary(unsigned frame,uint64_t methods){
 if(!nv_command_probe.event)return;
 if(nv_command_probe.active&&!--nv_command_probe.frames){
  uint64_t end=nv_command_clock();nv_command_probe.active=0;
  fprintf(nv_command_probe.out,"WINDOW\t%u\t%llu\t%llu\t%u\t%llu\t%u\t%u\n",nv_command_probe.burst,(unsigned long long)nv_command_probe.begin,(unsigned long long)end,frame,(unsigned long long)(methods-nv_command_probe.methods),nv_command_probe.timed,nv_command_probe.omitted);
  for(unsigned m=0;m<2048;m++){NvCommandBin*b=&nv_command_probe.bin[m];if(b->calls)fprintf(nv_command_probe.out,"CMD\t%u\t%04X\t%llu\t%u\t%llu\t%llu\t%08X\t%08X\t%02X\n",nv_command_probe.burst,m*4,(unsigned long long)b->calls,b->samples,(unsigned long long)b->ticks,(unsigned long long)b->max,b->get,b->param,b->submask);}
  NvNopShape*n=&nv_command_probe.nop;
  fprintf(nv_command_probe.out,"NOP_SHAPE\t%u\t%llu\t%llu\t%llu\t%llu\t%u\t%08X\t%08X\n",nv_command_probe.burst,(unsigned long long)n->zero_incrementing,(unsigned long long)n->zero_repeated,(unsigned long long)n->zero_tail,(unsigned long long)n->nonzero,n->max_remaining,n->first_nonzero_get,n->first_nonzero_param);
  for(unsigned op=0;op<32;op++)if(n->operations[op])fprintf(nv_command_probe.out,"NOP_OP\t%u\t%u\t%llu\n",nv_command_probe.burst,op,(unsigned long long)n->operations[op]);
  fprintf(nv_command_probe.out,"FLUSH_END\t%llu\n",(unsigned long long)nv_command_clock());fflush(nv_command_probe.out);
 }
 if(!nv_command_probe.active&&nv_command_probe.burst<4&&WaitForSingleObject(nv_command_probe.event,0)==WAIT_OBJECT_0){
  memset(nv_command_probe.bin,0,sizeof(nv_command_probe.bin));memset(&nv_command_probe.nop,0,sizeof(nv_command_probe.nop));nv_command_probe.timed=nv_command_probe.omitted=0;
  nv_command_probe.frames=2;++nv_command_probe.burst;nv_command_probe.methods=methods;
  nv_command_probe.begin=nv_command_clock();nv_command_probe.active=1;
 }
}
static uint64_t nv_command_probe_begin(unsigned method,unsigned sub){
 if(method>=8192||(method&3)){++nv_command_probe.omitted;return 0;}
 NvCommandBin*b=&nv_command_probe.bin[method/4];++b->calls;if(sub<8)b->submask|=1u<<sub;
 nv_command_probe.random=nv_command_probe.random*1664525u+1013904223u;
 if((nv_command_probe.random>>26)!=0)return 0;
 if(nv_command_probe.timed>=4096){++nv_command_probe.omitted;return 0;}
 ++nv_command_probe.timed;return nv_command_clock();
}
static void nv_command_probe_end(uint64_t start,unsigned method,unsigned get,unsigned param){
 if(!start)return;uint64_t end=nv_command_clock(),ticks=end>=start?end-start:0;
 NvCommandBin*b=&nv_command_probe.bin[method/4];++b->samples;b->ticks+=ticks;
 if(ticks>=b->max){b->max=ticks;b->get=get;b->param=param;}
}
#endif
