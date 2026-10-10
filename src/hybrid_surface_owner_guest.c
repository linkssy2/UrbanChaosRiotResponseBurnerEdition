#include "hybrid_surface_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00082528(void),sub_003E5570(void),sub_003E6140(void),sub_003EAFD0(void);
static int surface_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_NATIVE_SURFACE_OWNER");f=v&&!strcmp(v,"1");}return f;}
static int surface_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int surface_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Original D3 /4 uses the low five count bits, including a zero effective
 * count. Unsigned products retain IMUL's low word without signed C overflow. */
static uint32_t surface_shift(uint32_t value,uint32_t count){return value<<(count&31u);}

/* The integer layout routine has no calls. Retain its ordered output writes:
 * format may alias the source header, and later size reads must see that write.
 * Saved registers and the conditional inherited frame are observable scratch. */
static void surface_layout(void){
 uint32_t frame=g_seh_ebp;
 g_esp-=24;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+32);
 g_eax=MEM8(MEM8(g_ebx+13)+0x3f1d70);g_ecx=MEM32(g_ebx+4);g_edx=MEM32(g_ebx+16);
 g_ecx|=0x80000000u;g_eax&=0x3c;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 MEM32(g_esp+16)=g_ecx;MEM32(g_esp+20)=g_eax;
 if(!g_edx){
  g_eax=MEM32(g_ebx+12);g_esi=MEM8(g_ebx+13);g_edx=(g_eax>>20)&15;g_edi=(g_eax>>24)&15;g_eax>>=28;
  unsigned block=g_esi==12||(g_esi>13&&g_esi<=15);
  PUSH32(g_esp,frame);g_ecx=block?2u:0u;frame=g_ecx;g_ecx=MEM32(g_esp+48);MEM32(g_esp+32)=frame;
  if(g_ecx){
   g_ecx=MEM8(g_ebx+14)&15;g_esi=g_edx;MEM32(g_esp+16)=g_edi;MEM32(g_esp+28)=g_eax;frame=0;
   if(g_ecx){
    MEM32(g_esp+36)=g_ecx;
    do{
     g_ebx=MEM32(g_esp+32);if(g_esi>g_ebx)g_ebx=g_esi;
     g_ecx=MEM32(g_esp+32);if(MEM32(g_esp+16)>g_ecx)g_ecx=MEM32(g_esp+16);
     g_ecx+=g_ebx;g_ebx=surface_shift(1,g_ecx);g_ebx*=MEM32(g_esp+24);g_ebx>>=3;frame+=g_ebx;
     if(g_esi)--g_esi;
     g_ecx=MEM32(g_esp+16);if(g_ecx){--g_ecx;MEM32(g_esp+16)=g_ecx;}
     g_ecx=MEM32(g_esp+28);if(g_ecx){--g_ecx;MEM32(g_esp+28)=g_ecx;}
     --MEM32(g_esp+36);
    }while(MEM32(g_esp+36));
    g_ebx=MEM32(g_esp+44);
   }
   g_ecx=MEM32(g_esp+20);frame=(frame+127)&0xffffff80u;frame*=MEM32(g_esp+48);g_ecx+=frame;frame=MEM32(g_esp+32);MEM32(g_esp+20)=g_ecx;
  }
  g_ecx=MEM32(g_esp+52);
  if(g_ecx){
   MEM32(g_esp+44)=g_ecx;
   do{
    g_esi=g_edx>frame?g_edx:frame;g_ecx=g_edi>frame?g_edi:frame;g_ecx+=g_esi;g_ecx+=g_eax;
    g_esi=surface_shift(1,g_ecx);g_ecx=MEM32(g_esp+20);g_esi*=MEM32(g_esp+24);g_esi>>=3;g_ecx+=g_esi;MEM32(g_esp+20)=g_ecx;
    if(g_edx)--g_edx;if(g_edi)--g_edi;if(g_eax)--g_eax;--MEM32(g_esp+44);
   }while(MEM32(g_esp+44));
  }
  g_ecx=MEM32(g_ebx+12);g_eax=((((g_eax<<4)|g_edi)<<4)|g_edx)<<20;g_ecx&=0xfffff;g_eax|=g_ecx;g_esi=g_eax;
  if(g_edx<=frame)g_edx=frame;if(g_edi<=frame)g_edi=frame;
  g_eax=MEM8(g_ebx+13);POP32(g_esp,frame);
  unsigned format=g_eax;g_eax=1;g_ecx=g_edx;g_eax=surface_shift(g_eax,g_ecx);
  if(format==12)g_eax<<=1;else if(format>13&&format<=15)g_eax<<=2;else{g_eax*=MEM32(g_esp+20);g_eax>>=3;}
  g_ecx=g_edi+g_edx;g_edx=surface_shift(1,g_ecx);g_ecx=MEM32(g_esp+64);MEM32(g_ecx)=g_esi;
  g_ecx=MEM32(g_ebx+16);g_edx*=MEM32(g_esp+20);g_esi=MEM32(g_esp+68);MEM32(g_esi)=g_ecx;g_ecx=MEM32(g_esp+16);g_edx>>=3;
 }else{
  g_esi=MEM32(g_ebx+12);g_edi=MEM32(g_esp+64);g_eax=(g_edx>>24)+1;g_edx=(g_edx>>12)&0xfff;MEM32(g_edi)=g_esi;
  g_esi=MEM32(g_ebx+16);g_edi=MEM32(g_esp+68);g_eax<<=6;++g_edx;g_edx*=g_eax;MEM32(g_edi)=g_esi;
 }
 g_esi=MEM32(g_esp+52);MEM32(g_esi)=g_ecx;g_ecx=MEM32(g_esp+56);POP32(g_esp,g_edi);MEM32(g_ecx)=g_eax;
 g_eax=MEM32(g_esp+56);POP32(g_esp,g_esi);MEM32(g_eax)=g_edx;POP32(g_esp,g_ebx);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=36;
}

