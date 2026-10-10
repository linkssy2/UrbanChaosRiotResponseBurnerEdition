#ifndef D3D8_FRAME_CAPTURE_H
#define D3D8_FRAME_CAPTURE_H
/* Opt-in, single presentation-thread owner. No per-frame disk writes or ETW.
 * Records host Present-return intervals, NOT GPU/display latency or simulation FPS.
 * One 24-second recording per process (60 for self-timed geometry removal);
 * second signal marks look-away. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#ifndef UC_RENDER_CAPTURE_BOUNDARY
#define UC_RENDER_CAPTURE_BOUNDARY(start,path) ((void)0)
#endif
#define UCRR_LEAF_COVERAGE_DEFINE
#include "d3d8_leaf_coverage.h"
#define UCRR_BATCH_COVERAGE_DEFINE
#include "d3d8_batch_coverage.h"
#define UCRR_HOST_UPLOAD_COVERAGE_DEFINE
#include "d3d8_host_upload_coverage.h"
#define UC_TEXTURE_REASONS_DEFINE
#include "../nv2a/nv2a_texture_reasons.h"
#define UC_INLINE_GPU_COVERAGE_DEFINE
#include "../nv2a/nv2a_inline_gpu_coverage.h"
#if UCRR_PLAY_ONLY
/* Presentation/FPS remains live; recording cannot create events, poll keys,
 * collect counters or publish automatic feature-removal state in this build. */
volatile LONG ucrr_removal_auto_state=-1;
volatile LONG ucrr_removal_capture_phase=-1;
void uc_frame_inventory_marker(void) {}
static void uc_frame_present(HRESULT hr) {(void)hr;}
#else
#if defined(UCRR_CUSTOM_LIGHTING)
extern void uc_custom_lighting_capture_boundary(int start, uint64_t qpc);
extern void uc_custom_lighting_host_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_LIGHT_RESTORATION)
extern void uc_light_restoration_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_PARTICLE_RESTORATION)
extern void uc_particle_restoration_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_DECAL_RESTORATION)
extern void uc_decal_restoration_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_DECAL_REMOVAL)
extern void uc_decal_removal_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_SPECULAR_REMOVAL)
extern void uc_specular_removal_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_GLOW_REMOVAL)
extern void uc_glow_removal_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_LIGHT_REMOVAL)
extern void uc_light_removal_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_SHADOW_REMOVAL)
extern void uc_shadow_removal_capture_boundary(int start, uint64_t qpc);
#endif
#if defined(UCRR_FEATURE_REMOVAL)
extern void uc_feature_capture_boundary(int start, uint64_t qpc);
#endif
typedef struct { LONGLONG qpc; double ms; unsigned phase; HRESULT hr; LONG64 leaf[5],batch[UC_BATCH_COUNT]; uint64_t host_upload[NV_HOST_UPLOAD_COUNT]; unsigned host_overflow,host_owner_error; } UcFrameRow;
/* Presentation and decoder are the same owner; interlocked access also makes
 * the diagnostic handoff explicit. -1 leaves manual F10 behavior unchanged. */
volatile LONG ucrr_removal_auto_state=-1;
volatile LONG ucrr_removal_capture_phase=-1;
static struct {
    int init,enabled,active,done,coverage,batch_coverage,batch_policy,strip_policy,retain_policy;
    int arena_policy,bounds_policy,equal_state_policy,copy_recovery_policy,index_cache_policy,host_upload_coverage,prepared_raw_policy; HANDLE event; LARGE_INTEGER freq;
    LONGLONG start,last; unsigned phase,count,overflow,seconds; DWORD owner;
    wchar_t path[MAX_PATH]; UcFrameRow rows[8192];
} uc_frames;
/* Explicit diagnostic trigger. Does not synthesize input or change guest state.
 * The normal recorder still owns timing, counters and the completion sentinel. */
