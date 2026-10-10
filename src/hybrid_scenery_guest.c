#include "hybrid_scenery_guest.h"
#include "hybrid_triangle_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/collision_mopp.h"
#include "../native/collision_hit.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint32_t physical(uint32_t p){return p>=0x80000000u&&p<0x84000000u?p-0x80000000u:p;}
static unsigned char*view(uint32_t p,unsigned n){
 uint32_t at=physical(p);return at>=0x10000u&&(uint64_t)at+n<=g_xbox_total_ram?(unsigned char*)XBOX_PTR(p):NULL;
}
static int separate(uint32_t a,unsigned n,uint32_t b,unsigned m){
 return (uint64_t)physical(a)+n<=physical(b)||(uint64_t)physical(b)+m<=physical(a);
}
static int readable(uint32_t p,unsigned n,uint32_t collector,uint32_t reserved,unsigned bytes){
 return view(p,n)&&separate(p,n,collector,0x48)&&separate(p,n,reserved,bytes);
}
typedef struct Query {
 uint32_t mesh,collector,reserved,reserved_bytes,packed,filtered;
 float from[4],to[4],axes[3][4];
 UCCollisionBodyMetadata root;
 UCCollisionClosest closest;
} Query;
#ifndef UC_SCENERY_OBSERVE
#define UC_SCENERY_OBSERVE(phase,key,state) ((void)0)
#endif
static UCCollisionStatus terminal(void*opaque,uint32_t key,float*fraction){
 Query*q=(Query*)opaque;UCHybridMeshMaterial material={0};UCCollisionMeshChild child;UCCollisionTriangleHit hit,world;
 UC_SCENERY_OBSERVE(0,key,&q->closest);
 if(q->filtered){
  if(!uc_hybrid_mesh_material_read_checked(q->mesh,key,q->collector,0x48,&material)||
     !uc_hybrid_mesh_material_read_checked(q->mesh,key,q->reserved,q->reserved_bytes,&material))return UC_COLLISION_UNSUPPORTED;
  if(!uc_collision_terminal_filter(16,q->packed,material.material))return UC_COLLISION_OK;
 }
 if(!uc_hybrid_mesh_child_read_checked(q->mesh,key,q->collector,0x48,&child)||
    !uc_hybrid_mesh_child_read_checked(q->mesh,key,q->reserved,q->reserved_bytes,&child))return UC_COLLISION_UNSUPPORTED;
 int accepted=uc_collision_triangle_cast_control(child.vertices,q->from,q->to,q->closest.limit,&hit);
 if(accepted<0)return UC_COLLISION_UNSUPPORTED;if(!accepted)return UC_COLLISION_OK;
 if(uc_collision_triangle_world_hit(&hit,q->axes,key,&world)!=1)return UC_COLLISION_UNSUPPORTED;
 UCCollisionBodyMetadata ancestry[2]={{&child,6,key,0,0},q->root};UCCollisionHitRoute route;
 accepted=uc_collision_hit_route(&q->closest,&world,ancestry,2,&route);
 if(accepted<0)return UC_COLLISION_UNSUPPORTED;
 if(accepted){
  if(!q->filtered&&(!uc_hybrid_mesh_material_read_checked(q->mesh,key,q->collector,0x48,&material)||
      !uc_hybrid_mesh_material_read_checked(q->mesh,key,q->reserved,q->reserved_bytes,&material)))return UC_COLLISION_UNSUPPORTED;
  if(uc_collision_hit_commit(&q->closest,&route,material.material)!=1)return UC_COLLISION_UNSUPPORTED;
  UC_SCENERY_OBSERVE(1,key,&q->closest);
 }
 *fraction=q->closest.limit;return UC_COLLISION_OK;
}
static int query(uint32_t context,uint32_t stack,uint32_t frame){
 /* Worker-disabled is an explicit title configuration guard, not a lock or
  * a guarantee that a previously created worker has stopped. The pinned title
  * initializes this flag to zero and its sole discovered setter passes zero. */
 if(!view(0x51c178,1)||MEM8(0x51c178))return 0;
 if(g_fp_control_word!=0x37Fu||context!=g_ecx||stack!=g_esp||frame!=g_ebp||
    !view(frame,28)||!view(stack,12)||!view(context,0x68)||MEM32(frame+4)!=0x303ffcu||
    !view(0x51f5e2,1)||!MEM8(0x51f5e2)||!view(0x520208,8)||MEM32(0x520208)||MEM32(0x52020c))return 0;
 uint32_t shape_frame=MEM32(frame);
 if(!view(shape_frame,20)||MEM32(shape_frame+4)!=0x319002u||context!=(shape_frame&~15u)-0x70u)return 0;
 uint32_t body_frame=MEM32(shape_frame);
 if(!view(body_frame,16))return 0;
 uint32_t caller=MEM32(body_frame+4);
 if(caller!=0x3264d1u&&caller!=0x326541u&&caller!=0x3265b1u&&caller!=0x326621u&&caller!=0x32698fu)return 0;
 uint32_t sap_frame=MEM32(body_frame);
 if(!view(sap_frame,20)||sap_frame<stack||sap_frame-stack>0x10000u)return 0;
 if(physical(stack)<0x42000u)return 0;
 /*256 bounded native recursive levels,0x310 original bytes per level, plus
  * root/leaf frames. Exclude reads of original discarded stack as well as writes. */
 uint32_t reserved=stack-0x32000u,reserved_bytes=sap_frame+20-reserved;
 if(!view(reserved,reserved_bytes))return 0;
 uint32_t collector=MEM32(context+0x5c),body=MEM32(context+0x60),mesh=MEM32(context+0x64),code=MEM32(context+0x10);
 if(!view(collector,0x48)||!separate(collector,0x48,reserved,reserved_bytes)||MEM32(collector)!=0x48de08u||
    MEM32(context+0x58)||!readable(body,32,collector,reserved,reserved_bytes)||MEM32(body+12)||
    !readable(mesh,64,collector,reserved,reserved_bytes)||MEM32(mesh)!=0x49c3c8u||
    !readable(code,48,collector,reserved,reserved_bytes)||MEM32(code)!=0x49deacu)return 0;
 uint32_t shape=MEM32(body),matrix=MEM32(body+8),filter=MEM32(context+0x44);
 if(!readable(shape,24,collector,reserved,reserved_bytes)||MEM32(shape)!=0x49c4b4u||MEM32(shape+12)!=mesh||MEM32(shape+16)!=code||
    !readable(matrix,48,collector,reserved,reserved_bytes)||!separate(collector,0x48,0x520208,8)||
    !separate(collector,0x48,0x51f5e2,1)||!separate(collector,0x48,0x51c178,1))return 0;
 const uint32_t tables[][3]={{0x49c3c8,8,0x9d970},{0x49c3c8,0x28,0x304350},{0x49c3c8,0x2c,0x3042e0},
  {0x49c33c,8,0x302660},{0x49c33c,0x14,0x3027c0},{0x49c33c,0x18,0x3025d0},
  {0x49c4b4,8,0x24cb50},{0x49c4b4,0x18,0x303fa0},{0x48de08,0,0x2a8d00}};
 for(unsigned i=0;i<sizeof(tables)/sizeof(tables[0]);i++){
  uint32_t at=tables[i][0]+tables[i][1];if(!readable(at,4,collector,reserved,reserved_bytes)||MEM32(at)!=tables[i][2])return 0;
 }
 if(filter&&(!readable(filter,4,collector,reserved,reserved_bytes)||!readable(MEM32(filter),4,collector,reserved,reserved_bytes)||MEM32(MEM32(filter))!=0x2a8800u))return 0;
 /* The original interpreter reads its guest dispatch tables on every opcode. */
 static const unsigned char dispatch_table[236]={
 0x88,0xce,0x32,0x00,0x19,0xcb,0x32,0x00,0xd7,0xca,0x32,0x00,0xe4,0xca,0x32,0x00,
 0xfa,0xca,0x32,0x00,0xea,0xcb,0x32,0x00,0x16,0xcc,0x32,0x00,0x4b,0xcc,0x32,0x00,
 0x01,0xc5,0x32,0x00,0x22,0xc2,0x32,0x00,0x57,0xc2,0x32,0x00,0x96,0xc2,0x32,0x00,
 0xca,0xc2,0x32,0x00,0x08,0xc3,0x32,0x00,0x3c,0xc3,0x32,0x00,0x7a,0xc3,0x32,0x00,
 0xb6,0xc3,0x32,0x00,0xf8,0xc3,0x32,0x00,0x3a,0xc4,0x32,0x00,0x80,0xc4,0x32,0x00,
 0xb0,0xc4,0x32,0x00,0x02,0xc9,0x32,0x00,0x9d,0xc8,0x32,0x00,0xd4,0xcd,0x32,0x00,
 0x85,0xcd,0x32,0x00,0x8b,0xcd,0x32,0x00,0x9a,0xcd,0x32,0x00,0xb2,0xcd,0x32,0x00,
 0x92,0xcc,0x32,0x00,0xa2,0xcc,0x32,0x00,0xbb,0xcc,0x32,0x00,0x20,0xcd,0x32,0x00,
 0x00,0x01,0x01,0x01,0x01,0x02,0x03,0x04,0x1f,0x05,0x06,0x07,0x1f,0x1f,0x1f,0x1f,
 0x08,0x08,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x1f,0x1f,0x1f,
 0x13,0x13,0x13,0x14,0x14,0x14,0x15,0x15,0x15,0x16,0x16,0x16,0x1f,0x1f,0x1f,0x1f,
 0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
 0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
 0x18,0x19,0x1a,0x1b,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
 0x1c,0x1c,0x1c,0x1c,0x1d,0x1d,0x1d,0x1d,0x1e,0x1e,0x1e,0x1e,
 };
 if(!readable(0x32ce94,sizeof dispatch_table,collector,reserved,reserved_bytes)||
    memcmp(view(0x32ce94,sizeof dispatch_table),dispatch_table,sizeof dispatch_table))return 0;
 const uint32_t constants[][2]={{0x44bf94,0x3f800000},{0x44c174,0},{0x44ff9c,0xb8d1b717},{0x49e2d8,0x37800000},
  {0x48ddf0,0x3f800000},{0x48ddf4,0x3f800000},{0x48ddf8,0x3f800000},{0x48ddfc,0x3f800000},
  {0x49b790,0x80000000},{0x49b794,0x80000000},{0x49b798,0x80000000},{0x49b79c,0x80000000}};
 for(unsigned i=0;i<sizeof(constants)/sizeof(constants[0]);i++)
  if(!readable(constants[i][0],4,collector,reserved,reserved_bytes)||MEM32(constants[i][0])!=constants[i][1])return 0;
 /* 3027C0 cross-product shuffles C9/D2 never move W into XYZ; every dot
  * reduction selects only X,Y,Z. Its initial normal W comes from mesh W=0.
  * Ray W affects only discarded SIMD/scratch.318EC0 can duplicate Z into W. */
 if((MEM32(context+0x2c)&0x7f800000u)==0x7f800000u||(MEM32(context+0x3c)&0x7f800000u)==0x7f800000u)return 0;
 uint32_t data=MEM32(code+0x20),size=MEM32(code+0x24),capacity=MEM32(code+0x28)&0x3fffffffu;
 if(!size||size>64u*1024u*1024u||size>capacity||!readable(data,size,collector,reserved,reserved_bytes))return 0;
 Query q={0};q.mesh=mesh;q.collector=collector;q.reserved=reserved;q.reserved_bytes=reserved_bytes;q.packed=MEM32(context+0x40);q.filtered=!!filter;
 memcpy(q.from,view(context+0x20,16),16);memcpy(q.to,view(context+0x30,16),16);memcpy(q.axes,view(matrix,48),48);
 q.root=(UCCollisionBodyMetadata){(const void*)(uintptr_t)body,24,MEM32(body+4),MEM32(shape+8),MEM32(body+0x1c)};
 memcpy(&q.closest.limit,view(collector+4,4),4);memcpy(&q.closest.hit,view(collector+0x10,24),24);
 q.closest.root_body=(const void*)(uintptr_t)MEM32(collector+0x30);q.closest.material=MEM32(collector+0x40);q.closest.filter_info=MEM32(collector+0x44);
 uint32_t old_fraction=MEM32(collector+0x24)&0x7fffffffu;
 if(old_fraction&&old_fraction<0x800000u)return 0;
 if(!isfinite(q.closest.hit.fraction)||q.closest.hit.fraction<0||q.closest.hit.fraction>1)return 0;
 UCCollisionMopp m={view(data,size),size,{0}};memcpy(m.info,view(code+0x10,16),16);
 float fraction=q.closest.limit;
 UCCollisionStatus status=uc_collision_mopp_cast_borrowed_staged(&m,q.from,q.to,&fraction,terminal,&q);
 if(status)return 0;
 /* Known getters are pure and read spans exclude these writes; no guest call,
  * allocator, pump or yield occurs while the private hit state is evaluated. */
 if(q.closest.hit.fraction<MEMF(collector+0x24)){
  memcpy(view(collector+0x10,24),&q.closest.hit,24);MEM32(collector+0x30)=(uint32_t)(uintptr_t)q.closest.root_body;
  MEMF(collector+4)=q.closest.limit;MEM32(collector+0x40)=q.closest.material;MEM32(collector+0x44)=q.closest.filter_info;
 }
 MEMF(context+0x54)=q.closest.limit;g_esp+=12;return 1;
}

int uc_hybrid_scenery_query(uint32_t context,uint32_t stack,uint32_t frame){
 static int enabled=-1;
 if(enabled<0){const char*flag=getenv("UCRR_NATIVE_SCENERY_QUERY");enabled=flag&&!strcmp(flag,"1");}
 if(!enabled)return 0;
 unsigned csr=_mm_getcsr();
 int result=query(context,stack,frame);
 _mm_setcsr(csr);return result;
}
