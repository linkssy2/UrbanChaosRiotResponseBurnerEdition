/* World-level original joint ownership, fixed-body completion and listeners. */
#include "migration_physics_joint_world.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002F9FB0(void);void sub_002FF110(void);void sub_002F6600(void);void sub_002ED490(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-256,288);}
static int joint_valid(uint32_t joint){
 if(!safe(joint,40))return 0;
 for(unsigned i=0;i<2;i++){uint32_t b=MEM32(joint+16+i*4);if(b){if(!safe(b,12))return 0;uint32_t w=MEM32(b+8);if(w&&(!safe(w,0x38)||!safe(MEM32(w+0x34),8)))return 0;}}
 return 1;
}
static int world_valid(uint32_t world){
 if(!safe(world,0xec)||!safe(MEM32(world+0x80),16))return 0;uint32_t n=MEM32(world+0xe4),p=MEM32(world+0xe0);
 if(n>65536||(n&&!safe(p,n*4)))return 0;
 for(unsigned i=0;i<n;i++){uint32_t listener=MEM32(p+i*4);if(listener&&(!safe(listener,4)||!safe(MEM32(listener),8)))return 0;}return 1;
}
static void fixed_bodies(void){
 g_eax=MEM32(g_ecx+16);if(!g_eax){g_eax=MEM32(g_ecx+20)^MEM32(g_ecx+16);if(g_eax){g_eax=MEM32(g_eax+8);if(g_eax){g_eax=MEM32(g_eax+52);MEM32(g_ecx+16)=g_eax;if(MEM16(g_eax+4))++MEM16(g_eax+6);}}}
 g_eax=MEM32(g_ecx+20);if(!g_eax){g_eax^=MEM32(g_ecx+16);if(g_eax){g_eax=MEM32(g_eax+8);if(g_eax){g_eax=MEM32(g_eax+52);MEM32(g_ecx+20)=g_eax;if(MEM16(g_eax+4))++MEM16(g_eax+6);}}}g_esp+=4;
}
static void notify_added(void){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+0xe4)-1;
 if((int32_t)g_edi>=0){g_ebx=MEM32(g_esp+20);do{g_eax=MEM32(g_esi+0xe0);g_ecx=MEM32(g_eax+g_edi*4);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x2fade7);RECOMP_ICALL_SAFE(target,saved);}--g_edi;}while((int32_t)g_edi>=0);}
 g_edx=MEM32(g_esi+0xe4)-1;if((int32_t)g_edx>=0)do{g_eax=MEM32(g_esi+0xe0);if(!MEM32(g_eax+g_edx*4)){g_ebx=MEM32(g_esi+0xe4)-1;g_ecx=g_ebx;MEM32(g_esi+0xe4)=g_ebx;g_eax=g_edx;if((int32_t)g_edx<(int32_t)g_ecx)do{g_ecx=MEM32(g_esi+0xe0);g_edi=MEM32(g_ecx+g_eax*4+4);g_ecx+=g_eax*4;MEM32(g_ecx)=g_edi;g_ecx=MEM32(g_esi+0xe4);++g_eax;}while((int32_t)g_eax<(int32_t)g_ecx);}--g_edx;}while((int32_t)g_edx>=0);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=g_edx==0;g_esp+=4;
}
static void add_joint(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);g_ecx=g_edi;CALL(0x2f659d,fixed_bodies);g_esi=MEM32(g_esp+12);g_ecx=MEM32(g_esi+0x88)+1;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);MEM32(g_esi+0x88)=g_ecx;CALL(0x2f65b5,sub_002FF110);g_eax=MEM32(g_esp+28);g_esp+=8;
 if(g_eax){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);CALL(0x2f65c7,notify_added);g_esp+=8;}
 --MEM32(g_esi+0x88);if(!MEM32(g_esi+0x88)){g_eax=MEM32(g_esi+0x84);if(g_eax){SET_LO8(g_eax,MEM8(g_esi+0x90));if(!(uint8_t)g_eax){g_ecx=g_esi;CALL(0x2f65ed,sub_002ED490);}}}
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
}
static void request_joint(int remove){
 g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x88);
 if(g_eax){g_eax=MEM32(g_esp+(remove?20:16));g_ecx=g_esp+4;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+0x80);MEM8(g_esp+8)=(unsigned char)(remove?9:8);MEM32(g_esp+12)=g_eax;CALL(remove?0x2ed94d:0x2ed8ed,sub_002F9FB0);++MEM32(g_esi+0x84);if(remove){g_eax=MEM32(g_esp+16);MEM8(g_eax)=0;}else g_eax=0;}
 else{g_edx=MEM32(g_esp+(remove?20:16));PUSH32(g_esp,1);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);if(remove){CALL(0x2ed96e,sub_002F6600);g_eax=MEM32(g_esp+28);g_esp+=12;MEM8(g_eax)=1;}else{CALL(0x2ed909,add_joint);g_esp+=12;}}
 POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=remove?12:8;
}
int uc_migration_physics_joint_world(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_JOINT_WORLD");enabled=v&&!strcmp(v,"1");}
 if(!enabled||!span(g_esp-256,288))return 0;uint32_t world=0,joint=0;
 if(entry==0x2f1070){if(!joint_valid(g_ecx))return 0;fixed_bodies();return 1;}
 if(entry==0x2fadc0||entry==0x2f6590){world=MEM32(g_esp+4);joint=MEM32(g_esp+8);}
 else if(entry==0x2ed8c0||entry==0x2ed920){world=g_ecx;joint=MEM32(g_esp+(entry==0x2ed920?8:4));if(entry==0x2ed920&&!safe(MEM32(g_esp+4),1))return 0;}
 else return 0;
 if(!world_valid(world)||!joint_valid(joint))return 0;
 if(entry==0x2fadc0)notify_added();else if(entry==0x2f6590)add_joint();else request_joint(entry==0x2ed920);return 1;
}
#undef CALL
