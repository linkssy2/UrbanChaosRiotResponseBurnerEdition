#include "migration_graphics_spot_geometry.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
extern void sub_002D4360(void);
static int spot_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SPOT_GEOMETRY");flag=v&&!strcmp(v,"1");}return flag;}
static int spot_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static RecompXmm spot_row(float a,float b,float c,float d){RecompXmm r;r.f[0]=a;r.f[1]=b;r.f[2]=c;r.f[3]=d;return r;}
static RecompXmm spot_scale(RecompXmm v,unsigned lane){RecompXmm r;for(unsigned i=0;i<4;i++)r.f[i]=v.f[lane];return r;}
static RecompXmm spot_product(RecompXmm v,RecompXmm a,RecompXmm b,RecompXmm c,RecompXmm d){return XMM_ADD(XMM_ADD(XMM_MUL(a,spot_scale(v,0)),XMM_MUL(b,spot_scale(v,1))),XMM_ADD(XMM_MUL(c,spot_scale(v,2)),XMM_MUL(d,spot_scale(v,3))));}
static RecompXmm spot_cross(RecompXmm a,RecompXmm b){return XMM_SUB(XMM_MUL(XMM_SHUFFLE(b,b,0xc9),XMM_SHUFFLE(a,a,0xd2)),XMM_MUL(XMM_SHUFFLE(b,b,0xd2),XMM_SHUFFLE(a,a,0xc9)));}
/* Preserve the original reciprocal-square-root refinement and dead scratch lanes. */
static RecompXmm spot_normalize(RecompXmm v,RecompXmm*scale,RecompXmm*diff,RecompXmm*broadcast){
 RecompXmm q=XMM_MUL(v,v),h=XMM_MOVE_HIGH_TO_LOW(q,q);q=XMM_ADD(q,h);h=XMM_SHUFFLE(q,q,1);q.f[0]+=h.f[0];XMM_STORE(esp+16,q);
 h.f[0]=1.0f/sqrtf(q.f[0]);MEMF(esp+16)=h.f[0];h=XMM_MEM(esp+16);RecompXmm t=h;t.f[0]*=q.f[0];t.f[0]*=h.f[0];*diff=XMM_MEM(0x51e420);diff->f[0]-=t.f[0];*scale=XMM_MEM(0x51e430);scale->f[0]*=h.f[0];scale->f[0]*=diff->f[0];*broadcast=spot_scale(*scale,0);return XMM_MUL(v,*broadcast);
}
int uc_migration_graphics_spot_geometry(uint32_t entry){
 if(entry!=0xaf5f0||!spot_enabled())return 0;uint32_t s=esp,object=ecx;
 if((s&3)||s<0x10200||!spot_span(s-256,260)||!spot_span(object,0x250)||(object&15)||!spot_span(0x51e4a0,16)||MEM8(0x51c178)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 if((uint64_t)object<s+4&&(uint64_t)object+0x250>s-256)return 0;
 for(unsigned i=0;i<16;i++)if((MEM32(object+16+i*4)&0x7fffffffu)>0x45800000u)return 0;
 for(unsigned i=0;i<10;i++)if((MEM32(object+0xc0+i*4)&0x7fffffffu)>0x45800000u)return 0;
 if(!MEM32(object+0xe4)||MEM32(object+0xe4)>=0x3fc90cb3u)return 0;
 uint32_t frame=g_ebp;PUSH32(esp,frame);frame=esp;g_ebp=frame;esp=(esp&~15u)-152;PUSH32(esp,esi);esi=ecx;
 RecompXmm local=XMM_MEM(esi+0xc0);XMM_STORE(esi+0x240,spot_product(local,XMM_MEM(esi+16),XMM_MEM(esi+32),XMM_MEM(esi+48),XMM_MEM(esi+64)));
 xmm0=XMM_ZERO();xmm2=XMM_SCALAR(MEMF(0x44bf94));xmm3=XMM_SCALAR(MEMF(0x44c530));PUSH32(esp,edi);
 if(MEMF(esi+0xe0)>0){xmm4=XMM_SCALAR(MEMF(esi+0xe0));xmm4.f[0]*=xmm3.f[0];xmm1=xmm2;xmm1.f[0]/=xmm4.f[0];}else xmm1=XMM_SCALAR(MEMF(0x457268));
 unsigned top=(g_fp_top+7)&7;g_fp_top=(int)top;g_fp_stack[top]=MEMF(esi+0xe4);MEMF(esp+60)=xmm1.f[0];g_fp_stack[top]=tan(g_fp_stack[top]);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=1.0;
 for(unsigned i=0;i<4;i++){RecompXmm row=XMM_ZERO();row.f[i]=xmm2.f[0];XMM_STORE(esp+32,row);xmm1=XMM_MEM(esp+32);XMM_STORE(esp+96+i*16,xmm1);}
 xmm1=XMM_SCALAR(MEMF(esi+0xe0));g_fp_top=(g_fp_top+1)&7;g_fp_stack[g_fp_top]=(double)MEMF(0x44bf94)/g_fp_stack[g_fp_top];MEMF(esp+96)=(float)g_fp_stack[g_fp_top];MEMF(esp+116)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 if(xmm1.f[0]>=0){xmm1=xmm0;xmm1.f[0]-=MEMF(esi+0xe0);MEMF(esp+136)=xmm3.f[0];MEMF(esp+152)=xmm1.f[0];}else{MEMF(esp+136)=0;MEMF(esp+152)=0;}
 float half=MEMF(0x44c1f8),one=xmm2.f[0];xmm1=XMM_SCALAR(half);
 XMM_STORE(esp+32,spot_row(half,0,0,0));XMM_STORE(esp+64,spot_row(0,half,0,0));XMM_STORE(esp+80,spot_row(0,0,half,0));XMM_STORE(esp+16,spot_row(0,0,0,one));
 for(unsigned i=0;i<3;i++)XMM_STORE(esi+0x170+i*16,XMM_MEM(esp+32+(i?16+i*16:0)));
 xmm3=XMM_MEM(esp+16);XMM_STORE(esi+0x1a0,xmm3);MEMF(esi+0x1a0)=half;MEMF(esi+0x1a4)=half;xmm1=XMM_SCALAR(MEMF(0x44d660));MEMF(esi+0x1a8)=xmm1.f[0];
 edi=esi+0x130;for(unsigned i=0;i<4;i++){RecompXmm row=XMM_ZERO();row.f[i]=one;XMM_STORE(esp+16,row);xmm1=row;XMM_STORE(edi+i*16,row);}
 xmm1=XMM_MEM(esi+0xd0);XMM_STORE(esi+0x150,xmm1);xmm3=XMM_MEM(esi+0x150);xmm3=spot_normalize(xmm3,&xmm1,&xmm5,&xmm4);MEMF(esp+140)=one;MEMF(esp+156)=0;XMM_STORE(esi+0x150,xmm3);
 g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=fabs((double)MEMF(esi+0xd4));MEMF(esp+28)=0;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(0x457260);g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;MEMF(esp+24)=0;
 MEMF(esp+16)=g_fp_cmp<=0?one:0;MEMF(esp+20)=g_fp_cmp<=0?0:one;xmm0=XMM_MEM(esp+16);XMM_STORE(esi+0x140,xmm0);
 xmm0=spot_cross(XMM_MEM(esi+0x150),XMM_MEM(esi+0x140));XMM_STORE(edi,xmm0);xmm1=spot_normalize(XMM_MEM(edi),&xmm0,&xmm3,&xmm2);XMM_STORE(edi,xmm1);
 xmm0=XMM_MEM(edi);xmm1=XMM_MEM(esi+0x150);xmm3=XMM_MUL(XMM_SHUFFLE(xmm1,xmm1,0xd2),XMM_SHUFFLE(xmm0,xmm0,0xc9));xmm2=XMM_SHUFFLE(xmm0,xmm0,0xd2);xmm0=spot_cross(xmm0,xmm1);XMM_STORE(esi+0x140,xmm0);xmm0=XMM_MEM(esi+0xc0);ecx=edi;XMM_STORE(esi+0x160,xmm0);g_ebp=frame;PUSH32(esp,0xafa72);sub_002D4360();
 RecompXmm p[4],v[4],result[4];for(unsigned i=0;i<4;i++){p[i]=XMM_MEM(esp+96+i*16);v[i]=XMM_MEM(edi+i*16);}for(unsigned i=0;i<4;i++)result[i]=spot_product(v[i],p[0],p[1],p[2],p[3]);
 XMM_STORE(esp+96,result[0]);XMM_STORE(esp+112,result[1]);for(unsigned i=0;i<4;i++)XMM_STORE(edi+i*16,result[i]);
 xmm3=XMM_MUL(p[1],spot_scale(v[3],1));xmm4=XMM_ADD(XMM_MUL(p[0],spot_scale(v[3],0)),xmm3);xmm5=result[2];xmm6=spot_scale(v[3],2);xmm7=XMM_ADD(XMM_MUL(p[2],spot_scale(v[2],2)),XMM_MUL(p[3],spot_scale(v[2],3)));
 xmm1=XMM_SCALAR(MEMF(esp+60));float factor=xmm1.f[0];XMM_STORE(esp+16,spot_row(factor,0,0,0));XMM_STORE(esp+32,spot_row(0,0,0,MEMF(0x44bf94)));XMM_STORE(esp+64,spot_row(0,0,factor,0));XMM_STORE(esp+80,spot_row(0,factor,0,0));
 XMM_STORE(esi+0xf0,XMM_MEM(esp+16));XMM_STORE(esi+0x100,XMM_MEM(esp+80));XMM_STORE(esi+0x110,XMM_MEM(esp+64));XMM_STORE(esi+0x120,XMM_MEM(esp+32));xmm0=spot_scale(xmm1,0);xmm1=XMM_MEM(esi+0x240);xmm2=XMM_MUL(XMM_XOR(XMM_MEM(0x51e4a0),xmm1),xmm0);POP32(esp,edi);XMM_STORE(esi+0x120,xmm2);POP32(esp,esi);esp=frame;POP32(esp,frame);g_recomp_zf=g_fp_cmp==0;esp+=4;return 1;
}
