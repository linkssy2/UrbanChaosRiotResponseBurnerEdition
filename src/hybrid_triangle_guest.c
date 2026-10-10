#include "hybrid_triangle_guest.h"
#include "hybrid_guest_cpu.h"
#include "xbox_memory_layout.h"
#include "../native/collision_triangle.h"
#include "../native/collision_mesh.h"
#include <stdlib.h>
static int enabled=-1;
static unsigned char*span(uint32_t address,unsigned bytes){
 if(address>=0x80000000u&&address<0x84000000u)address-=0x80000000u;
 if(address<0x10000u||(uint64_t)address+bytes>g_xbox_total_ram)return NULL;
 return (unsigned char*)XBOX_PTR(address);
}
static uint32_t word(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static int overlaps(const unsigned char*a,unsigned na,const unsigned char*b,unsigned nb){
 return a<b+nb&&b<a+na;
}
int uc_hybrid_triangle_cast(uint32_t shape,uint32_t ray,uint32_t hit){
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_TRIANGLE_CAST");enabled=v&&!strcmp(v,"1");}
 if(!enabled)return 0;
 unsigned char*s=span(shape,64),*r=span(ray,32),*h=span(hit,24),*profile=span(0x520208,8);
 if(!s||!r||!h||!profile||word(profile)||word(profile+4)||word(s)!=0x49C33Cu)return 0;
 if(overlaps(h,24,s,64)||overlaps(h,24,r,32))return 0;
 for(unsigned i=0;i<3;i++)if(word(s+0x1c+i*16))return 0;
 if(word(r+12)||word(r+28))return 0;
 /* Pure kernel never touches guest registers; only host FP status can change. */
 unsigned csr=_mm_getcsr();
 float vertices[3][3],from[3],to[3],limit;UCCollisionTriangleHit result;
 for(unsigned i=0;i<3;i++)memcpy(vertices[i],s+0x10+i*16,12);
 memcpy(from,r,12);memcpy(to,r+16,12);memcpy(&limit,h+20,4);
 int status=uc_collision_triangle_cast_control(vertices,from,to,limit,&result);
 _mm_setcsr(csr);
 if(status<0)return 0;
 if(status)memcpy(h,&result,sizeof result);
 return status+1;
}

int uc_hybrid_triangle_world_hit(uint32_t body,uint32_t hit){
 if(enabled<0){const char*v=getenv("UCRR_NATIVE_TRIANGLE_CAST");enabled=v&&!strcmp(v,"1");}
 if(!enabled)return 0;
 unsigned char*b=span(body,12),*h=span(hit,24);
 if(!b||!h||overlaps(h,24,b,12))return 0;
 unsigned char*m=span(word(b+8),48);
 if(!m||overlaps(h,24,m,48))return 0;
 float axes[3][4];UCCollisionTriangleHit local,result;
 memcpy(axes,m,48);memcpy(&local,h,24);
 unsigned csr=_mm_getcsr();
 int status=uc_collision_triangle_world_hit(&local,axes,word(b+4),&result);
 _mm_setcsr(csr);
 if(status!=1)return 0;
 memcpy(h,&result,24);return 1;
}

static unsigned char*mesh_part(const unsigned char*m,uint32_t key,uint32_t*local){
 uint32_t bits=word(m+32);if(!bits||bits>=32)return NULL;
 uint32_t part=key>>(32-bits);*local=key&(UINT32_MAX>>bits);
 if(part>=word(m+40))return NULL;
 uint64_t at=(uint64_t)word(m+36)+part*48ull;if(at>UINT32_MAX)return NULL;
 unsigned char*p=span((uint32_t)at,48);
 return p&&*local<word(p+24)?p:NULL;
}
/* Read live mesh data without a second asset cache or a separate physics world. */
int uc_hybrid_mesh_child_read_checked(uint32_t mesh,uint32_t key,uint32_t reserved,unsigned bytes,UCCollisionMeshChild*result){
 unsigned char*m=span(mesh,64),*out=bytes?span(reserved,bytes):NULL;
 if(!result||!m||(bytes&&!out)||word(m)!=0x49C3C8u||(bytes&&overlaps(out,bytes,m,64)))return 0;
 uint32_t local;unsigned char*p=mesh_part(m,key,&local);
 if(!p||(bytes&&overlaps(out,bytes,p,48)))return 0;
 unsigned width=p[16]==1?2:p[16]==2?4:0;
 if(!width||word(p+20)<3*width||word(p+4)<12)return 0;
 uint64_t at=(uint64_t)word(p+12)+(uint64_t)local*word(p+20);if(at>UINT32_MAX)return 0;
 unsigned char*indices=span((uint32_t)at,3*width);
 if(!indices||(bytes&&overlaps(out,bytes,indices,3*width)))return 0;
 float vertices[3][3];
 for(unsigned i=0;i<3;i++){
  uint32_t index=0;memcpy(&index,indices+i*width,width);
  if(index>=word(p+8))return 0;
  at=(uint64_t)word(p)+(uint64_t)index*word(p+4);if(at>UINT32_MAX)return 0;
  unsigned char*v=span((uint32_t)at,12);
  if(!v||(bytes&&overlaps(out,bytes,v,12)))return 0;
  memcpy(vertices[i],v,12);
 }
 float scaling[3],radius;memcpy(scaling,m+16,12);memcpy(&radius,m+48,4);
 unsigned csr=_mm_getcsr();
 int status=uc_collision_mesh_scale(vertices,scaling,radius,result);_mm_setcsr(csr);
 return status==UC_COLLISION_OK;
}
int uc_hybrid_mesh_child(uint32_t mesh,uint32_t key,uint32_t child){
 static int mesh_enabled=-1;
 if(mesh_enabled<0){const char*v=getenv("UCRR_NATIVE_MESH_CHILD");mesh_enabled=v&&!strcmp(v,"1");}
 if(!mesh_enabled)return 0;
 unsigned char*out=span(child,64);
 UCCollisionMeshChild result;
 if(!out||!uc_hybrid_mesh_child_read_checked(mesh,key,child,64,&result))return 0;
 uint32_t table=0x49C33C,zero=0;uint16_t refs=1;
 memcpy(out,&table,4);memcpy(out+6,&refs,2);memcpy(out+8,&zero,4);memcpy(out+12,&result.radius,4);
 for(unsigned i=0;i<3;i++){memcpy(out+16+i*16,result.vertices[i],12);memcpy(out+28+i*16,&zero,4);}
 return 1;
}

int uc_hybrid_mesh_material_read_checked(uint32_t mesh,uint32_t key,uint32_t scratch,unsigned bytes,UCHybridMeshMaterial*result){
 if(!result)return 0;
 unsigned char*reserved=bytes?span(scratch,bytes):NULL;if(bytes&&!reserved)return 0;
 unsigned char*m=span(mesh,64);if(!m||word(m)!=0x49C3C8u||(bytes&&overlaps(m,64,reserved,bytes)))return 0;
 uint32_t local;unsigned char*p=mesh_part(m,key,&local);if(!p)return 0;
 unsigned width=p[17]==1?1:p[17]==2?2:0;
 if(!width||word(p+32)<width||word(p+40)<4)return 0;
 uint64_t at=(uint64_t)word(p+28)+(uint64_t)local*word(p+32);if(at>UINT32_MAX)return 0;
 unsigned char*i=span((uint32_t)at,width);if(!i)return 0;
 uint32_t index=0;memcpy(&index,i,width);if(index>=word(p+44))return 0;
 at=(uint64_t)word(p+36)+(uint64_t)index*word(p+40);if(at>UINT32_MAX)return 0;
 unsigned char*material=span((uint32_t)at,4);if(!material)return 0;
 if(bytes&&(overlaps(p,48,reserved,bytes)||overlaps(i,width,reserved,bytes)||overlaps(material,4,reserved,bytes)))return 0;
 result->material=word(material);result->index=index;result->offset=word(p+40)*index;return 1;
}

int uc_hybrid_mesh_material_read(uint32_t mesh,uint32_t key,uint32_t scratch,unsigned bytes,UCHybridMeshMaterial*result){
 static int material_enabled=-1;
 if(material_enabled<0){const char*v=getenv("UCRR_NATIVE_MESH_MATERIAL");material_enabled=v&&!strcmp(v,"1");}
 return material_enabled&&uc_hybrid_mesh_material_read_checked(mesh,key,scratch,bytes,result);
}

int uc_hybrid_mesh_material(uint32_t mesh,uint32_t key,uint32_t*result){
 UCHybridMeshMaterial read;if(!result||!uc_hybrid_mesh_material_read(mesh,key,0,0,&read))return 0;
 *result=read.material;return 1;
}

int uc_hybrid_mesh_material_guest(uint32_t mesh,uint32_t key,uint32_t stack){
 if(stack!=g_esp||mesh!=g_ecx||!span(stack-8,16)||key!=MEM32(stack+4))return 0;
 UCHybridMeshMaterial read;if(!uc_hybrid_mesh_material_read(mesh,key,stack-8,8,&read))return 0;
 MEM32(stack-4)=g_esi;MEM32(stack-8)=g_edi;
 g_eax=read.material;g_ecx=read.index;g_edx=read.offset;g_esp+=8;return 1;
}
