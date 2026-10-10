#include "migration_interface_input.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>

static int enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_INPUT");v=s&&!strcmp(s,"1");}return v;}
static int span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int apart(uint64_t a,unsigned n,uint64_t b,unsigned m){return a+n<=b||b+m<=a;}
static void push_fp(double value){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;}

/* Guest stack writes and volatile results are part of the boundary contract.
 * The data operation itself is one ordered filter over the selected preset. */
static void resolve(void){
 const uint32_t s=g_esp,action=MEM32(s+8),out=MEM32(s+12);
 const uint32_t set=MEM32(g_ecx+0x948+MEM32(s+4)*4);
 PUSH32(g_esp,g_seh_ebp);PUSH32(g_esp,g_esi);g_eax=0;
 if((int32_t)action<18){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);
  for(g_edx=0;g_edx<24;g_edx++){
   const uint32_t entry=g_ecx+0x958+set*192+g_edx*8;
   if(MEM32(entry+4)==action)MEM32(out+4*g_eax++)=MEM32(entry);
  }
  POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_recomp_zf=1;
 }else{g_ecx=out;MEM32(out)=action;g_eax=1;g_recomp_zf=action==18;}
 POP32(g_esp,g_esi);g_esp+=20;
}
static const uint32_t button_offsets[16]={0x10,0x1c,0x28,0x34,0x40,0x4c,0x88,0x94,0xa0,0xac,0xb8,0xc4,0x58,0x64,0x70,0x7c};
static void button(void){
 g_ecx=MEM32(g_esp+8)-22;g_eax=0;
 if(g_ecx<16)g_eax=MEM32(g_esp+4)+button_offsets[g_ecx];
 /* Original55D8B falls into a comparison-derived ZF, unlike its siblings. */
 g_recomp_zf=g_ecx==15?1:g_ecx<15?g_eax==0:0;g_esp+=12;
}
static void query_button(uint32_t physical,uint32_t return_pc){
 PUSH32(g_esp,physical);PUSH32(g_esp,g_esi);g_ecx=g_ebx;
 PUSH32(g_esp,return_pc);button();
}
static int compare_zero(void){
 push_fp(MEMF(0x44c174));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);
 g_fp_top=(g_fp_top+2)&7;
 SET_HI8(g_eax,g_fp_cmp==2?0x47:g_fp_cmp<0?3:g_fp_cmp>0?2:0x42);
 return RECOMP_PARITY8(HI8(g_eax)&0x44);
}
static void query(void){
 uint32_t frame=g_seh_ebp;
 g_esp-=0x12c;g_eax=MEM32(g_esp+0x130);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;
 g_ecx=MEM32(0x5096e0);g_edx=g_eax;MEM32(g_esp+12)=g_edx;
 if(MEM8(g_ecx+g_eax+0x10)==255||!MEM8(g_ecx+g_eax+0x14))goto disconnected;
 g_eax=(uint32_t)(int32_t)SMEM8(g_ecx+g_eax+12);if(g_eax==UINT32_MAX)goto disconnected;
 g_ecx=MEM32(g_esp+0x138);g_eax=g_eax*0xd0;
 PUSH32(g_esp,g_esi);g_eax+=MEM32(g_ebx+0x20);PUSH32(g_esp,g_edi);g_esi=g_eax;
 g_eax=g_esp+0xd8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_ebx;
 PUSH32(g_esp,0x56859);resolve();g_edi=g_eax;g_eax=MEM32(g_esp+0x144);MEM32(g_esp+12)=g_edi;
 const uint32_t mode=g_eax;
 double result=MEMF(0x44c174);
 if(mode==0||mode==1){
  for(g_edx=0;g_edx<g_edi;g_edx++){
   const uint32_t code=MEM32(g_esp+g_edx*4+0xd8);
   if(mode==0)g_eax=code;else g_ecx=code;
   query_button(code,mode==0?0x56890:0x568c6);
   if(g_eax){
    if(mode==1)g_eax=(uint32_t)(int32_t)SMEM8(g_eax)>>1;
    if((mode==0?MEM8(g_eax):g_eax)&1){result=MEMF(0x44bf94);break;}
   }
  }
 }else if(mode==2){
  /* Chord selection preserves source group order, duplicates and -1 holes.
   * Recursive queries still use the live original public host-input merge. */
  g_eax=UINT32_MAX;g_ecx=UINT32_MAX;
  for(unsigned i=0;i<24;i++){MEM32(g_esp+0x78+4*i)=UINT32_MAX;MEM32(g_esp+0x18+4*i)=UINT32_MAX;}
  g_edx=0;g_eax=MEM32(g_esp+12);PUSH32(g_esp,frame);frame=0;g_esi=g_edi=0;
  MEM32(g_esp+0x74)=g_ecx;MEM32(g_esp+0x78)=g_ecx;MEM32(g_esp+0x14)=0;
  while((int32_t)MEM32(g_esp+0x14)<(int32_t)MEM32(g_esp+0x10)){
   g_ecx=MEM32(g_esp+0x14);g_ecx=MEM32(g_esp+g_ecx*4+0xdc);g_eax=g_ecx-22;
   if(g_eax<=15){
    if(g_ecx>=30&&g_ecx<=33){
     if(!g_edi){for(unsigned i=30;i<=33;i++)MEM32(g_esp+0x1c+4*g_esi++)=i;g_edi=1;}
     for(g_eax=0;g_eax<g_esi;g_eax++)if(MEM32(g_esp+0x1c+4*g_eax)==g_ecx)MEM32(g_esp+0x1c+4*g_eax)=UINT32_MAX;
    }else if(g_ecx<=27||g_ecx==34||g_ecx==35){
     if(!g_edx){static const uint32_t analog[]={22,23,24,25,26,27,34,35};for(unsigned i=0;i<8;i++)MEM32(g_esp+0x1c+4*g_esi++)=analog[i];g_edx=1;}
     for(g_eax=0;g_eax<g_esi;g_eax++)if(MEM32(g_esp+0x1c+4*g_eax)==g_ecx)MEM32(g_esp+0x1c+4*g_eax)=UINT32_MAX;
    }
    MEM32(g_esp+0x7c+frame++*4)=g_ecx;
   }
   g_eax=MEM32(g_esp+0x14)+1;g_ecx=MEM32(g_esp+0x10);MEM32(g_esp+0x14)=g_eax;
  }
  g_edi=0;
  while(g_edi<g_esi){
   g_eax=MEM32(g_esp+0x1c+g_edi*4);g_ecx=MEM32(g_esp+0x18);g_edx=MEM32(g_ebx);
   uint32_t s=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_ebx;
   uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x56b36);RECOMP_ICALL_SAFE(target,s);
   if(compare_zero())goto chord_done;
   g_edi++;
  }
  g_esi=0;
  if(frame)g_edi=MEM32(g_esp+0x18);
  while(g_esi<frame){
   g_eax=MEM32(g_esp+0x7c+g_esi*4);g_edx=MEM32(g_ebx);
   uint32_t s=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ecx=g_ebx;
   uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x56b64);RECOMP_ICALL_SAFE(target,s);
   if(compare_zero()){result=MEMF(0x44bf94);break;}
   g_esi++;
  }
