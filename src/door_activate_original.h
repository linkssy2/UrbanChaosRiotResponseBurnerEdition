#ifndef UCRR_DOOR_ACTIVATE_ORIGINAL_H
#define UCRR_DOOR_ACTIVATE_ORIGINAL_H
/* Original 001AFF10..001AFF6D, verified by repair_function_metadata.py.
 * Registered indirectly at 001B191C; absent from scanner's function list.
 * This is the original callback, not a forced door-completion workaround. */
extern void sub_002084E0(void);
extern void sub_00210780(void);
static void ucrr_door_activate_original(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;
 g_eax=MEM32(g_esi+0xFC);PUSH32(g_esp,g_edi);g_edi=0;
 if((int32_t)g_eax>0){
  do{
   g_eax=MEM32(g_esi+0x100);g_ecx=MEM32(g_eax+g_edi*4);
   PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x51C4DC);
   PUSH32(g_esp,0x1AFF35);sub_002084E0();
   g_ecx=g_eax;PUSH32(g_esp,0x1AFF3C);sub_00210780();
   g_eax=MEM32(g_esi+0xFC);++g_edi;
  }while((int32_t)g_edi<(int32_t)g_eax);
 }
 g_edx=MEM32(g_esp+0xC);SET_LO8(g_eax,1);
 MEM8(g_esi+0x122)=LO8(g_eax);MEM8(g_esi+0x152)=LO8(g_eax);MEM8(g_esi+0x120)=LO8(g_eax);
 g_eax=MEM32(g_edx+0x10);POP32(g_esp,g_edi);
 MEM32(g_esi+0x15C)=g_eax;POP32(g_esp,g_esi);g_esp+=12; /* RET 8 */
}
#endif
