#include "hybrid_particle_rotating_guest.h"
#include <stdlib.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/particle_sprite.h"
#include <string.h>
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_ROTATING");flag=v&&!strcmp(v,"1");}return flag;}
static void*view(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram?(void*)XBOX_PTR(p):NULL;}
static int disjoint(uint32_t a,unsigned n,uint32_t b,unsigned m){return(uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int bounded(const void*p,unsigned n){const unsigned char*b=p;for(unsigned i=0;i<n;i++){uint32_t u;memcpy(&u,b+i*4,4);if((u&0x7fffffffu)>0x47800000u)return 0;}return 1;}
/* Optional exact39-record world-space23B930..23BAE7 phase. Original plane setup, curve
 * preparation, startup random table, wrapper and graphics calls stay intact.
 * Worker0 is the pinned title configuration, not a synchronization lock. */
int uc_hybrid_particle_rotating(uint32_t s,uint32_t frame){
 if(!enabled()||!view(0x51c178,1)||MEM8(0x51c178)!=0||s!=g_esp||frame!=g_ebp||(s&15)||s!=((frame&~15u)-0x70u)||(_mm_getcsr()&~63u)!=0x1f80u||!view(s-16,0x80)||!view(frame,32)||!view(g_esi,0x124))return 0;
 uint32_t owner=g_esi,desc=MEM32(owner+0x20),particles=MEM32(owner+0x10),env=MEM32(owner+0xac),n=MEM32(frame+0xc),out=g_ebx,gap=MEM32(s+0x20),matrix=MEM32(frame+0x10),position=out+gap,angles=MEM32(frame+0x18);
 if(n!=39||MEM32(frame+4)!=0x9a215u||MEM32(owner)!=0x483020||!view(desc,0xcc)||!view(env,16)||!view(matrix,64)||!view(0x483020,16)||MEM32(0x48302c)!=0x23b6b0||MEM8(desc+0x9c)||g_ecx||MEM32(s+0x18)||MEM32(s+0x24)!=desc||MEM32(frame+0x1c)!=out||MEM32(frame+0x14)!=position)return 0;
 uint32_t start=MEM32(frame+8),capacity=MEM32(owner+0xc);
 if(start>capacity||n>capacity-start||start>UINT32_MAX/32u||g_edi!=start*32u||(uint64_t)particles+start*32u+n*32u>UINT32_MAX)return 0;
 uint32_t input=particles+start*32u;
 const uint32_t read[][2]={{owner,0x124},{desc,0xcc},{env,16},{input,n*32},{matrix,64},{frame,32},{0x44bf94,4},{0x482f40,4},{0x4fe7d0,4},{0x482fbc,4},{0x44d95c,4},{0x51ca98,64},{0x483020,16},{0x51c178,1}};
 const uint32_t write[][2]={{s-16,0x80},{position,n*16},{out,n*16},{angles,n*4}};
 for(unsigned i=0;i<4;i++){if(!view(write[i][0],write[i][1]))return 0;for(unsigned j=0;j<i;j++)if(!disjoint(write[i][0],write[i][1],write[j][0],write[j][1]))return 0;for(unsigned j=0;j<14;j++)if(!view(read[j][0],read[j][1])||!disjoint(write[i][0],write[i][1],read[j][0],read[j][1]))return 0;}
 if(MEM32(0x44bf94)!=0x3f800000||MEM32(0x482f40)!=0x43160000||MEM32(0x4fe7d0)!=0x3bda740e||MEM32(0x482fbc)!=0x37800080||MEM32(0x44d95c)!=0x40c90fdb)return 0;
 if(memcmp(g_xmm3.u,view(s+0x60,16),16))return 0;
 unsigned fade=!!MEM8(s+0x17),normal=!(MEM8(desc+0xa6)&&MEM8(env+12));
 if(!bounded(view(owner+0xb4,20),5)||!bounded(view(s+0x60,16),4)||!bounded(view(0x51ca98,64),16)||(normal?!bounded(view(owner+0xcc,88),22):!bounded(view(desc+0xb0,16),4)))return 0;
 for(unsigned i=0;i<n;i++)if(!bounded(view(input+i*32,16),4))return 0;
 UCParticleSprite records[39];unsigned csr=_mm_getcsr();
 const UCParticleColorCurve*c=(const UCParticleColorCurve*)view(owner+0xcc,88);
 RecompXmm point=XMM_ZERO(),center=XMM_ZERO(),plane=g_xmm3,x1=XMM_ZERO(),x2=XMM_ZERO(),x4=XMM_ZERO(),x5=g_xmm5;
 float scale=1.f,depth=0,age=0;int zf=g_recomp_zf;
 for(unsigned i=0;i<n;i++){
  const UCParticle*p=(const UCParticle*)view(input+i*32,32);age=(float)p->age*MEMF(0x482fbc);float random=MEMF(0x51ca98+(p->seed&15u)*4);
  float life=MEMF(owner+0xc4)*age,speed=MEMF(owner+0xb8)*random,offset=MEMF(owner+0xc0)*random;
  speed=speed+MEMF(owner+0xb4);offset=offset+MEMF(owner+0xbc);offset=offset*MEMF(0x44d95c);speed=speed*life;records[i].angle=speed+offset;
  memcpy(point.f,p->position,12);point.f[3]=1.f;center=point;x2=point;x4=XMM_SCALAR(p->radius);scale=1.f;
  if(fade){x1=XMM_MUL(plane,point);x5=XMM_SHUFFLE(x1,x1,2);x2=XMM_SHUFFLE(plane,plane,3);x5.f[0]=x5.f[0]+x2.f[0];x2=XMM_SHUFFLE(x1,x1,1);x1.f[0]=x1.f[0]+x2.f[0];x2=XMM_SCALAR(150.f);x1.f[0]=x1.f[0]+x5.f[0];depth=x1.f[0];if((double)depth<150.){scale=depth*MEMF(0x4fe7d0);scale=scale*scale;}x4=XMM_SCALAR(scale*p->radius);}
  center.f[3]=x4.f[0];memcpy(&records[i],center.f,16);x1=center;
  if(normal){if(uc_particle_color_sample(c,age,records[i].color)){_mm_setcsr(csr);return 0;}records[i].color[3]=records[i].color[3]*scale;zf=age<=0?age==0:age>=1?age==1:c->hold[0]>age?0:age==c->hold[1];
  }else memcpy(records[i].color,view(desc+0xb0,16),16);
 }
 const UCParticleSprite*last=&records[n-1];
 /* Color sampling already produced every record's FP status. Only the last
  * source XMM1 survives; its duplicate projection is needed once per batch. */
 if(normal){x1=XMM_ZERO();if(age>0&&age<1){x1=XMM_SCALAR(c->hold[0]);if(c->hold[0]>age)x1=XMM_MUL(XMM_MEM(owner+0xfc),XMM_SHUFFLE(XMM_SCALAR(age),XMM_SCALAR(age),0));else if(age>c->hold[1]){float t=age-c->hold[1];x1=XMM_MUL(XMM_MEM(owner+0x10c),XMM_SHUFFLE(XMM_SCALAR(t),XMM_SCALAR(t),0));}}}
 for(unsigned i=0;i<n;i++){memcpy(view(position+i*16,16),&records[i],16);memcpy(view(out+i*16,16),records[i].color,16);MEMF(angles+i*4)=records[i].angle;}
 MEM32(s+0x18)=n;MEMF(s+0x1c)=scale;MEMF(s+0x2c)=age;XMM_STORE(s+0x40,point);XMM_STORE(s+0x30,center);if(fade)MEMF(s+0x28)=depth;
 if(normal){memcpy(view(s+0x50,16),last->color,16);MEM32(s-16)=frame;MEM32(s-12)=0x23baae;MEM32(s-8)=s+0x50;MEMF(s-4)=age;g_ebp=s-16;g_recomp_zf=zf;}
 g_eax=n;g_ecx=n;g_edi+=n*32;g_ebx+=n*16;g_edx=MEM8(desc+0xa6)?env:((gap&0xffffff00u)|MEM8(desc+0xa6));memcpy(g_xmm0.f,last->color,16);g_xmm1=x1;g_xmm2=x2;g_xmm3=plane;g_xmm4=x4;g_xmm5=x5;
 return 1;
}

/* Camera-relative23B700..23B897. Preserve the four-lane transform residual,
 * including W, and leave the source epilogue to export its final comparison. */
int uc_hybrid_particle_rotating_camera(uint32_t s,uint32_t frame){
 if(!enabled()||!view(0x51c178,1)||MEM8(0x51c178)!=0||s!=g_esp||frame!=g_ebp||(s&15)||s!=((frame&~15u)-0x70u)||(_mm_getcsr()&~63u)!=0x1f80u||!view(s-16,0x80)||!view(frame,32)||!view(g_esi,0x124))return 0;
 uint32_t owner=g_esi,desc=MEM32(owner+0x20),particles=MEM32(owner+0x10),env=MEM32(owner+0xac),n=MEM32(frame+0xc),out=g_edx,gap=MEM32(s+0x20),matrix=MEM32(frame+0x10),position=out+gap,angles=MEM32(frame+0x18);
 if(n!=39||MEM32(frame+4)!=0x9a215u||MEM32(owner)!=0x483020||!view(desc,0xcc)||!view(env,16)||!view(matrix,64)||!view(0x483020,16)||MEM32(0x48302c)!=0x23b6b0||!MEM8(desc+0x9c)||MEM32(s+0x18)||MEM32(s+0x1c)!=out||MEM32(s+0x24)!=desc||MEM32(frame+0x1c)!=out||MEM32(frame+0x14)!=position||g_edi!=matrix)return 0;
 uint32_t start=MEM32(frame+8),capacity=MEM32(owner+0xc);
 if(start>capacity||n>capacity-start||start>UINT32_MAX/32u||g_ebx!=start*32u||(uint64_t)particles+start*32u+n*32u>UINT32_MAX)return 0;
 uint32_t input=particles+start*32u;
 const uint32_t read[][2]={{owner,0x124},{desc,0xcc},{env,16},{input,n*32},{matrix,64},{frame,32},{0x44bf94,4},{0x482fbc,4},{0x44d95c,4},{0x51ca98,64},{0x483020,16},{0x51c178,1}};
 const uint32_t write[][2]={{s-16,0x80},{position,n*16},{out,n*16},{angles,n*4}};
 for(unsigned i=0;i<4;i++){if(!view(write[i][0],write[i][1]))return 0;for(unsigned j=0;j<i;j++)if(!disjoint(write[i][0],write[i][1],write[j][0],write[j][1]))return 0;for(unsigned j=0;j<12;j++)if(!view(read[j][0],read[j][1])||!disjoint(write[i][0],write[i][1],read[j][0],read[j][1]))return 0;}
 if(MEM32(0x44bf94)!=0x3f800000||MEM32(0x482fbc)!=0x37800080||MEM32(0x44d95c)!=0x40c90fdb)return 0;
 unsigned normal=!(MEM8(desc+0xa6)&&MEM8(env+12));
 if(!bounded(view(matrix,64),16)||!bounded(view(owner+0xb4,20),5)||!bounded(view(0x51ca98,64),16)||(normal?!bounded(view(owner+0xcc,88),22):!bounded(view(desc+0xb0,16),4)))return 0;
 for(unsigned i=0;i<n;i++)if(!bounded(view(input+i*32,16),4))return 0;
 UCParticleSprite records[39];unsigned csr=_mm_getcsr();
 const UCParticleColorCurve*c=(const UCParticleColorCurve*)view(owner+0xcc,88);
 RecompXmm point=XMM_ZERO(),center=XMM_ZERO(),transformed=XMM_ZERO(),x1=XMM_ZERO(),x3=XMM_ZERO(),x4=XMM_MEM(matrix+48),x5=XMM_ZERO(),x6=XMM_ZERO();
 float age=0;int zf=g_recomp_zf;
 for(unsigned i=0;i<n;i++){
  const UCParticle*p=(const UCParticle*)view(input+i*32,32);age=(float)p->age*MEMF(0x482fbc);float random=MEMF(0x51ca98+(p->seed&15u)*4);
  x3=XMM_SCALAR(MEMF(owner+0xc4)*age);float speed=MEMF(owner+0xb8)*random,offset=MEMF(owner+0xc0)*random;
  speed=speed+MEMF(owner+0xb4);offset=offset+MEMF(owner+0xbc);offset=offset*MEMF(0x44d95c);x1=XMM_SCALAR(offset);speed=speed*x3.f[0];records[i].angle=speed+offset;
  memcpy(point.f,p->position,12);point.f[3]=0;RecompXmm z=XMM_SHUFFLE(point,point,0xaa),y=XMM_SHUFFLE(point,point,0x55);x5=XMM_SHUFFLE(point,point,0);x6=XMM_ADD(XMM_MUL(XMM_MEM(matrix+16),y),XMM_MUL(XMM_MEM(matrix+32),z));transformed=XMM_ADD(XMM_ADD(XMM_MUL(XMM_MEM(matrix),x5),x4),x6);center=transformed;center.f[3]=p->radius;memcpy(&records[i],center.f,16);
  if(normal){if(uc_particle_color_sample(c,age,records[i].color)){_mm_setcsr(csr);return 0;}zf=age<=0?age==0:age>=1?age==1:c->hold[0]>age?0:age==c->hold[1];
  }else memcpy(records[i].color,view(desc+0xb0,16),16);
 }
 const UCParticleSprite*last=&records[n-1];
 /* Color sampling already produced every record's FP status. Only the last
  * source XMM1 survives; its duplicate projection is needed once per batch. */
 if(normal){x1=XMM_ZERO();if(age>0&&age<1){x1=XMM_SCALAR(c->hold[0]);if(c->hold[0]>age)x1=XMM_MUL(XMM_MEM(owner+0xfc),XMM_SHUFFLE(XMM_SCALAR(age),XMM_SCALAR(age),0));else if(age>c->hold[1]){float t=age-c->hold[1];x1=XMM_MUL(XMM_MEM(owner+0x10c),XMM_SHUFFLE(XMM_SCALAR(t),XMM_SCALAR(t),0));}}}
 for(unsigned i=0;i<n;i++){memcpy(view(position+i*16,16),&records[i],16);memcpy(view(out+i*16,16),records[i].color,16);MEMF(angles+i*4)=records[i].angle;}
 MEM32(s+0x18)=n;MEM32(s+0x1c)=out+n*16;MEMF(s+0x28)=age;XMM_STORE(s+0x30,point);XMM_STORE(s+0x40,center);XMM_STORE(s+0x50,transformed);
 if(normal){memcpy(view(s+0x60,16),last->color,16);MEM32(s-16)=frame;MEM32(s-12)=0x23b86d;MEM32(s-8)=s+0x60;MEMF(s-4)=age;g_ebp=s-16;g_recomp_zf=zf;}
 g_eax=n;g_ecx=n;g_edx+=n*16;g_ebx+=n*32;memcpy(g_xmm0.f,last->color,16);g_xmm1=x1;g_xmm2=center;g_xmm3=x3;g_xmm4=x4;g_xmm5=x5;g_xmm6=x6;
 return 1;
}
