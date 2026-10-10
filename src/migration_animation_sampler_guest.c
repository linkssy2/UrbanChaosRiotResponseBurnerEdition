#include "migration_animation_sampler_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
extern void sub_001F2DE0(void);
extern void sub_001F3140(void);
extern void sub_001F3380(void);
extern void sub_001F2880(void);
extern void sub_001F2A00(void);
extern void sub_001F1A40(void);
typedef struct {uint32_t p,n;} SamplerRange;
static int sampler_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int sampler_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return!n||!m||(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int sampler_read(uint32_t p,uint32_t n,const SamplerRange*w,unsigned count){if(!sampler_span(p,n))return 0;for(unsigned i=0;i<count;i++)if(!sampler_separate(p,n,w[i].p,w[i].n))return 0;return 1;}
static int sampler_time(uint32_t table,unsigned i,uint32_t*value){uint32_t bits=MEM32(table+i*4),m=bits&0x7fffffffu;if(m>=0x7f800000u||((bits>>31)&&m))return 0;if((_mm_getcsr()&64)&&m<0x800000)m=0;*value=m;return 1;}
static int sampler_admit(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_SAMPLER");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,clip=g_ecx;
 if(!enabled||(s&3)||s<0x100e0||!sampler_span(s-224,252)||!sampler_span(clip,88)||!sampler_span(0x51c158,36)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 uint32_t state=MEM32(s+4);if(!sampler_span(state,40))return 0;
 uint32_t n=MEM32(clip+20),frames=MEM32(clip+28),bones=MEM32(state+36),records=MEM32(state+32),ids=MEM32(clip+24),mask=MEM32(state+12),times=MEM32(clip+32),rot=MEM32(clip+44),pos=MEM32(clip+48);
 /* ponytail: proven title-sized batches; larger assets retain the old path. */
 if(!n||n>256||frames<2||frames>4096||!bones||bones>4096)return 0;
 uint32_t cursor=MEM32(0x51c15c),base=MEM32(0x51c160),phase_bits=MEM32(s+8),phase_m=phase_bits&0x7fffffffu;
 if(cursor<base||!sampler_span(base,cursor-base)||((cursor-base)%48&&cursor-base<n*96)||phase_m>=0x3f800000u||((phase_bits>>31)&&phase_m))return 0;
 uint32_t pool_bytes=cursor-base;if(pool_bytes>n*96)pool_bytes=n*96;
 SamplerRange writes[8]={{s-224,252},{records,bones*48},{cursor-pool_bytes,pool_bytes},{0x51c15c,4}};unsigned nw=4;
 uint32_t cache=0,history=0,omit=0,stride=(n+7)/8,bytes=(MEM32(clip+64)&2)?6:4,current=0;
 if(MEM8(clip+72)&8){
  cache=MEM32(s+24);if(!sampler_span(cache,16))return 0;history=MEM32(cache+4);omit=MEM32(clip+40);current=MEM32(cache);
  if(current>=frames&&current!=0x80000001u)return 0;
  writes[nw++]=(SamplerRange){cache,16};writes[nw++]=(SamplerRange){history,n*8};writes[nw++]=(SamplerRange){MEM32(cache+8),n*bytes};writes[nw++]=(SamplerRange){MEM32(cache+12),n*bytes};
 }
 for(unsigned i=0;i<nw;i++){if(!sampler_span(writes[i].p,writes[i].n))return 0;for(unsigned j=0;j<i;j++)if(!sampler_separate(writes[i].p,writes[i].n,writes[j].p,writes[j].n))return 0;}
 const SamplerRange reads[]={{clip,88},{state,40},{ids,n*4},{mask,bones},{times,frames*4},{rot,frames*4},{pos,frames*4},{0x51c158,4},{0x51c160,4},{0x51c178,1},{0x44c174,4},{0x44bf94,4},{0x47faf0,8},{0x47fad0,4}};
 for(unsigned i=0;i<sizeof reads/sizeof reads[0];i++)if(!sampler_read(reads[i].p,reads[i].n,writes,nw))return 0;
 for(unsigned i=0;i<n;i++){int32_t id=SMEM32(ids+i*4);if((uint32_t)(id<0?~id:id)>=bones)return 0;}
 unsigned csr=_mm_getcsr();if((csr&0x1f80)!=0x1f80)return 0;
 float phase;memcpy(&phase,&phase_bits,4);float estimate=(float)(frames-1);estimate*=phase;int32_t key=(int32_t)estimate;_mm_setcsr(csr);
 uint32_t phase_order=(csr&64)&&phase_m<0x800000?0:phase_m;
 uint32_t value;if(key<0||(uint32_t)key>=frames||!sampler_time(times,(unsigned)key,&value))return 0;
 while(value>phase_order){if(!key||!sampler_time(times,(unsigned)--key,&value))return 0;}
 do{if((uint32_t)key>=frames-1||!sampler_time(times,(unsigned)key+1,&value))return 0;if(phase_order<value)break;++key;}while(1);
 const uint32_t sources[][2]={{MEM32(rot+key*4),n*bytes},{MEM32(rot+key*4+4),n*bytes},{MEM32(pos),n*12},{MEM32(pos+key*4),n*12},{MEM32(pos+key*4+4),n*12}};
 for(unsigned i=0;i<5;i++)if(!sampler_read(sources[i][0],sources[i][1],writes,nw))return 0;
 if(cache){
  if(!sampler_read(omit,frames*stride,writes,nw))return 0;
  uint8_t pending[256];unsigned remaining=0;
  for(unsigned i=0;i<n;i++){
   uint32_t lower=MEM32(history+i*8),upper=MEM32(history+i*8+4);if(lower>frames||upper>frames)return 0;
   pending[i]=(uint8_t)((uint32_t)key==current||((uint32_t)key==current+1)?0:lower>(uint32_t)key||((uint32_t)key+1!=current&&upper<=(uint32_t)key));remaining+=pending[i];
  }
  for(uint32_t f=(uint32_t)key+1;remaining&&f;){--f;if(!sampler_read(MEM32(rot+f*4),n*bytes,writes,nw))return 0;for(unsigned i=0;i<n;i++)if(pending[i]&&!(MEM8(omit+f*stride+i/8)&(1u<<(i&7)))){pending[i]=0;--remaining;}}
 }
 return 1;
}
static unsigned sampler_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
/* Fused original1F17F0 lookup, including its high-EAX and stack effects. */
static void sampler_mask(void){g_eax=MEM32(g_ecx+12);g_ecx=MEM32(g_esp+4);SET_LO8(g_eax,MEM8(g_ecx+g_eax));g_esp+=8;}
static void sampler_first_rotation(void){
 g_ecx=MEM32(g_eax);MEM32(g_esp+0x2c)=g_ecx;
 g_edx=MEM32(g_eax+4);MEM32(g_esp+0x30)=g_edx;
 g_ecx=MEM32(g_eax+8);MEM32(g_esp+0x34)=g_ecx;
 g_edx=MEM32(g_eax+12);
}
/* Complete phase selection and ordered track sampling. Guest scratch and call
 * frames remain observable at synchronous decoder/contribution boundaries. */
int uc_migration_animation_sampler(void){
 if(!sampler_admit())return 0;
 uint32_t frame=g_seh_ebp;
 g_esp-=0x84;g_xmm0=XMM_SCALAR(MEMF(g_esp+0x8c));
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=g_ecx;g_eax=MEM32(g_edi+28)-1;g_ecx=MEM32(g_edi+32);
 g_xmm1.f[0]=(float)(int32_t)g_eax;g_xmm1.f[0]*=g_xmm0.f[0];g_ebx=(int32_t)g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_ecx+g_ebx*4));g_eax=g_ecx+g_ebx*4;MEM32(g_esp+0x3c)=g_ebx;
 while(!(sampler_compare(g_xmm1.f[0],g_xmm0.f[0])&0x41)){
  g_xmm1=XMM_SCALAR(MEMF(g_eax-4));g_eax-=4;--g_ebx;MEM32(g_esp+0x3c)=g_ebx;
 }
 unsigned flags=sampler_compare(g_xmm0.f[0],MEMF(g_ecx+g_ebx*4+4));g_eax=g_ecx+g_ebx*4+4;
 while(!(flags&1)){g_eax+=4;++g_ebx;flags=sampler_compare(g_xmm0.f[0],MEMF(g_eax));MEM32(g_esp+0x3c)=g_ebx;}
 g_ecx=MEM32(g_edi+32);g_xmm1=XMM_SCALAR(MEMF(g_ecx+g_ebx*4+4));
 g_xmm0.f[0]-=MEMF(g_ecx+g_ebx*4);g_xmm1.f[0]-=MEMF(g_ecx+g_ebx*4);
 g_edx=MEM32(g_edi+44);g_eax=g_ecx+g_ebx*4;g_ecx=MEM32(g_edx+g_ebx*4);
 g_eax=g_edx+g_ebx*4;g_eax=MEM32(g_eax+4);g_edx=MEM32(g_edi+48);frame=MEM32(g_edx+g_ebx*4);
 MEM32(g_esp+0x24)=g_eax;g_eax=g_edx;g_edx=MEM32(g_eax);MEM32(g_esp+0x14)=g_ecx;
 g_ecx=MEM32(g_eax+g_ebx*4+4);g_eax=MEM32(g_edi+64)>>1;SET_LO8(g_eax,LO8(g_eax)&1);MEM8(g_esp+0x13)=LO8(g_eax);
 g_xmm0.f[0]/=g_xmm1.f[0];MEMF(g_esp+0x28)=g_xmm0.f[0];MEM32(g_esp+0x18)=frame;MEM32(g_esp+0x40)=g_ecx;MEM32(g_esp+0x20)=g_edx;
 if(MEM8(g_edi+72)&8){
  g_esi=MEM32(g_esp+0xac);g_eax=MEM32(g_esi);
  if(g_ebx!=g_eax){
   g_ecx=g_eax+1;int forward=g_ebx==g_ecx;g_ecx=g_edi;
   if(forward){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1f3c88);sub_001F3140();}
   else{
    --g_eax;
    if(g_ebx==g_eax){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1f3c95);sub_001F3380();}
    else{PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1f3c9e);sub_001F2DE0();}
   }
  }
  g_edx=MEM32(g_esi+8);g_eax=MEM32(g_esi+12);MEM32(g_esp+0x14)=g_edx;MEM32(g_esp+0x24)=g_eax;
 }
 flags=sampler_compare(g_xmm0.f[0],MEMF(0x44c174));SET_HI8(g_eax,flags);
 int dual=RECOMP_PARITY8(HI8(g_eax)&0x44);g_eax=MEM32(g_edi+20);MEM32(g_esp+0x1c)=0;
 do{
  if(dual){
   g_eax=MEM32(g_edi+24);g_ecx=MEM32(g_esp+0x1c);g_esi=MEM32(g_eax+g_ecx*4);
   if((int32_t)g_esi<0){
    g_esi=~g_esi;
    if((int32_t)g_ebx>0){frame=MEM32(g_esp+0x20);g_ebx=frame;}
    else{g_eax=MEM32(g_esp+0x18);g_ebx=MEM32(g_esp+0x20);g_eax+=12;MEM32(g_esp+0x18)=g_eax;}
   }else{
    g_ebx=MEM32(g_esp+0x40);g_edx=MEM32(g_esp+0x18);g_eax=12;g_edx+=g_eax;g_ecx=g_ebx+g_eax;MEM32(g_esp+0x18)=g_edx;MEM32(g_esp+0x40)=g_ecx;
   }
  }else{
   g_ecx=MEM32(g_edi+24);g_edx=MEM32(g_esp+0x1c);g_esi=MEM32(g_ecx+g_edx*4);
   if((int32_t)g_esi<0){g_esi=~g_esi;if((int32_t)g_ebx>0)frame=MEM32(g_esp+0x20);else MEM32(g_esp+0x18)+=12;}
   else MEM32(g_esp+0x18)+=12;
  }
  g_ecx=MEM32(g_esp+0x98);PUSH32(g_esp,g_esi);PUSH32(g_esp,dual?0x1f3e45:0x1f3d02);sampler_mask();
  if(LO8(g_eax)){
   SET_LO8(g_eax,MEM8(g_esp+0x13));g_ecx=MEM32(g_esp+0x14);int wide=LO8(g_eax)!=0;
   if(dual){
    if(wide){g_edx=g_esp+0x54;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f3e63);sub_001F2A00();}
    else{g_eax=g_esp+0x64;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f3e98);sub_001F2880();}
    sampler_first_rotation();g_ecx=MEM32(g_esp+0x24);g_eax=g_esp+(wide?0x74:0x84);PUSH32(g_esp,g_eax);MEM32(g_esp+0x3c)=g_edx;
    if(wide){PUSH32(g_esp,0x1f3e8c);sub_001F2A00();}else{PUSH32(g_esp,0x1f3ec4);sub_001F2880();}
    g_ecx=MEM32(g_eax);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm0.f[0]-=MEMF(g_esp+0x28);g_xmm0.f[0]*=MEMF(g_esp+0xa0);
    MEM32(g_esp+0x44)=g_ecx;g_edx=MEM32(g_eax+4);MEM32(g_esp+0x48)=g_edx;g_ecx=MEM32(g_eax+8);MEM32(g_esp+0x4c)=g_ecx;g_edx=MEM32(g_eax+12);
    g_eax=MEM32(g_esp+0x1c);g_ecx=MEM32(g_edi+84);g_esi=(g_esi+g_esi*2)<<4;int root=g_eax==g_ecx;MEM32(g_esp+0x50)=g_edx;g_edx=g_esp+0x2c;
    if(root){g_ecx=MEM32(g_esp+0xa4);PUSH32(g_esp,g_ecx);}else PUSH32(g_esp,0x3f800000);
    PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,frame);frame=MEM32(g_esp+0xa4);g_ecx=MEM32(frame+32);PUSH32(g_esp,g_edx);g_ecx+=g_esi;
    PUSH32(g_esp,root?0x1f3f2c:0x1f3f6f);sub_001F1A40();
    if(root)g_eax=MEM32(g_esp+0xa4);
    g_xmm0=XMM_SCALAR(MEMF(g_esp+0x28));g_xmm0.f[0]*=MEMF(g_esp+0xa0);
    if(root)PUSH32(g_esp,g_eax);else PUSH32(g_esp,0x3f800000);
    PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_ebx);
    if(root){g_ecx=g_esp+0x50;PUSH32(g_esp,g_ecx);}else{g_eax=g_esp+0x50;PUSH32(g_esp,g_eax);}
    g_ecx=MEM32(frame+32)+g_esi;PUSH32(g_esp,0x1f3f99);sub_001F1A40();
   }else{
    g_eax=g_esp+(wide?0x44:0x54);PUSH32(g_esp,g_eax);
    if(wide){PUSH32(g_esp,0x1f3d20);sub_001F2A00();}else{PUSH32(g_esp,0x1f3d2c);sub_001F2880();}
    sampler_first_rotation();g_eax=MEM32(g_esp+0x1c);MEM32(g_esp+0x38)=g_edx;
    if(g_eax==MEM32(g_edi+84)){
     g_ecx=MEM32(g_esp+0xa4);g_edx=MEM32(g_esp+0xa0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,frame);g_eax=g_esp+0x38;PUSH32(g_esp,g_eax);
    }else{
     g_eax=MEM32(g_esp+0xa0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,frame);g_ecx=g_esp+0x38;PUSH32(g_esp,g_ecx);
    }
    g_edx=MEM32(g_esp+0xa8);g_eax=MEM32(g_edx+32);g_ecx=((g_esi+g_esi*2)<<4)+g_eax;PUSH32(g_esp,0x1f3d92);sub_001F1A40();
   }
  }
  SET_LO8(g_eax,MEM8(g_esp+0x13));unsigned width=LO8(g_eax)?6:4;
  if(dual){
   g_ecx=MEM32(g_esp+0x24);g_edx=MEM32(g_esp+0x14);g_eax=width;g_esi=MEM32(g_esp+0x20);g_ecx+=g_eax;g_edx+=g_eax;g_eax=MEM32(g_esp+0x1c);g_esi+=12;
   MEM32(g_esp+0x24)=g_ecx;g_ecx=MEM32(g_edi+20);++g_eax;MEM32(g_esp+0x14)=g_edx;
  }else{
   g_eax=MEM32(g_esp+0x14)+width;g_esi=MEM32(g_esp+0x20);g_ecx=MEM32(g_edi+20);MEM32(g_esp+0x14)=g_eax;g_eax=MEM32(g_esp+0x1c);g_esi+=12;++g_eax;
  }
  MEM32(g_esp+0x20)=g_esi;MEM32(g_esp+0x1c)=g_eax;
  if((int32_t)g_eax>=(int32_t)g_ecx)break;
  if(dual)g_ebx=MEM32(g_esp+0x3c);frame=MEM32(g_esp+0x18);
 }while(1);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=0x84;
 g_recomp_zf=g_esp==0;RECOMP_TRACE_EXIT("sub_001F3BA0",0x1f3ba0);g_esp+=28;return 1;
}
