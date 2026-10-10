#ifndef NV2A_REMOVAL_DIAGNOSTIC_H
#define NV2A_REMOVAL_DIAGNOSTIC_H
#include "nv2a_removal_policy.h"
/* Renderer owner consumes key requests at a draw boundary. No file IO per
 * vertex; counters and at most 64 descriptor witnesses per process. */
static volatile LONG nv_removal_requests;
extern volatile LONG ucrr_removal_auto_state;
extern volatile LONG ucrr_removal_capture_phase;
static struct {int phase;uint64_t routes[4],flags[32];} nv_removal_census={-1};
static void nv_removal_census_flush(FILE*f){
 if(!f||nv_removal_census.phase<0||nv_removal_census.phase>2)return;
 fprintf(f,"census phase=%d routes=%llu,%llu,%llu,%llu\n",nv_removal_census.phase,
  (unsigned long long)nv_removal_census.routes[0],(unsigned long long)nv_removal_census.routes[1],
  (unsigned long long)nv_removal_census.routes[2],(unsigned long long)nv_removal_census.routes[3]);
 for(unsigned i=0;i<32;++i)if(nv_removal_census.flags[i])fprintf(f,"indexed_flags phase=%d flags=%u draws=%llu\n",
  nv_removal_census.phase,i,(unsigned long long)nv_removal_census.flags[i]);
 fflush(f);
}
static struct {
 int initialized,enabled,active,geometry,blended,late,late_tag,prehost,preupload,pretexture,posttexture,presampler,poststage0;uint32_t mask,value;
 uint64_t seen,eligible,selected,removed,vertices,rejected;
 uint64_t host_seen,host_removed,host_elements,host_query_retained;
 unsigned witness_count;uint32_t witnesses[64][4];FILE*file;
} nv_removal;
static void nv_removal_set_late_tag(int tag,void (*flush)(void)){
 if(tag!=nv_removal.late_tag){flush();nv_removal.late_tag=tag;}
}
/* Called only at the final, validated GPU draw boundary. The tag is owned by
 * the queued group, not recomputed from mutable current guest state. */
