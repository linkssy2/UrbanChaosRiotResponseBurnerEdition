#ifndef NV_TEXTURE_REASONS_H
#define NV_TEXTURE_REASONS_H
#ifdef UCRR_TEXTURE_REASON_CAPTURE
#include <windows.h>
#include <stdint.h>
#include <string.h>
enum { UC_TEX_SMALL,UC_TEX_WARMUP,UC_TEX_UNTRACKED,UC_TEX_DIRTY,UC_TEX_DISABLED,UC_TEX_COPY_MISMATCH,UC_TEX_EXACT,UC_TEX_REASON_COUNT };
#define UC_TEX_STATUS_COUNT 13
typedef struct { uint64_t calls,equal,source_bytes,palette_bytes; } UcTextureReasonCount;
typedef struct { UcTextureReasonCount counts[2][UC_TEX_REASON_COUNT][UC_TEX_STATUS_COUNT][UC_TEX_STATUS_COUNT]; unsigned phase,overflow; } UcTextureReasonLedger;
#ifdef UC_TEXTURE_REASONS_DEFINE
UcTextureReasonLedger uc_texture_reasons;
__declspec(thread) int uc_texture_reason_owner;
volatile LONG uc_texture_reason_status;
#else
extern UcTextureReasonLedger uc_texture_reasons;
extern __declspec(thread) int uc_texture_reason_owner;
extern volatile LONG uc_texture_reason_status;
#endif
static void uc_texture_reason_begin(int enabled){
 memset(&uc_texture_reasons,0,sizeof(uc_texture_reasons));uc_texture_reason_owner=!!enabled;
 InterlockedExchange(&uc_texture_reason_status,enabled?1:0);
}
static void uc_texture_reason_stop(void){InterlockedAnd(&uc_texture_reason_status,~1L);uc_texture_reason_owner=0;}
static void uc_texture_reason_add(unsigned reason,unsigned source_status,unsigned palette_status,
 size_t source_bytes,size_t palette_bytes,int equal){
 if(!uc_texture_reason_owner){if(uc_texture_reason_status&1)InterlockedOr(&uc_texture_reason_status,2);return;}
 if(reason>=UC_TEX_REASON_COUNT||source_status>=UC_TEX_STATUS_COUNT||palette_status>=UC_TEX_STATUS_COUNT||uc_texture_reasons.phase>1){uc_texture_reasons.overflow=1;return;}
 UcTextureReasonCount*c=&uc_texture_reasons.counts[uc_texture_reasons.phase][reason][source_status][palette_status];
 if(c->calls==UINT64_MAX||c->source_bytes>UINT64_MAX-source_bytes||c->palette_bytes>UINT64_MAX-palette_bytes){uc_texture_reasons.overflow=1;return;}
 ++c->calls;c->equal+=!!equal;c->source_bytes+=source_bytes;c->palette_bytes+=palette_bytes;
}
#define UC_TEX_RECORD(reason,r,bytes,pbytes,equal) uc_texture_reason_add(reason,(unsigned)(r)->source_status,(unsigned)(r)->palette_status,bytes,pbytes,equal)
#else
#define UC_TEX_RECORD(reason,r,bytes,pbytes,equal) ((void)0)
#endif
#endif
