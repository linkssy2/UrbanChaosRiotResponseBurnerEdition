#include "migration_graphics_inverse.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern void sub_003E8990(void);
static int inverse_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_INVERSE");flag=v&&!strcmp(v,"1");}return flag;}
static int inverse_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void inverse_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void inverse_store(uint32_t p){MEMF(p)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
/* Cofactors use the original mixed float/double minor precision and operand
 * order. Publish the guest scratch and inactive x87 slots only after all input
 * reads, which requires input/output to be disjoint from the scratch frame. */
int uc_migration_graphics_inverse(uint32_t entry){
 if(!inverse_enabled()||entry!=0x3e8a40)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!inverse_span(s-128,160)||!inverse_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 uint32_t input=MEM32(s+8),output=MEM32(s+4);
 if(!inverse_span(input,64)||!inverse_span(output,64)||!((uint64_t)input+64<=s-128||input>=s+32)||!((uint64_t)output+64<=s-128||output>=s+32))return 0;
 double m[16];uint32_t bits[16];for(unsigned i=0;i<16;i++){bits[i]=MEM32(input+i*4);float f;memcpy(&f,bits+i,4);if(!isfinite(f)||fabsf(f)>4096.f)return 0;m[i]=f;}
 /* The first two upper minors and the last three lower minors are rounded
  * to float in the original. The other minors retain double precision. */
 double a01=(float)(m[5]*m[0]-m[4]*m[1]),a02=(float)(m[9]*m[0]-m[8]*m[1]);
 double a03=m[0]*m[13]-m[12]*m[1],a12=m[9]*m[4]-m[8]*m[5],a13=m[13]*m[4]-m[12]*m[5],a23=m[13]*m[8]-m[12]*m[9];
 float c[16];
 c[15]=(float)((m[2]*a12-m[6]*a02)+m[10]*a01);
 c[14]=(float)((m[6]*a03-m[14]*a01)-m[2]*a13);
 c[13]=(float)((m[2]*a23-m[10]*a03)+m[14]*a02);
 c[12]=(float)((m[10]*a13-m[14]*a12)-m[6]*a23);
 c[11]=(float)((a02*m[7]-a01*m[11])-a12*m[3]);
 c[10]=(float)((a13*m[3]-a03*m[7])+a01*m[15]);
 c[9]=(float)((a03*m[11]-a02*m[15])-a23*m[3]);
 c[8]=(float)((a23*m[7]-a13*m[11])+a12*m[15]);
 double b01=m[2]*m[7]-m[6]*m[3],b02=m[2]*m[11]-m[10]*m[3],b03=m[2]*m[15]-m[14]*m[3];
 double b12=(float)(m[6]*m[11]-m[10]*m[7]),b13=(float)(m[6]*m[15]-m[14]*m[7]),b23=(float)(m[10]*m[15]-m[14]*m[11]);
 c[3]=(float)((b02*m[5]-b01*m[9])-b12*m[1]);
 c[2]=(float)((b13*m[1]-b03*m[5])+b01*m[13]);
 c[1]=(float)((b03*m[9]-b02*m[13])-b23*m[1]);
 double cofactor0=(b23*m[5]-b13*m[9])+b12*m[13];c[0]=(float)cofactor0;
 c[7]=(float)((b12*m[0]-b02*m[4])+b01*m[8]);
 c[6]=(float)((m[4]*b03-b01*m[12])-b13*m[0]);
 c[5]=(float)((b23*m[0]-b03*m[8])+b02*m[12]);
 c[4]=(float)((b13*m[8]-b12*m[12])-b23*m[4]);
 float determinant=(float)((((double)c[3]*m[12]+(double)c[2]*m[8])+(double)c[1]*m[4])+(double)c[0]*m[0]);
 static const unsigned scratch[]={0x1c,0xc,0x10,0x14,0x18,4,8,0,0x34,0x3c,0x44,0x4c,0x38,0x40,0x48,0x50};
 g_esp-=84;for(unsigned i=0;i<16;i++)MEMF(g_esp+scratch[i])=c[i];MEM32(g_esp+32)=bits[8];MEM32(g_esp+36)=bits[4];MEMF(g_esp+40)=(float)b12;MEMF(g_esp+44)=(float)b13;MEM32(g_esp+48)=bits[0];MEMF(g_esp+92)=determinant;
 unsigned top=(unsigned)g_fp_top;g_fp_stack[top]=b12*m[13];g_fp_stack[(top+1)&7]=cofactor0;g_fp_stack[(top+2)&7]=m[9];g_fp_stack[(top+3)&7]=b01*m[8];g_fp_stack[(top+4)&7]=b13*m[0];g_fp_stack[(top+5)&7]=b03*m[8];g_fp_stack[(top+6)&7]=determinant;g_fp_stack[(top+7)&7]=MEMF(0x44c174);
 g_ecx=bits[8];g_edx=bits[12];g_fp_cmp=RECOMP_FCMP((double)determinant,g_fp_stack[(top+7)&7]);
 g_eax=(bits[13]&0xffff0000u)|(uint16_t)(((top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700u):g_fp_cmp==2?0x4500u:g_fp_cmp<0?0x100u:g_fp_cmp>0?0:0x4000u));
 if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){g_eax=0xffffffff;g_esp+=84;g_recomp_zf=g_esp==0;g_esp+=16;return 1;}
 g_eax=MEM32(g_esp+96);
 static const unsigned order[]={0,4,1,8,2,12,3,5,9,6,13,7,10,14,11,15};
 if(g_eax){
  g_ecx=MEM32(g_esp+92);inverse_push(MEMF(g_esp+92));g_fp_stack[g_fp_top]*=MEMF(g_esp+92);PUSH32(g_esp,g_ecx);MEM32(g_esp+100)=g_ecx;inverse_store(g_esp);PUSH32(g_esp,0x3e8dd2);sub_003E8990();inverse_store(g_esp+48);
  g_edx=MEM32(g_esp+96);g_eax=MEM32(g_esp+48);g_edx=(g_edx&0x80000000u)|g_eax;g_eax=MEM32(g_esp+88);MEM32(g_esp+92)=g_edx;
  for(unsigned i=0;i<16;i++){unsigned j=order[i];inverse_push(MEMF(g_esp+92));g_fp_stack[g_fp_top]*=MEMF(g_esp+scratch[j]);inverse_store(g_eax+j*4);}
 }else{
  g_ecx=MEM32(g_esp+88);g_eax=MEM32(g_esp+92)&0x80000000u;
  for(unsigned i=0;i<16;i++){unsigned j=order[i];g_edx=MEM32(g_esp+scratch[j])^g_eax;if(i&&i!=15)MEM32(g_esp+scratch[j])=g_edx;MEM32(g_ecx+j*4)=g_edx;if(i==15){g_eax=g_edx;MEM32(g_esp+80)=g_edx;}}
 }
 g_eax=0;g_esp+=84;g_recomp_zf=g_esp==0;g_esp+=16;return 1;
}
