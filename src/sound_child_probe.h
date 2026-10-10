#ifndef UCRR_SOUND_CHILD_PROBE_H
#define UCRR_SOUND_CHILD_PROBE_H
/* Read-only snapshots. Deferred execution is logged as uncorrelated unless an
 * outer request is currently active; guest pointers are not lifetime IDs. */
static uint32_t sound_child_field(uint32_t p,unsigned offset)
{ return p<=UINT32_MAX-offset ? sound_stage_read(p+offset) : 0; }
void uc_sound_bind_failure(uint32_t gate,uint32_t raw,uint32_t resource,uint32_t playback,uint32_t child,uint32_t backend)
{
    static __declspec(thread) unsigned reports[6];
    unsigned scope=sound_request_player_scope<6?sound_request_player_scope:0;
    if(!gameplay_probe_enabled() || reports[scope]>=4096)return;
    ++reports[scope];
    /* raw is pre-AL-clear EAX. At wrapper gates it can be a wrapper pointer;
     * elsewhere it is just a raw value. Do not interpret it universally. */
    fprintf(stderr,"[SOUND-BIND-FAIL] ms=%llu thread=%lu scope=%s request=%u weapon_event=%u report=%u/4096 gate=%08X eax_before_clear=%08X resource=%08X playback=%08X child=%08X ebp_raw=%08X eax_plus3c=%08X resource_vt=%08X child_flags=%08X child_resource=%08X playback_owner=%08X\n",
        (unsigned long long)GetTickCount64(),(unsigned long)GetCurrentThreadId(),sound_scope_name(scope),sound_stage_request,sound_weapon_event,reports[scope],gate,raw,resource,playback,child,backend,
        sound_child_field(raw,0x3C),sound_child_field(resource,0),sound_child_field(child,0x48),sound_child_field(child,0x38),sound_child_field(playback,0x3C));
}
static unsigned sound_child_count(uint32_t p)
{
    if(!gameplay_probe_range(p,0x30))return UINT32_MAX;
    uint32_t end=p+0x28,node=MEM32(end);
    for(unsigned n=0;n<32;++n){
        if(node==end)return n;
        if(!gameplay_probe_range(node,4))return UINT32_MAX;
        node=MEM32(node);
    }
    return node==end?32:UINT32_MAX-1; /* bounded or cyclic, not exact count */
}
static void sound_child_probe(unsigned stage,void (*original)(void))
{
    static const char *names[]={"prepare","initialize","bind","schedule","enqueue","execute"};
    static const unsigned argc[]={3,0,2,1,1,1};
    static __declspec(thread) unsigned reports[6][6],failures[6];
    unsigned scope=sound_request_player_scope<6?sound_request_player_scope:0;
    if(!gameplay_probe_enabled() || stage>=6 || !gameplay_probe_range(g_esp,4*(argc[stage]+1))) {original();return;}
    unsigned cap=scope?4096:(stage>=3?8192:256);
    /* Queue rejection gets its own budget even after general traffic fills up. */
    unsigned regular=reports[stage][scope]<cap;
    if(!regular && (stage!=4 || failures[scope]>=512)){original();return;}
    uint32_t caller=MEM32(g_esp),object=g_ecx,a[3]={0};
    for(unsigned i=0;i<argc[stage];++i)a[i]=MEM32(g_esp+4+i*4);
    uint32_t event=stage>=3?a[0]:0;
    uint32_t child=stage==2?a[0]:(stage>=3?sound_child_field(event,0x50):0);
    uint32_t playback=stage<=2?object:sound_child_field(child,0x24);
    uint32_t descriptor=sound_child_field(playback,0x50);
    unsigned count_before=sound_child_count(playback);
    uint32_t flags_before=sound_child_field(child,0x48);
    uint32_t init_before=sound_child_field(playback,0x64);
    uint32_t track=sound_child_field(child,0x34);
    uint32_t bound_before=sound_child_field(child,0x38);
    uint32_t due_lo=sound_child_field(event,8),due_hi=sound_child_field(event,12);
    uint32_t event_desc=sound_child_field(event,0x54);
    uint32_t queue=stage==4?object:(stage==3||stage==5?sound_child_field(object,0x10):0);
    uint32_t capacity=sound_child_field(queue,4),used=sound_child_field(queue,8);
    uint32_t now_lo=stage==3||stage==5?sound_child_field(object,8):0;
    uint32_t now_hi=stage==3||stage==5?sound_child_field(object,12):0;
    unsigned descriptor_children=gameplay_probe_range(descriptor,7)?MEM8(descriptor+6):UINT32_MAX;
    original();
    uint32_t result=g_eax;
    if(!regular && (result&255))return;
    unsigned report=regular?++reports[stage][scope]:++failures[scope];
    /* Event dispatch may destroy the event/child. Never inspect those after it. */
    uint32_t after_flags=stage<=2?sound_child_field(child,0x48):0;
    uint32_t after_bound=stage<=2?sound_child_field(child,0x38):0;
    unsigned count_after=stage<=2?sound_child_count(playback):UINT32_MAX;
    fprintf(stderr,"[SOUND-CHILD] ms=%llu thread=%lu scope=%s request=%u weapon_event=%u stage=%s report=%u/%u failure_budget=%u caller=%08X object=%08X args=%08X,%08X,%08X playback=%08X child=%08X event=%08X descriptor=%08X descriptor_children=%u children_before=%u children_after=%u init_before=%08X child_flags_before=%08X child_flags_after=%08X track=%08X bound_before=%08X bound_after=%08X due=%08X:%08X now=%08X:%08X event_descriptor=%08X queue=%08X used=%u capacity=%u result_raw=%08X result_al=%u\n",
        (unsigned long long)GetTickCount64(),(unsigned long)GetCurrentThreadId(),sound_scope_name(scope),sound_stage_request,sound_weapon_event,names[stage],report,regular?cap:512,!regular,caller,object,a[0],a[1],a[2],playback,child,event,descriptor,descriptor_children,count_before,count_after,init_before,flags_before,after_flags,track,bound_before,after_bound,due_hi,due_lo,now_hi,now_lo,event_desc,queue,used,capacity,result,result&255);
}
#endif