void uc_frame_inventory_marker(void){
    static int mode=-1,signalled;
    if(mode<0){const char*v=getenv("UCRR_CAPTURE_FIRST_HUD");mode=v&&!strcmp(v,"1");}
    if(!mode||signalled||!uc_frames.enabled||!uc_frames.event||uc_frames.done||
       GetCurrentThreadId()!=uc_frames.owner)return;
    if(SetEvent(uc_frames.event))signalled=1;
}
static int uc_frame_write(HANDLE f,const void*data,DWORD size){
    DWORD written=0;return WriteFile(f,data,size,&written,NULL)&&written==size;
}
static int uc_inline_gpu_save(void){
    wchar_t path[MAX_PATH];swprintf(path,MAX_PATH,L"%ls.inline-gpu.csv",uc_frames.path);
    HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return 0;
    const char*head="Schema,ProcessID,OwnerThread,Enabled,Attempts,Admitted,Vertices,GpuDraws,GpuVertices,UploadFailures,CpuFallbacks,DrawFailures,IndexedFirstGpu,IndexedFirstCpu,IndexedFallbackCpu,PackedFirstGpu,ImmediateCaptured,ImmediateMaterialized,ImmediateDraws,ImmediateVertices,ImmediateFailed,EffectMemoAttempts,EffectMemoHits,EffectCullQuads,EffectCulledQuads,CompactDraws,CompactParticles,Overflow,GpuFirstEnabled,PackedFirstEnabled,ImmediateEnabled\n";
    char line[1024];const char*enabled=getenv("UCRR_INLINE_GPU_EXPERIMENT");
    int n=snprintf(line,sizeof(line),"5,%lu,%lu,%u",GetCurrentProcessId(),uc_frames.owner,enabled&&!strcmp(enabled,"1"));
    for(unsigned i=0;i<UC_INLINE_COUNT;i++)n+=snprintf(line+n,sizeof(line)-n,",%llu",(unsigned long long)uc_inline_gpu_counts[i]);
    const char*first=getenv("UCRR_GPU_FIRST_VERTEX_EXPERIMENT");
    const char*packed=getenv("UCRR_PACKED_FIRST_GPU_EXPERIMENT");
    const char*immediate=getenv("UCRR_IMMEDIATE_GPU_EXPERIMENT");
    n+=snprintf(line+n,sizeof(line)-n,",%u,%u,%u,%u\n",uc_inline_gpu_overflow,first&&!strcmp(first,"1"),packed&&!strcmp(packed,"1"),immediate&&!strcmp(immediate,"1"));
    int ok=n>0&&(size_t)n<sizeof(line)&&uc_frame_write(f,head,(DWORD)strlen(head))&&uc_frame_write(f,line,n);
    if(ok)ok=FlushFileBuffers(f)!=0;CloseHandle(f);return ok;
}
#ifdef UCRR_TEXTURE_REASON_CAPTURE
static int uc_texture_reason_save(void){
 static const char*names[]={"small","warmup","untracked","dirty","disabled","copy_mismatch","exact_certificate"};
 wchar_t path[MAX_PATH];swprintf(path,MAX_PATH,L"%ls.textures.csv",uc_frames.path);
 HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
 if(f==INVALID_HANDLE_VALUE)return 0;
 const char*head="Schema,Phase,Reason,SourceStatus,PaletteStatus,Calls,Equal,SourceBytesRequested,PaletteBytesRequested,Overflow,OwnerError\n";
 int ok=uc_frame_write(f,head,(DWORD)strlen(head));char line[512];
 for(unsigned p=0;p<2&&ok;p++)for(unsigned r=0;r<UC_TEX_REASON_COUNT&&ok;r++)
 for(unsigned s=0;s<UC_TEX_STATUS_COUNT&&ok;s++)for(unsigned t=0;t<UC_TEX_STATUS_COUNT&&ok;t++){
  UcTextureReasonCount*c=&uc_texture_reasons.counts[p][r][s][t];if(!c->calls)continue;
  int n=snprintf(line,sizeof(line),"1,%u,%s,%u,%u,%llu,%llu,%llu,%llu,%u,%u\n",p,names[r],s,t,
   (unsigned long long)c->calls,(unsigned long long)c->equal,(unsigned long long)c->source_bytes,
   (unsigned long long)c->palette_bytes,uc_texture_reasons.overflow,!!(uc_texture_reason_status&2));
  ok=n>0&&n<sizeof(line)&&uc_frame_write(f,line,n);
 }
 if(ok)ok=FlushFileBuffers(f)!=0;CloseHandle(f);return ok;
}
#endif
static void uc_frame_finish(void){
    if(!uc_frames.active||uc_frames.done)return;
    /* atexit may run on a different guest thread after an abnormal exit.
     * Do not publish a supposedly complete owner-thread snapshot in that case. */
    if(uc_frames.owner&&GetCurrentThreadId()!=uc_frames.owner)return;
    if(uc_frames.seconds==60)InterlockedExchange(&ucrr_removal_capture_phase,3);
    InterlockedExchange(&ucrr_leaf_coverage_active,0);
    InterlockedExchange(&ucrr_batch_coverage_active,0);
    uc_host_upload_stop();
#ifdef UCRR_TEXTURE_REASON_CAPTURE
    uc_texture_reason_stop();
#endif
    unsigned final_owner_error=uc_host_upload_owner_error();
    uc_inline_gpu_capture_thread=0;
    uc_frames.done=1;uc_frames.active=0;
    UC_RENDER_CAPTURE_BOUNDARY(0,uc_frames.path);
#if defined(UCRR_CUSTOM_LIGHTING)
    if(uc_frames.count){
        uc_custom_lighting_capture_boundary(0,(uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
        uc_custom_lighting_host_capture_boundary(0,(uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
    }
#endif
#if defined(UCRR_LIGHT_RESTORATION)
    if (uc_frames.count) uc_light_restoration_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_PARTICLE_RESTORATION)
    if (uc_frames.count) uc_particle_restoration_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_DECAL_RESTORATION)
    if (uc_frames.count) uc_decal_restoration_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_DECAL_REMOVAL)
    if (uc_frames.count) uc_decal_removal_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_SPECULAR_REMOVAL)
    if (uc_frames.count) uc_specular_removal_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_GLOW_REMOVAL)
    if (uc_frames.count) uc_glow_removal_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_LIGHT_REMOVAL)
    if (uc_frames.count) uc_light_removal_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_SHADOW_REMOVAL)
    if (uc_frames.count) uc_shadow_removal_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