static void surface_construct(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,0x64800000);PUSH32(g_esp,24);PUSH32(g_esp,0x3e614d);sub_00082528();
 g_esi=g_eax;
 if(!g_esi){POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=20;return;}
 g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_esp+12);g_eax&=0x0fffffffu;MEM32(g_esi+4)=g_eax;
 g_eax=MEM32(g_esp+16);PUSH32(g_esp,g_eax);MEM32(g_esi)=0x01050001;MEM32(g_esi+12)=g_ecx;MEM32(g_esi+16)=g_edx;MEM32(g_esi+20)=g_eax;MEM32(g_esi+8)=0;
 PUSH32(g_esp,0x3e618b);sub_003E5570();g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=20;
}

static void surface_level(void){
 g_esp-=12;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+20);g_eax=g_esp+20;PUSH32(g_esp,g_eax);
 g_ecx=g_esp+8;PUSH32(g_esp,g_ecx);g_edx=g_esp+16;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+36);
 g_eax=g_esp+24;PUSH32(g_esp,g_eax);g_ecx=g_esp+40;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x3f192e);sub_003EAFD0();
 g_eax=MEM32(g_esp+24);g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_esp+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3f1943);sub_003E6140();
 POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;
}

static void surface_pitch(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi+16);
 if(g_eax){g_eax=(MEM8(g_esi+19)+1u)<<6;g_recomp_zf=g_eax==0;}
 else{
  g_ecx=(MEM32(g_esi+12)>>20)&15;g_eax=MEM8(g_esi+13);g_edx=surface_shift(1,g_ecx);
  if(g_eax==12){g_eax=g_edx+g_edx;g_recomp_zf=1;}
  else if(g_eax>13&&g_eax<=15){unsigned equal=g_eax==15;g_eax=g_edx*4;g_recomp_zf=equal;}
  else{g_eax=(MEM8(MEM8(g_esi+13)+0x3f1d70)&0x3cu)*g_edx;g_eax>>=3;g_recomp_zf=g_eax==0;}
 }
 POP32(g_esp,g_esi);g_esp+=8;
}
static int surface_color_route(uint32_t target){return target==0x3eac46||target==0x3eac6d||target==0x3eac99||target==0x3eaca0||target==0x3eaca7||target==0x3eacae||target==0x3eacb5||target==0x3eacbc||target==0x3eacc3||target==0x3eacca;}
static void surface_format(void){
 g_ecx=MEM32(g_esp+4);g_edx=MEM8(g_ecx+13);g_eax=MEM8(g_edx+0x3ead15);unsigned target=MEM32(g_eax*4+0x3eacec);
 switch(target){case 0x3eac46:case 0x3eac6d:g_eax=8;break;case 0x3eac99:case 0x3eaca0:g_eax=4;break;case 0x3eaca7:case 0x3eacae:g_eax=3;break;case 0x3eacb5:case 0x3eacbc:g_eax=1;break;case 0x3eacc3:g_eax=9;break;default:g_eax=10;break;}
 if(target==0x3eac46||target==0x3eac99||target==0x3eaca7||target==0x3eacb5){
  g_ecx=MEM32(g_ecx+12);PUSH32(g_esp,g_esi);g_esi=((g_ecx&0xf00000)|0x2000)>>4;g_ecx&=0xf000000;g_esi|=g_ecx;g_eax|=g_esi;POP32(g_esp,g_esi);
 }else g_eax|=0x100;
 g_ecx=MEM32(g_esp+8);
 if(g_ecx){
  g_ecx=MEM8(g_ecx+13)-42u;
  if(g_ecx>7){g_esp+=12;return;}
  g_ecx=MEM8(g_ecx+0x3ead3c);target=MEM32(g_ecx*4+0x3ead34);
  if(target==0x3eacd1){g_eax|=0x20;g_esp+=12;return;}
 }else{
  SET_LO8(g_edx,MEM8(g_edx+0x3f1d70));SET_LO8(g_edx,LO8(g_edx)&0x3c);
  if(LO8(g_edx)==0x20){g_eax|=0x20;g_esp+=12;return;}
 }
 g_eax|=0x10;g_recomp_zf=g_eax==0;g_esp+=12;
}
static void surface_depth(void){
 g_eax=MEM32(g_esp+4);g_ecx=MEM8(g_eax+13)-42u;unsigned target=MEM32(g_ecx*4+0x3ead98);
 switch(target){
 case 0x3ead62:g_edx=MEM32(g_esp+8);MEM32(g_edx)=0x477fff00;break;
 case 0x3ead6f:g_eax=MEM32(g_esp+8);MEM32(g_eax)=0x4b7fffff;break;
 case 0x3ead7c:g_ecx=MEM32(g_esp+8);MEM32(g_ecx)=0x43fff800;break;
 default:g_edx=MEM32(g_esp+8);MEM32(g_edx)=0x7149f2ca;break;
 }
 g_esp+=12;
}

