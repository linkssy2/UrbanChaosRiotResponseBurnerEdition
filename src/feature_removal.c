/* Separate opt-in feature experiment. Original getters and object updates run;
 * only their first render-admission output may be reduced to zero. The original
 * consumers already take a no-geometry path for that output. No texture guess,
 * query fabrication, guest object deletion, or changed executable asset. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "feature_removal.h"
#include "effect_diagnostics.h"
#include "graphics_features.h"
#include "particle_registration_guard.h"
#include "sky_visibility_guard.h"

extern void sub_00239AB0(void), sub_002B9BA0(void);
extern void sub_0023A330(void), sub_0023AD00(void), sub_0023B570(void);
extern void sub_0023BD50(void), sub_0023D1B0(void);
typedef struct {
    uint64_t calls, suppressed, omitted_items, invalid_output, forwarded;
} UCFeatureCounts;
typedef struct { uint64_t qpc; unsigned mask; } UCFeatureEvent;
static struct {
    int enabled;
    unsigned selected_class; /* zero: all; 1..6: exact particle render table */
    unsigned graphics_class4_disabled;
    int registration_mode, registration_available;
    int sky_cut;
    unsigned mask;
    DWORD owner_tid;
#if !UCRR_PLAY_ONLY
    uint64_t sky_calls,sky_eligible,sky_suppressed,sky_setups;
    uint64_t registration_eligible[6], registration_cut[6];
    uint64_t class_calls[6],class_items[6],class_omitted[6];
    uint64_t start_class_calls[6],start_class_items[6],start_class_omitted[6];
    unsigned initial_mask, event_count, overflow;
    LONG owner_error;
    uint64_t frequency, birth;
    UCFeatureCounts counts[2];
    UCFeatureEvent events[64];
    uint64_t capture_start, capture_end;
    UCFeatureCounts start_counts[2], end_counts[2];
#endif
} uc_feature;
static void uc_feature_snapshot(void);
static int uc_feature_owner(void);
#if !UCRR_PLAY_ONLY
static struct {int enabled;uint64_t entries,window_entries,rejected,calls[3],items[3],window_calls[3],window_items[3];} uc_glass;
int uc_glass_probe_enabled(void){return uc_glass.enabled;}
void uc_glass_observe(unsigned stage,uint32_t object,uint32_t count){
 if(!uc_glass.enabled || !uc_feature_owner())return;
 int window=uc_feature.capture_start && !uc_feature.capture_end;
 if(!stage){++uc_glass.entries;if(window)++uc_glass.window_entries;return;}
 if(!uc_sky_range(object,0x14)||MEM32(object)!=0x457180u){++uc_glass.rejected;return;}
 uint32_t provider=MEM32(object+4);
 if(!uc_sky_range(provider,4)||count>0x100000u){++uc_glass.rejected;return;}
 uint32_t table=MEM32(provider);
 unsigned kind=table==0x450B64u?0:table==0x450B7Cu?1:2;
 ++uc_glass.calls[kind];uc_glass.items[kind]+=count;
 if(window){++uc_glass.window_calls[kind];uc_glass.window_items[kind]+=count;}
}
static struct {
    int enabled;
    uint64_t items, zero, nonfinite, consecutive, cache_hits;
    uint32_t last, bits[1024];
    unsigned char valid[1024];
} uc_rotation;

/* Observe only the actual class4 geometry loop, during the capture window.
 * Integer-only bounded counters; no allocation, timing, guest writes or logs. */
void uc_particle_rotation_observe(uint32_t bits) {
    if (!uc_rotation.enabled || !uc_feature.capture_start || uc_feature.capture_end) return;
    if (!uc_feature_owner()) return;
    unsigned slot = ((bits ^ (bits >> 16)) * 2654435761u) >> 22;
    if (uc_rotation.items && uc_rotation.last == bits) ++uc_rotation.consecutive;
    if (uc_rotation.valid[slot] && uc_rotation.bits[slot] == bits) ++uc_rotation.cache_hits;
    uc_rotation.valid[slot] = 1; uc_rotation.bits[slot] = bits;
    uc_rotation.last = bits; ++uc_rotation.items;
    if (!(bits & 0x7fffffffu)) ++uc_rotation.zero;
    if ((bits & 0x7f800000u) == 0x7f800000u) ++uc_rotation.nonfinite;
}

#else
int uc_glass_probe_enabled(void) { return 0; }
void uc_glass_observe(unsigned stage, uint32_t object, uint32_t count) {
    (void)stage; (void)object; (void)count;
}
void uc_particle_rotation_observe(uint32_t bits) { (void)bits; }
#endif

