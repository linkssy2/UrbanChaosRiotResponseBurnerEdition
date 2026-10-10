#ifndef UCRR_SOUND_REQUEST_PROBE_H
#define UCRR_SOUND_REQUEST_PROBE_H
/* Read-only request-boundary diagnostics. Bank lookup and dispatch are not
 * proof of audible playback. Keep original bodies and return registers intact. */
extern void sub_00216020_gen(void);
extern void sub_00216C60_gen(void);
extern void sub_00216D30_gen(void);
extern void sub_00216BD0_gen(void);
static __declspec(thread) unsigned sound_request_player_scope;
static __declspec(thread) unsigned sound_weapon_sequence, sound_weapon_event;
static const char *sound_scope_name(unsigned scope)
{
    static const char *names[]={"general","player-caller","weapon-fire","weapon-load","weapon-other","player-fire"};
    return names[scope<6?scope:0];
}
static unsigned sound_weapon_category(uint32_t caller)
{
    switch(caller) {
    case 0xBC901: case 0xBC91E: return 5;
    case 0xD85B3: case 0xD9937: case 0xD9950: return 2;
    case 0xD8593: case 0xD20B8: return 3;
    default: return caller>=0xD0000 && caller<0xE0000 ? 4 : 0;
    }
}
static void sound_weapon_probe(void)
{
    static __declspec(thread) unsigned reports[6];
    if(!gameplay_probe_enabled() || !gameplay_probe_range(g_esp,16)) {sub_00216BD0_gen();return;}
    uint32_t caller=MEM32(g_esp),actor=g_ecx,bank=MEM32(g_esp+4),name=MEM32(g_esp+8),out=MEM32(g_esp+12);
    unsigned scope=sound_weapon_category(caller),cap=scope?4096:64;
    if(reports[scope]>=cap){sub_00216BD0_gen();return;}
    unsigned serial=++reports[scope],saved=sound_request_player_scope,saved_event=sound_weapon_event;
    unsigned event=++sound_weapon_sequence;
    char label[96]={0};
    for(unsigned i=0;i<sizeof(label)-1;++i){
        if(name>UINT32_MAX-i || !gameplay_probe_range(name+i,1))break;
        unsigned c=MEM8(name+i);if(!c)break;label[i]=(c>=32&&c<127)?(char)c:'?';
    }
    sound_request_player_scope=scope;sound_weapon_event=event;
    sub_00216BD0_gen();
    fprintf(stderr,"[SOUND-WEAPON] ms=%llu thread=%lu scope=%s event=%u report=%u/%u caller=%08X actor=%08X bank=%08X output_slot=%08X result_raw=%08X result_al=%u name=%s\n",
        (unsigned long long)GetTickCount64(),(unsigned long)GetCurrentThreadId(),sound_scope_name(scope),event,serial,cap,caller,actor,bank,out,g_eax,g_eax&255,label);
    sound_request_player_scope=saved;sound_weapon_event=saved_event;
}
static void sound_dispatch_probe(void)
{
    static __declspec(thread) unsigned reports[6];
    unsigned player=sound_request_player_scope<6?sound_request_player_scope:0, cap=player?8192:256;
    if (!gameplay_probe_enabled() || reports[player]>=cap ||
        !gameplay_probe_range(g_esp,16)) { sub_00216D30_gen(); return; }
    unsigned serial=++reports[player];
    uint32_t object=g_ecx, caller=MEM32(g_esp), bank=MEM32(g_esp+4);
    uint32_t before=gameplay_probe_range(object,0x18)?MEM32(object+0x14):0;
    sub_00216D30_gen();
    uint32_t after=gameplay_probe_range(object,0x18)?MEM32(object+0x14):0;
    uint32_t vt=gameplay_probe_range(after,4)?MEM32(after):0;
    uint32_t target=gameplay_probe_range(vt,0x14)?MEM32(vt+0x10):0;
    fprintf(stderr,"[SOUND-DISPATCH] ms=%llu scope=%s report=%u/%u caller=%08X object=%08X bank=%08X backend_before=%08X backend_after=%08X submit_target_snapshot=%08X result_al=%u weapon_event=%u\n",
        (unsigned long long)GetTickCount64(),sound_scope_name(player),serial,cap,caller,object,bank,before,after,target,g_eax&255,sound_weapon_event);
}
static void sound_request_probe(unsigned dispatch)
{
    static __declspec(thread) unsigned reports[2][2];
    if (!gameplay_probe_enabled() ||
        !gameplay_probe_range(g_esp, dispatch ? 16 : 12)) {
        if (dispatch) sub_00216C60_gen(); else sub_00216020_gen();
        return;
    }
    uint32_t caller=MEM32(g_esp), actor=g_ecx, arg0=MEM32(g_esp+4), arg1=MEM32(g_esp+8);
    /* Original named-footstep caller; do not infer player identity from a
     * recyclable actor pointer. General and category budgets are independent. */
    unsigned player=caller==(dispatch?0xCA612u:0xCA60Au), cap=player?4096:256;
    if (reports[dispatch][player]>=cap) {
        if (dispatch) sub_00216C60_gen(); else sub_00216020_gen();
        return;
    }
    unsigned serial=++reports[dispatch][player];
    uint32_t arg2=dispatch ? MEM32(g_esp+12) : 0;
    char bank[96]={0};
    if (!dispatch && gameplay_probe_range(arg0,4)) {
        uint32_t name=MEM32(arg0);
        for (unsigned i=0;i<sizeof(bank)-1;++i) {
            if (name>UINT32_MAX-i || !gameplay_probe_range(name+i,1)) break;
            unsigned c=MEM8(name+i); if (!c) break;
            bank[i]=(c>=32 && c<127) ? (char)c : '?';
        }
    }
    uint32_t manager=gameplay_probe_range(0x51C4DC,4)?MEM32(0x51C4DC):0;
    uint32_t lookup=gameplay_probe_range(manager,0x70)?MEM32(manager+0x6C):0;
    uint32_t before=dispatch&&gameplay_probe_range(actor,0x7C)?MEM32(actor+0x78):0;
    unsigned saved_scope=sound_request_player_scope;
    sound_request_player_scope=player;
    if (dispatch) sub_00216C60_gen(); else sub_00216020_gen();
    sound_request_player_scope=saved_scope;
    uint32_t after=dispatch&&gameplay_probe_range(actor,0x7C)?MEM32(actor+0x78):0;
    uint32_t vt=gameplay_probe_range(after,4)?MEM32(after):0;
    uint32_t target=gameplay_probe_range(vt,8)?MEM32(vt+4):0;
    fprintf(stderr,"[SOUND-REQUEST] ms=%llu kind=%s scope=%s report=%u/%u caller=%08X actor=%08X arg0=%08X arg1=%08X arg2=%08X manager=%08X lookup=%08X before=%08X after=%08X target=%08X result_raw=%08X bank=%s\n",
        (unsigned long long)GetTickCount64(),dispatch?"dispatch":"lookup",player?"player-caller":"general",serial,cap,caller,actor,arg0,arg1,arg2,manager,lookup,before,after,target,g_eax,bank);
}
#endif