chord_done:
  g_esp+=4;
 }else if(mode==3||mode==4){
  g_xmm0=XMM_ZERO();g_edx=0;MEMF(g_esp+0x10)=0;
  for(;g_edx<g_edi;g_edx++){
   g_eax=MEM32(g_esp+0xd8+g_edx*4);g_ecx=g_eax-18;
   if(g_ecx<4){
    uint32_t bits=MEM32(g_esi+g_ecx*4);if(g_ecx&1)g_eax=bits;else g_ecx=bits;MEM32(g_esp+12)=bits;
    push_fp(MEMF(g_esp+12));g_fp_stack[g_fp_top]=fabs(g_fp_stack[g_fp_top]);
    push_fp(MEMF(g_esp+16));g_fp_stack[g_fp_top]=fabs(g_fp_stack[g_fp_top]);
    double t=g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=t;
    g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;
    if(g_fp_cmp<=0)continue;
    g_xmm0=XMM_SCALAR(MEMF(g_esp+12));
   }else{
    query_button(g_eax,mode==3?0x56c08:0x56c9b);if(!g_eax)continue;
    g_xmm1=XMM_SCALAR(MEMF(g_eax+(mode==3?4:8)));
    double a=g_xmm1.f[0],b=g_xmm0.f[0];
    if(a!=a||b!=b||a<=b)continue;
    g_xmm0=g_xmm1;
   }
   MEMF(g_esp+16)=g_xmm0.f[0];
  }
  result=MEMF(g_esp+16);
 }
 push_fp(result);POP32(g_esp,g_edi);POP32(g_esp,g_esi);goto finish;
