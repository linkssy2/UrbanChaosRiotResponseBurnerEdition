#ifndef NV2A_DEEP_TRACE_H
#define NV2A_DEEP_TRACE_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>
/* Single decoder owner. Counts are exact in selected frames; command timings
 * sample 1/256 dispatch attempts, bounded to 2048. Inclusive timings overlap
 * submission timings. Never interpret their sum as exclusive CPU/GPU work. */
typedef struct NvDeepCommand {uint64_t calls,words,ticks,max;unsigned samples,declined,get,sub;} NvDeepCommand;
typedef struct NvDeepDraw {uint64_t ticks;unsigned get,method,vertices,route,mode,program,stage,target,blend,depth,raw_words,gpu,failed;unsigned tex[4],format[4],vp[4];} NvDeepDraw;
typedef struct NvDeepGroup {unsigned key[20],used,get;uint64_t calls,ticks,vertices;} NvDeepGroup;
typedef struct NvDeepTrace {
 unsigned enabled,active,burst,frame,random,timed,omitted,get,method;
 uint64_t (*clock)(void);double tick_ms;
 NvDeepCommand command[6][2048];
 uint64_t invalidations[2056],valid_clears[2056],dma_clears[2056];
 NvDeepDraw draws[16];unsigned kept;uint64_t draw_calls,vertices,route[4];
 NvDeepGroup groups[256];uint64_t group_omitted;
} NvDeepTrace;
extern NvDeepTrace nv_deep_trace;
typedef struct NvDeepToken {uint64_t start;unsigned method,route,words,get,sub,prior_get,prior_method,active,timed;} NvDeepToken;
static NvDeepToken nv_deep_begin(unsigned route,unsigned method,unsigned words,unsigned get,unsigned sub){
 NvDeepTrace*s=&nv_deep_trace;NvDeepToken t={0};if(!s->active)return t;
 t.active=1;t.prior_get=s->get;t.prior_method=s->method;s->get=get;s->method=method;
 t.route=route;t.method=method;t.words=words;t.get=get;t.sub=sub;
 if(route>=6||method>=8192||(method&3))return t;
 NvDeepCommand*c=&s->command[route][method/4];++c->calls;c->words+=words;
 s->random=s->random*1664525u+1013904223u;
 if((s->random>>24)==0){if(s->timed<2048){++s->timed;t.timed=1;t.start=s->clock();}else ++s->omitted;}
 return t;
}
static void nv_deep_end(NvDeepToken t,int accepted){
 if(!t.active)return;NvDeepTrace*s=&nv_deep_trace;
 if(t.route<6&&t.method<8192&&!(t.method&3)){
  NvDeepCommand*c=&s->command[t.route][t.method/4];if(!accepted)++c->declined;
  if(t.timed){uint64_t now=s->clock(),dt=now>=t.start?now-t.start:0;++c->samples;c->ticks+=dt;
   if(dt>=c->max){c->max=dt;c->get=t.get;c->sub=t.sub;}}
 }
 s->get=t.prior_get;s->method=t.prior_method;
}
static void nv_deep_invalidate(unsigned reason,unsigned valid,unsigned dma){
 NvDeepTrace*s=&nv_deep_trace;if(!s->active||reason>=2056)return;
 ++s->invalidations[reason];s->valid_clears[reason]+=!!valid;s->dma_clears[reason]+=!!dma;
}
static void nv_deep_draw(NvDeepDraw d){
 NvDeepTrace*s=&nv_deep_trace;if(!s->active)return;++s->draw_calls;s->vertices+=d.vertices;
 if(d.route<4)++s->route[d.route];
 unsigned key[20]={d.route,d.mode,d.program,d.stage,d.target,d.blend,d.depth,d.gpu};
 memcpy(key+8,d.tex,16);memcpy(key+12,d.format,16);memcpy(key+16,d.vp,16);
 unsigned h=2166136261u;for(unsigned i=0;i<20;++i)h=(h^key[i])*16777619u;
 unsigned found=0;
 for(unsigned n=0;n<256;++n){NvDeepGroup*g=&s->groups[(h+n)&255];
  if(!g->used){g->used=1;memcpy(g->key,key,sizeof(key));g->get=d.get;}
  if(!memcmp(g->key,key,sizeof(key))){++g->calls;g->ticks+=d.ticks;g->vertices+=d.vertices;found=1;break;}}
 if(!found)++s->group_omitted;
 unsigned slot=s->kept;if(slot<16)++s->kept;else{slot=0;for(unsigned i=1;i<16;++i)if(s->draws[i].ticks<s->draws[slot].ticks)slot=i;if(d.ticks<=s->draws[slot].ticks)return;}
 s->draws[slot]=d;
}
static void nv_deep_start(unsigned burst,unsigned frame){
 NvDeepTrace*s=&nv_deep_trace;if(!s->enabled)return;
 unsigned random=s->random?s->random:0x8317265u;double scale=s->tick_ms;uint64_t(*clock)(void)=s->clock;
 memset(s,0,sizeof(*s));s->enabled=s->active=1;s->burst=burst;s->frame=frame;s->random=random;s->tick_ms=scale;s->clock=clock;
}
static void nv_deep_report(void){
 NvDeepTrace*s=&nv_deep_trace;if(!s->active)return;s->active=0;
 fprintf(stderr,"[DEEP-FRAME] burst=%u frame=%u timed=%u omitted=%u draws=%llu vertices=%llu routes=%llu,%llu,%llu,%llu (instrumented; command times sampled/inclusive; submit times inclusive; NOT FPS)\n",s->burst,s->frame,s->timed,s->omitted,(unsigned long long)s->draw_calls,(unsigned long long)s->vertices,(unsigned long long)s->route[0],(unsigned long long)s->route[1],(unsigned long long)s->route[2],(unsigned long long)s->route[3]);
 /* All encountered command bins: finite 6*2048 bound, no hot-path I/O. */
 for(unsigned r=0;r<6;++r)for(unsigned m=0;m<2048;++m){NvDeepCommand*c=&s->command[r][m];if(c->calls)
  fprintf(stderr,"[DEEP-CMD] burst=%u frame=%u route=%u method=%04X calls=%llu words=%llu sampled=%u sampled_ms=%.6f max_ms=%.6f declined=%u max_get=%08X sub=%u\n",s->burst,s->frame,r,m*4,(unsigned long long)c->calls,(unsigned long long)c->words,c->samples,c->ticks*s->tick_ms,c->max*s->tick_ms,c->declined,c->get,c->sub);}
 for(unsigned i=0;i<2056;++i)if(s->invalidations[i])fprintf(stderr,"[DEEP-RESET] burst=%u frame=%u reason=%u calls=%llu valid=%llu dma=%llu\n",s->burst,s->frame,i,(unsigned long long)s->invalidations[i],(unsigned long long)s->valid_clears[i],(unsigned long long)s->dma_clears[i]);
 for(unsigned i=0;i<256;++i){NvDeepGroup*g=&s->groups[i];if(!g->used)continue;
  fprintf(stderr,"[DEEP-GROUP] burst=%u frame=%u calls=%llu submit_ms=%.6f vertices=%llu first_get=%08X key=",s->burst,s->frame,(unsigned long long)g->calls,g->ticks*s->tick_ms,(unsigned long long)g->vertices,g->get);
  for(unsigned j=0;j<20;++j)fprintf(stderr,"%s%08X",j?",":"",g->key[j]);fputc('\n',stderr);}
 fprintf(stderr,"[DEEP-GROUP-COVERAGE] burst=%u frame=%u omitted_draws=%llu\n",s->burst,s->frame,(unsigned long long)s->group_omitted);
 for(unsigned i=0;i<s->kept;++i){NvDeepDraw*d=&s->draws[i];fprintf(stderr,"[DEEP-DRAW] burst=%u frame=%u get=%08X method=%04X submit_ms=%.6f vertices=%u route=%u mode=%u program=%u stage=%08X target=%08X blend=%u depth=%u raw_words=%u gpu_indexed=%u indexed_failed=%u tex=%08X,%08X,%08X,%08X formats=%08X,%08X,%08X,%08X vp=%08X,%08X,%08X,%08X\n",s->burst,s->frame,d->get,d->method,d->ticks*s->tick_ms,d->vertices,d->route,d->mode,d->program,d->stage,d->target,d->blend,d->depth,d->raw_words,d->gpu,d->failed,d->tex[0],d->tex[1],d->tex[2],d->tex[3],d->format[0],d->format[1],d->format[2],d->format[3],d->vp[0],d->vp[1],d->vp[2],d->vp[3]);}
 fprintf(stderr,"[DEEP-END] burst=%u frame=%u\n",s->burst,s->frame);
}
#endif
