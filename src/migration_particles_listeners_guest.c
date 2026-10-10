#include "migration_particles_listeners_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_002D5CCD(void);
/* The four constructors append one record. Their private insertion chain has
 * no other direct callers. Keep its guest ABI, but own the allocation/copy/
 * append algorithm here rather than re-entering translated vector helpers. */
typedef struct{unsigned entry,stride,seh,fast,insert,grow,size,alloc,first,fill,last,release;}ListenerPath;
static const ListenerPath paths[]={
 {0x1752d0,24,0x394e20,0x175329,0x17534c,0x1752b4,0x17503d,0x17504d,0x17506a,0x175080,0x17509f,0x1750ce},
 {0x187600,12,0x395f00,0x187657,0x18767a,0x1875e2,0x187367,0x187377,0x187394,0x1873aa,0x1873c9,0x1873f7},
 {0x1903c0,12,0x396630,0x190417,0x19043a,0x1903a2,0x1900a7,0x1900b7,0x1900d4,0x1900ea,0x190109,0x190137},
 {0x1cb800,12,0x399ba0,0x1cb857,0x1cb87a,0x1cb7e2,0x1cb5c7,0x1cb5d7,0x1cb5f4,0x1cb60a,0x1cb629,0x1cb657}
};
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_EFFECT_LISTENERS");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Signed pointer-difference division also reconstructs the live multiply high
 * word and low word used at the original heap callback boundary. */
