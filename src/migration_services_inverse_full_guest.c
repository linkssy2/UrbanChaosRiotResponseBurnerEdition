#include "migration_services_inverse_full_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static RecompXmm adjacent(RecompXmm v){return XMM_SHUFFLE(v,v,0xb1);}
static RecompXmm halves(RecompXmm v){return XMM_SHUFFLE(v,v,0x4e);}
int uc_migration_services_inverse_full(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_INVERSE_FULL");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x2d4360)return 0;
 uint32_t s=g_esp,m=g_ecx;
 if(s<0x10100||s>=0x80000000u||(uint64_t)s+4>g_xbox_total_ram||m<0x10000||m>=0x80000000u||(uint64_t)m+64>g_xbox_total_ram)return 0;
 if((uint64_t)m<s+4u&&(uint64_t)m+64>s-224u)return 0;
 uint32_t f=s-4,work=(f&~15u)-192;MEM32(f)=g_ebp;g_ebp=f;
 RecompXmm a,b,c,d,q[12],p,r4,r5,r6,r7;
 /* Cofactor expansion in four lanes. Odd columns start half-rotated; the
  * pairing and subtraction order match the title's single-precision SIMD. */
 for(unsigned i=0;i<4;i++){
  a.u[i]=MEM32(m+16*i);c.u[i]=MEM32(m+16*i+8);
  b.u[i]=MEM32(m+16*((i+2)&3)+4);d.u[i]=MEM32(m+16*((i+2)&3)+12);
 }
 p=adjacent(XMM_MUL(c,d));q[0]=XMM_MUL(b,p);r6=XMM_MUL(a,p);p=halves(p);
 r5=XMM_SUB(XMM_MUL(b,p),q[0]);q[6]=XMM_SUB(XMM_MUL(a,p),r6);
 p=adjacent(XMM_MUL(b,c));q[4]=XMM_MUL(a,p);r6=XMM_MUL(d,p);p=halves(p);
 q[2]=XMM_MUL(d,p);q[4]=XMM_SUB(XMM_MUL(a,p),q[4]);
 p=adjacent(XMM_MUL(halves(b),d));c=halves(c);q[3]=XMM_MUL(c,p);q[5]=XMM_MUL(a,p);q[1]=halves(p);
 RecompXmm subtract=XMM_MUL(c,q[1]);r6=XMM_ADD(r6,r5);r6=XMM_SUB(r6,q[2]);
 q[0]=XMM_SUB(XMM_ADD(q[3],r6),subtract);q[5]=XMM_SUB(XMM_MUL(a,q[1]),q[5]);
 p=adjacent(XMM_MUL(a,b));r5=XMM_MUL(d,p);r6=XMM_MUL(c,p);p=halves(p);
 q[8]=XMM_MUL(d,p);q[9]=XMM_MUL(c,p);
 p=adjacent(XMM_MUL(a,d));q[1]=XMM_MUL(c,p);q[10]=XMM_MUL(b,p);p=halves(p);
 q[3]=XMM_MUL(c,p);q[7]=XMM_MUL(b,p);
 p=adjacent(XMM_MUL(a,c));r4=XMM_MUL(d,p);q[11]=XMM_MUL(b,p);p=halves(p);q[2]=XMM_MUL(d,p);
 RecompXmm r1=XMM_MUL(b,p);r7=q[0];
 /* Determinant reduction and one reciprocal refinement, deliberately retaining
  * singular/NaN behavior instead of substituting an identity or clamping. */
 RecompXmm r0=XMM_MUL(a,r7),r2=halves(r0),r3=XMM_ADD(r2,r0);
 r0=adjacent(r3);r0.f[0]+=r3.f[0];q[0]=r0;r2.f[0]=1.0f/r0.f[0];q[0].f[0]=r2.f[0];
 r3=q[0];r2=r3;r2.f[0]*=r3.f[0];r0.f[0]*=r2.f[0];r2=r0;r0=r3;r0.f[0]+=r3.f[0];r0.f[0]-=r2.f[0];
 r0=XMM_SHUFFLE(r0,r0,0);RecompXmm row=XMM_MUL(r0,r7);XMM_STORE(m,row);
 r3=XMM_SUB(halves(q[6]),q[1]);r3=XMM_ADD(q[3],r3);r4=XMM_ADD(r4,r3);r4=XMM_SUB(r4,q[2]);
 row=XMM_MUL(r0,r4);XMM_STORE(m+16,row);
 r3=halves(q[5]);r5=XMM_ADD(r5,r3);r2=XMM_SUB(q[8],r5);r2=XMM_ADD(q[10],r2);r2=XMM_SUB(r2,q[7]);
 r3=r2;row=XMM_MUL(r0,r3);XMM_STORE(m+32,row);
 r2=q[4];r3=halves(r2);r6=XMM_SUB(r6,r3);r6=XMM_SUB(r6,q[9]);r6=XMM_SUB(r6,q[11]);r1=XMM_ADD(r1,r6);r0=XMM_MUL(r0,r1);XMM_STORE(m+48,r0);
 for(unsigned i=0;i<12;i++)XMM_STORE(work+16*i,q[i]);
 g_xmm0=r0;g_xmm1=r1;g_xmm2=r2;g_xmm3=r3;g_xmm4=r4;g_xmm5=r5;g_xmm6=r6;g_xmm7=r7;
 g_eax=m;g_recomp_zf=s==0;g_esp=s+4;return 1;
}
