#include "migration_services_scheduler_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_002D5CCD(void),sub_00207C70(void),sub_002219E0(void),sub_00221BA0(void),sub_00221CA0(void),sub_00221D00(void),sub_00221A10(void),sub_0020A140(void);
#define SCHEDULE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int scheduler_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void scheduler_enqueue(void){
 g_esp-=16;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+24);PUSH32(g_esp,g_seh_ebp);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+4);SET_LO8(g_ecx,MEM8(g_eax+0x2044));uint32_t parent=0;
 if(LO8(g_ecx)){g_ecx=MEM32(g_esp+40);g_edx=MEM32(g_ecx+32);g_eax=g_edx?MEM32(g_edx):MEM32(g_esp+20);PUSH32(g_esp,g_esi);
  while(g_edx){g_ecx=MEM32(g_eax+4);g_esi=MEM32(g_eax);if(g_ecx)g_eax=g_ecx;else{g_edx=MEM32(g_edx+4);if(g_edx)g_eax=MEM32(g_edx);}g_ecx=MEM32(g_esi);if(g_ecx==MEM32(g_ebx+0x58)){parent=g_esi;break;}}POP32(g_esp,g_esi);
 }
 PUSH32(g_esp,8);SCHEDULE_CALL(sub_002D57FB,0x207eab);g_esp+=4;if(g_eax){g_edx=MEM32(g_esp+36);MEM32(g_eax)=g_edx;MEM32(g_eax+4)=g_ebx;}else g_eax=0;
 g_ecx=MEM32(g_edi+4);SET_LO8(g_edx,MEM8(g_ecx+0x2044));if(LO8(g_edx)){g_ecx=MEM32(g_esp+40);PUSH32(g_esp,g_eax);PUSH32(g_esp,parent);SCHEDULE_CALL(sub_00221CA0,0x207ed7);}
 else{g_ecx=MEM32(g_ecx+0x2048);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+44);PUSH32(g_esp,g_eax);SCHEDULE_CALL(sub_00221BA0,0x207ef1);}
 POP32(g_esp,g_edi);g_esp+=4;POP32(g_esp,g_ebx);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=16;
}
static void scheduler_activate(void){
 g_edx=MEM32(g_ecx+4);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+8);g_edx+=0x3c;
 if(g_edi!=0xffffffffu){g_eax=g_edi&1023;PUSH32(g_esp,g_esi);if(MEM32(g_edx+g_eax*8)==g_edi){g_esi=MEM32(g_edx+g_eax*8+4);if(g_esi&&(MEM8(g_esi+0x5c)&8)){
  g_edx=MEM32(g_ecx+4);g_eax=MEM32(g_esi+0x58);MEM8(g_edx+0x2044)=0;g_edx=MEM32(g_ecx+4);MEM32(g_edx+0x2048)=1;g_edx=MEM32(g_ecx+4);MEM32(g_edx+0x204c)=g_eax;g_eax=MEM32(g_ecx+4);g_edx=MEM32(g_eax+48);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x207f5c);scheduler_enqueue();MEM32(g_esi+0x5c)&=~8u;
 }}POP32(g_esp,g_esi);}POP32(g_esp,g_edi);g_esp+=8;
}
static void scheduler_register(void){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);PUSH32(g_esp,g_edi);g_ecx+=0x3c;SCHEDULE_CALL(sub_00207C70,0x208165);
 g_ebx=g_eax;g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+0x204c);MEM32(g_edi+0x58)=g_ecx;g_edx=MEM32(g_esi+4);g_ecx=MEM32(g_edx+52);PUSH32(g_esp,g_edi);SCHEDULE_CALL(sub_002219E0,0x20817f);
 if(LO8(g_eax)){
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+44);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x208193);scheduler_enqueue();
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+0x204c);if(g_ecx!=0xffffffffu){g_edx=g_ecx&1023;if(MEM32(g_eax+g_edx*8+0x3c)==g_ecx)g_eax=MEM32(g_eax+g_edx*8+0x40);else g_eax=0;}else g_eax=0;
  g_ecx=MEM32(g_esi+4);SET_LO8(g_edx,MEM8(g_ecx+0x2044));if(LO8(g_edx)&&g_eax&&(MEM8(g_eax+0x5c)&8)){PUSH32(g_esp,g_ebx);g_ecx=g_esi;PUSH32(g_esp,0x2081d6);scheduler_activate();}
  else{g_edx=MEM32(g_ecx+48);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x2081ef);scheduler_enqueue();}
 }
 MEM32(g_edi+0x5c)&=~8u;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=g_ebx;POP32(g_esp,g_ebx);g_esp+=8;
}
static void scheduler_deactivate(void){
 g_esp-=16;PUSH32(g_esp,g_seh_ebp);uint32_t owner=g_ecx;g_eax=MEM32(owner+4);g_eax=MEM32(g_eax+48);
 if(g_eax){PUSH32(g_esp,g_edi);g_edi=MEM32(g_eax+32);g_edx=g_edi?MEM32(g_edi):MEM32(g_esp+16);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+32);PUSH32(g_esp,g_esi);
  while(g_edi){g_eax=MEM32(g_edx+4);g_esi=MEM32(g_edx);g_ecx=MEM32(g_esi+4);if(g_eax)g_edx=g_eax;else{g_edi=MEM32(g_edi+4);if(g_edi)g_edx=MEM32(g_edi);}if(MEM32(g_esi)!=g_ebx)continue;
   if(g_ecx){if(MEM8(g_ecx+0x5c)&8)break;PUSH32(g_esp,8);SCHEDULE_CALL(sub_0020A140,0x207fc8);}
   g_ecx=MEM32(owner+4);g_ecx=MEM32(g_ecx+48);PUSH32(g_esp,g_esi);SCHEDULE_CALL(sub_00221D00,0x207fd4);PUSH32(g_esp,g_esi);SCHEDULE_CALL(sub_002D5CCD,0x207fda);g_esp+=4;break;
  }POP32(g_esp,g_esi);POP32(g_esp,g_ebx);POP32(g_esp,g_edi);
 }g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void scheduler_unregister(void){
 g_esp-=16;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+24);PUSH32(g_esp,g_seh_ebp);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);uint32_t owner=g_ecx;PUSH32(g_esp,0x208003);scheduler_deactivate();
 g_eax=MEM32(owner+4);g_ecx=MEM32(g_eax+44);g_eax=MEM32(g_ecx+32);g_edx=g_eax?MEM32(g_eax):MEM32(g_esp+24);
 while(g_eax){g_esi=MEM32(g_edx+4);g_edi=MEM32(g_edx);if(g_esi)g_edx=g_esi;else{g_eax=MEM32(g_eax+4);if(g_eax)g_edx=MEM32(g_eax);}if(MEM32(g_edi)!=g_ebx)continue;PUSH32(g_esp,g_edi);SCHEDULE_CALL(sub_00221D00,0x20803c);PUSH32(g_esp,g_edi);SCHEDULE_CALL(sub_002D5CCD,0x208042);g_esp+=4;break;}
 g_eax=MEM32(owner+4);if(g_ebx!=0xffffffffu){g_esi=g_ebx&1023;if(MEM32(g_eax+g_esi*8+0x3c)==g_ebx){g_edi=MEM32(g_eax+g_esi*8+0x40);if(g_edi){
  g_ecx=MEM32(g_eax+52);PUSH32(g_esp,g_edi);SCHEDULE_CALL(sub_00221A10,0x20806c);g_eax=MEM32(owner+4);g_ecx=MEM32(g_eax+g_esi*8+0x3c);g_eax+=0x3c;if(g_ecx==g_ebx)MEM32(g_eax+g_esi*8)=0xffffffffu;g_ecx=MEM32(owner+4);g_ecx=MEM32(g_ecx+56);PUSH32(g_esp,g_edi);SCHEDULE_CALL(sub_002219E0,0x20808d);
 }}}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;POP32(g_esp,g_ebx);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_services_scheduler(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_SCHEDULER");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!scheduler_span(g_esp-256,280))return 0;
 uint32_t p=g_ecx;if(entry==0x208c10){if(!scheduler_span(p,0x87c))return 0;g_eax=MEM32(p+0x878);g_edx=g_eax+1;MEM32(p+0x878)=(int32_t)g_edx>=0x100000?1:g_edx;g_recomp_zf=g_edx==0x100000;g_esp+=4;return 1;}
 if(entry!=0x207e50&&entry!=0x207f00&&entry!=0x207f70&&entry!=0x207ff0&&entry!=0x208150)return 0;
 if(!scheduler_span(p,8)||!scheduler_span(MEM32(p+4),0x2050))return 0;
 if((entry==0x207e50||entry==0x208150)&&!scheduler_span(MEM32(g_esp+4),0x60))return 0;
 switch(entry){case 0x207e50:scheduler_enqueue();break;case 0x207f00:scheduler_activate();break;case 0x207f70:scheduler_deactivate();break;case 0x207ff0:scheduler_unregister();break;case 0x208150:scheduler_register();break;}return 1;
}
#undef SCHEDULE_CALL