static int nv_removal_host_draw(unsigned elements,int query){
 if(!nv_removal.enabled||!nv_removal.late)return 0;
 ++nv_removal.host_seen;
 if(!nv_removal.late_tag)return 0;
 if(query){++nv_removal.host_query_retained;return 0;}
 ++nv_removal.host_removed;nv_removal.host_elements+=elements;return 1;
}
static void nv_removal_observe(int indexed,int inline_array,int immediate,int blend,int stencil,int query,int failed,unsigned mode){
 if(!nv_removal.enabled||!nv_removal.geometry)return;
 int phase=(int)InterlockedCompareExchange(&ucrr_removal_capture_phase,-1,-1);
 if(phase!=nv_removal_census.phase){
  nv_removal_census_flush(nv_removal.file);memset(&nv_removal_census,0,sizeof(nv_removal_census));nv_removal_census.phase=phase;
 }
 if(phase<0||phase>2)return;
 unsigned route=indexed?0:inline_array?1:immediate?2:3;++nv_removal_census.routes[route];
 if(indexed){unsigned flags=!!blend | (!!stencil<<1) | (!!query<<2) | (!!failed<<3) | ((mode!=5&&mode!=6)<<4);
  ++nv_removal_census.flags[flags];}
}
static void nv_removal_report(const char*event){
 if(!nv_removal.file)return;
 LARGE_INTEGER q;QueryPerformanceCounter(&q);
 fprintf(nv_removal.file,"event=%s qpc=%lld active=%d seen=%llu eligible=%llu selected=%llu removed=%llu vertices=%llu rejected=%llu mask=%08X value=%08X\n",
 event,q.QuadPart,nv_removal.active,(unsigned long long)nv_removal.seen,
 (unsigned long long)nv_removal.eligible,(unsigned long long)nv_removal.selected,
 (unsigned long long)nv_removal.removed,(unsigned long long)nv_removal.vertices,
 (unsigned long long)nv_removal.rejected,nv_removal.mask,nv_removal.value);
 fprintf(nv_removal.file,"host_boundary qpc=%lld late=%d seen=%llu removed=%llu elements=%llu query_retained=%llu\n",q.QuadPart,nv_removal.late,
  (unsigned long long)nv_removal.host_seen,(unsigned long long)nv_removal.host_removed,
  (unsigned long long)nv_removal.host_elements,(unsigned long long)nv_removal.host_query_retained);
 fprintf(nv_removal.file,"host_stage prehost=%d preupload_direct_and_raw=%d pretexture=%d posttexture=%d presampler=%d poststage0=%d\n",nv_removal.prehost,nv_removal.preupload,nv_removal.pretexture,nv_removal.posttexture,nv_removal.presampler,nv_removal.poststage0);fflush(nv_removal.file);
}
static void nv_removal_init(void){
 if(nv_removal.initialized)return;nv_removal.initialized=1;
 const char*s=getenv("UCRR_REMOVAL_INLINE");nv_removal.enabled=s&&!strcmp(s,"1");
 s=getenv("UCRR_REMOVAL_GEOMETRY");nv_removal.geometry=s&&!strcmp(s,"1");
 s=getenv("UCRR_REMOVAL_BLENDED");nv_removal.blended=s&&!strcmp(s,"1");
 s=getenv("UCRR_REMOVAL_LATE");nv_removal.late=s&&!strcmp(s,"1")&&nv_removal.geometry&&nv_removal.blended;
 s=getenv("UCRR_REMOVAL_PREHOST");nv_removal.prehost=s&&!strcmp(s,"1")&&nv_removal.late;
 s=getenv("UCRR_REMOVAL_PREUPLOAD");nv_removal.preupload=s&&!strcmp(s,"1")&&nv_removal.late;
 s=getenv("UCRR_REMOVAL_PRETEXTURE");nv_removal.pretexture=s&&!strcmp(s,"1")&&nv_removal.late&&!nv_removal.prehost&&!nv_removal.preupload;
 s=getenv("UCRR_REMOVAL_POSTTEXTURE");nv_removal.posttexture=s&&!strcmp(s,"1")&&nv_removal.late&&!nv_removal.prehost&&!nv_removal.preupload&&!nv_removal.pretexture;
 s=getenv("UCRR_REMOVAL_PRESAMPLER");nv_removal.presampler=s&&!strcmp(s,"1")&&nv_removal.posttexture;
 s=getenv("UCRR_REMOVAL_POSTSTAGE0");nv_removal.poststage0=s&&!strcmp(s,"1")&&nv_removal.posttexture&&!nv_removal.presampler;
 if(!nv_removal.enabled)return;
 const char*m=getenv("UCRR_REMOVAL_MASK"),*v=getenv("UCRR_REMOVAL_VALUE");char*end;
 if(m){nv_removal.mask=strtoul(m,&end,0);if(!*m||*end)nv_removal.enabled=0;}
 if(v){nv_removal.value=strtoul(v,&end,0);if(!*v||*end)nv_removal.enabled=0;}
 if(nv_removal.value&~nv_removal.mask)nv_removal.enabled=0;
 char path[96];snprintf(path,sizeof(path),"removal-inline-%lu-%llu.log",GetCurrentProcessId(),(unsigned long long)GetTickCount64());
 nv_removal.file=fopen(path,"wx");
 if(!nv_removal.file)nv_removal.enabled=0; /* No unrecorded removal. */
 nv_removal_report(nv_removal.enabled?"armed-default-normal":"invalid-disabled");
}
void pgraph_d3d11_removal_toggle(void){InterlockedIncrement(&nv_removal_requests);}
static int nv_removal_wants_inline(int route,int blend,int depth,int stencil,int query,uint32_t shader,
 uint32_t offset,uint32_t format,uint32_t program,uint32_t mode){
 nv_removal_init();if(!nv_removal.enabled)return 0;
 LONG requests=InterlockedExchange(&nv_removal_requests,0);
 if(requests&1){nv_removal.active=!nv_removal.active;nv_removal_report("toggle");}
 if(nv_removal.geometry){
  LONG automatic=InterlockedCompareExchange(&ucrr_removal_auto_state,-1,-1);
  if(automatic>=0&&automatic!=nv_removal.active){nv_removal.active=automatic;nv_removal_report("toggle");}
 }
 ++nv_removal.seen;
 if(nv_removal.geometry ? route!=2 : !nv_removal_eligible(route,blend,depth,stencil,query,shader))return 0;
 ++nv_removal.eligible;
 uint32_t key=nv_removal_bucket(offset,format,program,mode);
 if(!nv_removal_selected(key,nv_removal.mask,nv_removal.value))return 0;
 ++nv_removal.selected;
 uint32_t desc[4]={offset,format,program,mode};unsigned i;
 for(i=0;i<nv_removal.witness_count;++i)if(!memcmp(desc,nv_removal.witnesses[i],sizeof(desc)))break;
 if(i==nv_removal.witness_count&&i<64){memcpy(nv_removal.witnesses[i],desc,sizeof(desc));++nv_removal.witness_count;
  fprintf(nv_removal.file,"resource bucket=%08X offset=%08X format=%08X program=%u mode=%u\n",key,offset,format,program,mode);}
 return nv_removal.active;
}
static void nv_removal_shutdown(void){if(nv_removal.file){nv_removal_census_flush(nv_removal.file);nv_removal_census.phase=-1;nv_removal_report("shutdown");fclose(nv_removal.file);nv_removal.file=NULL;}}
#endif
