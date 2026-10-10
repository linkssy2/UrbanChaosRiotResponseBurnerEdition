#ifndef UC_TEXTURE_REVISIONS_H
#define UC_TEXTURE_REVISIONS_H
#include "resource_revisions.h"
#include "xbox_memory_layout.h"
#include "nv2a_texture_reasons.h"
/* Stored in the resource entry, never keyed solely by a reusable guest address.
 * Clear token on owned-copy replacement/release. Palette is independently tracked.
 * Arm BEFORE equality, publish token only after equality and a clean recheck. */
typedef struct NvTextureRevision {
    const uint8_t *source,*palette;size_t bytes,palette_bytes;
    unsigned source_id,palette_id,stable,attempted,exact,small_attempted;
    UcResourceRegistrationStatus source_status,palette_status;
    uint64_t source_retry_epoch,palette_retry_epoch,source_token,palette_token;
} NvTextureRevision;
static uint64_t texture_revision_requests,texture_revision_hits,texture_revision_checks,texture_revision_changes,texture_revision_registration_attempts;
static int texture_revision_enabled(void){static int e=-1;if(e<0){const char*s=getenv("UCRR_TEXTURE_REVISIONS");e=s&&!strcmp(s,"1");}return e;}
static int texture_small_owner_enabled(void){static int e=-1;if(e<0){const char*s=getenv("UCRR_SMALL_OWNER_REVISIONS");e=s&&!strcmp(s,"1");}return e;}
static unsigned texture_small_owner(const uint8_t*p,size_t bytes){
    uintptr_t base=(uintptr_t)xbox_GetMemoryBase(),at=(uintptr_t)p;
    if(!base||!p||!bytes||at<base||at-base<0x1000000u)return 0;
    return uc_resource_find_existing(at-base,bytes);
}
static UcResourceRegistration texture_revision_register_result(const uint8_t*p,size_t bytes){
    uintptr_t base=(uintptr_t)xbox_GetMemoryBase(),at=(uintptr_t)p;
    /* Physical asset memory only; header/stack/code pages are not candidates.
     * The complete alias inventory independently checks the upper boundary. */
    if(!p||!bytes||at<base||at-base<0x1000000u){UcResourceRegistration result={0,UC_RESOURCE_INVALID_RANGE,0};return result;}
    return uc_resource_register_result(at-base,bytes);
}
static unsigned texture_revision_register(const uint8_t*p,size_t bytes){return texture_revision_register_result(p,bytes).id;}
static void texture_revision_try_register(const uint8_t*p,size_t bytes,unsigned*id,
    UcResourceRegistrationStatus*status,uint64_t*retry_epoch){
    if(*id)return;
    if(*status!=UC_RESOURCE_UNATTEMPTED){
        if(*status!=UC_RESOURCE_NATIVE_BUSY)return;
        uint64_t now=uc_resource_native_epoch();
        /* At most one registration attempt per completed native interval.
         * A still-busy interval costs no registration lock or mapping scan. */
        if(!now||now==*retry_epoch)return;
    }
    ++texture_revision_registration_attempts;
    UcResourceRegistration result=texture_revision_register_result(p,bytes);
    *id=result.id;*status=result.status;*retry_epoch=result.native_epoch;
}
static int texture_revision_small_range(const uint8_t*source,size_t bytes,const uint8_t*palette,size_t palette_bytes){
    uintptr_t base=(uintptr_t)xbox_GetMemoryBase(),at=(uintptr_t)source,p=(uintptr_t)palette;
    /* Exact scans stay bounded including palette bytes. They use the already
     * validated canonical RAM view, so writes through every shared alias are
     * visible without claiming page ownership or consuming tracker slots. */
    return base&&source&&bytes&&bytes<4096&&palette_bytes<=4095-bytes&&at>=base&&at-base>=0x1000000u&&
        (!palette_bytes||(palette&&p>=base&&p-base>=0x1000000u));
}
static int texture_revision_exact_clean(const NvTextureRevision*r,const NvSourceCopy*c,
    const uint8_t*source,size_t bytes,const uint8_t*palette,size_t palette_bytes){
    if(!r->exact||r->source!=source||r->bytes!=bytes||r->palette!=palette||r->palette_bytes!=palette_bytes||
        !texture_revision_small_range(source,bytes,palette,palette_bytes))return 0;
    uint64_t epoch=uc_resource_native_epoch();
    if(!epoch)return 0;
    int equal=nv_source_copy_equal(c,source,bytes,palette,palette_bytes);
    UC_TEX_RECORD(UC_TEX_EXACT,r,bytes,palette_bytes,equal);
    return equal&&epoch==uc_resource_native_epoch();
}
static int texture_revision_equal(NvTextureRevision*r,const NvSourceCopy*c,const uint8_t*source,size_t bytes,
    const uint8_t*palette,size_t palette_bytes){
    ++texture_revision_requests;
    if(!texture_revision_enabled()||!c->data||c->source_size!=bytes||c->palette_size!=palette_bytes){
        int equal=nv_source_copy_equal(c,source,bytes,palette,palette_bytes);
        UC_TEX_RECORD(texture_revision_enabled()?UC_TEX_COPY_MISMATCH:UC_TEX_DISABLED,r,bytes,palette_bytes,equal);
        return equal;
    }
    if(r->source!=source||r->bytes!=bytes||r->palette!=palette||r->palette_bytes!=palette_bytes){
        memset(r,0,sizeof(*r));r->source=source;r->bytes=bytes;r->palette=palette;r->palette_bytes=palette_bytes;
    }
    if(bytes<4096){
        /* Try once per resource identity, outside native output. A miss stays
         * exact: no per-draw registry scans and no new protected pages. */
        if(texture_small_owner_enabled()&&!r->small_attempted&&uc_resource_native_epoch()){
            r->small_attempted=1;
            r->source_id=texture_small_owner(source,bytes);
            if(palette_bytes)r->palette_id=texture_small_owner(palette,palette_bytes);
        }
        if(r->source_id&&(!palette_bytes||r->palette_id)){
            r->exact=0;r->source_status=UC_RESOURCE_REGISTERED;
            if(palette_bytes)r->palette_status=UC_RESOURCE_REGISTERED;
            goto tracked;
        }
        /* This marker selects a fresh exact scan at every certificate check;
         * it is never a remembered assertion that the RAM remains unchanged. */
        r->exact=texture_revision_small_range(source,bytes,palette,palette_bytes);
        int equal=nv_source_copy_equal(c,source,bytes,palette,palette_bytes);
        UC_TEX_RECORD(UC_TEX_SMALL,r,bytes,palette_bytes,equal);return equal;
    }
    /* Do not protect movie/streaming textures on first use. Four exact equal
     * observations establish that protection has a chance to amortize. */
    if(r->stable<4){
        int equal=nv_source_copy_equal(c,source,bytes,palette,palette_bytes);
        UC_TEX_RECORD(UC_TEX_WARMUP,r,bytes,palette_bytes,equal);
        if(equal)++r->stable;else r->stable=0;return equal;
    }
    r->attempted=1;
    texture_revision_try_register(source,bytes,&r->source_id,&r->source_status,&r->source_retry_epoch);
    if(palette_bytes)texture_revision_try_register(palette,palette_bytes,&r->palette_id,&r->palette_status,&r->palette_retry_epoch);
    if(!r->source_id||(palette_bytes&&!r->palette_id)){
        int equal=nv_source_copy_equal(c,source,bytes,palette,palette_bytes);
        UC_TEX_RECORD(UC_TEX_UNTRACKED,r,bytes,palette_bytes,equal);return equal;
    }
tracked:
    if(uc_resource_clean(r->source_id,r->source_token)&&(!palette_bytes||uc_resource_clean(r->palette_id,r->palette_token))){
        ++texture_revision_hits;return 1;
    }
    ++texture_revision_checks;
    uint64_t source_token=uc_resource_arm(r->source_id);
    uint64_t palette_token=palette_bytes?uc_resource_arm(r->palette_id):0;
    int equal=nv_source_copy_equal(c,source,bytes,palette,palette_bytes);
    UC_TEX_RECORD(UC_TEX_DIRTY,r,bytes,palette_bytes,equal);
    r->source_token=equal&&uc_resource_clean(r->source_id,source_token)?source_token:0;
    r->palette_token=equal&&palette_bytes&&uc_resource_clean(r->palette_id,palette_token)?palette_token:0;
    if(!equal){++texture_revision_changes;r->stable=0;}return equal;
}
static void texture_revision_report(void){
    if(texture_revision_enabled())fprintf(stderr,"[TEXTURE-REVISIONS] requests=%llu skipped_scans=%llu tracked_comparisons=%llu tracked_changes=%llu (lifetime counts, not FPS)\n",
        (unsigned long long)texture_revision_requests,(unsigned long long)texture_revision_hits,(unsigned long long)texture_revision_checks,(unsigned long long)texture_revision_changes);
}
#endif
