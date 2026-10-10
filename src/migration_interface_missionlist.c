#include "migration_interface_missionlist.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00017060(void),sub_00018130(void),sub_000589C0(void),sub_00058AE0(void),sub_00063C20(void),sub_00063C70(void),sub_00063C90(void),sub_000643E0(void),sub_00070BB0(void),sub_000722A0(void),sub_00072380(void),sub_00208980(void),sub_0025DB00(void),sub_0026EAB0(void),sub_0026EC60(void),sub_0026ED90(void),sub_00273590(void),sub_00274300(void),sub_002BCFA0(void),sub_002D57FB(void),sub_002D6322(void);
#define ML_CALL(n,r) do{g_ebp=frame;PUSH32(g_esp,r);sub_##n();}while(0)
#define ML_FP g_fp_stack[g_fp_top]
static void missionlist_pop(unsigned out){MEMF(out)=(float)ML_FP;g_fp_top=(g_fp_top+1)&7;}
static void missionlist_insert(unsigned frame,unsigned slot,unsigned build,unsigned count){
 g_edi=MEM32(g_esi+4);g_ecx=MEM32(g_edi+4);MEM32(g_esp+slot)=g_eax;g_eax=g_esp+slot;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ecx=g_esi;MEM32(g_esp+224)=0xffffffff;ML_CALL(002BCFA0,build);PUSH32(g_esp,1);g_ecx=g_esi;g_ebx=g_eax;ML_CALL(000722A0,count);
}
static void missionlist_link(void){MEM32(g_edi+4)=g_ebx;g_eax=MEM32(g_ebx+4);MEM32(g_eax)=g_ebx;}
static void missionlist_alloc(unsigned frame,unsigned ret,unsigned state,unsigned cleanup){
 PUSH32(g_esp,180);ML_CALL(002D57FB,ret);g_esp+=cleanup;MEM32(g_esp+16)=g_eax;MEM32(g_esp+212)=state;
}
static void missionlist_caption(unsigned frame,unsigned kind){
 /* Description, difficulty, score and completion rows share the caption constructor. */
 if(!g_eax){g_eax=0;return;}
 if(kind<3)g_ecx=MEM32(kind==0?0x51cf28:0x51cf5c);else g_edx=MEM32(kind==3?0x51cf5c:0x51cf28);
 g_xmm0=XMM_SCALAR(MEMF(kind==0?0x4859f0:kind==4?0x451290:g_esp+44));PUSH32(g_esp,0x44d9b8);PUSH32(g_esp,0);PUSH32(g_esp,kind==0?16:kind==4?1:0);PUSH32(g_esp,kind<3?g_ecx:g_edx);PUSH32(g_esp,1);PUSH32(g_esp,kind==0||kind==4?g_edi:kind==1?0x4885d4:0x488598);PUSH32(g_esp,0);PUSH32(g_esp,kind==0?0x3e800000:0x3e99999a);
 if(kind<3)g_edx=g_esp+56;else g_ecx=g_esp+56;MEMF(g_esp+56)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+44));PUSH32(g_esp,kind<3?g_edx:g_ecx);g_ecx=g_eax;MEMF(g_esp+64)=g_xmm0.f[0];ML_CALL(00273590,kind==0?0x26f89b:kind==1?0x26f9aa:kind==3?0x26fb3a:0x26fbb1);
}
static void missionlist_header(unsigned frame){
 g_eax=MEM32(g_edi+28);g_edx=g_eax*3;g_eax=MEM32(g_edi+g_edx*4+36);g_ecx=MEM32(g_edi+g_edx*4+32);g_edi+=g_edx*4+32;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);MEM32(g_esp+56)=g_edi;ML_CALL(000643E0,0x26f736);g_ecx=g_eax;ML_CALL(00063C20,0x26f73d);g_xmm0=XMM_SCALAR(MEMF(0x500a90));g_xmm0.f[0]-=MEMF(0x44bf18);g_ecx=MEM32(0x509704);PUSH32(g_esp,2);MEM32(g_esp+36)=g_eax;MEMF(g_esp+16)=g_xmm0.f[0];ML_CALL(00017060,0x26f764);
 PUSH32(g_esp,180);ML_CALL(002D57FB,0x26f76e);g_ebx=g_eax;g_esp+=4;MEM32(g_esp+16)=g_ebx;g_eax=0;MEM32(g_esp+212)=0;
 if(g_ebx){g_edx=MEM32(0x51cf5c);g_xmm0=XMM_SCALAR(MEMF(0x4859f0));g_ecx=MEM32(g_edi);PUSH32(g_esp,0x44d9b8);PUSH32(g_esp,g_eax);PUSH32(g_esp,64);PUSH32(g_esp,g_edx);PUSH32(g_esp,2);PUSH32(g_esp,g_eax);g_eax=MEM32(g_edi+4);PUSH32(g_esp,g_eax);MEMF(g_esp+52)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+40));PUSH32(g_esp,g_ecx);MEMF(g_esp+60)=g_xmm0.f[0];ML_CALL(0026EAB0,0x26f7bc);g_esp+=12;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0x3e4ccccd);g_edx=g_esp+56;PUSH32(g_esp,g_edx);g_ecx=g_ebx;ML_CALL(00273590,0x26f7d3);}
 missionlist_insert(frame,20,0x26f7f6,0x26f801);missionlist_link();g_ecx=MEM32(0x509704);ML_CALL(00018130,0x26f814);ML_FP+=MEMF(g_esp+12);g_ecx=MEM32(0x509704);PUSH32(g_esp,1);missionlist_pop(g_esp+16);ML_CALL(00017060,0x26f829);g_eax=MEM32(g_esp+48);g_edx=MEM32(g_eax+4);g_eax=MEM32(g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);ML_CALL(0026EC60,0x26f839);PUSH32(g_esp,180);g_edi=g_eax;ML_CALL(002D57FB,0x26f845);g_esp+=12;MEM32(g_esp+16)=g_eax;MEM32(g_esp+212)=1;missionlist_caption(frame,0);g_xmm0=XMM_SCALAR(MEMF(0x4859f0));MEMF(g_eax+172)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x451290));MEMF(g_eax+176)=g_xmm0.f[0];missionlist_insert(frame,20,0x26f8e2,0x26f8ed);MEM32(g_edi+4)=g_ebx;g_eax=MEM32(g_ebx+4);g_edi=MEM32(g_esp+32);MEM32(g_eax)=g_ebx;
}
static void missionlist_details(unsigned frame){
 g_fp_top=(g_fp_top+7)&7;ML_FP=MEMF(0x500a94);g_xmm0=XMM_SCALAR(MEMF(0x4859f0));ML_FP-=MEMF(0x44bf18);g_ecx=MEM32(0x509704);MEMF(g_esp+44)=g_xmm0.f[0];missionlist_pop(g_esp+12);ML_CALL(00018130,0x26f92b);g_edx=MEM32(g_edi);ML_FP=MEMF(g_esp+12)-ML_FP;g_edx+=2;PUSH32(g_esp,g_edx);g_eax=g_esp+108;missionlist_pop(g_esp+16);PUSH32(g_esp,0x4885e8);PUSH32(g_esp,g_eax);ML_CALL(002D6322,0x26f948);
 missionlist_alloc(frame,0x26f952,2,16);missionlist_caption(frame,1);missionlist_insert(frame,20,0x26f9d1,0x26f9dc);MEM32(g_edi+4)=g_ebx;g_eax=MEM32(g_ebx+4);PUSH32(g_esp,180);MEM32(g_eax)=g_ebx;ML_CALL(002D57FB,0x26f9ee);g_esp+=4;MEM32(g_esp+16)=g_eax;MEM32(g_esp+212)=3;
 if(g_eax){g_edx=MEM32(g_esp+32);g_ecx=MEM32(g_edx);g_xmm0=XMM_SCALAR(MEMF(0x451290));PUSH32(g_esp,0x44d9b8);PUSH32(g_esp,0);PUSH32(g_esp,1);int special=g_ecx==3;g_ecx=MEM32(0x51cf54);if(!special)g_ecx=MEM32(0x51cf28);PUSH32(g_esp,g_ecx);PUSH32(g_esp,1);g_ecx=g_esp+124;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0x3e99999a);g_edx=g_esp+56;MEMF(g_esp+56)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+44));PUSH32(g_esp,g_edx);g_ecx=g_eax;MEMF(g_esp+64)=g_xmm0.f[0];ML_CALL(00273590,0x26fa58);}else g_eax=0;
 missionlist_insert(frame,20,0x26fa7f,0x26fa8a);missionlist_link();g_ecx=MEM32(0x509704);ML_CALL(00018130,0x26fa9d);ML_FP=MEMF(g_esp+12)-ML_FP;g_ebx=MEM32(g_esp+48);g_eax=MEM32(g_ebx);missionlist_pop(g_esp+12);
 if(g_eax!=0xffffffff&&g_eax!=10){PUSH32(g_esp,g_eax);ML_CALL(000643E0,0x26fac3);g_ecx=g_eax;ML_CALL(00063C90,0x26faca);g_edi=LO8(g_eax)?0x4885c0:0x4885ac;missionlist_alloc(frame,0x26fae2,4,4);missionlist_caption(frame,3);PUSH32(g_esp,g_eax);g_ecx=g_esi;MEM32(g_esp+216)=0xffffffff;ML_CALL(00072380,0x26fb51);missionlist_alloc(frame,0x26fb5b,5,4);missionlist_caption(frame,4);PUSH32(g_esp,g_eax);g_ecx=g_esi;MEM32(g_esp+216)=0xffffffff;ML_CALL(00072380,0x26fbc8);}
}
static void missionlist_icon(unsigned frame,unsigned mode){
 PUSH32(g_esp,80);ML_CALL(002D57FB,mode==0?0x26fe20:mode==1?0x26ff2e:0x26ff7a);g_esp+=4;MEM32(g_esp+(mode?52:16))=g_eax;MEM32(g_esp+212)=mode+7;
 if(g_eax){
  if(mode==1){g_edx=MEM32(0x51cf94);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+48);g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edx);PUSH32(g_esp,g_ecx);}
  else{PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ecx);g_edx=g_esp;MEM32(g_esp+(mode?28:64))=g_esp;g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);if(!mode){PUSH32(g_esp,0x487464);PUSH32(g_esp,0);MEM32(g_edx)=0x40ffffff;}else{MEM32(g_edx)=0x40ffffff;PUSH32(g_esp,0x487464);}}
  if(mode)PUSH32(g_esp,0);PUSH32(g_esp,0x3e99999a);g_edx=g_esp+84;PUSH32(g_esp,g_edx);g_ecx=g_eax;ML_CALL(00070BB0,mode?0x26ff71:0x26fe67);
 }else g_eax=0;
 missionlist_insert(frame,16,mode?0x26ffd5:0x26fe8e,mode?0x26ffe0:0x26fe99);
 if(mode){g_eax=MEM32(g_esp+32);MEM32(g_edi+4)=g_ebx;g_edx=MEM32(g_ebx+4);g_eax++;MEM32(g_edx)=g_ebx;MEM32(g_esp+32)=g_eax;g_edi=g_eax;return;}
 g_eax=MEM32(g_esp+32);PUSH32(g_esp,0x3ecccccd);MEM32(g_edi+4)=g_ebx;g_edx=MEM32(g_ebx+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);MEM32(g_edx)=g_ebx;g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,0x488570);ML_CALL(00208980,0x26febd);g_ecx=MEM32(g_esp+44);g_edx=MEM32(g_ecx);g_edi=MEM32(g_esp+84);PUSH32(g_esp,g_eax);g_eax=MEM32(g_edi+272);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=g_esp+76;PUSH32(g_esp,g_ecx);ML_CALL(0025DB00,0x26feda);g_ebx=MEM32(g_edi+260);g_esp+=24;g_edi+=256;g_edx=g_esp+16;MEM32(g_esp+16)=g_eax;g_eax=MEM32(g_ebx+4);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_ecx=g_edi;ML_CALL(002BCFA0,0x26fefe);PUSH32(g_esp,1);g_ecx=g_edi;MEM32(g_esp+20)=g_eax;ML_CALL(000722A0,0x26ff0b);g_eax=MEM32(g_esp+16);MEM32(g_ebx+4)=g_eax;g_ecx=MEM32(g_eax+4);MEM32(g_ecx)=g_eax;g_eax=MEM32(g_esp+32)+1;MEM32(g_esp+32)=g_eax;g_edi=g_eax;
}
static void missionlist_progress(unsigned frame){
 for(;;){
  PUSH32(g_esp,48);ML_CALL(000643E0,0x26fd77);g_ecx=g_eax;ML_CALL(00063C70,0x26fd7e);int unlocked=LO8(g_eax)!=0;
  if(!unlocked){ML_CALL(000643E0,0x26fd87);SET_LO8(g_ecx,MEM8(g_eax+383));unlocked=LO8(g_ecx)!=0;}
  if(!unlocked){ML_CALL(000643E0,0x26fd96);int all=MEM32(g_eax+72)==3;g_eax=2;if(all)g_eax=3;}else g_eax=3;
  if((int32_t)g_edi>=(int32_t)g_eax)break;
  g_edx=MEM32(g_esp+64);g_eax=MEM32(g_esp+40);g_ecx=ZX8(MEM8(g_edx+g_eax));int complete=(int32_t)g_edi<(int32_t)g_ecx;g_xmm0.f[0]=(float)(int32_t)g_edi;g_xmm0.f[0]*=MEMF(0x44c13c);g_xmm0.f[0]+=MEMF(g_esp+68);MEMF(g_esp+56)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+12));g_xmm0.f[0]-=MEMF(0x44c530);MEMF(g_esp+60)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44c798));MEMF(g_esp+24)=MEMF(g_esp+28)=g_xmm0.f[0];unsigned mode=2;
  if(complete){g_edx=ZX8(MEM8(g_eax));mode=1;if((int32_t)g_edi>=(int32_t)g_edx){g_eax=MEM32(frame+8);if(MEM32(g_eax)!=4)mode=0;}}
  missionlist_icon(frame,mode);
 }
}
static void missionlist_rows(unsigned frame){
 g_ecx=MEM32(0x509704);ML_CALL(00018130,0x26fbd3);ML_FP=MEMF(g_esp+12)-ML_FP;g_ecx=MEM32(0x509704);missionlist_pop(g_esp+12);ML_CALL(00018130,0x26fbe6);ML_FP*=MEMF(0x45341c);MEM32(g_esp+88)=0x45367c;MEM32(g_esp+92)=0x453654;MEM32(g_esp+96)=0x45362c;ML_FP=MEMF(g_esp+12)-ML_FP;MEM32(g_esp+100)=0x453604;missionlist_pop(g_esp+12);ML_CALL(0026ED90,0x26fc19);g_xmm0=XMM_SCALAR(MEMF(0x451290));g_eax--;g_xmm1.f[0]=(float)(int32_t)g_eax;g_eax=MEM32(frame+8);g_xmm1.f[0]*=MEMF(0x44c13c);g_edx=g_esp+100;MEM32(g_esp+36)=g_edx;g_edx=g_eax+7;MEM32(g_esp+40)=g_edx;g_edx=MEM32(g_esp+32);g_edx-=g_eax;g_edi=0xfffffffd;g_xmm0.f[0]-=g_xmm1.f[0];g_xmm0.f[0]-=MEMF(0x44c798);g_ecx=3;g_edi-=g_eax;MEMF(g_esp+68)=g_xmm0.f[0];MEM32(g_esp+20)=g_ecx;MEM32(g_esp+64)=g_edx;MEM32(g_esp+72)=g_edi;
 for(;;){
  g_eax=MEM32(g_ebx);g_edx=MEM32(g_ebx+4);PUSH32(g_esp,g_ecx);if(g_eax!=0xffffffff){PUSH32(g_esp,g_eax);ML_CALL(000589C0,0x26fc91);}else{PUSH32(g_esp,g_edx);ML_CALL(00058AE0,0x26fc99);}g_esp+=8;PUSH32(g_esp,g_eax);g_eax=g_esp+140;PUSH32(g_esp,0x450a30);PUSH32(g_esp,g_eax);ML_CALL(002D6322,0x26fcaf);g_ecx=MEM32(g_esp+52);g_edi+=g_ecx;PUSH32(g_esp,g_edi);g_edx=g_esp+184;PUSH32(g_esp,0x488584);PUSH32(g_esp,g_edx);ML_CALL(002D6322,0x26fcc8);missionlist_alloc(frame,0x26fcd2,6,28);
  if(g_eax){g_edx=MEM32(0x51cf5c);g_xmm0=XMM_SCALAR(MEMF(g_esp+44));g_ecx=g_esp+136;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,1);g_ecx=g_esp+188;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0x3e99999a);g_edx=g_esp+112;MEMF(g_esp+112)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+44));PUSH32(g_esp,g_edx);g_ecx=g_eax;MEMF(g_esp+120)=g_xmm0.f[0];ML_CALL(00273590,0x26fd30);}else g_eax=0;
  missionlist_insert(frame,32,0x26fd57,0x26fd62);MEM32(g_edi+4)=g_ebx;g_eax=MEM32(g_ebx+4);g_edi=0;MEM32(g_eax)=g_ebx;MEM32(g_esp+32)=g_edi;missionlist_progress(frame);g_ecx=MEM32(0x509704);ML_CALL(00018130,0x270003);ML_FP=MEMF(g_esp+12)-ML_FP;g_eax=MEM32(g_esp+20);g_edi=MEM32(g_esp+40);g_edx=MEM32(g_esp+36);missionlist_pop(g_esp+12);g_eax--;g_edi--;g_edx-=4;MEM32(g_esp+20)=g_eax;MEM32(g_esp+40)=g_edi;MEM32(g_esp+36)=g_edx;if((int32_t)g_eax<0)break;g_edi=MEM32(g_esp+72);g_ecx=MEM32(g_esp+20);g_ebx=MEM32(g_esp+48);
 }
}
static int missionlist_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_missionlist(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MISSIONLIST");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x11000||(s&3)||!missionlist_span(s-4096,4128)||!missionlist_span(g_ecx,280)||MEM32(g_ecx+28)>16)return 0;unsigned profile=MEM32(s+4);if(profile&&!missionlist_span(profile,8))return 0;for(unsigned i=0;i<2;i++){unsigned head=MEM32(g_ecx+248+i*12);if(!missionlist_span(head,8)||!missionlist_span(MEM32(head+4),8))return 0;}
 PUSH32(g_esp,g_ebp);unsigned frame=g_esp;g_ebp=frame;g_esp&=~7u;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39fc9c);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=192;g_eax=MEM32(frame+8);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_esp+76)=g_edi;
 if(!g_eax){g_eax=MEM32(g_edi+28);g_ecx=g_eax*3+9;g_edx=MEM32(g_edi+g_ecx*4);g_eax*=3;g_ecx=MEM32(g_edi+g_eax*4+32);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);ML_CALL(000643E0,0x26f6e9);g_ecx=g_eax;ML_CALL(00063C20,0x26f6f0);MEM32(frame+8)=g_eax;}
 g_esi=g_edi+244;g_ecx=g_esi;ML_CALL(00274300,0x26f700);g_ecx=g_edi+256;ML_CALL(00274300,0x26f70b);g_eax=MEM32(g_edi+236);
 if(g_eax){missionlist_header(frame);if(MEM32(g_edi)!=0xffffffff){missionlist_details(frame);missionlist_rows(frame);}}
 g_ecx=MEM32(g_esp+204);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;return 1;
}
