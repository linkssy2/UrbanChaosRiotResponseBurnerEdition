/* One diagnostic capture. Counts cover the whole window; fixed-size selection
 * reservoir and periodic projected-value sampling bound storage and work.
 * Guest ownership/invalidation is NOT certified by these observations. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include "recomp_types.h"
#include "guest_lighting_coverage.h"
extern size_t g_xbox_total_ram;
#ifndef UC_GL_SELECTION_CAP
#define UC_GL_SELECTION_CAP 4096u
#endif
#define UC_GL_PROJECTED_CAP 16u
typedef struct UCGLLane {
 uint64_t calls,positive_calls,declared_entries,entry_no_refill_span,max_entries,histogram[8];
 uint64_t uploads,refill_entries,range_reject,ordinary_nonalias_entries;
} UCGLLane;
typedef struct UCGLSelection {
 uint64_t sequence;
 uint32_t receiver,key,result,guest_stamp,sentinel,root;
} UCGLSelection;
typedef struct UCGLProjection { uint32_t words[32],mxcsr; } UCGLProjection;
static struct {
 int initialized,enabled,active,finished;
 DWORD owner;volatile LONG owner_errors;
 uint32_t depth,max_depth,pid,selection_stored,projected_stored,projected_next;
 uint64_t start_qpc,end_qpc,rng,traversal_calls,depth_errors;
 uint64_t palette_stack_reject,material_calls,projected_calls,projected_sampled,projected_range_reject;
 uint64_t projected_exact_hits,projected_adjacent_hits,projected_nonfinite;
 uint64_t selection_calls,selection_positive,selection_negative,selection_nodes,selection_range_reject;
 uint64_t record_write_failures;
 UCGLLane palette[3];
 UCGLProjection projected[UC_GL_PROJECTED_CAP],last_projected;
 UCGLSelection selections[UC_GL_SELECTION_CAP];
} uc_gl;
static void uc_gl_init(void){
 if(uc_gl.initialized)return;uc_gl.initialized=1;
#if UCRR_PLAY_ONLY
 uc_gl.enabled=0;
#else
 const char*s=getenv("UCRR_LIGHTING_LIST_PROBE");uc_gl.enabled=s&&!strcmp(s,"1");
#endif
}
static int uc_gl_owner(void){
 if(GetCurrentThreadId()==uc_gl.owner)return 1;
 InterlockedIncrement(&uc_gl.owner_errors);return 0;
}
static int uc_gl_active(void){return uc_gl.active&&uc_gl_owner();}
static const void*uc_gl_ram(uint32_t va,uint32_t bytes,uint32_t*physical){
 if(!bytes||(uint64_t)va+bytes>UINT64_C(0x100000000))return NULL;
 uint32_t first=recomp_normalize_xbox_va(va),last=recomp_normalize_xbox_va(va+bytes-1);
 if(first<0x10000u||(uint64_t)first+bytes>g_xbox_total_ram||(uint64_t)first+bytes-1!=last)return NULL;
 if(physical)*physical=first;return (const void*)(g_xbox_mem_offset+(uintptr_t)first);
}
static int uc_gl_read(uint32_t va,void*out,uint32_t bytes){const void*p=uc_gl_ram(va,bytes,NULL);if(!p)return 0;memcpy(out,p,bytes);return 1;}
static int uc_gl_overlap(uint32_t a,uint32_t n,uint32_t b,uint32_t m){return (uint64_t)a<b+(uint64_t)m&&(uint64_t)b<a+(uint64_t)n;}
static unsigned uc_gl_lane(uint32_t caller){return caller==0xBB1EBu||caller==0xBB262u?0:caller==0xBBA9Bu?1:2;}
void uc_guest_light_traversal_enter(void){if(!uc_gl_active())return;++uc_gl.traversal_calls;++uc_gl.depth;if(uc_gl.depth>uc_gl.max_depth)uc_gl.max_depth=uc_gl.depth;}
void uc_guest_light_traversal_leave(void){if(!uc_gl_active())return;if(uc_gl.depth)--uc_gl.depth;else ++uc_gl.depth_errors;}
int uc_guest_light_in_traversal(void){return uc_gl.active&&GetCurrentThreadId()==uc_gl.owner&&uc_gl.depth!=0;}
void uc_guest_light_palette_enter(uint32_t stack){
 if(!uc_gl_active())return;
 uint32_t args[2],count,cursor[2];
 if(!uc_gl_read(stack,args,8)){++uc_gl.palette_stack_reject;return;}
 UCGLLane*s=&uc_gl.palette[uc_gl_lane(args[0])];++s->calls;
 if(!uc_gl_read(args[1],&count,4)){++s->range_reject;return;}
 if((int32_t)count<=0){++s->histogram[0];return;}
 ++s->positive_calls;s->declared_entries+=count;if(count>s->max_entries)s->max_entries=count;
 unsigned bin=count==1?1:count<=4?2:count<=8?3:count<=16?4:count<=32?5:count<=64?6:7;++s->histogram[bin];
 uint64_t bytes=(uint64_t)count*60,last=(uint64_t)(count-1)*60;
 if(uc_gl_read(0x3F4240,cursor,8)&&bytes<=UINT32_MAX&&last+cursor[0]<cursor[1]&&uc_gl_ram(cursor[0],(uint32_t)bytes,NULL))++s->entry_no_refill_span;
}
void uc_guest_light_palette_upload(uint32_t stack,uint32_t source){
 if(!uc_gl_active())return;
 /* Hook immediately before PUSH return BABE4: literal12 is already pushed.
  * Original outer BABB0 return address is at this stack+16. */
 uint32_t caller,cursor[2],out,src,frame;
 if(stack>UINT32_MAX-20||!uc_gl_read(stack+16,&caller,4)){++uc_gl.palette_stack_reject;return;}
 UCGLLane*s=&uc_gl.palette[uc_gl_lane(caller)];++s->uploads;
 if(!uc_gl_read(0x3F4240,cursor,8)){++s->range_reject;return;}
 if(cursor[0]>=cursor[1]){++s->refill_entries;return;}
 if(stack<16||!uc_gl_ram(cursor[0],60,&out)||!uc_gl_ram(source,48,&src)||!uc_gl_ram(stack-16,48,&frame)){++s->range_reject;return;}
 if(!uc_gl_overlap(out,60,src,48)&&!uc_gl_overlap(out,60,frame,48)&&!uc_gl_overlap(src,48,frame,48)&&!uc_gl_overlap(out,60,0x3F4240,8))++s->ordinary_nonalias_entries;
}
void uc_guest_light_material_enter(void){if(uc_gl_active())++uc_gl.material_calls;}
void uc_guest_light_projected(uint32_t matrix,uint32_t projection){
 if(!uc_gl_active())return;++uc_gl.projected_calls;
 /* Whole-window cadence, with one early observation for rare branches. */
 if(uc_gl.projected_calls!=1&&(uc_gl.projected_calls&15u))return;
 UCGLProjection value;value.mxcsr=_mm_getcsr();
 if(!uc_gl_read(matrix,value.words,64)||!uc_gl_read(projection,value.words+16,64)){++uc_gl.projected_range_reject;return;}
 ++uc_gl.projected_sampled;
 for(unsigned i=0;i<32;i++)if((value.words[i]&0x7F800000u)==0x7F800000u){++uc_gl.projected_nonfinite;break;}
 if(uc_gl.projected_sampled>1&&!memcmp(&value,&uc_gl.last_projected,sizeof(value)))++uc_gl.projected_adjacent_hits;
 for(unsigned i=0;i<uc_gl.projected_stored;i++)if(!memcmp(&value,&uc_gl.projected[i],sizeof(value))){++uc_gl.projected_exact_hits;break;}
 uc_gl.last_projected=value;uc_gl.projected[uc_gl.projected_next]=value;uc_gl.projected_next=(uc_gl.projected_next+1)%UC_GL_PROJECTED_CAP;
 if(uc_gl.projected_stored<UC_GL_PROJECTED_CAP)++uc_gl.projected_stored;
}
void uc_guest_light_selection_node(void){if(uc_gl_active())++uc_gl.selection_nodes;}
void uc_guest_light_selection(uint32_t receiver,uint32_t stack_after,uint32_t result){
 if(!uc_gl_active())return;
 uint64_t sequence=++uc_gl.selection_calls;
 if(result&255u)++uc_gl.selection_positive;else ++uc_gl.selection_negative;
 /* Deterministic Algorithm R reservoir spans the entire window. Sequence
  * numbers allow offline ordering; missing rows are never implied negatives. */
 uint64_t slot=sequence-1;
 if(sequence>UC_GL_SELECTION_CAP){uc_gl.rng^=uc_gl.rng<<13;uc_gl.rng^=uc_gl.rng>>7;uc_gl.rng^=uc_gl.rng<<17;slot=uc_gl.rng%sequence;if(slot>=UC_GL_SELECTION_CAP)return;}
 UCGLSelection row={0};row.sequence=sequence;row.receiver=receiver;row.result=(result&255u)!=0;
 if(stack_after<4||receiver>UINT32_MAX-0x108||!uc_gl_read(stack_after-4,&row.key,4)||!uc_gl_read(0x51A8C0,&row.guest_stamp,4)||!uc_gl_read(receiver+0x104,&row.sentinel,4)||row.sentinel>UINT32_MAX-8||!uc_gl_read(row.sentinel+4,&row.root,4)){++uc_gl.selection_range_reject;return;}
 uc_gl.selections[slot]=row;if(uc_gl.selection_stored<UC_GL_SELECTION_CAP)++uc_gl.selection_stored;
}
static void uc_gl_write(uint64_t qpc){
 char path[128];snprintf(path,sizeof(path),"guest-light-coverage-%lu-%llu.json",(unsigned long)uc_gl.pid,(unsigned long long)uc_gl.start_qpc);
 FILE*f=fopen(path,"wx");if(!f){++uc_gl.record_write_failures;fprintf(stderr,"[GUEST-LIGHT-COVERAGE] cannot create %s\n",path);return;}
 fprintf(f,"{\"schema\":1,\"pid\":%lu,\"owner_tid\":%lu,\"start_qpc\":%llu,\"end_qpc\":%llu,\"owner_errors\":%ld,\"active_depth_at_end\":%u,\"depth_errors\":%llu,\"traversal_calls\":%llu,\"max_depth\":%u,\"palette_stack_reject\":%llu,\"palette\":[",
  (unsigned long)uc_gl.pid,(unsigned long)uc_gl.owner,(unsigned long long)uc_gl.start_qpc,(unsigned long long)qpc,(long)uc_gl.owner_errors,uc_gl.depth,(unsigned long long)uc_gl.depth_errors,(unsigned long long)uc_gl.traversal_calls,uc_gl.max_depth,(unsigned long long)uc_gl.palette_stack_reject);
 for(unsigned i=0;i<3;i++){
  UCGLLane*s=&uc_gl.palette[i];fprintf(f,"%s{\"caller_class\":\"%s\",\"calls\":%llu,\"positive_calls\":%llu,\"declared_entries\":%llu,\"max_entries\":%llu,\"entry_no_refill_span\":%llu,\"uploads\":%llu,\"refill_entries\":%llu,\"range_reject\":%llu,\"ordinary_nonalias_entries\":%llu,\"histogram\":[",i?",":"",i==0?"light":i==1?"base":"other",(unsigned long long)s->calls,(unsigned long long)s->positive_calls,(unsigned long long)s->declared_entries,(unsigned long long)s->max_entries,(unsigned long long)s->entry_no_refill_span,(unsigned long long)s->uploads,(unsigned long long)s->refill_entries,(unsigned long long)s->range_reject,(unsigned long long)s->ordinary_nonalias_entries);
  for(unsigned j=0;j<8;j++)fprintf(f,"%s%llu",j?",":"",(unsigned long long)s->histogram[j]);fprintf(f,"]}");
 }
 fprintf(f,"],\"material_calls\":%llu,\"projected_calls\":%llu,\"projected_sampled\":%llu,\"projected_range_reject\":%llu,\"projected_exact_hits\":%llu,\"projected_adjacent_hits\":%llu,\"projected_nonfinite\":%llu,\"projected_period\":16,\"projected_ring_capacity\":%u,\"selection_calls\":%llu,\"selection_positive\":%llu,\"selection_negative\":%llu,\"selection_nodes\":%llu,\"selection_range_reject\":%llu,\"selection_reservoir_capacity\":%u,\"selection_records\":[",
 (unsigned long long)uc_gl.material_calls,(unsigned long long)uc_gl.projected_calls,(unsigned long long)uc_gl.projected_sampled,(unsigned long long)uc_gl.projected_range_reject,(unsigned long long)uc_gl.projected_exact_hits,(unsigned long long)uc_gl.projected_adjacent_hits,(unsigned long long)uc_gl.projected_nonfinite,UC_GL_PROJECTED_CAP,(unsigned long long)uc_gl.selection_calls,(unsigned long long)uc_gl.selection_positive,(unsigned long long)uc_gl.selection_negative,(unsigned long long)uc_gl.selection_nodes,(unsigned long long)uc_gl.selection_range_reject,UC_GL_SELECTION_CAP);
 unsigned written=0;for(unsigned i=0;i<UC_GL_SELECTION_CAP;i++){UCGLSelection*r=&uc_gl.selections[i];if(!r->sequence)continue;fprintf(f,"%s[%llu,%u,%u,%u,%u,%u,%u]",written++?",":"",(unsigned long long)r->sequence,r->receiver,r->key,r->result,r->guest_stamp,r->sentinel,r->root);}
 fprintf(f,"]}\n");int failed=ferror(f)||fflush(f);if(fclose(f))failed=1;if(failed){++uc_gl.record_write_failures;fprintf(stderr,"[GUEST-LIGHT-COVERAGE] incomplete output %s\n",path);}
}
void uc_guest_light_coverage_boundary(int start,uint64_t qpc){
 uc_gl_init();if(!uc_gl.enabled||uc_gl.finished)return;
 if(start){if(uc_gl.active){uc_gl_owner();return;}uc_gl.owner=GetCurrentThreadId();uc_gl.pid=GetCurrentProcessId();uc_gl.start_qpc=qpc;uc_gl.rng=UINT64_C(0x9E3779B97F4A7C15);uc_gl.active=1;return;}
 if(!uc_gl.active||!uc_gl_owner())return;uc_gl.active=0;uc_gl.finished=1;uc_gl.end_qpc=qpc;uc_gl_write(qpc);
}
