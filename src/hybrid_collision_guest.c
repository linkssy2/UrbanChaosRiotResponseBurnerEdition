#include "hybrid_collision_guest.h"
#include "hybrid_triangle_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/collision_hit.h"
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
static int filter_span(uint32_t at,unsigned bytes){
 if(at>=0x80000000u&&at<0x84000000u)at-=0x80000000u;
 return at>=0x10000u&&(uint64_t)at+bytes<=g_xbox_total_ram;
}
int uc_hybrid_collision_filter(uint32_t stack){
 static int enabled=-1;
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_MESH_FILTER");enabled=v&&!strcmp(v,"1");}
 if(!enabled||stack!=g_esp||!filter_span(stack-20,40))return 0;
 uint32_t physical=stack>=0x80000000u?stack-0x80000000u:stack;
 if(physical-20<0x49c3f8u&&physical>0x49c3c8u)return 0;
 uint32_t out=MEM32(stack+4),ray=MEM32(stack+8),mesh=MEM32(stack+12),key=MEM32(stack+16);
 if(!filter_span(out,1)||!filter_span(ray,36)||!filter_span(mesh,64)||MEM32(mesh)!=0x49c3c8u)return 0;
 if(!filter_span(0x49c3c8u,48)||MEM32(0x49c3d0u)!=0x9d970u||MEM32(0x49c3f4u)!=0x3042e0u)return 0;
 UCHybridMeshMaterial read;
 if(!uc_hybrid_mesh_material_read(mesh,key,stack-20,20,&read))return 0;
 /* The admitted constant getters have no effects remaining after these exact
  * original filter/material stack writes. Read the ray only after publication. */
 MEM32(stack-4)=g_esi;MEM32(stack-8)=key;MEM32(stack-12)=0x2a8840u;
 MEM32(stack-16)=mesh;MEM32(stack-20)=g_edi;
 uint32_t packed=MEM32(ray+32);
 int accepted=uc_collision_terminal_filter(16,packed,read.material);
 g_eax=out;g_ecx=ray;g_edx=(packed>>20)&read.material;
 MEM8(out)=(uint8_t)accepted;g_esi=MEM32(stack-4);g_esp+=20;
 if(!accepted)g_recomp_zf=1;
 return 1;
}

static uint32_t physical_address(uint32_t at){
 return at>=0x80000000u&&at<0x84000000u?at-0x80000000u:at;
}
static int collector_overlap(uint32_t a,unsigned na,uint32_t b,unsigned nb){
 uint64_t pa=physical_address(a),pb=physical_address(b);
 return pa<pb+nb&&pb<pa+na;
}
static int collector_source(uint32_t at,unsigned bytes,uint32_t out,uint32_t stack){
 return filter_span(at,bytes)&&!collector_overlap(at,bytes,out,72)&&
        !collector_overlap(at,bytes,stack-28,28);
}
/* Normal finite fractions in[0,1], plus signed zero. Subnormals/NaNs decline:
 * their host FP exception/DAZ behavior belongs to the original comparison. */
static int collector_fraction(uint32_t bits){
 uint32_t magnitude=bits&0x7fffffffu;
 return !magnitude||(!(bits&0x80000000u)&&magnitude>=0x00800000u&&magnitude<=0x3f800000u);
}
int uc_hybrid_collision_collector(uint32_t stack){
 static int enabled=-1;
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_MESH_COLLECTOR");enabled=v&&!strcmp(v,"1");}
 if(!enabled||stack!=g_esp||!filter_span(stack-28,40))return 0;
 uint32_t out=g_ecx,hit=MEM32(stack+8);
 if(!filter_span(out,72)||collector_overlap(out,72,stack-28,40)||
    !collector_source(hit,24,out,stack))return 0;
 uint32_t old_fraction=MEM32(out+36),new_fraction=MEM32(hit+20);
 if(!collector_fraction(old_fraction)||!collector_fraction(new_fraction))return 0;
 /* The original miss never reads body ancestry or calls a getter. In this
  * validated nonnegative normal/zero domain, magnitude bits order exactly as
  * the original comparison, without touching host FP status. */
 if((old_fraction&0x7fffffffu)<=(new_fraction&0x7fffffffu)){
  MEM32(stack-4)=g_edi;g_ecx=hit;
  memset(&g_xmm0,0,sizeof g_xmm0);g_xmm0.u[0]=old_fraction;g_esp+=12;return 1;
 }
 uint32_t leaf=MEM32(stack+4);
 if(!collector_source(leaf,16,out,stack))return 0;
 uint32_t triangle=MEM32(leaf),root=MEM32(leaf+12);
 if(!collector_source(triangle,4,out,stack)||MEM32(triangle)!=0x49c33cu||
    !collector_source(root,32,out,stack)||MEM32(root+12))return 0;
 uint32_t shape=MEM32(root);
 if(!collector_source(shape,16,out,stack)||MEM32(shape)!=0x49c4b4u)return 0;
 uint32_t mesh=MEM32(shape+12),key=MEM32(leaf+4);
 if(!collector_source(mesh,64,out,stack)||MEM32(mesh)!=0x49c3c8u||
    !collector_source(0x49c33cu,12,out,stack)||MEM32(0x49c344u)!=0x302660u||
    !collector_source(0x49c4b4u,12,out,stack)||MEM32(0x49c4bcu)!=0x24cb50u||
    !collector_source(0x49c3c8u,48,out,stack)||MEM32(0x49c3f4u)!=0x3042e0u)return 0;
 UCHybridMeshMaterial material;
 /* Both reservations cover every material read, including indices/payload.
  * No guest writes occur during these independently gated pure reads. */
 if(!uc_hybrid_mesh_material_read_checked(mesh,key,stack-28,28,&material)||
    !uc_hybrid_mesh_material_read_checked(mesh,key,out,72,&material))return 0;
 UCCollisionClosest current={0};UCCollisionTriangleHit incoming;UCCollisionHitRoute route;
 UCCollisionBodyMetadata ancestry[2]={{(const void*)(uintptr_t)leaf,6,key,0,0},
  {(const void*)(uintptr_t)root,24,0,0,MEM32(root+28)}};
 memcpy(&current.hit.fraction,&old_fraction,4);memcpy(&incoming,(const void*)XBOX_PTR(hit),24);
 unsigned csr=_mm_getcsr();
 int status=uc_collision_hit_route(&current,&incoming,ancestry,2,&route);
 _mm_setcsr(csr);
 if(status!=1)return 0;
 /* Successful routing validated the unchanged private input. There is no
  * intervening callback or publication requiring a second closest-hit check. */
 /* All mutable aliases were excluded above. Reproduce the retained raw stack
  * and volatile CPU results, including the pure getter calls' scratch. */
 MEM32(stack-4)=g_edi;
 MEM32(stack-8)=g_esi;MEM32(stack-12)=g_ebx;MEM32(stack-16)=key;
 MEM32(stack-20)=0x2a8dffu;MEM32(stack-24)=leaf;MEM32(stack-28)=out;
 memcpy((void*)XBOX_PTR(out+16),&route.hit,24);MEM32(out+4)=new_fraction;
 MEM32(out+48)=root;MEM32(out+64)=material.material;MEM32(out+68)=route.filter_info;
 memcpy(&g_xmm0,&incoming,16);g_eax=route.filter_info;g_ecx=0;g_edx=material.offset;
 g_recomp_zf=1;g_esp+=12;return 1;
}