#if defined(UCRR_FEATURE_REMOVAL)
    if (uc_frames.count) uc_feature_capture_boundary(0, (uint64_t)uc_frames.rows[uc_frames.count-1].qpc);
#endif
    HANDLE f=CreateFileW(uc_frames.path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return;
    char line[2048];
    const char *head="ProcessID,QPC,FrameTime,Phase,PresentResult,Overflow,LeafNative,LeafRangeFallback,LeafNaNFallback,LeafInfFallback,LeafDisabled,CoverageEnabled,BatchCoverageEnabled,RawNOPPolicy";
    if(!uc_frame_write(f,head,(DWORD)strlen(head)))goto save_failed;
    for(unsigned k=0;k<UC_BATCH_COUNT;k++){int n=snprintf(line,sizeof(line),",%s",uc_batch_names[k]);if(!uc_frame_write(f,line,n))goto save_failed;}
    const char *policies=",RawStripPolicy,RawRetainPolicy,RawArenaPolicy,RawBoundsPolicy,EqualStatePolicy,CopyRecoveryPolicy,StripIndexCachePolicy,HostUploadCoverage,HostUploadSchema,HostUploadOverflow,HostUploadOwnerError,PreparedRawPolicy";
    if(!uc_frame_write(f,policies,(DWORD)strlen(policies)))goto save_failed;
    for(unsigned k=0;k<NV_HOST_UPLOAD_COUNT;k++){int n=snprintf(line,sizeof(line),",%s",nv_host_upload_names[k]);if(!uc_frame_write(f,line,n))goto save_failed;}
    if(!uc_frame_write(f,"\n",1))goto save_failed;
    for(unsigned i=0;i<uc_frames.count;i++){
        UcFrameRow *r=&uc_frames.rows[i];
        int n=snprintf(line,sizeof(line),"%lu,%lld,%.9f,%u,%ld,%u,%lld,%lld,%lld,%lld,%lld,%d,%d,%d",GetCurrentProcessId(),r->qpc,r->ms,r->phase,(long)r->hr,uc_frames.overflow,r->leaf[0],r->leaf[1],r->leaf[2],r->leaf[3],r->leaf[4],uc_frames.coverage,uc_frames.batch_coverage,uc_frames.batch_policy);
        for(unsigned k=0;k<UC_BATCH_COUNT;k++)n+=snprintf(line+n,sizeof(line)-n,",%lld",r->batch[k]);
        n+=snprintf(line+n,sizeof(line)-n,",%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%d",uc_frames.strip_policy,uc_frames.retain_policy,
            uc_frames.arena_policy,uc_frames.bounds_policy,uc_frames.equal_state_policy,uc_frames.copy_recovery_policy,uc_frames.index_cache_policy,
            uc_frames.host_upload_coverage,NV_HOST_UPLOAD_SCHEMA,r->host_overflow,r->host_owner_error|final_owner_error,uc_frames.prepared_raw_policy);
        if(n<0||(size_t)n>=sizeof(line)||!uc_frame_write(f,line,n))goto save_failed;
        /* Separate bounded writes after capture, never grow one unbounded row. */
        for(unsigned k=0;k<NV_HOST_UPLOAD_COUNT;k++){n=snprintf(line,sizeof(line),",%llu",(unsigned long long)r->host_upload[k]);if(!uc_frame_write(f,line,n))goto save_failed;}
        if(!uc_frame_write(f,"\n",1))goto save_failed;
    }
    if(!FlushFileBuffers(f))goto save_failed;
    CloseHandle(f);
    if(!uc_inline_gpu_save())return;
#ifdef UCRR_TEXTURE_REASON_CAPTURE
    if(!uc_texture_reason_save())return;
#endif
    /* Sentinel is published only after CSV has been closed completely. */
    wchar_t ready[MAX_PATH];swprintf(ready,MAX_PATH,L"%ls.done",uc_frames.path);
    f=CreateFileW(ready,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);
    return;
save_failed:
    CloseHandle(f); /* A partial CSV never receives a completion sentinel. */
}
static void uc_frame_present(HRESULT hr){
    if(!uc_frames.init){
        uc_frames.init=1;const char *v=getenv("UCRR_FRAME_CAPTURE");
        uc_frames.enabled=v&&!strcmp(v,"1");if(!uc_frames.enabled)return;
        v=getenv("UCRR_REMOVAL_GEOMETRY");
        uc_frames.seconds=v&&!strcmp(v,"1")?60:24;
        v=getenv("UCRR_LEAF_COVERAGE");uc_frames.coverage=v&&!strcmp(v,"1");
        v=getenv("UCRR_BATCH_COVERAGE");uc_frames.batch_coverage=v&&!strcmp(v,"1");
        v=getenv("UCRR_RAW_NOP_BATCH");uc_frames.batch_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_RAW_STRIP_BATCH");uc_frames.strip_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_RAW_RETAIN_BATCH");uc_frames.retain_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_RAW_ARENA_TRANSFER");uc_frames.arena_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_RAW_BOUNDS_ONCE");uc_frames.bounds_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_EQUAL_STATE_BATCH");uc_frames.equal_state_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_TEXTURE_COPY_RECOVERY");uc_frames.copy_recovery_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_STRIP_INDEX_CACHE");uc_frames.index_cache_policy=v&&!strcmp(v,"1");
        v=getenv("UCRR_HOST_UPLOAD_COVERAGE");uc_frames.host_upload_coverage=v&&!strcmp(v,"1");
        v=getenv("UCRR_PREPARED_RAW_COMBINER");uc_frames.prepared_raw_policy=v&&!strcmp(v,"1");
        uc_frames.owner=GetCurrentThreadId();QueryPerformanceFrequency(&uc_frames.freq);
        wchar_t name[80];swprintf(name,80,L"Local\\UCRR-Frames-%lu",GetCurrentProcessId());
        uc_frames.event=CreateEventW(NULL,FALSE,FALSE,name);if(!uc_frames.event){uc_frames.enabled=0;return;}
        DWORD n=GetModuleFileNameW(NULL,uc_frames.path,MAX_PATH);
        wchar_t *slash=wcsrchr(uc_frames.path,L'\\');
        if(!n||n>=MAX_PATH||!slash||(slash-uc_frames.path)>MAX_PATH-85){uc_frames.enabled=0;return;}
        FILETIME born,exit,kernel,user;GetProcessTimes(GetCurrentProcess(),&born,&exit,&kernel,&user);
        ULARGE_INTEGER id;id.LowPart=born.dwLowDateTime;id.HighPart=born.dwHighDateTime;
        swprintf(slash+1,MAX_PATH-(slash+1-uc_frames.path),L"frame-capture-%lu-%llu.csv",GetCurrentProcessId(),id.QuadPart);
        atexit(uc_frame_finish);
    }
    if(!uc_frames.enabled||uc_frames.done||GetCurrentThreadId()!=uc_frames.owner)return;
    LARGE_INTEGER now;QueryPerformanceCounter(&now);
    if(WaitForSingleObject(uc_frames.event,0)==WAIT_OBJECT_0){
        if(!uc_frames.active){
            uc_frames.active=1;uc_frames.start=uc_frames.last=now.QuadPart;
            UC_RENDER_CAPTURE_BOUNDARY(1,uc_frames.path);
#if defined(UCRR_CUSTOM_LIGHTING)
            uc_custom_lighting_capture_boundary(1,(uint64_t)now.QuadPart);
            uc_custom_lighting_host_capture_boundary(1,(uint64_t)now.QuadPart);
#endif
#if defined(UCRR_LIGHT_RESTORATION)
            uc_light_restoration_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_PARTICLE_RESTORATION)
            uc_particle_restoration_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_DECAL_RESTORATION)
            uc_decal_restoration_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_DECAL_REMOVAL)
            uc_decal_removal_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_SPECULAR_REMOVAL)
            uc_specular_removal_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_GLOW_REMOVAL)
            uc_glow_removal_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_LIGHT_REMOVAL)
            uc_light_removal_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_SHADOW_REMOVAL)
            uc_shadow_removal_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