static int uc_feature_owner(void) {
    if (GetCurrentThreadId() == uc_feature.owner_tid) return 1;
    UC_EFFECT_DIAGNOSTIC(InterlockedIncrement(&uc_feature.owner_error));
    return 0;
}
static void uc_feature_event(void) {
#if !UCRR_PLAY_ONLY

    if (uc_feature.event_count == 64) { ++uc_feature.overflow; return; }
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    uc_feature.events[uc_feature.event_count++] = (UCFeatureEvent){(uint64_t)now.QuadPart, uc_feature.mask};

#else
    (void)0;
#endif
}
void uc_feature_removal_init(void) {
    const char *enabled = getenv("UCRR_FEATURE_REMOVAL");
    if (!enabled || strcmp(enabled, "1")) return;
    const char *value = getenv("UCRR_FEATURE_REMOVAL_MASK");
    if (!value || value[0] < '0' || value[0] > '3' || value[1]) return;
    uc_feature.enabled = 1;
    const char *selector=getenv("UCRR_PARTICLE_CLASS");
    if(selector&&*selector){if(selector[0]<'1'||selector[0]>'6'||selector[1]){uc_feature.enabled=0;return;}uc_feature.selected_class=(unsigned)(selector[0]-'0');}
    uc_feature.mask = (unsigned)(value[0] - '0');
    UC_EFFECT_DIAGNOSTIC(uc_feature.initial_mask = uc_feature.mask);
    if(uc_graphics_features_enabled()){
        unsigned visibility=uc_graphics_features_load();
        uc_feature.mask=((visibility&1)?0:1)|((visibility&4)?0:2);
        UC_EFFECT_DIAGNOSTIC(uc_feature.initial_mask=uc_feature.mask);
        uc_feature.selected_class=0;uc_feature.graphics_class4_disabled=!(visibility&2);
    }
    uc_feature.registration_available=uc_graphics_features_enabled();
    const char *registration=getenv("UCRR_PARTICLE_REGISTRATION_CUT");
    int registration_requested=!(uc_graphics_features_load()&32u);
    /* Explicit benchmark modes keep the historical A/B launchers reproducible. */
    if(registration)registration_requested=!strcmp(registration,"1");
    uc_feature.registration_mode=registration_requested && uc_feature.registration_available && (uc_feature.mask&1);
    uc_feature.sky_cut=uc_feature.registration_available && !(uc_graphics_features_load()&64u);
    uc_feature.owner_tid = GetCurrentThreadId();
#if !UCRR_PLAY_ONLY
    const char *glass=getenv("UCRR_GLASS_RENDER_PROBE");
    uc_glass.enabled=glass&&!strcmp(glass,"1");
    const char *rotation = getenv("UCRR_PARTICLE_ROTATION_PROBE");
    uc_rotation.enabled = rotation && !strcmp(rotation, "1");
    LARGE_INTEGER freq; QueryPerformanceFrequency(&freq);
    uc_feature.frequency = (uint64_t)freq.QuadPart;
    FILETIME born, dead, kernel, user;
    GetProcessTimes(GetCurrentProcess(), &born, &dead, &kernel, &user);
    uc_feature.birth = ((uint64_t)born.dwHighDateTime << 32) | born.dwLowDateTime;
    uc_feature_event();
#endif
}
int uc_feature_removal_enabled(void) { return uc_feature.enabled; }
extern int uc_decal_visibility_ready(void),uc_shadow_visibility_ready(void);
extern void uc_decal_visibility_apply(unsigned),uc_shadow_visibility_apply(unsigned);
int uc_graphics_features_apply(unsigned value){
    if(!uc_feature.enabled||!uc_feature_owner()||!uc_decal_visibility_ready()||!uc_shadow_visibility_ready()||!uc_graphics_features_save(value))return 0;
    uc_feature.mask=((value&1)?0:1)|((value&4)?0:2);
    uc_feature.selected_class=0;uc_feature.graphics_class4_disabled=!(value&2);
    const char *override=getenv("UCRR_PARTICLE_REGISTRATION_CUT");
    int cut=override?!strcmp(override,"1"):!(value&32);
    uc_feature.registration_mode=cut&&uc_feature.registration_available&&(uc_feature.mask&1);
    uc_feature.sky_cut=uc_feature.registration_available&&!(value&64);
    uc_decal_visibility_apply(value);uc_shadow_visibility_apply(value);uc_feature_event();return 1;
}


