#ifndef NV2A_VISIBILITY_DEMAND_H
#define NV2A_VISIBILITY_DEMAND_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* This header requires the actual report and semaphore queue declarations.
 * A ready slot is not enough if a queued write can replace its mapping/result.
 * No fence word is treated as generation identity; pending always falls back. */
typedef struct NvVisibilityBudget { uint64_t last_submit;unsigned valid,deferred; } NvVisibilityBudget;
static int nv_visibility_overlap(uint64_t a,size_t bytes,uint64_t b,size_t count){
 return a<b+count&&b<a+bytes;
}
static int nv_visibility_read32(const uint8_t*ram,size_t size,uint64_t at,uint32_t*out){
 if(!ram||at>size||size-at<4)return 0;memcpy(out,ram+(size_t)at,4);return 1;
}
static int nv_visibility_ready(const uint8_t*ram,size_t size,uint32_t root,unsigned index,
 const NvReports*r,const NvSemaphores*s){
 uint32_t device,page,status;uint64_t entry,slot;
 if(index>=65536||r->failed||s->failed||r->active||r->count>NV_REPORT_QUEUE||s->count>NV_SEMAPHORE_QUEUE||
    !nv_visibility_read32(ram,size,root,&device)||!device)return 0;
 entry=(uint64_t)device+0x7d4u+4u*(index>>8);
 if(!nv_visibility_read32(ram,size,entry,&page)||!page)return 0;
 slot=(uint64_t)page+16u*(index&255u);
 if(slot>size||size-slot<16||!nv_visibility_read32(ram,size,slot+12,&status)||status==UINT32_MAX)return 0;
 for(unsigned i=0;i<r->count;i++){
  const NvReportItem*q=&r->queue[(r->head+i)%NV_REPORT_QUEUE];
  if(q->kind>2)return 0;
  if(q->kind==2&&(nv_visibility_overlap(q->address,16,root,4)||
     nv_visibility_overlap(q->address,16,entry,4)||nv_visibility_overlap(q->address,16,slot,16)))return 0;
 }
 for(unsigned i=0;i<s->count;i++){
  const NvSemaphoreItem*q=&s->items[(s->head+i)%NV_SEMAPHORE_QUEUE];
  if(nv_visibility_overlap(q->address,4,root,4)||nv_visibility_overlap(q->address,4,entry,4)||
     nv_visibility_overlap(q->address,4,slot,16))return 0;
 }
 return 1;
}
static int nv_visibility_budget_open(const NvVisibilityBudget*s,uint64_t now,uint64_t max_ticks){
 return s->valid&&max_ticks&&now>=s->last_submit&&now-s->last_submit<max_ticks&&s->deferred<8;
}
static int nv_visibility_can_defer(NvVisibilityBudget*s,uint64_t now,uint64_t max_ticks){
 if(!nv_visibility_budget_open(s,now,max_ticks))return 0;
 ++s->deferred;return 1;
}
static void nv_visibility_submitted(NvVisibilityBudget*s,uint64_t now){s->last_submit=now;s->deferred=0;s->valid=1;}
#endif