disconnected:
 push_fp(MEMF(0x44c174));
finish:
 POP32(g_esp,g_ebx);g_esp+=0x12c;g_recomp_zf=g_esp==0;g_esp+=16;
}

int migration_interface_input(unsigned op){
 if(!enabled()||op>7)return 0;
 const uint32_t s=g_esp,m=g_ecx;
 if(s<0x11000||(s&3)||!span(s-4096,4112))return 0;
 if(op==1){button();return 1;}
 if(op==3){
  if(!span(0x51c4dc,4)||!span(0x44c174,4))return 0;
  if(MEM32(0x44c174)!=0)return 0;
  uint32_t manager=MEM32(0x51c4dc);
  if(!span(manager,4)||!span(MEM32(manager),12)||MEM32(MEM32(manager)+8)!=0x567f0)return 0;
  g_edx=MEM32(s+8);g_ecx=manager;g_eax=MEM32(manager);
  PUSH32(g_esp,1);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_edx);
  uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x274757);RECOMP_ICALL_SAFE(target,s);
  int pressed=compare_zero();SET_LO8(g_eax,pressed);
  g_recomp_zf=pressed?!(HI8(g_eax)&0x44):1;g_esp+=4;return 1;
 }
 if(op>=4){
  if(!span(m,0xf58)||!apart(m,0xf58,s-4096,4112))return 0;
  if(op==4){
   static const uint32_t defaults[18][2]={{18,0},{19,1},{20,2},{21,3},{22,7},{35,11},{27,12},{26,9},{26,10},{23,6},{37,13},{29,14},{24,4},{26,17},{36,15},{37,16},{34,8},{25,5}};
   PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
   memset((void*)XBOX_PTR(m+0x958),255,8*192);
   memcpy((void*)XBOX_PTR(m+0x958),defaults,sizeof(defaults));memcpy((void*)XBOX_PTR(m+0xa18),defaults,sizeof(defaults));
   for(unsigned i=0;i<4;i++)MEM32(m+0x948+i*4)=1;
   g_eax=m+0xf58;g_ecx=m+0x958;g_edx=0;g_recomp_zf=1;
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;return 1;
  }
  if(op==7){MEM32(m+0x948)=0;g_esp+=4;return 1;}
  uint32_t port=MEM32(s+4);if(port>3)return 0;
  if(op==5){g_eax=MEM32(m+0x948+port*4);g_esp+=8;return 1;}
  g_eax=MEM32(s+8);g_edx=port;MEM32(m+0x948+port*4)=g_eax;g_esp+=12;return 1;
 }
 const uint32_t port=MEM32(s+4),action=MEM32(s+8);
 if(port>3||!span(m,0xf58)||!apart(m,0xf58,s-4096,4112))return 0;
 uint64_t base=(uint64_t)m+0x958+(uint64_t)MEM32(m+0x948+port*4)*192;
 if((int32_t)action<18&&(!span(base,192)||!apart(base,192,s-4096,4112)))return 0;
 if(op==0){
  const uint32_t out=MEM32(s+12);unsigned n=(int32_t)action<18?96:4;
  if(!span(out,n)||!apart(out,n,s-16,32)||!apart(out,n,m,0xf58)||!apart(out,n,base,192))return 0;
  resolve();return 1;
 }
 if(!span(0x5096e0,4)||!span(0x44bf94,4)||!span(0x44c174,4))return 0;
 if(MEM32(0x44c174)!=0||MEM32(0x44bf94)!=0x3f800000)return 0;
 uint32_t ports=MEM32(0x5096e0);
 if(!span(ports,24)||!apart(ports,24,s-4096,4112))return 0;
 if(MEM8(ports+port+0x10)!=255&&MEM8(ports+port+0x14)&&SMEM8(ports+port+12)!=-1){
  int device=SMEM8(ports+port+12);uint64_t state=(uint64_t)MEM32(m+0x20)+(uint64_t)(unsigned)device*0xd0;
  if(device<0||device>3||!span(state,0xd0)||!apart(state,0xd0,s-4096,4112))return 0;
  if(MEM32(s+12)==2&&(!span(MEM32(m),12)||MEM32(MEM32(m)+8)!=0x567f0))return 0;
 }
 query();return 1;
}