/* Startup-only. Original pool predicate and original refusal epilogue execute.
 * No simulation, RNG, allocation ownership or registration globals are changed. */
int uc_particle_registration_enabled(void) { return uc_feature.enabled && uc_feature.registration_available; }
int uc_particle_registration_admit(uint32_t r,uint32_t caller) {
    if(!uc_feature.enabled || !uc_feature_owner())return 0;
#if UCRR_PLAY_ONLY
    if(!uc_feature.registration_mode)return 0;
#endif
    unsigned kind=uc_particle_registration_class(r,caller);
    if(!kind)return 0;
    UC_EFFECT_DIAGNOSTIC(++uc_feature.registration_eligible[kind-1]);
    if(!uc_feature.registration_mode)return 0;
    UC_EFFECT_DIAGNOSTIC(++uc_feature.registration_cut[kind-1]);return 1;
}

void uc_sky_setup_observe(void){
#if !UCRR_PLAY_ONLY

 if(uc_feature.enabled && uc_feature_owner())++uc_feature.sky_setups;

#else
    (void)0;
#endif
}
unsigned uc_sky_visibility_value(uint32_t owner,uint32_t renderer,unsigned index){
 if(!uc_feature.enabled||!uc_feature_owner())return 1;
#if UCRR_PLAY_ONLY
 if(!uc_feature.sky_cut)return 1;
#endif
 UC_EFFECT_DIAGNOSTIC(++uc_feature.sky_calls);
 if(!uc_sky_visibility_guard(owner,renderer,index))return 1;
 UC_EFFECT_DIAGNOSTIC(++uc_feature.sky_eligible);
 if(!uc_feature.sky_cut)return 1;
 UC_EFFECT_DIAGNOSTIC(++uc_feature.sky_suppressed);return 0;
}
/* This wrapper adds no speculative reads: output must be an aligned ordinary
 * RAM dword and must have just been produced by the unmodified getter. Alias,
 * malformed, and unexpected-thread calls always retain original behavior. */
static int uc_feature_dword(uint32_t address) {
    return address >= 0x10000u && !(address & 3u) &&
           (uint64_t)address + 4u <= (uint64_t)g_xbox_total_ram;
}
static void uc_feature_get(unsigned feature, uint32_t expected_table, void (*original)(void)) {
#if UCRR_PLAY_ONLY
    if (!(uc_feature.mask & (1u << feature)) &&
        (feature || expected_table != 0x00483020u || !uc_feature.graphics_class4_disabled)) {
        original();
        return;
    }
#endif
    int owned = uc_feature_owner();
    uint32_t output = 0;
    if (owned && g_esp <= UINT32_MAX - 12u && uc_feature_dword(g_esp) &&
        uc_feature_dword(g_esp + 12u) && uc_feature_dword(g_ecx) && MEM32(g_ecx) == expected_table) {
        uint32_t caller = MEM32(g_esp);
        int known_caller = caller == 0x0009AD9Eu;
        if (!feature) known_caller |= caller == 0x0009B1D4u || caller == 0x00099E3Eu || caller == 0x0009A55Cu;
        uint32_t first = MEM32(g_esp + 4u);
        uint32_t second = MEM32(g_esp + 8u), third = MEM32(g_esp + 12u);
        if (known_caller && uc_feature_dword(first) && uc_feature_dword(second) &&
            uc_feature_dword(third) && first != second && first != third && second != third) output = first;
    }
    original();
    if (!owned) return;
#if !UCRR_PLAY_ONLY
    UCFeatureCounts *count = &uc_feature.counts[feature];
#endif
    UC_EFFECT_DIAGNOSTIC(++count->calls);
    if (!uc_feature_dword(output)) {
        UC_EFFECT_DIAGNOSTIC(++count->invalid_output); UC_EFFECT_DIAGNOSTIC(++count->forwarded); return;
    }
    uint32_t items = MEM32(output);
    unsigned kind=feature?6u:(expected_table-0x00482F90u)/0x30u;
    UC_EFFECT_DIAGNOSTIC(if(kind<6){++uc_feature.class_calls[kind];uc_feature.class_items[kind]+=items;});
    int selected=feature||!uc_feature.selected_class||uc_feature.selected_class==kind+1u;
    if (selected && ((uc_feature.mask & (1u << feature)) || (!feature&&kind==3&&uc_feature.graphics_class4_disabled)) && items) {
        MEM32(output) = 0;
        UC_EFFECT_DIAGNOSTIC(++count->suppressed);
        UC_EFFECT_DIAGNOSTIC(count->omitted_items += items);
        UC_EFFECT_DIAGNOSTIC(if(kind<6)uc_feature.class_omitted[kind]+=items);
    } else UC_EFFECT_DIAGNOSTIC(++count->forwarded);
}
void uc_feature_particles(void) { uc_feature_get(0, 0x00482F90u, sub_00239AB0); }
void uc_feature_particles_1(void) { uc_feature_get(0, 0x00482FC0u, sub_0023A330); }
void uc_feature_particles_2(void) { uc_feature_get(0, 0x00482FF0u, sub_0023AD00); }
void uc_feature_particles_3(void) { uc_feature_get(0, 0x00483020u, sub_0023B570); }
void uc_feature_particles_4(void) { uc_feature_get(0, 0x00483050u, sub_0023BD50); }
void uc_feature_particles_5(void) { uc_feature_get(0, 0x00483080u, sub_0023D1B0); }
void uc_feature_blob_shadow(void) { uc_feature_get(1, 0x0048F4D0u, sub_002B9BA0); }

