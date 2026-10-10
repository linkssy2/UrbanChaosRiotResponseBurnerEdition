/* Original Xbox .text witnesses are checked by test_transition_callbacks.py.
 * MOV and RET8 do not alter arithmetic flags or general registers. */
static void uc_hostage_transition_original(void){
    MEM8(g_ecx+0x121u)=1;
    g_esp+=12;
}
static void uc_mission_detach_original(void){
    MEM32(g_ecx+0xB0u)=0;
    g_esp+=12;
}
