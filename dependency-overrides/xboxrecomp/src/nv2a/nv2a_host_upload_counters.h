#ifndef NV2A_HOST_UPLOAD_COUNTERS_H
#define NV2A_HOST_UPLOAD_COUNTERS_H
#include <stdint.h>
#include <string.h>

/* One immediate-context owner. The recorder must begin/snapshot/stop on that
 * same thread. No clocks, atomics, allocations, logging or GPU operations.
 * Values are cumulative within an explicitly enabled F8 window, not timings.
 * Runtime hooks only cover packed_enabled==2. Arena bytes are deliberately
 * omitted; ARENA_COVERAGE_GAPS prevents treating those transactions as complete.
 */
#define NV_HOST_UPLOAD_COUNTER_LIST(X) \
 X(RAW_GROUPS,"HostRawGroups") \
 X(ATTEMPTS,"HostRawUploadAttempts") \
 X(VALIDATED,"HostRawUploadValidated") \
 X(SUCCESSES,"HostRawUploadSuccesses") \
 X(PACKETS,"HostRawUploadPackets") \
 X(VERTICES,"HostRawUploadVertices") \
 X(REQUEST_BYTES,"HostRawRequestedBytes") \
 X(RESIDENT_HITS,"HostRawResidentHits") \
 X(RESIDENT_MISSES,"HostRawResidentMisses") \
 X(RESIDENT_COOLDOWNS,"HostRawResidentCooldowns") \
 X(RESIDENT_BYPASS,"HostRawResidentBypass") \
 X(RESIDENT_BYTES,"HostRawResidentBytes") \
 X(VERTEX_MAP_DISCARD,"HostRawVertexMapDiscard") \
 X(VERTEX_MAP_APPEND,"HostRawVertexMapAppend") \
 X(VERTEX_MAP_FAILURES,"HostRawVertexMapFailures") \
 X(VERTEX_COPY_BYTES,"HostRawVertexCopyBytes") \
 X(SHADOW_COPY_BYTES,"HostRawShadowCopyBytes") \
 X(VERTEX_UPDATES,"HostRawVertexUpdates") \
 X(VERTEX_UPDATE_BYTES,"HostRawVertexUpdateBytes") \
 X(PROGRAM_UPDATES,"HostRawProgramUpdates") \
 X(PROGRAM_UPDATE_BYTES,"HostRawProgramUpdateBytes") \
 X(CONVERSION_UPDATES,"HostRawConversionUpdates") \
 X(CONVERSION_UPDATE_BYTES,"HostRawConversionUpdateBytes") \
 X(METADATA_UPDATES,"HostRawMetadataUpdates") \
 X(METADATA_UPDATE_BYTES,"HostRawMetadataUpdateBytes") \
 X(PARAM_UPDATES,"HostRawParamUpdates") \
 X(PARAM_UPDATE_BYTES,"HostRawParamUpdateBytes") \
 X(METADATA_MAPS,"HostRawMetadataMaps") \
 X(METADATA_MAP_FAILURES,"HostRawMetadataMapFailures") \
 X(METADATA_COPY_BYTES,"HostRawMetadataCopyBytes") \
 X(METADATA_BIND_CALLS,"HostRawMetadataBindCalls") \
 X(METADATA_BIND_BUFFERS,"HostRawMetadataBindBuffers") \
 X(ARENA_CALLS,"HostRawArenaCalls") \
 X(ARENA_FAILURES,"HostRawArenaFailures") \
 X(ARENA_COVERAGE_GAPS,"HostRawArenaCoverageGaps") \
 X(COMBINER_SET_CALLS,"HostRawCombinerSetCalls") \
 X(COMBINER_PREPARED_HITS,"HostRawCombinerPreparedHits") \
 X(COMBINER_PARSES,"HostRawCombinerParses") \
 X(COMBINER_SHADER_REUSES,"HostRawCombinerShaderReuses") \
 X(COMBINER_SHADER_LOOKUPS,"HostRawCombinerShaderLookups")

enum {
#define NV_HOST_UPLOAD_ENUM(kind,name) NV_HOST_UPLOAD_##kind,
 NV_HOST_UPLOAD_COUNTER_LIST(NV_HOST_UPLOAD_ENUM)
#undef NV_HOST_UPLOAD_ENUM
 NV_HOST_UPLOAD_COUNT
};
#define NV_HOST_UPLOAD_SCHEMA 1u
static const char *const nv_host_upload_names[NV_HOST_UPLOAD_COUNT]={
#define NV_HOST_UPLOAD_NAME(kind,name) name,
 NV_HOST_UPLOAD_COUNTER_LIST(NV_HOST_UPLOAD_NAME)
#undef NV_HOST_UPLOAD_NAME
};
typedef struct NvHostUploadCounters {
 unsigned active,overflow;
 uint64_t values[NV_HOST_UPLOAD_COUNT];
} NvHostUploadCounters;
#ifdef NV_HOST_UPLOAD_DEFINE
NvHostUploadCounters nv_host_upload_counters;
#else
extern NvHostUploadCounters nv_host_upload_counters;
#endif

static void nv_host_upload_begin(NvHostUploadCounters*c,int enabled){
 if(!c)return;
 c->active=0;c->overflow=0;memset(c->values,0,sizeof(c->values));c->active=!!enabled;
}
static void nv_host_upload_stop(NvHostUploadCounters*c){if(c)c->active=0;}
static void nv_host_upload_add(NvHostUploadCounters*c,unsigned kind,uint64_t count){
 if(!c||!c->active||!count)return;
 if(kind>=NV_HOST_UPLOAD_COUNT){c->overflow=1;return;}
 if(UINT64_MAX-c->values[kind]<count){c->values[kind]=UINT64_MAX;c->overflow=1;}
 else c->values[kind]+=count;
}
static int nv_host_upload_snapshot(const NvHostUploadCounters*c,uint64_t*out,unsigned count,unsigned*overflow){
 if(!c||!out||count<NV_HOST_UPLOAD_COUNT)return 0;
 memcpy(out,c->values,sizeof(c->values));if(overflow)*overflow=c->overflow;return 1;
}
#endif