int uc_hybrid_surface_owner(uint32_t entry){
 if(!surface_enabled()||(entry!=0x3f1900&&entry!=0x3e6140&&entry!=0x3eafd0&&entry!=0x3e2e10&&entry!=0x3eac30&&entry!=0x3ead50))return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10200u||!surface_span(s-256,292)||!surface_span(0x51c178,1)||MEM8(0x51c178)||!surface_apart(s-256,292,0x51c178,1))return 0;
 if(entry!=0x3e6140){
  uint32_t texture=MEM32(s+4);
  if(!surface_span(texture,20)||!surface_apart(texture,20,s-256,256))return 0;
 }
 if(entry==0x3eafd0){
  /* Keep unrelated level/face readers on the original path until their whole
   * consumers are replayed. No level-zero specialization is used here. */
  if(MEM32(s)!=0x3f192e||!surface_span(0x3f1d70,256)||!surface_apart(s-256,292,0x3f1d70,256))return 0;
  for(unsigned i=4;i<=8;i++){
   uint32_t out=MEM32(s+4*i);
   if(!surface_span(out,4)||!surface_apart(out,4,s-40,76)||!surface_apart(out,4,0x51c178,1))return 0;
  }
 }
 if(entry==0x3e2e10||entry==0x3eac30){if(!surface_span(0x3f1d70,256)||!surface_apart(s-256,292,0x3f1d70,256))return 0;}
 if(entry==0x3eac30){
  if(!surface_span(0x3eacec,1280)||!surface_apart(s-256,292,0x3eacec,1280))return 0;
  uint32_t format=MEM8(MEM32(s+4)+13),target=MEM32(MEM8(format+0x3ead15)*4+0x3eacec),depth=MEM32(s+8);
  if(!surface_color_route(target))return 0;
  if(depth){
   if(!surface_span(depth,14)||!surface_apart(depth,14,s-256,256))return 0;
   uint32_t index=MEM8(depth+13)-42u;
   if(index<=7){target=MEM32(MEM8(index+0x3ead3c)*4+0x3ead34);if(target!=0x3eacd1&&target!=0x3eace5)return 0;}
  }
 }
 if(entry==0x3ead50){
  if(!surface_span(0x3ead98,32))return 0;
  uint32_t index=MEM8(MEM32(s+4)+13)-42u,out=MEM32(s+8);
  if(index>7||!surface_span(out,4)||!surface_apart(out,4,0x51c178,1))return 0;
  uint32_t target=MEM32(index*4+0x3ead98);
  if(target!=0x3ead62&&target!=0x3ead6f&&target!=0x3ead7c&&target!=0x3ead89)return 0;
 }
 switch(entry){case 0x3f1900:surface_level();break;case 0x3e6140:surface_construct();break;case 0x3eafd0:surface_layout();break;case 0x3e2e10:surface_pitch();break;case 0x3eac30:surface_format();break;default:surface_depth();break;}
 return 1;
}
