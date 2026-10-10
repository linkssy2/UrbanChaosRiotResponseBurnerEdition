#include "hybrid_debris_guest.h"
#include "hybrid_guest_cpu.h"
#include "xbox_memory_layout.h"
#include "../native/explosion_debris.h"
#include <stdlib.h>
static int enabled=-1;
static unsigned char *span(uint32_t address,unsigned bytes){
 if(address>=0x80000000u&&address<0x84000000u)address-=0x80000000u;
 if(address<0x10000u||(uint64_t)address+bytes>g_xbox_total_ram)return NULL;
 return (unsigned char*)XBOX_PTR(address);
}
int uc_hybrid_debris_bounce(uint32_t frame,uint32_t particle,uint32_t hit){
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_DEBRIS_BOUNCE");enabled=v&&!strcmp(v,"1");}
 if(!enabled)return 0;
 unsigned char*s=span(frame,0x2c),*p=span(particle,32),*h=span(hit,24);
 if(!s||!p||!h)return 0;
 HybridCPU cpu;save_cpu(&cpu);
 UCExplosionDebrisParticle value;float velocity[3],to[3],point[3],normal[3],delta;
 /* Full frame admission includes the source delta at+60, before any write. */
 if(!span(frame,0x64)){restore_cpu(&cpu);return 0;}
 memcpy(&value,p,32);memcpy(velocity,s+0x14,12);memcpy(to,s+0x20,12);
 memcpy(point,h,12);memcpy(normal,h+12,12);memcpy(&delta,s+0x60,4);
 uc_explosion_debris_resolve(&value,delta,velocity,to,point,normal);
 p[0x1c]=value.stopped;memcpy(s+0x14,value.velocity,12);memcpy(s+0x20,value.position,12);
 restore_cpu(&cpu);return 1;
}
