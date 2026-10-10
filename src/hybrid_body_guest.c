#include "hybrid_body_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/collision_body.h"
#include <stdlib.h>
#include <string.h>

static int enabled(void){
 static int flag=-1;
 if(flag<0){const char*v=getenv("UCRR_NATIVE_BODY_PREP");flag=v&&!strcmp(v,"1");}
 return flag;
}
static uint32_t physical(uint32_t p){return p>=0x80000000u&&p<0x84000000u?p-0x80000000u:p;}
static unsigned char*view(uint32_t p,unsigned n){
 uint32_t a=physical(p);return a>=0x10000u&&(uint64_t)a+n<=g_xbox_total_ram?(unsigned char*)XBOX_PTR(p):NULL;
}
static int separate(uint32_t a,unsigned n,uint32_t b,unsigned m){
 return (uint64_t)physical(a)+n<=physical(b)||(uint64_t)physical(b)+m<=physical(a);
}
int uc_hybrid_body_filter(uint32_t stack){
 if(!enabled()||stack!=g_esp||physical(stack)<0x10010u||!view(stack-16,32))return 0;
 uint32_t out=MEM32(stack+4),ray=MEM32(stack+8),body=MEM32(stack+12);
 if(!view(out,1)||!view(ray,40)||!view(body,32)||MEM8(body+0x18)==2)return 0;
 uint32_t shape=MEM32(body);
 if(!view(shape,12)||MEM32(shape)!=0x49c4b4u||!view(0x49c4bc,4)||MEM32(0x49c4bc)!=0x24cb50u)return 0;
 const uint32_t inputs[][2]={{stack,16},{body,32},{ray,40},{shape,12},{0x49c4bc,4}};
 for(unsigned i=0;i<sizeof(inputs)/sizeof(inputs[0]);i++)
  if(!separate(inputs[i][0],inputs[i][1],out,1)||!separate(inputs[i][0],inputs[i][1],stack-16,16))return 0;
 if(!separate(out,1,stack-16,16))return 0;
 uint32_t packed=MEM32(ray+0x24),filter=MEM32(body+0x1c);
 UCCollisionBodyFilter metadata={24,MEM32(shape+8),filter,MEM8(body+0x18),0,0,0};
 int accepted=uc_collision_body_filter(packed,&metadata);
 if(accepted<0)return 0;
 /* Exact scratch and visible state of the single pure24CB50 getter path.
  * CONTROL's reject epilogue leaves g_recomp_zf unchanged. */
 MEM32(stack-4)=g_esi;MEM32(stack-8)=g_edi;MEM32(stack-12)=g_ebx;MEM32(stack-16)=0x2a88aeu;
 MEM8(out)=(uint8_t)accepted;g_eax=out;g_ecx=packed^filter;
 if(accepted)g_recomp_zf=0;
 g_esp+=16;return 1;
}
int uc_hybrid_body_local(uint32_t state,uint32_t body,uint32_t ray,uint32_t stack){
 if(!enabled()||state!=g_esi||body!=g_ebx||ray!=g_edi||stack!=g_esp||
    !view(state,0x44)||!view(body,12)||!view(ray,40)||!view(stack,32))return 0;
 uint32_t matrix=MEM32(body+8),common=MEM32(state+4),shape=MEM32(stack+0x1c);
 if(!view(matrix,64)||!view(common,16))return 0;
 /* The original publishes local-from before reloading the matrix/end.
  * Excluding all read/output aliases permits computing both endpoints first. */
 const uint32_t inputs[][2]={{state,0x20},{body,12},{ray,40},{matrix,64},{common,16},{stack,32}};
 for(unsigned i=0;i<sizeof(inputs)/sizeof(inputs[0]);i++)
  if(!separate(inputs[i][0],inputs[i][1],state+0x20,32))return 0;
 float from[4],axes[4][4];UCCollisionRay selected,result;
 memcpy(from,view(common,16),16);memcpy(axes,view(matrix,64),64);
 memset(&selected,0,sizeof selected);memcpy(selected.to,view(ray+16,16),16);selected.filter=MEM32(ray+0x24);
 unsigned csr=_mm_getcsr();
 if(uc_collision_body_local(from,&selected,axes,&result)!=1){_mm_setcsr(csr);return 0;}
 /* Preserve every original SIMD intermediate visible to the unchanged virtual
  * shape call. The kernel supplies the results; these are ABI projections. */
 RecompXmm a,b,c,d,x,y,z,cl,ch,t;
 memcpy(&a,axes[0],16);memcpy(&b,axes[1],16);memcpy(&c,axes[2],16);memcpy(&t,axes[3],16);
 memcpy(&d,selected.to,16);d=XMM_SUB(d,t);
 cl=XMM_SHUFFLE(c,c,0x44);ch=XMM_SHUFFLE(c,c,0xee);
 x=XMM_SHUFFLE(a,b,0x44);y=XMM_SHUFFLE(x,cl,0xdd);
 z=XMM_SHUFFLE(a,b,0xee);z=XMM_SHUFFLE(z,ch,0x88);
 RecompXmm dx=XMM_SHUFFLE(d,d,0),dy=XMM_SHUFFLE(d,d,0x55),dz=XMM_SHUFFLE(d,d,0xaa);
 y=XMM_MUL(y,dy);z=XMM_MUL(z,dz);
 memcpy(view(state+0x20,16),result.from,16);memcpy(view(state+0x30,16),result.to,16);
 g_xmm0=d;memcpy(&g_xmm1,result.to,16);g_xmm2=z;g_xmm3=dx;g_xmm4=cl;g_xmm5=y;g_xmm6=dz;g_xmm7=ch;
 g_eax=matrix;g_ecx=shape;g_edx=state+0x20;
 return 1;
}