void uc_feature_capture_boundary(int start, uint64_t qpc) {
#if !UCRR_PLAY_ONLY

    extern void uc_scenery_probe_boundary(int, uint64_t);
    uc_scenery_probe_boundary(start, qpc);
#if UCRR_LIGHTING_LIST_PROBE
    extern void uc_geometry_census_boundary(int, uint64_t);
    extern void uc_draw_census_boundary(int, uint64_t);
    extern void uc_guest_light_coverage_boundary(int, uint64_t);
    uc_geometry_census_boundary(start, qpc);
    uc_draw_census_boundary(start, qpc);
    uc_guest_light_coverage_boundary(start, qpc);
#endif
    if (!uc_feature.enabled || !uc_feature_owner()) return;
    if (start) {
        int rotation_enabled = uc_rotation.enabled;
        memset(&uc_rotation, 0, sizeof(uc_rotation));
        uc_rotation.enabled = rotation_enabled;
        uc_feature.capture_end = 0;
        uc_feature.capture_start = qpc;
        memcpy(uc_feature.start_counts, uc_feature.counts, sizeof(uc_feature.counts));
        memcpy(uc_feature.start_class_calls,uc_feature.class_calls,sizeof(uc_feature.class_calls));
        memcpy(uc_feature.start_class_items,uc_feature.class_items,sizeof(uc_feature.class_items));
        memcpy(uc_feature.start_class_omitted,uc_feature.class_omitted,sizeof(uc_feature.class_omitted));
    } else {
        uc_feature.capture_end = qpc;
        memcpy(uc_feature.end_counts, uc_feature.counts, sizeof(uc_feature.counts));
        uc_feature_snapshot();
    }

#else
    (void)start; (void)qpc;
#endif
}
#if !UCRR_PLAY_ONLY
static void uc_feature_write_counts(FILE *file, const UCFeatureCounts *counts) {
    const char *names[] = {"particles", "blob_shadow"};
    fputc('{', file);
    for (unsigned i = 0; i < 2; ++i) {
        const UCFeatureCounts *c = &counts[i];
        fprintf(file, "%s\"%s\":{\"calls\":%llu,\"suppressed\":%llu,\"omitted_items\":%llu,\"invalid_output\":%llu,\"forwarded\":%llu}",
            i ? "," : "", names[i], (unsigned long long)c->calls,
            (unsigned long long)c->suppressed, (unsigned long long)c->omitted_items,
            (unsigned long long)c->invalid_output, (unsigned long long)c->forwarded);
    }
    fputc('}', file);
}

/* F7 exports after a capture. No file writes or clock calls in the getter.
 * A bounded, complete transition history lets the analyzer reject a toggle
 * during the measured interval rather than assuming launch flags stayed active. */
