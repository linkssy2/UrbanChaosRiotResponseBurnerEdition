#ifndef UCRR_GAMEPLAY_EVENT_PROBE_H
#define UCRR_GAMEPLAY_EVENT_PROBE_H
/* Diagnostic only: guest calls/returns and writes remain owned by originals.
 * One TLS scope follows the actual target lookup, never an extra guest call. */
static int gameplay_probe_enabled(void){
#if UCRR_PLAY_ONLY
    return 0;
#endif
static int e=-1;if(e<0){const char*p=getenv("UCRR_GAMEPLAY_EVENT_TRACE");e=p&&!strcmp(p,"1");}return e;}
static int gameplay_probe_range(uint32_t p,uint32_t n){p=recomp_normalize_xbox_va(p);return p>=0x10000u&&p<g_xbox_total_ram&&n<=g_xbox_total_ram-p;}
static __declspec(thread) struct {unsigned active;uint32_t object,getter;} gameplay_property_scope;
extern void sub_001F7A70_gen(void);
extern void sub_00207D70_gen(void);
extern void sub_001F90A0_gen(void);
static void gameplay_lookup_probe(void){
    int selected=gameplay_property_scope.active&&gameplay_probe_range(g_esp,4)&&MEM32(g_esp)==0x001F7A80u;
    sub_00207D70_gen();
    if(selected&&gameplay_probe_range(g_eax,4)){
        uint32_t vt=MEM32(g_eax);gameplay_property_scope.object=g_eax;
        if(gameplay_probe_range(vt,12))gameplay_property_scope.getter=MEM32(vt+8);
    }
}
static void gameplay_property_probe(void){
    static __declspec(thread) unsigned reports;
    static __declspec(thread) uint64_t last;
    if(!gameplay_probe_enabled()||!gameplay_probe_range(g_esp,16)||MEM32(g_esp+8)!=0x4651E0u){sub_001F7A70_gen();return;}
    uint32_t caller=MEM32(g_esp),id=MEM32(g_esp+4),out=MEM32(g_esp+12),actor=g_esi;
    unsigned saved_active=gameplay_property_scope.active;uint32_t saved_object=gameplay_property_scope.object,saved_getter=gameplay_property_scope.getter;
    gameplay_property_scope.active=1;gameplay_property_scope.object=gameplay_property_scope.getter=0;
    sub_001F7A70_gen();
    uint64_t now=GetTickCount64();
    if(reports<512&&(!last||now-last>=500)){
        last=now;++reports;int valid=gameplay_probe_range(out,8);
        fprintf(stderr,"[DOOR-PROPERTY] ms=%llu report=%u/512 caller=%08X actor=%08X target_id=%08X object=%08X getter=%08X success=%u out_valid=%d type=%08X value=%08X\n",(unsigned long long)now,reports,caller,actor,id,gameplay_property_scope.object,gameplay_property_scope.getter,(unsigned)(uint8_t)g_eax,valid,valid?MEM32(out):0,valid?MEM32(out+4):0);
    }
    gameplay_property_scope.active=saved_active;gameplay_property_scope.object=saved_object;gameplay_property_scope.getter=saved_getter;
}
static void gameplay_axe_probe(void){
    static __declspec(thread) unsigned reports;
    if(!gameplay_probe_enabled()||reports>=512||!gameplay_probe_range(g_esp,24)){sub_001F90A0_gen();return;}
    uint32_t caller=MEM32(g_esp);
    if(caller!=0x0011268Bu&&caller!=0x00112704u){sub_001F90A0_gen();return;}
    uint32_t args[5];for(unsigned i=0;i<5;++i)args[i]=MEM32(g_esp+4+i*4);
    uint32_t actor=g_esi;sub_001F90A0_gen();++reports;
    fprintf(stderr,"[AXE-EVENT] ms=%llu report=%u/512 caller=%08X actor=%08X args=%08X:%08X:%08X:%08X:%08X result=%08X\n",(unsigned long long)GetTickCount64(),reports,caller,actor,args[0],args[1],args[2],args[3],args[4],g_eax);
}
void recomp_audio_control_trace(uint32_t address,uint32_t value){
    if(address!=0xFE820124u&&address!=0xFE820128u&&address!=0xFE82012Cu&&address!=0xFE820140u)return;
    if(!gameplay_probe_enabled())return;
    static __declspec(thread) struct {uint64_t start;unsigned reports,window,omitted,voices[256];} trace;
    if(trace.reports>=2048)return;
    uint64_t now=GetTickCount64();unsigned voice=value&65535;
    if(now-trace.start>=5000){trace.start=now;trace.window=0;memset(trace.voices,0,sizeof(trace.voices));}
    if(trace.window>=64||voice>=256||trace.voices[voice]>=4){++trace.omitted;return;}
    ++trace.window;++trace.reports;++trace.voices[voice];
    int valid=gameplay_probe_range(g_esp,64);
    fprintf(stderr,"[AUDIO-CONTROL-CALLER] ms=%llu report=%u/2048 omitted=%u address=%08X value=%08X voice=%u esp=%08X ebp=%08X esi=%08X edi=%08X stack_valid=%d stack_words=",(unsigned long long)now,trace.reports,trace.omitted,address,value,voice,g_esp,g_ebp,g_esi,g_edi,valid);
    if(valid)for(unsigned i=0;i<16;++i)fprintf(stderr,"%s%08X",i?":":"",MEM32(g_esp+i*4));
    fputc('\n',stderr);
}
#endif