#if defined(UCRR_FEATURE_REMOVAL)
            uc_feature_capture_boundary(1, (uint64_t)now.QuadPart);
#endif
            if(uc_frames.seconds==60){InterlockedExchange(&ucrr_removal_auto_state,0);InterlockedExchange(&ucrr_removal_capture_phase,0);}
            InterlockedExchange(&ucrr_leaf_coverage_active,uc_frames.coverage);
            for(unsigned i=0;i<UC_BATCH_COUNT;i++)InterlockedExchange64(&ucrr_batch_coverage_counts[i],0);
            InterlockedExchange(&ucrr_batch_coverage_active,uc_frames.batch_coverage);
            uc_host_upload_begin(uc_frames.host_upload_coverage);
            memset(uc_inline_gpu_counts,0,sizeof(uc_inline_gpu_counts));
            uc_inline_gpu_overflow=0;uc_inline_gpu_capture_thread=1;
#ifdef UCRR_TEXTURE_REASON_CAPTURE
            uc_texture_reason_begin(1);
#endif
            return;
        }
        uc_frames.phase=1;
#ifdef UCRR_TEXTURE_REASON_CAPTURE
        uc_texture_reasons.phase=1;
#endif
    }
    if(!uc_frames.active)return;
    if(uc_frames.seconds==60){
        LONGLONG elapsed=now.QuadPart-uc_frames.start;
        LONG removal=elapsed>=uc_frames.freq.QuadPart*20&&elapsed<uc_frames.freq.QuadPart*40;
        InterlockedExchange(&ucrr_removal_auto_state,removal);
        InterlockedExchange(&ucrr_removal_capture_phase,elapsed<uc_frames.freq.QuadPart*20?0:elapsed<uc_frames.freq.QuadPart*40?1:2);
    }
    if(uc_frames.count<8192){
        UcFrameRow *r=&uc_frames.rows[uc_frames.count++];r->qpc=now.QuadPart;
        r->ms=(now.QuadPart-uc_frames.last)*1000.0/uc_frames.freq.QuadPart;r->phase=uc_frames.phase;r->hr=hr;
        if(uc_frames.coverage)for(unsigned i=0;i<5;i++)r->leaf[i]=InterlockedCompareExchange64(&ucrr_leaf_coverage_counts[i],0,0);
        if(uc_frames.batch_coverage)for(unsigned i=0;i<UC_BATCH_COUNT;i++)r->batch[i]=InterlockedCompareExchange64(&ucrr_batch_coverage_counts[i],0,0);
        if(uc_frames.host_upload_coverage){
            nv_host_upload_snapshot(&nv_host_upload_counters,r->host_upload,NV_HOST_UPLOAD_COUNT,&r->host_overflow);
            r->host_owner_error=uc_host_upload_owner_error();
        }
    }else uc_frames.overflow++;
    uc_frames.last=now.QuadPart;
    if(now.QuadPart-uc_frames.start>=uc_frames.freq.QuadPart*uc_frames.seconds)uc_frame_finish();
}
#endif /* !UCRR_PLAY_ONLY */
#endif
