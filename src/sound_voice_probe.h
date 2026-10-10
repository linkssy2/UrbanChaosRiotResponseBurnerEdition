#ifndef UCRR_SOUND_VOICE_PROBE_H
#define UCRR_SOUND_VOICE_PROBE_H
/* Observe actual status queries under pool pressure, never call GetStatus from
 * diagnostics. Cache controls sampling only, not object lifetime/ownership. */
static void sound_voice_status_probe(void (*original)(void))
{
    static __declspec(thread) struct {uint32_t key;uint64_t at;} samples[1024];
    static __declspec(thread) unsigned reports;
    if(!gameplay_probe_enabled() || !gameplay_probe_range(g_esp,8) || reports>=8192){original();return;}
    uint32_t manager=sound_stage_read(0x51CB90),object=g_ecx;
    if(!gameplay_probe_range(manager,0x84) ||
       (sound_child_field(manager,0x78)&&sound_child_field(manager,0x7C))){original();return;}
    unsigned slot=(object>>2)&1023;uint64_t now=GetTickCount64();
    if(samples[slot].key==object && now-samples[slot].at<1000){original();return;}
    samples[slot].key=object;samples[slot].at=now;
    uint32_t caller=MEM32(g_esp),out=MEM32(g_esp+4);
    uint32_t buffer=sound_child_field(object,0x20);
    uint32_t voice=buffer>=0x1C?sound_child_field(buffer-0x1C,0x20):0;
    uint32_t voice_flags=gameplay_probe_range(voice,0x68)?(sound_child_field(voice,0x10)>>16):0;
    unsigned channels=gameplay_probe_range(voice,0x68)?MEM8(voice+0x64):0;
    uint32_t handle=channels>=1&&channels<=3?(sound_child_field(voice,0xA+channels*2)&65535):UINT32_MAX;
    uint32_t ram=sound_stage_read(0x442430),state=0,format=0;
    if(handle<256 && ram<=UINT32_MAX-0x8000 && gameplay_probe_range(ram,0x8000)){
        state=sound_child_field(ram,handle*0x80+0x54);format=sound_child_field(ram,handle*0x80+4);
    }
    uint32_t flags=sound_child_field(object,0x1C),stream=sound_child_field(object,0x24),decoder=sound_child_field(object,0x28);
    original();
    fprintf(stderr,"[SOUND-STATUS] ms=%llu report=%u/8192 caller=%08X backend=%08X flags=%08X buffer=%08X stream=%08X decoder=%08X voice=%08X voice_flags_before=%04X handle=%u hw_state_before=%08X hw_format_before=%08X status=%08X result_raw=%08X\n",
        (unsigned long long)now,++reports,caller,object,flags,buffer,stream,decoder,voice,voice_flags,handle,state,format,sound_stage_read(out),g_eax);
}
/* Allocation/lifetime snapshots only. No guessed voice limits or guest calls. */
static void sound_voice_probe(unsigned stage,void (*original)(void))
{
    static const char *names[]={"factory","backend-init","steal","release","device-factory"};
    static const unsigned argc[]={2,1,2,2,2};
    static __declspec(thread) unsigned reports[5][6];
    unsigned scope=sound_request_player_scope<6?sound_request_player_scope:0;
    if(stage>=5 || !gameplay_probe_enabled() || !gameplay_probe_range(g_esp,4*(argc[stage]+1))){original();return;}
    unsigned cap=8192;
    if(reports[stage][scope]>=cap){original();return;}
    uint32_t object=g_ecx,caller=MEM32(g_esp),a=MEM32(g_esp+4);
    uint32_t b=argc[stage]>1?MEM32(g_esp+8):0;
    uint32_t manager=sound_stage_read(0x51CB90);
    uint32_t before[3],after[3];
    for(unsigned i=0;i<3;++i)before[i]=sound_child_field(manager,0x78+4*i);
    uint32_t type=(stage==0||stage==3||stage==4)?sound_stage_read(a):0;
    uint32_t desc=(stage==0||stage==4)?sound_stage_read(b):0;
    uint32_t backend=stage==1?sound_child_field(object,0x3C):(stage==3&&type==8?b:0);
    uint32_t backend_flags=sound_child_field(backend,0x1C);
    uint32_t wrapper_flags=stage==1?sound_child_field(object,0x44):0;
    uint32_t priority=stage==1?sound_child_field(object,0x4C):0;
    original();
    uint32_t result=g_eax;
    for(unsigned i=0;i<3;++i)after[i]=sound_child_field(manager,0x78+4*i);
    /* Release can destroy its argument: only entry snapshots of that object. */
    uint32_t out_backend=stage==1?sound_child_field(object,0x3C):0;
    fprintf(stderr,"[SOUND-VOICE] ms=%llu thread=%lu scope=%s request=%u weapon_event=%u stage=%s report=%u/%u caller=%08X object=%08X manager=%08X args=%08X,%08X type=%u descriptor_flags=%08X wrapper_flags=%08X priority_pair=%08X backend_before=%08X backend_flags_before=%08X backend_after=%08X available_before=%u,%u,%u available_after=%u,%u,%u result_raw=%08X\n",
        (unsigned long long)GetTickCount64(),(unsigned long)GetCurrentThreadId(),sound_scope_name(scope),sound_stage_request,sound_weapon_event,names[stage],++reports[stage][scope],cap,caller,object,manager,a,b,type,desc,wrapper_flags,priority,backend,backend_flags,out_backend,before[0],before[1],before[2],after[0],after[1],after[2],result);
}
#endif
