#ifndef UCRR_HOSTAGE_TRACE_H
#define UCRR_HOSTAGE_TRACE_H
static int hostage_trace_enabled(void);
/* Recovered live callbacks have their own budget: general dispatch traffic must
 * not consume the evidence for the relationship/state transition. */
void ucrr_hostage_callback_event(uint32_t address,uint32_t phase,uint32_t actor){
 static RECOMP_TLS unsigned count;
 if(!hostage_trace_enabled()||count>=2048)return;
 ++count;
 fprintf(stderr,"[HOSTAGE-CALLBACK] ms=%llu address=%08X phase=%u actor=%08X",
  (unsigned long long)GetTickCount64(),address,phase,actor);
 if(address==0x00161D10u&&gameplay_probe_range(actor,0x124))
  fprintf(stderr," target_id=%08X raw_11c=%08X raw_120=%u link_guard_123=%u",MEM32(actor+0xF0),MEM32(actor+0x11C),MEM8(actor+0x120),MEM8(actor+0x123));
 if(!phase&&gameplay_probe_range(g_esp,12)){
  uint32_t p=MEM32(g_esp+4);fprintf(stderr," arg1=%08X arg2=%08X",p,MEM32(g_esp+8));
  if(gameplay_probe_range(p,4))fprintf(stderr," arg1_word=%08X",MEM32(p));
 }
 fputc('\n',stderr);
}
/* Read-only first-pass script provenance. Raw fields are not decoded poses. */
static int hostage_trace_enabled(void){
#if UCRR_PLAY_ONLY
    return 0;
#endif
static int e=-1;if(e<0){const char*s=getenv("UCRR_HOSTAGE_TRACE");e=s&&!strcmp(s,"1");}return e;}
static RECOMP_TLS struct {uint64_t second;unsigned window,total,omitted,ids[128],count;} hostage_trace;
static int hostage_trace_budget(int priority){
 uint64_t now=GetTickCount64()/1000;
 if(now!=hostage_trace.second){hostage_trace.second=now;hostage_trace.window=0;}
 if(hostage_trace.total>=131072||hostage_trace.window>=(priority?128u:32u)){++hostage_trace.omitted;return 0;}
 ++hostage_trace.total;++hostage_trace.window;return 1;
}
extern void sub_0020A230_gen(void);
static void hostage_binding_trace(void){
 if(!hostage_trace_enabled()||!gameplay_probe_range(g_esp,16)){sub_0020A230_gen();return;}
 uint32_t actor=g_ecx,caller=MEM32(g_esp),key=MEM32(g_esp+4);char name[96]={0};
 if(gameplay_probe_range(key,4)){
  uint32_t s=MEM32(key);for(unsigned i=0;i<95&&gameplay_probe_range(s+i,1);++i){unsigned c=MEM8(s+i);if(c<32||c>126)break;name[i]=(char)c;}
 }
 char lower[96];for(unsigned i=0;i<96;++i)lower[i]=(name[i]>='A'&&name[i]<='Z')?name[i]+32:name[i];
 int matched=strstr(lower,"hostage")!=NULL||strstr(lower,"captor")!=NULL;
 sub_0020A230_gen();
 if(matched){
  if(hostage_trace.count<128)hostage_trace.ids[hostage_trace.count++]=g_eax;
  if(hostage_trace_budget(1))fprintf(stderr,"[HOSTAGE-BIND] ms=%llu thread=%lu caller=%08X owner=%08X name=%s result_id=%08X omitted=%u\n",(unsigned long long)GetTickCount64(),GetCurrentThreadId(),caller,actor,name,g_eax,hostage_trace.omitted);
 }
}
static void hostage_script_trace(void){
 if(!hostage_trace_enabled()||!gameplay_probe_range(g_esp,24)){gameplay_axe_probe();return;}
 uint32_t args[5],caller=MEM32(g_esp),esi=g_esi,edi=g_edi;
 for(unsigned i=0;i<5;++i)args[i]=MEM32(g_esp+4+i*4);
 int matched=0;for(unsigned i=0;i<hostage_trace.count;++i)if(args[0]==hostage_trace.ids[i])matched=1;
 int record=hostage_trace_budget(matched);
 gameplay_axe_probe();
 if(record){
  fprintf(stderr,"[SCRIPT-CALL] ms=%llu thread=%lu caller=%08X esi=%08X edi=%08X hostage_id_match=%d args=%08X:%08X:%08X:%08X:%08X result_al=%u omitted=%u",(unsigned long long)GetTickCount64(),GetCurrentThreadId(),caller,esi,edi,matched,args[0],args[1],args[2],args[3],args[4],(unsigned)(uint8_t)g_eax,hostage_trace.omitted);
  if(matched&&gameplay_probe_range(esi,64)){fprintf(stderr," esi_raw=");for(unsigned i=0;i<16;++i)fprintf(stderr,"%s%08X",i?":":"",MEM32(esi+i*4));}
  fputc('\n',stderr);
 }
}
#endif
