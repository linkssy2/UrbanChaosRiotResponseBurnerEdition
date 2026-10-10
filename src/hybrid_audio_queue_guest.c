#include "hybrid_audio_queue_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_NATIVE_AUDIO_QUEUE");f=v&&!strcmp(v,"1");}return f;}
static uint32_t phys(uint32_t p){return p>=0x80000000u&&p<0x84000000u?p-0x80000000u:p;}
static int range(uint32_t p,uint32_t n){return phys(p)>=0x10000u&&(uint64_t)phys(p)+n<=g_xbox_total_ram;}
static int apart(uint32_t a,uint32_t n,uint32_t b,uint32_t m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
/* At most sixteen heap levels, including the upward selected-removal path and
 * both child candidates on the following downward path. These bounded local
 * records preserve guest identities; there is no copied owner or retained cache. */
typedef struct Command{uint32_t p,a,index,dirty;uint64_t time;}Command;
typedef struct Slot{uint32_t i,p,dirty;}Slot;
typedef struct Word{uint32_t p,value;}Word;
typedef struct Plan{
 uint32_t q,array,capacity,count,offset,stack,qa,aa,sa;
 uint32_t nc,ns,nw,eax,ecx,edx,esp;int zf;
 Command commands[64];Slot slots[64];Word words[16];
}Plan;
static int word(Plan*p,uint32_t a,uint32_t v){
 for(unsigned i=0;i<p->nw;i++)if(p->words[i].p==a){p->words[i].value=v;return 1;}
 if(p->nw==16)return 0;p->words[p->nw++]=(Word){a,v};return 1;
}
static Slot*slot(Plan*p,uint32_t i){
 if(!i||i>p->capacity)return NULL;
 for(unsigned j=0;j<p->ns;j++)if(p->slots[j].i==i)return &p->slots[j];
 if(p->ns==64)return NULL;Slot*s=&p->slots[p->ns++];*s=(Slot){i,MEM32(p->array+4*(i-1)),0};return s;
}
static Command*command(Plan*p,uint32_t a){
 uint32_t b=phys(a);
 for(unsigned i=0;i<p->nc;i++){Command*c=&p->commands[i];if(c->p==a)return c;if(!apart(b,0x5c,c->a,0x5c))return NULL;}
 if(p->nc==64||!range(a,0x5c)||!apart(b,0x5c,p->qa,12)||!apart(b,0x5c,p->aa,p->capacity*4)||!apart(b,0x5c,p->sa,52)||!apart(b,0x5c,0x51c178,1))return NULL;
 Command*c=&p->commands[p->nc++];c->p=a;c->a=b;c->index=MEM16(a+p->offset);c->dirty=0;
 c->time=(uint64_t)MEM32(a+8)|((uint64_t)MEM32(a+12)<<32);
 if(c->time>INT64_MAX)return NULL;return c;
}
static Command*at(Plan*p,uint32_t i){Slot*s=slot(p,i);if(!s)return NULL;Command*c=command(p,s->p);return c&&c->index==i?c:NULL;}
static int put(Plan*p,uint32_t i,Command*c){Slot*s=slot(p,i);if(!s)return 0;s->p=c?c->p:0;s->dirty=1;if(c){c->index=i;c->dirty=1;}return 1;}
static int swap(Plan*p,uint32_t a,uint32_t b){Command*x=at(p,a),*y=at(p,b);if(!x||!y)return 0;return put(p,a,y)&&put(p,b,x);}
static int init(Plan*p,uint32_t offset){
 p->q=g_ecx;p->stack=g_esp;p->qa=phys(p->q);p->sa=phys(p->stack-44);p->offset=offset;
 p->nc=p->ns=p->nw=0;p->eax=g_eax;p->ecx=g_ecx;p->edx=g_edx;p->esp=g_esp;p->zf=g_recomp_zf;
 if(!range(p->q,12)||!range(p->stack-44,52))return 0;
 p->array=MEM32(p->q);p->aa=phys(p->array);p->capacity=MEM32(p->q+4);p->count=MEM32(p->q+8);
 if(!p->capacity||p->capacity>65535||p->count>p->capacity||!range(p->array,p->capacity*4))return 0;
 return apart(p->qa,12,0x51c178,1)&&apart(p->aa,p->capacity*4,0x51c178,1)&&apart(p->sa,52,0x51c178,1)&&apart(p->qa,12,p->aa,p->capacity*4)&&apart(p->qa,12,p->sa,52)&&apart(p->aa,p->capacity*4,p->sa,52);
}
static int pop(Plan*p,uint32_t s,uint32_t ebx,uint32_t esi,uint32_t edi){
 uint32_t old=p->count;p->eax=old;p->esp=s+4;p->zf=s==0;if(!old)return 1;
 Command*root=at(p,1);if(!root||!word(p,s-16,esi)||!word(p,s-12,root->p))return 0;
 if(old!=1){Command*last=at(p,old);if(!last||!put(p,1,last)||!put(p,old,NULL))return 0;}
 uint32_t i=1;
 if(old>2){
  if(!word(p,s-20,ebx)||!word(p,s-24,g_seh_ebp)||!word(p,s-28,edi))return 0;
  do{
   uint32_t j=i*2;Command*left=at(p,j);if(!left)return 0;
   if(j+1<old){Command*right=at(p,j+1);if(!right)return 0;if(left->time>=right->time)++j;}
   Command*cur=at(p,i),*child=at(p,j);if(!cur||!child||!word(p,s-4,(uint32_t)((cur->time-child->time)>>32)))return 0;
   if(cur->time<=child->time)break;if(!swap(p,i,j))return 0;i=j;
  }while(i*2<old);
 }
 root->index=0;root->dirty=1;p->count=old-1;p->eax=root->p;p->edx=i;return 1;
}
static int push(Plan*p,int ready){
 uint32_t n=p->count,s=p->stack;
 p->eax=n;p->esp=s+8;
 if(!ready)p->edx=p->capacity;
 if(n==p->capacity){p->eax&=0xffffff00u;p->zf=ready?1:s==0;return 1;}
 Command*c=command(p,MEM32(s+4));if(!c||c->index)return 0;
 ++n;if(!put(p,n,c))return 0;p->count=n;
 if(ready){p->edx=p->array;p->ecx=c->p;p->eax=(n&0xffffff00u)|1;p->zf=0;return 1;}
 if(!word(p,s-12,g_esi))return 0;p->eax=c->p;p->edx=n;p->zf=s==0;
 if(n>1){
  if(!word(p,s-16,g_ebx)||!word(p,s-20,g_seh_ebp)||!word(p,s-24,g_edi))return 0;
  uint32_t i=n;
  do{uint32_t j=i/2;Command*parent=at(p,j);if(!parent)return 0;p->eax=j;
   if(!word(p,s-4,(uint32_t)((parent->time-c->time)>>32)))return 0;
   if(parent->time<=c->time)break;if(!swap(p,i,j))return 0;i=j;p->edx=i;
  }while(i>1);
 }
 p->eax=(p->eax&0xffffff00u)|1;return 1;
}
static int remove_selected(Plan*p,int ready){
 uint32_t s=p->stack,a=MEM32(s+4);Command*c=command(p,a);if(!c)return 0;uint32_t i=c->index;
 p->eax=(a&0xffff0000u)|i;p->esp=s+8;
 if(!i){p->eax&=0xffffff00u;p->zf=1;return 1;}
 if(i>p->count||at(p,i)!=c)return 0;
 if(ready){
  if(!word(p,s-4,g_esi)||!word(p,s-8,g_edi)||!swap(p,1,i))return 0;
  uint32_t n=p->count;
  if(n!=1){Command*last=at(p,n);if(!last||!put(p,1,last)||!put(p,n,NULL))return 0;}
  p->count=n-1;c->index=0;c->dirty=1;p->eax=1;p->edx=n;return 1;
 }
 if(i>1){
  if(!word(p,s-4,g_ebx)||!word(p,s-8,g_esi)||!word(p,s-12,g_edi))return 0;
  do{if(!swap(p,i,i/2))return 0;i/=2;}while(i>1);
 }
 if(!word(p,s-4,0x244fcf)||!pop(p,s-4,g_ebx,g_esi,g_edi))return 0;
 p->eax=(p->eax&0xffffff00u)|1;p->esp=s+8;return 1;
}
static int due(Plan*p){
 uint32_t s=p->stack;p->eax=0;p->edx=p->count;p->esp=s+8;if(!p->count)return 1;
 Command*c=at(p,1);uint32_t limit=MEM32(s+4),la=phys(limit);
 if(!c||!range(limit,8)||!apart(la,8,p->sa,52))return 0;
 uint64_t t=(uint64_t)MEM32(limit)|((uint64_t)MEM32(limit+4)<<32);if(t>INT64_MAX)return 0;
 if(!word(p,s-4,g_ebx)||!word(p,s-8,g_esi)||!word(p,s-12,g_edi))return 0;
 p->edx=(uint32_t)(c->time>>32)==(uint32_t)(t>>32)?(uint32_t)c->time:c->p;
 if(c->time<=t){if(!word(p,s-16,0x24500c)||!pop(p,s-16,(uint32_t)(t>>32),limit,(uint32_t)(c->time>>32)))return 0;p->esp=s+8;}
 return 1;
}
int uc_hybrid_audio_queue(uint32_t address){
 /* Pinned title startup disables guest workers. This qualifies the supported
  * mode; it is not a lock or a way to stop already launched workers. */
 if(!enabled()||!range(0x51c178,1)||MEM8(0x51c178))return 0;
 Plan p;if(!init(&p,address==0x244b80||address==0x245020?0x5a:0x58))return 0;
 int ok;
 switch(address){
 case 0x244ab0:ok=push(&p,0);break;case 0x244b80:ok=push(&p,1);break;
 case 0x2449e0:ok=pop(&p,p.stack,g_ebx,g_esi,g_edi);break;
 case 0x244f80:ok=remove_selected(&p,0);break;case 0x245020:ok=remove_selected(&p,1);break;
 case 0x244fe0:ok=due(&p);break;default:return 0;
 }
 if(!ok)return 0;
 /* No callbacks, allocation or further admission after this point. Source and
  * output spans are disjoint; preserve stale slots and all retired stack words. */
 for(unsigned i=0;i<p.nw;i++)MEM32(p.words[i].p)=p.words[i].value;
 for(unsigned i=0;i<p.ns;i++)if(p.slots[i].dirty)MEM32(p.array+4*(p.slots[i].i-1))=p.slots[i].p;
 for(unsigned i=0;i<p.nc;i++)if(p.commands[i].dirty)MEM16(p.commands[i].p+p.offset)=(uint16_t)p.commands[i].index;
 MEM32(p.q+8)=p.count;g_eax=p.eax;g_ecx=p.ecx;g_edx=p.edx;g_esp=p.esp;g_recomp_zf=p.zf;
 return 1;
}
