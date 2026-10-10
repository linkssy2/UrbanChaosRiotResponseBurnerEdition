#ifndef UCRR_HOST_UPLOAD_COVERAGE_H
#define UCRR_HOST_UPLOAD_COVERAGE_H
#include <windows.h>
#ifdef UCRR_HOST_UPLOAD_COVERAGE_DEFINE
#define NV_HOST_UPLOAD_DEFINE
#endif
#include "../nv2a/nv2a_host_upload_counters.h"
#ifdef UCRR_HOST_UPLOAD_COVERAGE_DEFINE
__declspec(thread) unsigned uc_host_upload_owner;
volatile LONG uc_host_upload_status; /* bit0 active publication, bit1 bad owner */
#else
extern __declspec(thread) unsigned uc_host_upload_owner;
extern volatile LONG uc_host_upload_status;
#endif
/* Normal upload increments are single-owner. A different thread must not write
 * that ledger; latch a rejected-evidence bit instead of using a lock per draw.
 * The existing frame recorder has one lifetime/owner and one window per process. */
#if UCRR_PLAY_ONLY
static __forceinline int uc_host_upload_active(void){return 0;}
static __forceinline void uc_host_upload_enter(void){}
static unsigned uc_host_upload_owner_error(void){return 0;}
static void uc_host_upload_begin(int enabled){(void)enabled;}
static void uc_host_upload_stop(void){}
#else
static __forceinline int uc_host_upload_active(void){
 return uc_host_upload_owner && nv_host_upload_counters.active;
}
static __forceinline void uc_host_upload_enter(void){
 /* Aligned LONG volatile publication uses MSVC's Windows acquire-load
  * contract. No locked instruction in the usual disabled path. Confirm with
  * CAS only for a foreign writer during an actually active capture. */
 if(!uc_host_upload_owner&&(uc_host_upload_status&1)){
  LONG state=InterlockedCompareExchange(&uc_host_upload_status,0,0);
  while(state&1){
   LONG previous=InterlockedCompareExchange(&uc_host_upload_status,state|2,state);
   if(previous==state)break;
   state=previous;
  }
 }
}
static unsigned uc_host_upload_owner_error(void){
 return !!(InterlockedCompareExchange(&uc_host_upload_status,0,0)&2);
}
static void uc_host_upload_begin(int enabled){
 uc_host_upload_owner=!!enabled;
 nv_host_upload_begin(&nv_host_upload_counters,enabled);
 InterlockedExchange(&uc_host_upload_status,enabled?1:0);
}
static void uc_host_upload_stop(void){
 /* Stop and freeze error publication atomically before serializing the CSV. */
 InterlockedAnd(&uc_host_upload_status,~1L);
 nv_host_upload_stop(&nv_host_upload_counters);uc_host_upload_owner=0;
}
#endif /* !UCRR_PLAY_ONLY */
#endif
