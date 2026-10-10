#ifndef UCRR_LIGHT_WRITE_WATCH_SHADOW_H
#define UCRR_LIGHT_WRITE_WATCH_SHADOW_H
#include "ram_write_watch_hooks.h"
#include "xbox_memory_layout.h"
/* Default shadow mode executes every original comparison. Explicit experimental
 * skip mode requires the same owned copy/range/key under a clean token. */
typedef struct {const void*copy;size_t bytes;uint32_t offset,format,signs;uint64_t token,calls,clean,arms,changed,false_clean;unsigned reports,last_frame;} NvLightWatch;
static NvLightWatch light_watch,light_watch_volume;
#if UCRR_LEAN_RUNTIME
#define NV_LIGHT_WATCH_STAT(x) ((void)0)
#else
#define NV_LIGHT_WATCH_STAT(x) ((void)(x))
#endif
static int nv_light_watch_compare(const void*a,const void*b,size_t n,uint32_t offset,uint32_t format,uint32_t signs){
    static int skip=-1;if(skip<0){const char*s=getenv("UCRR_LIGHT_WRITE_SKIP");skip=s&&!strcmp(s,"1");}
    static int enabled=-1;if(enabled<0)enabled=getenv("UCRR_LIGHT_WRITE_WATCH")!=NULL;
    static int volume=-1;if(volume<0){const char*s=getenv("UCRR_LIGHT_WATCH_VOLUME");volume=s&&!strcmp(s,"1");}
    int second=volume&&n==32768;
    if(!enabled||(!second&&n<256*1024)||n>1024*1024||(offset&4095)||(n&4095)||offset<0x1000000u||
       b!=(const uint8_t*)xbox_GetMemoryBase()+offset)return nv_light_compare(a,b,n,offset,format,signs);
    if(second){if(!uc_ram_watch_second_matches(offset,n)&&!uc_ram_watch_second_start(offset,n))return nv_light_compare(a,b,n,offset,format,signs);}
    else if(!uc_ram_watch_matches(offset,n)&&!uc_ram_watch_start(offset,n))return nv_light_compare(a,b,n,offset,format,signs);
    NvLightWatch*w=second?&light_watch_volume:&light_watch;
    NV_LIGHT_WATCH_STAT(++w->calls);
    int same=w->copy==a&&w->bytes==n&&w->offset==offset&&w->format==format&&w->signs==signs;
    uint64_t token=same?w->token:0;
    int clean=second?uc_ram_watch_second_clean(token):uc_ram_watch_clean(token);
    if(clean)NV_LIGHT_WATCH_STAT(++w->clean);
    else {token=second?uc_ram_watch_second_arm():uc_ram_watch_arm();NV_LIGHT_WATCH_STAT(++w->arms);}
    if(clean&&skip)return 0;
    int result=nv_light_compare(a,b,n,offset,format,signs);
    /* A concurrent dirty transition legitimately invalidates an earlier hit. */
    int still_clean=second?uc_ram_watch_second_clean(token):uc_ram_watch_clean(token);
    if(clean&&result&&still_clean){NV_LIGHT_WATCH_STAT(++w->false_clean);UC_WATCH_DISABLE();}
    if(result)NV_LIGHT_WATCH_STAT(++w->changed);
    w->copy=a;w->bytes=n;w->offset=offset;w->format=format;w->signs=signs;
    w->token=!result&&still_clean?token:0;
    return result;
}
static void nv_light_watch_forget(void){light_watch.copy=NULL;light_watch.token=0;light_watch_volume.copy=NULL;light_watch_volume.token=0;}
/* A previously established equality witness only. Never arm a watch or read
 * guest bytes to manufacture a certificate for an already uploaded resource. */
typedef struct NvLightWatchCertificate {
    const void *copy;
    size_t bytes;
    uint32_t offset,format,signs;
    uint64_t token;
    unsigned slot;
} NvLightWatchCertificate;
static int nv_light_watch_certificate_clean(const NvLightWatchCertificate *c){
    if(!c || !c->token || (c->slot!=1 && c->slot!=2))return 0;
    const NvLightWatch *w=c->slot==2?&light_watch_volume:&light_watch;
    if(w->copy!=c->copy || w->bytes!=c->bytes || w->offset!=c->offset ||
       w->format!=c->format || w->signs!=c->signs || w->token!=c->token)return 0;
    return c->slot==2?
        uc_ram_watch_second_matches(c->offset,c->bytes)&&uc_ram_watch_second_clean(c->token):
        uc_ram_watch_matches(c->offset,c->bytes)&&uc_ram_watch_clean(c->token);
}
static int nv_light_watch_certificate(const void *copy,size_t bytes,uint32_t offset,
    uint32_t format,uint32_t signs,NvLightWatchCertificate *out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    for(unsigned slot=1;slot<=2;++slot){
        const NvLightWatch *w=slot==2?&light_watch_volume:&light_watch;
        NvLightWatchCertificate c={copy,bytes,offset,format,signs,w->token,slot};
        if(nv_light_watch_certificate_clean(&c)){*out=c;return 1;}
    }
    return 0;
}
static void nv_light_watch_report(unsigned frame){
#if UCRR_LEAN_RUNTIME
    (void)frame;
#else
    if(!light_watch.calls||light_watch.reports>=64||frame-light_watch.last_frame<600)return;
    light_watch.last_frame=frame;++light_watch.reports;
    fprintf(stderr,"[LIGHT-WATCH-SHADOW] frame=%u calls=%llu potential_clean=%llu arms=%llu changed=%llu false_clean=%llu failed=%d active=%d\n",frame,
      (unsigned long long)light_watch.calls,(unsigned long long)light_watch.clean,(unsigned long long)light_watch.arms,
      (unsigned long long)light_watch.changed,(unsigned long long)light_watch.false_clean,uc_ram_watch_failed(),uc_ram_watch_matches(light_watch.offset,light_watch.bytes));
    if(light_watch_volume.calls)fprintf(stderr,"[LIGHT-WATCH-VOLUME] calls=%llu clean=%llu arms=%llu changed=%llu false_clean=%llu active=%d\n",
      (unsigned long long)light_watch_volume.calls,(unsigned long long)light_watch_volume.clean,(unsigned long long)light_watch_volume.arms,
      (unsigned long long)light_watch_volume.changed,(unsigned long long)light_watch_volume.false_clean,uc_ram_watch_second_matches(light_watch_volume.offset,light_watch_volume.bytes));
#endif
}
#undef NV_LIGHT_WATCH_STAT
#endif
