#pragma once
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
/* Native terminal driver. Original virtual services remain real calls, in order.
 * Admission is before effects; after the first call there is no fallback/replay.
 * Return 0 declines unchanged, 1 completes with ZF=1, 2 completes with ZF=0. */
static int mopp_span(uint32_t p,unsigned n){
 if(p>=0x80000000u&&p<0x84000000u)p-=0x80000000u;
 return p>=0x10000u&&(uint64_t)p+n<=g_xbox_total_ram;
}
static int uc_hybrid_mopp_terminal(uint32_t context,uint32_t key,uint32_t frame){
 static int enabled=-1;
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_MOPP_TERMINAL");enabled=v&&!strcmp(v,"1");}
 if(!enabled||frame!=g_esp||context!=g_esi||key!=g_edi||
    !mopp_span(context,0x68)||!mopp_span(frame-20,0x304)||MEM32(context+0x58))return 0;
 uint32_t collection=MEM32(context+0x64),filter=MEM32(context+0x44);
 uint32_t body=MEM32(context+0x60),collector=MEM32(context+0x5c);
 if(!mopp_span(collection,4)||!mopp_span(body,16)||!mopp_span(collector,8))return 0;
 uint32_t table=MEM32(collection);
 if(!mopp_span(table,0x2c)||MEM32(table+0x28)!=0x304350u)return 0;
 uint32_t filter_target=0;
 if(filter){
  if(!mopp_span(filter,4)||!mopp_span(MEM32(filter),4))return 0;
  filter_target=MEM32(MEM32(filter));if(filter_target!=0x2a8800u)return 0;
 }
 /* Match the original guest registers and push/write order at each call. */
 g_ecx=filter;
 if(filter){
  uint32_t saved=g_esp;
  g_edx=MEM32(g_esi+0x64);g_eax=MEM32(g_ecx);
  PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);
  g_edx=g_esi+0x20;PUSH32(g_esp,g_edx);
  g_edx=g_esp+0x4b;PUSH32(g_esp,g_edx);
  uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x32cdfcu);
  RECOMP_ICALL_SAFE(target,saved);
  if(!MEM8(g_eax))return 1;
 }
 g_ecx=MEM32(g_esi+0x64);g_eax=MEM32(g_ecx);g_edx=g_esp+0xf0;
 {
  uint32_t saved=g_esp;
  PUSH32(g_esp,g_edx);PUSH32(g_esp,g_edi);
  uint32_t target=MEM32(g_eax+0x28);PUSH32(g_esp,0x32ce16u);
  RECOMP_ICALL_SAFE(target,saved);
 }
 /* Query mode and owners are live after callbacks, including the child call. */
 g_ecx=MEM32(g_esi+0x58);g_edx=MEM32(g_eax);
 if(g_ecx){
  uint32_t saved=g_esp;
  PUSH32(g_esp,g_ecx);g_ecx=g_esi+0x20;PUSH32(g_esp,g_ecx);
  g_ecx=g_esp+0x47;PUSH32(g_esp,g_ecx);g_ecx=g_eax;
  uint32_t target=MEM32(g_edx+0x14);PUSH32(g_esp,0x32ce2eu);
  RECOMP_ICALL_SAFE(target,saved);
  if(!MEM8(g_eax))return 1;
  MEM8(g_esi+0x50)=1;g_eax=MEM32(g_esi+0x58);g_edx=MEM32(g_eax+0x14);
  MEM32(g_esi+0x54)=g_edx;MEM32(g_eax+0x10)=g_edi;
  return 2;
 }
 g_ecx=MEM32(g_esi+0x60);MEM32(g_esp+0x6c)=g_ecx;
 g_ecx=MEM32(g_ecx+8);MEM32(g_esp+0x68)=g_ecx;g_ecx=MEM32(g_esi+0x5c);
 {
  uint32_t saved=g_esp;
  PUSH32(g_esp,g_ecx);g_ecx=g_esp+0x64;PUSH32(g_esp,g_ecx);
  g_ecx=g_esi+0x20;PUSH32(g_esp,g_ecx);g_ecx=g_eax;
  MEM32(g_esp+0x6c)=g_eax;MEM32(g_esp+0x70)=g_edi;
  uint32_t target=MEM32(g_edx+0x18);PUSH32(g_esp,0x32ce74u);
  RECOMP_ICALL_SAFE(target,saved);
 }
 g_edx=MEM32(g_esi+0x5c);g_eax=MEM32(g_edx+4);MEM32(g_esi+0x54)=g_eax;
 return 1;
}