static uint32_t quotient(uint32_t bytes,unsigned stride){
 int64_t product=(int64_t)(int32_t)bytes*0x2aaaaaab;
 g_eax=(uint32_t)product;g_edx=(uint32_t)((int32_t)(product>>32)>>(stride==24?2:1));
 return g_edx+(g_edx>>31);
}
static void size_call(unsigned stride){
 g_eax=MEM32(g_ecx+4);
 if(g_eax){g_ecx=MEM32(g_ecx+8)-g_eax;g_eax=quotient(g_ecx,stride);}
 g_recomp_zf=g_eax==0;g_esp+=4;
}
static void copy_record(unsigned stride){
 g_ebx=MEM32(g_esi);g_edi=g_eax;MEM32(g_edi)=g_ebx;
 for(unsigned off=4;off<stride-4;off+=4){g_ebx=MEM32(g_esi+off);MEM32(g_edi+off)=g_ebx;}
 g_esi=MEM32(g_esi+stride-4);MEM32(g_edi+stride-4)=g_esi;
}
static void copy_range(unsigned stride){
 g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);g_eax=MEM32(g_esp+12);
 if(g_ecx!=g_edx){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
  do{if(g_eax){g_esi=g_ecx;copy_record(stride);}g_ecx+=stride;g_eax+=stride;}while(g_ecx!=g_edx);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 }
 g_recomp_zf=g_ecx==g_edx;g_esp+=4;
}
static void fill_records(unsigned stride){
 g_eax=MEM32(g_esp+8);
 if(g_eax){
  g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ecx=g_eax;g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_edi);
  do{if(g_eax){g_esi=g_edx;copy_record(stride);}g_eax+=stride;g_ecx--;}while(g_ecx);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 }
 g_esp+=4;
}
#define LOCAL_CALL(fn,pc,n) do{PUSH32(g_esp,pc);fn(n);}while(0)
#define FRAME_LOCAL(fn,pc,n) do{g_ebp=frame;LOCAL_CALL(fn,pc,n);}while(0)
#define HEAP_CALL(fn,pc) do{g_ebp=frame;PUSH32(g_esp,pc);fn();}while(0)
static void grow_append(const ListenerPath*p){
 unsigned n=p->stride;uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,p->seh);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=n+4;
 g_eax=MEM32(frame+16);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;
 g_ecx=MEM32(g_eax);g_edx=MEM32(g_eax+4);g_ebx=MEM32(g_esi+4);
 if(n==12){g_eax=MEM32(g_eax+8);PUSH32(g_esp,g_edi);MEM32(frame-16)=g_esp;MEM32(frame-28)=g_ecx;MEM32(frame-24)=g_edx;MEM32(frame-20)=g_eax;}
 else{MEM32(frame-40)=g_ecx;g_ecx=MEM32(g_eax+8);MEM32(frame-36)=g_edx;g_edx=MEM32(g_eax+12);MEM32(frame-32)=g_ecx;g_ecx=MEM32(g_eax+16);MEM32(frame-28)=g_edx;g_edx=MEM32(g_eax+20);PUSH32(g_esp,g_edi);MEM32(frame-16)=g_esp;MEM32(frame-24)=g_ecx;MEM32(frame-20)=g_edx;}
 if(g_ebx){g_ecx=MEM32(g_esi+12)-g_ebx;g_ecx=quotient(g_ecx,n);}else g_ecx=0;
 g_edi=MEM32(frame+12);
 if(g_ebx){g_edx=MEM32(g_esi+8)-g_ebx;g_eax=quotient(g_edx,n);}else g_eax=0;
 g_edx=(n==12?0x15555555u:0xaaaaaaau)-g_eax;
 /* Entry admission bounds size/capacity, and no callback has run yet. The
  * only reachable insertion is count1 at end with exhausted capacity. */
 if(g_ebx){g_edx=MEM32(g_esi+8)-g_ebx;g_eax=quotient(g_edx,n);}else g_eax=0;
 g_eax+=g_edi;
 g_eax=g_ecx>>1;g_edx=(n==12?0x15555555u:0xaaaaaaau)-g_eax;g_ecx+=g_eax;
 if(g_ebx){g_edx=MEM32(g_esi+8)-g_ebx;g_eax=quotient(g_edx,n);}else g_eax=0;
 g_eax+=g_edi;
 if(g_ecx<g_eax){g_ecx=g_esi;FRAME_LOCAL(size_call,p->size,n);g_ecx=g_eax+g_edi;}
 g_ebx=g_ecx*n;PUSH32(g_esp,g_ebx);HEAP_CALL(sub_002D57FB,p->alloc);
 g_edx=MEM32(frame+8);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);MEM32(frame+16)=g_eax;g_eax=MEM32(frame+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);MEM32(frame-4)=0;FRAME_LOCAL(copy_range,p->first,n);
 g_ecx=MEM32(frame+8);g_esp+=24;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_edx=frame-n-16;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);MEM32(frame+12)=g_eax;FRAME_LOCAL(fill_records,p->fill,n);
 g_edx=MEM32(frame+8);g_ecx=MEM32(frame+12);g_esp+=20;PUSH32(g_esp,g_edx);g_eax=g_ecx+g_edi*n;g_ecx=MEM32(g_esi+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_eax=MEM32(frame+8);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);FRAME_LOCAL(copy_range,p->last,n);
 g_eax=MEM32(g_esi+4);g_esp+=20;if(g_eax){g_ecx=MEM32(g_esi+8)-g_eax;g_eax=quotient(g_ecx,n);}g_edi+=g_eax;
 g_eax=MEM32(g_esi+4);if(g_eax){PUSH32(g_esp,g_eax);HEAP_CALL(sub_002D5CCD,p->release);g_esp+=4;}
 g_eax=MEM32(frame+16);g_ecx=g_edi+g_edi*2;g_ebx+=g_eax;g_edx=g_eax+g_ecx*(n/3);MEM32(g_esi+12)=g_ebx;MEM32(g_esi+8)=g_edx;MEM32(g_esi+4)=g_eax;
 g_ecx=MEM32(frame-12);MEM32(0)=g_ecx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_ebx==0;g_esp+=16;
}
static void insert_append(const ListenerPath*p){
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+12);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=MEM32(g_edi+4);
 unsigned size=0;if(g_esi){g_ecx=MEM32(g_edi+8)-g_esi;g_eax=quotient(g_ecx,p->stride);MEM32(g_esp+20)=g_eax;size=g_eax;}
 if(size){g_ecx=g_ebx-g_esi;g_esi=quotient(g_ecx,p->stride);}else g_esi=0;
 g_ecx=MEM32(g_esp+24);PUSH32(g_esp,g_ecx);PUSH32(g_esp,1);PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,p->grow);grow_append(p);
 g_eax=MEM32(g_edi+4);g_edx=g_esi+g_esi*2;POP32(g_esp,g_edi);g_ecx=g_eax+g_edx*(p->stride/3);g_eax=MEM32(g_esp+12);POP32(g_esp,g_esi);MEM32(g_eax)=g_ecx;POP32(g_esp,g_ebx);g_esp+=16;
}
int uc_migration_particles_listeners(unsigned entry){
 if(!enabled())return 0;const ListenerPath*p=NULL;for(unsigned i=0;i<4;i++)if(paths[i].entry==entry)p=paths+i;if(!p)return 0;
 uint32_t s=g_esp,o=g_ecx,n=p->stride;
 if((s&3)||s<0x10200u||!span(s-256,288)||!span(o,16)||!apart(o,16,s-256,288)||!span(0x51c178,1)||MEM8(0x51c178)||!apart(s-256,288,0x51c178,1)||!apart(o,16,0x51c178,1))return 0;
 uint32_t source=MEM32(s+4),begin=MEM32(o+4),end=MEM32(o+8),limit=MEM32(o+12);
 if(!span(source,n)||!apart(source,n,s-256,264))return 0;
 /* ponytail: bounded4096-record mapped vectors; other metadata keeps original
  * behavior. Raise this ceiling only with matching whole-state fixtures. */
 if(!begin){if(end||limit)return 0;}
 else if(end<begin||limit<end||(end-begin)%n||(limit-begin)%n||(limit-begin)/n>4096||!span(begin,limit-begin)||!apart(begin,limit-begin,s-256,288)||!apart(begin,limit-begin,o,16)||!apart(begin,limit-begin,0x51c178,1))return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+4);
 if(g_edi){g_ecx=MEM32(g_esi+8)-g_edi;g_ecx=quotient(g_ecx,n);}else g_ecx=0;
 int room=0;if(g_edi){g_edx=MEM32(g_esi+12)-g_edi;g_eax=quotient(g_edx,n);room=g_ecx<g_eax;}
 if(room){g_eax=MEM32(g_esp+12);g_ecx=MEM32(g_esp+12);g_edi=MEM32(g_esi+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,1);PUSH32(g_esp,g_edi);LOCAL_CALL(fill_records,p->fast,n);g_esp+=20;g_edi+=n;MEM32(g_esi+8)=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_edi==0;g_esp+=8;}
 else{g_edx=MEM32(g_esp+12);g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,p->insert);insert_append(p);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;}
 return 1;
}
#undef LOCAL_CALL
#undef FRAME_LOCAL
#undef HEAP_CALL
