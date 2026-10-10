/* Live sweep-axis bitmap, including the original partition seed and exclusions.
 * Partition state belongs to the guest owner; no shadow spatial index is built. */
#include "migration_physics_broadphase_partitioned.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int broad_span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int broad_apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_physics_broadphase_partitioned(void){
 static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_PARTITIONED");flag=v&&!strcmp(v,"1");}
 if(!flag||(g_esp&3)||g_esp<0x10100||!broad_span(g_esp-128,152)||!broad_span(g_ecx,0x7c))return 0;
 uint32_t n=MEM32(g_esp+4),node=MEM32(g_esp+12),bits=MEM32(g_esp+20),axis=MEM32(g_ecx+0x4c);
 /* Original endpoint/node indices are 16 bit, with signed packed comparisons
  * elsewhere in this owner. Keep the established 16383-body admission. */
 if(!n||n>16384||!broad_span(node,16)||!broad_span(axis,n*8))return 0;
 uint32_t size=((n>>7)+1)*16,lo=MEM16(node+8),hi=MEM16(node+10);
 if(!lo||lo>=hi||hi>=n*2-1||(bits&15)||!broad_span(bits,size))return 0;
 if(!broad_apart(bits,size,axis,n*8)||!broad_apart(bits,size,node,16)||!broad_apart(bits,size,g_ecx,0x7c)||!broad_apart(bits,size,g_esp-128,152))return 0;
 if(!broad_apart(axis,n*8,g_esp-128,152)||!broad_apart(node,16,g_esp-128,152)||!broad_apart(g_ecx,0x7c,g_esp-128,152))return 0;
 for(uint32_t i=1;i<n*2;i++)if(MEM16(axis+i*4+2)>=n)return 0;
 if(MEM32(g_ecx+0x70)){
  uint32_t shift=MEM32(g_ecx+0x74),value=MEM32(g_esp+8);
  if(shift>16||value>65535)return 0;
  uint32_t partition=value>>(16-shift);
  if(partition){
   uint32_t table=MEM32(g_ecx+0x78);
   if(!broad_span(table,partition*16))return 0;
   uint32_t seed=table+(partition-1)*16,id=MEM16(seed),nodes=MEM32(g_ecx+0x40);
   uint32_t list=MEM32(seed+4),count=MEM32(seed+8);
   if(id>=n||count>65536||!broad_span(nodes,n*16)||(count&&!broad_span(list,count*2)))return 0;
   uint32_t a=MEM16(nodes+id*16+8),b=MEM16(nodes+id*16+10);
   if(a>=b||b>=n*2)return 0;
   if(!broad_apart(bits,size,table,partition*16)||!broad_apart(bits,size,nodes,n*16)||(count&&!broad_apart(bits,size,list,count*2)))return 0;
   if(!broad_apart(g_esp-128,152,table,partition*16)||!broad_apart(g_esp-128,152,nodes,n*16)||(count&&!broad_apart(g_esp-128,152,list,count*2)))return 0;
   for(uint32_t i=0;i<count;i++)if(MEM16(list+i*2)>=n)return 0;
  }
 }
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-20;
 g_eax=MEM32(frame+16);g_edx=MEM16(g_eax+10);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;
 g_ecx=MEM16(g_eax+8);g_eax=MEM32(frame+8);MEM32(g_esp+20)=g_edx;g_edx=MEM32(frame+24);g_eax>>=7;
 PUSH32(g_esp,g_esi);MEM32(g_esp+20)=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+20)=g_ebx;
 g_xmm0=XMM_ZERO();g_ecx=g_edx;++g_eax;
 do{g_esi=g_ecx;g_ecx+=16;--g_eax;XMM_STORE(g_esi,g_xmm0);}while(g_eax);
 g_eax=MEM32(g_ebx+0x4c);g_ecx=MEM32(g_ebx+0x70);g_eax+=4;
 if(g_ecx){
  g_esi=MEM32(g_ebx+0x74);g_ecx=16-g_esi;g_esi=MEM32(frame+12);g_esi=(uint32_t)((int32_t)g_esi>>(g_ecx&31));
  if((int32_t)g_esi>0){
   /* Seed membership is XOR-based, including repeated list entries. Only
    * list entries matching the selected node are excluded. */
   g_eax=MEM32(g_ebx+0x78);g_esi<<=4;g_ecx=MEM16(g_esi+g_eax-16);g_esi+=g_eax-16;
   g_eax=g_ecx;g_ecx&=31;g_eax>>=5;g_edi=1u<<g_ecx;MEM32(g_edx+g_eax*4)^=g_edi;
   g_eax=MEM32(g_esi+8)-1;g_ecx=MEM32(g_esi+4);MEM32(g_esp+12)=g_ecx;
   if((int32_t)g_eax>=0){
    ++g_eax;MEM32(g_esp+16)=g_eax;
    do{
     g_eax=MEM32(g_esp+12);SET_LO16(g_eax,MEM16(g_eax));
     if((uint16_t)g_eax!=MEM16(frame+20)){
      g_ecx=(uint16_t)g_eax;g_eax=g_ecx;g_ecx&=31;g_edi=1;g_eax>>=5;g_edi<<=g_ecx;MEM32(g_edx+g_eax*4)^=g_edi;
     }
     g_ecx=MEM32(g_esp+12);g_eax=MEM32(g_esp+16);g_ecx+=2;--g_eax;MEM32(g_esp+12)=g_ecx;MEM32(g_esp+16)=g_eax;
    }while(g_eax);
   }
   /* Remove minima inside the seed interval, then resume the ordinary
    * endpoint sweep immediately after the seed minimum. */
   g_edi=MEM16(g_esi);g_esi=MEM32(g_ebx+0x40);g_eax=MEM32(g_ebx+0x4c);g_edi<<=4;
   g_ecx=MEM16(g_edi+g_esi+8);g_edi+=g_esi;g_esi=g_eax+g_ecx*4+4;g_ecx=MEM16(g_edi+10);g_eax+=g_ecx*4;MEM32(g_esp+16)=g_eax;
   while(g_esi<g_eax){
    if(!(MEM8(g_esi)&1)){
     g_ecx=MEM16(g_esi+2);g_eax=g_ecx;g_ecx&=31;g_ebx=1u<<g_ecx;g_eax>>=5;g_ecx=MEM32(g_edx+g_eax*4);g_ebx=~g_ebx;g_ecx&=g_ebx;g_ebx=MEM32(g_esp+20);MEM32(g_edx+g_eax*4)=g_ecx;g_eax=MEM32(g_esp+16);
    }
    g_esi+=4;
   }
   g_ecx=MEM16(g_edi+8);g_eax=MEM32(g_ebx+0x4c)+g_ecx*4+4;
  }
 }
 g_esi=MEM32(g_esp+24);g_ecx=MEM32(g_ebx+0x4c)+g_esi*4;MEM32(g_esp+24)=g_ecx;
 int equal=g_eax==g_ecx;
 while(g_eax<MEM32(g_esp+24)){
  g_ecx=MEM16(g_eax+2);g_esi=g_ecx;g_ecx&=31;g_edi=1u<<g_ecx;g_esi>>=5;
  g_ecx=MEM32(g_edx+g_esi*4);g_eax+=4;g_ecx^=g_edi;MEM32(g_edx+g_esi*4)=g_ecx;
  equal=g_eax==MEM32(g_esp+24);
 }
 g_ecx=MEM32(g_ebx+0x4c);g_esi=MEM32(g_esp+28);g_eax+=4;g_edi=g_ecx+g_esi*4;equal=g_eax==g_edi;
 while(g_eax<g_edi){
  if(!(MEM8(g_eax)&1)){
   g_ecx=MEM16(g_eax+2);g_esi=g_ecx;g_ecx&=31;g_ebx=1;g_esi>>=5;g_ebx<<=g_ecx;MEM32(g_edx+g_esi*4)^=g_ebx;
  }
  g_eax+=4;equal=g_eax==g_edi;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);
 g_recomp_zf=equal;g_esp+=24;return 1;
}
