#ifndef UCRR_SOUND_STAGE_PROBE_H
#define UCRR_SOUND_STAGE_PROBE_H
/* Argument/return snapshots only. No guest calls beyond the original body. */
static __declspec(thread) unsigned sound_stage_sequence, sound_stage_request;
static uint32_t sound_stage_read(uint32_t p)
{ return gameplay_probe_range(p,4)?MEM32(p):0; }
static void sound_stage_probe(unsigned stage, void (*original)(void))
{
    static const char *names[]={"backend","indexed","direct","select","match","channel","object","start"};
    static const unsigned argc[]={3,2,1,5,1,1,1,2};
    static __declspec(thread) unsigned reports[8][6];
    unsigned player=sound_request_player_scope<6?sound_request_player_scope:0, cap=player?4096:64;
    if (!gameplay_probe_enabled() || stage>=8 || reports[stage][player]>=cap ||
        !gameplay_probe_range(g_esp,4*(argc[stage]+1))) { original(); return; }
    unsigned serial=++reports[stage][player], saved=sound_stage_request;
    if (stage==0 || !saved) sound_stage_request=++sound_stage_sequence;
    unsigned request=sound_stage_request;
    uint32_t caller=MEM32(g_esp), object=g_ecx, args[5]={0};
    for(unsigned i=0;i<argc[stage];++i) args[i]=MEM32(g_esp+4*(i+1));
    uint32_t owner=(stage<=2 && gameplay_probe_range(object,0x40))?MEM32(object+0x3C):0;
    uint32_t start_vt=stage==7?sound_stage_read(args[0]):0;
    uint32_t start_target=gameplay_probe_range(start_vt,0x14)?MEM32(start_vt+0x10):0;
    original();
    uint32_t result=g_eax;
    /* Selector output slots were saved before RET; never reread freed stack args. */
    uint32_t descriptor=stage==3?sound_stage_read(args[2]):0;
    uint32_t selected_owner=stage==3?sound_stage_read(args[3]):0;
    uint32_t flags=stage==3?sound_stage_read(args[1]):0;
    uint32_t vt=stage==6?sound_stage_read(result):0;
    uint32_t prep=gameplay_probe_range(vt,0x14)?MEM32(vt+0xC):0;
    uint32_t play=gameplay_probe_range(vt,0x14)?MEM32(vt+0x10):0;
    fprintf(stderr,"[SOUND-STAGE] ms=%llu thread=%lu scope=%s request=%u stage=%s report=%u/%u caller=%08X object=%08X args=%08X,%08X,%08X,%08X,%08X owner_before=%08X result_raw=%08X result_al=%u descriptor=%08X selected_owner=%08X flags=%08X prep_target_snapshot=%08X play_target_snapshot=%08X start_target_snapshot=%08X weapon_event=%u\n",
        (unsigned long long)GetTickCount64(),(unsigned long)GetCurrentThreadId(),sound_scope_name(player),request,names[stage],serial,cap,caller,object,
        args[0],args[1],args[2],args[3],args[4],owner,result,result&255,descriptor,selected_owner,flags,prep,play,start_target,sound_weapon_event);
    sound_stage_request=saved;
}
#endif
