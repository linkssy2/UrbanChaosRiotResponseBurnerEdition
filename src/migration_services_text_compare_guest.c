#include "migration_services_text_compare_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int tc_span(unsigned p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static unsigned tc_lower(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static void tc_class(void){g_eax=MEM32(g_esp+4);g_ecx=g_eax+1;if(g_ecx<=256){g_ecx=MEM32(0x504b88);g_eax=MEM16(g_ecx+g_eax*2)&MEM32(g_esp+8);}else g_eax=0;g_recomp_zf=!g_eax;g_esp+=4;}
static void tc_fold(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
 if(SMEM32(0x504b90)>1){PUSH32(g_esp,1);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2d824d);tc_class();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);}
 else{g_eax=MEM32(0x504b88);g_eax=MEM8(g_eax+g_esi*2)&1;}
 int upper=!!g_eax;g_eax=g_esi+(upper?32:0);POP32(g_esp,g_esi);g_recomp_zf=!upper;g_esp+=4;
}
static void tc_ascii(int bounded){
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);
 if(bounded){
  g_ecx=MEM32(f+16);
  if(g_ecx){
   g_esi=MEM32(f+8);g_edi=MEM32(f+12);SET_HI8(g_ebx,65);SET_LO8(g_ebx,90);SET_HI8(g_edx,32);
   for(;;){unsigned a=MEM8(g_esi),b=MEM8(g_edi);SET_HI8(g_eax,a);SET_LO8(g_eax,b);if(!a||!b){g_ecx=a==b?0:a<b?0xffffffff:1;break;}
    g_esi++;g_edi++;a=tc_lower(a);b=tc_lower(b);SET_HI8(g_eax,a);SET_LO8(g_eax,b);if(a!=b){g_ecx=a<b?0xffffffff:1;break;}if(!--g_ecx)break;
   }
  }g_eax=g_ecx;
 }else{
  g_esi=MEM32(f+12);g_edi=MEM32(f+8);SET_LO8(g_eax,255);
  while(LO8(g_eax)){
   unsigned right=MEM8(g_esi++),left=MEM8(g_edi++);SET_LO8(g_eax,right);SET_HI8(g_eax,left);if(left==right)continue;
   unsigned r=tc_lower(right),l=tc_lower(left);SET_LO8(g_ecx,l-left);SET_LO8(g_eax,l);SET_HI8(g_eax,r);
   if(l!=r){SET_LO8(g_eax,l<r?255:1);break;}
  }g_eax=(unsigned)(int32_t)(int8_t)LO8(g_eax);
 }
 POP32(g_esp,g_ebx);POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_esp=f+8;
}
static void tc_locale(int bounded){
 unsigned f=0;
 if(bounded){PUSH32(g_esp,g_ebp);f=g_esp;g_ebp=f;if(!MEM32(f+16)){g_eax=0;g_recomp_zf=1;g_esp+=8;return;}}
 if(!MEM32(0x51e6ac)){if(bounded){g_seh_ebp=MEM32(g_esp);g_esp+=4;}tc_ascii(bounded);return;}
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(bounded?f+8:g_esp+12);PUSH32(g_esp,g_edi);g_edi=MEM32(bounded?f+12:g_esp+20);
 for(;;){g_eax=MEM8(g_esi);PUSH32(g_esp,g_eax);if(bounded)g_ebp=f;PUSH32(g_esp,bounded?0x2d798a:0x2d7715);tc_fold();g_ebx=g_eax;g_eax=MEM8(g_edi);PUSH32(g_esp,g_eax);g_esi++;if(bounded)g_ebp=f;PUSH32(g_esp,bounded?0x2d7996:0x2d7721);tc_fold();POP32(g_esp,g_ecx);g_edi++;if(bounded)MEM32(f+16)--;POP32(g_esp,g_ecx);g_ecx=g_eax;if((bounded&&!MEM32(f+16))||!g_ebx||g_ebx!=g_ecx)break;}
 POP32(g_esp,g_edi);g_eax=g_ebx;POP32(g_esp,g_esi);g_eax-=g_ecx;POP32(g_esp,g_ebx);if(bounded)g_esp+=4;g_recomp_zf=!g_eax;g_esp+=4;
}
int uc_migration_services_text_compare(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_TEXT_COMPARE");enabled=p&&!strcmp(p,"1");}
 unsigned s=g_esp;if(!enabled||MEM8(0x51c178)||!tc_span(s-128,148))return 0;
 if(entry==0x2e0e00){if(!tc_span(g_ecx,4))return 0;g_eax=MEM32(g_ecx);g_esp+=4;return 1;}
 if(entry==0x2db150||entry==0x2d8237||entry==0x2d6a1b){
  unsigned ch=MEM32(s+4),p=MEM32(0x504b88)+ch*2;if(ch+1<=256||SMEM32(0x504b90)<=1){if(!tc_span(p,2))return 0;}
  if(entry==0x2db150)tc_class();else if(entry==0x2d8237)tc_fold();else if(SMEM32(0x504b90)>1){PUSH32(g_esp,0x103);PUSH32(g_esp,MEM32(g_esp+8));PUSH32(g_esp,0x2d6a32);tc_class();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);g_recomp_zf=0;g_esp+=4;}else{g_eax=MEM32(s+4);g_ecx=MEM32(0x504b88);g_eax=MEM16(g_ecx+g_eax*2)&0x103;g_recomp_zf=!g_eax;g_esp+=4;}return 1;
 }
 int bounded=entry==0x2d7960||entry==0x2dcf80;if(!bounded&&entry!=0x2d76f3&&entry!=0x2dcf30)return 0;
 unsigned n=bounded?MEM32(s+12):0xffffffff;
 if(n){for(unsigned i=1;i<=2;i++){unsigned p=MEM32(s+4*i);if(!tc_span(p,1))return 0;unsigned room=(unsigned)(g_xbox_total_ram-p);if(n>room&&!memchr((void*)XBOX_PTR(p),0,room))return 0;}if(!tc_span(MEM32(0x504b88)-2,514))return 0;}
 if(entry==0x2dcf30||entry==0x2dcf80)tc_ascii(bounded);else tc_locale(bounded);return 1;
}