static void uc_feature_snapshot(void) {
    if (!uc_feature_owner()) return;
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!length || length >= MAX_PATH || !slash || slash - path > MAX_PATH - 65) return;
    swprintf(slash + 1, MAX_PATH - (slash + 1 - path), L"feature-removal-%lu.json", GetCurrentProcessId());
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    FILE *file = _wfopen(path, L"wb");
    if (!file) return;
    fprintf(file, "{\"schema\":1,\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"feature_mask\":%u,\"initial_mask\":%u,\"owner_tid\":%lu,\"owner_error\":%ld,\"overflow\":%u,\"events_capacity\":64,\"qpc_frequency\":%llu,\"snapshot_qpc\":%llu,\"events\":[",
        GetCurrentProcessId(), (unsigned long long)uc_feature.birth,
        uc_feature.mask, uc_feature.initial_mask, uc_feature.owner_tid,
        uc_feature.owner_error, uc_feature.overflow,
        (unsigned long long)uc_feature.frequency, (unsigned long long)now.QuadPart);
    for (unsigned i = 0; i < uc_feature.event_count; ++i)
        fprintf(file, "%s{\"sequence\":%u,\"qpc\":%llu,\"mask\":%u,\"applied\":true}",
            i ? "," : "", i + 1, (unsigned long long)uc_feature.events[i].qpc, uc_feature.events[i].mask);
    fputs("],\"counters\":", file); uc_feature_write_counts(file, uc_feature.counts);
    fprintf(file, ",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",
        (unsigned long long)uc_feature.capture_start, (unsigned long long)uc_feature.capture_end);
    uc_feature_write_counts(file, uc_feature.start_counts);
    fputs(",\"end_counts\":", file); uc_feature_write_counts(file, uc_feature.end_counts);
    fprintf(file,"},\"selected_class\":%u,\"graphics_class4_disabled\":%u,\"class_window\":[",uc_feature.selected_class,uc_feature.graphics_class4_disabled);
    for(unsigned i=0;i<6;++i)fprintf(file,"%s{\"table\":%u,\"calls\":%llu,\"items\":%llu,\"omitted\":%llu}",i?",":"",0x482F90u+i*0x30u,(unsigned long long)(uc_feature.class_calls[i]-uc_feature.start_class_calls[i]),(unsigned long long)(uc_feature.class_items[i]-uc_feature.start_class_items[i]),(unsigned long long)(uc_feature.class_omitted[i]-uc_feature.start_class_omitted[i]));
    fprintf(file,"],\"sky\":{\"setups\":%llu,\"cut\":%d,\"calls\":%llu,\"eligible\":%llu,\"suppressed\":%llu},\"registration_mode\":%d,\"registration\":[",(unsigned long long)uc_feature.sky_setups,uc_feature.sky_cut,(unsigned long long)uc_feature.sky_calls,(unsigned long long)uc_feature.sky_eligible,(unsigned long long)uc_feature.sky_suppressed,uc_feature.registration_mode);
    for(unsigned i=0;i<6;i++)fprintf(file,"%s{\"class\":%u,\"eligible\":%llu,\"cut\":%llu}",i?",":"",i+1,(unsigned long long)uc_feature.registration_eligible[i],(unsigned long long)uc_feature.registration_cut[i]);
    fprintf(file,"],\"glass_probe\":{\"enabled\":%d,\"entries\":%llu,\"window_entries\":%llu,\"rejected\":%llu,\"providers\":[",uc_glass.enabled,(unsigned long long)uc_glass.entries,(unsigned long long)uc_glass.window_entries,(unsigned long long)uc_glass.rejected);
    for(unsigned i=0;i<3;i++)fprintf(file,"%s{\"table\":%u,\"calls\":%llu,\"items\":%llu,\"window_calls\":%llu,\"window_items\":%llu}",i?",":"",i==0?0x450B64u:i==1?0x450B7Cu:0,(unsigned long long)uc_glass.calls[i],(unsigned long long)uc_glass.items[i],(unsigned long long)uc_glass.window_calls[i],(unsigned long long)uc_glass.window_items[i]);
    fputs("]}",file);
    fprintf(file,",\"rotation_window\":{\"enabled\":%d,\"items\":%llu,\"zero\":%llu,\"nonfinite\":%llu,\"consecutive\":%llu,\"cache1024_hits\":%llu}}\n",uc_rotation.enabled,(unsigned long long)uc_rotation.items,(unsigned long long)uc_rotation.zero,(unsigned long long)uc_rotation.nonfinite,(unsigned long long)uc_rotation.consecutive,(unsigned long long)uc_rotation.cache_hits); fclose(file);
}
#else
static void uc_feature_snapshot(void) {}
#endif
int uc_feature_removal_key(UINT message, WPARAM key, LPARAM flags) {
    if(uc_graphics_features_enabled())return 0; /* Settings own this package. */
    if (!uc_feature.enabled || (key != VK_F7 && key != VK_F10) || (flags & (1LL << 29))) return 0;
    if (message != WM_KEYDOWN && message != WM_SYSKEYDOWN && message != WM_KEYUP && message != WM_SYSKEYUP) return 0;
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && !(flags & (1LL << 30))) {
        if (key == VK_F7) uc_feature_snapshot();
        else if (uc_feature_owner()) { uc_feature.mask = (uc_feature.mask + 1u) & 3u; uc_feature_event(); }
    }
    return 1;
}
