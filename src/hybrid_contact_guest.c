#include "hybrid_contact_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int enabled(void){
 static int flag=-1;
 if(flag<0){const char*v=getenv("UCRR_NATIVE_CONTACT_PREP");flag=v&&!strcmp(v,"1");}
 return flag;
}
static uint32_t physical(uint32_t p){return p>=0x80000000u&&p<0x84000000u?p-0x80000000u:p;}
static int readable(uint32_t p,uint32_t n){uint32_t a=physical(p);return a>=0x10000u&&(uint64_t)a+n<=g_xbox_total_ram;}
static int separate(uint32_t a,uint32_t n,uint32_t b,uint32_t m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}

int uc_hybrid_contact_publish(uint32_t frame,uint32_t stack,uint32_t stream){
 if(!enabled()||stack!=g_esp||stream!=g_edi||!readable(frame,32)||!readable(stack,0x70)||
    !readable(stream,24))return 0;
 uint32_t blocks=MEM32(stream),count=MEM32(stream+4),used=MEM32(stream+16);
 uint32_t stride=MEM16(stream+20),block_bytes=MEM16(stream+22);
 if(!count||count>16384||count>(MEM32(stream+8)&0x3fffffffu)||stride<32||used>block_bytes||stride>block_bytes-used)return 0;
 uint32_t last=blocks+(count-1)*4;
 if(last<blocks||!readable(last,4))return 0;
 uint32_t block=MEM32(last),agent=block+used,a=MEM32(frame+12),b=MEM32(frame+16);
 if(agent<block||!readable(block,block_bytes)||!readable(a,48)||!readable(b,48))return 0;
 uint32_t an=MEM32(a+40),bn=MEM32(b+40),ac=MEM32(a+44)&0x3fffffffu,bc=MEM32(b+44)&0x3fffffffu;
 if(an>65535||bn>65535||an>=ac||bn>=bc)return 0;
 uint32_t al=MEM32(a+36),bl=MEM32(b+36),as=al+an*8,bs=bl+bn*8;
 if(as<al||bs<bl||!readable(as,8)||!readable(bs,8))return 0;
 /* Every source reload skipped below is stable until its original use. This
  * excludes conflicting source/write aliases, including identical new slots
  * and physical high/low address aliases.
  * The frame and local scratch are separate in the original aligned frame. */
 const uint32_t spans[][2]={{physical(frame),32},{physical(stack+20),8},{physical(stream),24},{physical(last),4},{physical(a),48},{physical(b),48},{physical(agent),32},{physical(as),8},{physical(bs),8}};
 for(unsigned i=0;i<sizeof(spans)/sizeof(spans[0]);i++)
  for(unsigned j=0;j<i;j++)if(!separate(spans[i][0],spans[i][1],spans[j][0],spans[j][1]))return 0;
 uint32_t manager=MEM32(frame+28),config=MEM32(frame+24);
 /* All admission precedes publication. Original growth, matrix preparation,
  * registered constructor and its live stride reload remain in the caller. */
 MEM32(stream+16)=used+stride;
 MEM32(agent+16)=manager;MEM32(agent+20)=a;MEM32(agent+24)=b;
 MEM16(agent+12)=(uint16_t)an;MEM16(agent+14)=(uint16_t)bn;
 MEM32(a+40)=an+1;MEM32(b+40)=bn+1;
 MEM32(stack+20)=as;MEM32(stack+24)=bs;
 MEM32(as)=agent;MEM32(bs)=agent;MEM32(as+4)=b;MEM32(bs+4)=a;
 g_eax=a;g_ebx=b;g_esi=agent;g_ecx=bs;g_edx=config;
 return 1;
}
