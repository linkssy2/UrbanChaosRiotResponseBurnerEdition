#include "migration_services_queue_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int queue_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int queue_nodes(uint32_t p){unsigned n=0;while(p){if(++n>4096||!queue_span(p,12))return 0;p=MEM32(p+4);}return 1;}
static void queue_remove_plain(void){
 g_eax=MEM32(g_ecx+12);if(!g_eax){g_esp+=8;return;}g_edx=MEM32(g_esp+4);PUSH32(g_esp,g_esi);
 while(MEM32(g_eax)!=g_edx){g_eax=MEM32(g_eax+4);if(!g_eax){POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return;}}
 g_edx=MEM32(g_eax+4);if(g_edx){g_esi=MEM32(g_eax+8);MEM32(g_edx+8)=g_esi;}
 g_edx=MEM32(g_eax+8);if(g_edx){g_esi=MEM32(g_eax+4);MEM32(g_edx+4)=g_esi;}
 if(g_eax==MEM32(g_ecx+12)){g_edx=MEM32(g_eax+4);MEM32(g_ecx+12)=g_edx;}if(g_eax==MEM32(g_ecx+16)){g_edx=MEM32(g_eax+8);MEM32(g_ecx+16)=g_edx;}
 MEM32(g_eax+8)=0;g_edx=MEM32(g_ecx+8);MEM32(g_eax+4)=g_edx;MEM32(g_ecx+8)=g_eax;MEM32(g_ecx+4)--;POP32(g_esp,g_esi);g_esp+=8;
}
static void queue_insert_after(void){
 g_edx=MEM32(g_ecx+8);PUSH32(g_esp,g_esi);int found=0;
 if(g_edx){g_eax=MEM32(g_edx+4);MEM32(g_ecx+8)=g_eax;g_eax=MEM32(g_esp+12);MEM32(g_edx)=g_eax;g_ecx=MEM32(g_ecx+32);
  if(g_ecx){g_esi=MEM32(g_esp+8);for(;;){g_eax=MEM32(g_ecx);while(g_eax){if(MEM32(g_eax)==g_esi){found=1;break;}g_eax=MEM32(g_eax+4);}if(found)break;g_ecx=MEM32(g_ecx+4);if(!g_ecx)break;}}
 }
 if(found){g_ecx=MEM32(g_eax+4);MEM32(g_edx+4)=g_ecx;MEM32(g_edx+8)=g_eax;g_ecx=MEM32(g_eax+4);int last=!g_ecx;if(g_ecx)MEM32(g_ecx+8)=g_edx;MEM32(g_eax+4)=g_edx;SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_recomp_zf=last;}
 else{SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;}g_esp+=12;
}
static void queue_insert_bucket(void){
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_seh_ebp);uint32_t node=MEM32(g_ecx+8);SET_LO8(g_eax,0);MEM32(g_esp+4)=node;
 if(node){
  g_eax=MEM32(node+4);g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);MEM32(g_ecx+8)=g_eax;MEM32(node)=g_edx;g_eax=MEM32(g_ecx+40);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
  if((int32_t)g_ebx>=(int32_t)g_eax)g_ebx=g_eax-1;if((int32_t)g_ebx<0)g_ebx=0;
  g_edx=MEM32(g_ecx+28);g_eax=g_ebx<<4;g_edx=MEM32(g_eax+g_edx);MEM32(node+4)=g_edx;g_edi=MEM32(g_ecx+28);g_edx=MEM32(g_eax+g_edi);g_esi=g_eax+g_edi;
  if(g_edx)MEM32(g_edx+8)=node;
  else{
   g_edx=g_ebx;if(g_ebx){uint32_t prior=g_esi;do{prior-=16;g_edx--;if(MEM32(prior))break;}while(g_edx);
    if(MEM32(prior)){g_edx<<=4;g_edi+=g_edx;MEM32(g_esi+8)=g_edi;g_esi=MEM32(g_ecx+28);g_edi=MEM32(g_edx+g_esi+4);MEM32(g_eax+g_esi+4)=g_edi;g_esi=MEM32(g_ecx+28);g_edi=MEM32(g_edx+g_esi+4);if(g_edi){g_esi+=g_eax;MEM32(g_edi+8)=g_esi;}g_esi=MEM32(g_ecx+28);g_edi=g_eax+g_esi;MEM32(g_edx+g_esi+4)=g_edi;}
    node=MEM32(g_esp+16);
   }
   g_edx=MEM32(g_ecx+32);int first=!g_edx;if(!first){g_edx=MEM32(g_ecx+36);first=(int32_t)g_ebx<(int32_t)g_edx;if(first){g_esi=MEM32(g_ecx+28);g_edi=g_eax+g_esi;g_edx<<=4;MEM32(g_edx+g_esi+8)=g_edi;g_esi=MEM32(g_ecx+36);g_edx=MEM32(g_ecx+28);g_esi=(g_esi<<4)+g_edx;MEM32(g_eax+g_edx+4)=g_esi;}}
   if(first){g_edx=MEM32(g_ecx+28)+g_eax;MEM32(g_ecx+36)=g_ebx;MEM32(g_ecx+32)=g_edx;}
  }
  g_edx=MEM32(g_ecx+28);POP32(g_esp,g_edi);POP32(g_esp,g_esi);int bucket_zero=!g_ebx;MEM32(g_eax+g_edx)=node;POP32(g_esp,g_ebx);
  if(bucket_zero){g_eax=MEM32(g_ecx+12);if(g_eax)MEM32(g_eax+8)=node;MEM32(g_ecx+12)=node;}SET_LO8(g_eax,1);
 }
 g_esp+=4;POP32(g_esp,g_ecx);g_esp+=12;
}
static void queue_remove_bucket(void){
 g_edx=MEM32(g_ecx+32);if(!g_edx){g_esp+=8;return;}PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_ebx);int found=0;
 for(;;){g_eax=MEM32(g_edx);while(g_eax){if(MEM32(g_eax)==g_esi){found=1;break;}g_eax=MEM32(g_eax+4);}if(found)break;g_edx=MEM32(g_edx+4);if(!g_edx)break;}
 if(!found){POP32(g_esp,g_ebx);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return;}
 g_esi=MEM32(g_eax+4);if(g_esi){g_ebx=MEM32(g_eax+8);MEM32(g_esi+8)=g_ebx;}g_esi=MEM32(g_eax+8);if(g_esi){g_ebx=MEM32(g_eax+4);MEM32(g_esi+4)=g_ebx;}
 if(g_eax==MEM32(g_edx)){g_esi=MEM32(g_eax+4);if(g_esi)MEM32(g_edx)=g_esi;else{
  MEM32(g_edx)=0;if(g_edx==MEM32(g_ecx+32)){g_esi=MEM32(g_edx+4);if(g_esi){g_esi=MEM32(g_esi+12);g_ebx=MEM32(g_ecx+28);MEM32(g_ecx+36)=g_esi;g_esi=(g_esi<<4)+g_ebx;MEM32(g_ecx+32)=g_esi;}else MEM32(g_ecx+32)=0;}
  g_esi=MEM32(g_edx+8);if(g_esi){g_ebx=MEM32(g_edx+4);MEM32(g_esi+4)=g_ebx;g_esi=MEM32(g_edx+4);if(g_esi){g_edx=MEM32(g_edx+8);MEM32(g_esi+8)=g_edx;}}
 }}
 if(g_eax==MEM32(g_ecx+12)){g_edx=MEM32(g_eax+4);MEM32(g_ecx+12)=g_edx;}MEM32(g_eax+8)=0;g_edx=MEM32(g_ecx+8);POP32(g_esp,g_ebx);MEM32(g_eax+4)=g_edx;MEM32(g_ecx+8)=g_eax;POP32(g_esp,g_esi);g_esp+=8;
}
int uc_migration_services_queue(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_QUEUE");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!queue_span(g_esp-32,48)||!queue_span(g_ecx,44))return 0;
 uint32_t p=g_ecx;if(!queue_nodes(MEM32(p+8)))return 0;
 if(entry==0x221a10){if(!queue_nodes(MEM32(p+12)))return 0;queue_remove_plain();return 1;}
 if(entry!=0x221ba0&&entry!=0x221ca0&&entry!=0x221d00)return 0;
 unsigned count=MEM32(p+40);uint32_t buckets=MEM32(p+28);if(!count||count>4096||!queue_span(buckets,count*16))return 0;
 for(unsigned i=0;i<count;i++){uint32_t b=buckets+i*16;if(!queue_nodes(MEM32(b)))return 0;for(unsigned j=4;j<=8;j+=4){uint32_t link=MEM32(b+j);if(link&&(link<buckets||link>=buckets+count*16||(link-buckets)%16))return 0;}}
 uint32_t active=MEM32(p+32);unsigned visited=0;while(active){if(++visited>count||active<buckets||active>=buckets+count*16||(active-buckets)%16)return 0;active=MEM32(active+4);}
 if(entry==0x221ba0)queue_insert_bucket();else if(entry==0x221ca0)queue_insert_after();else queue_remove_bucket();return 1;
}
