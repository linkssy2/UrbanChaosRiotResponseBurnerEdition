/**
 * Manual function overrides and ICALL diagnostics
 *
 * This file provides:
 *   - recomp_lookup_manual()  : intercept specific Xbox VAs with hand-written code
 *   - recomp_icall_fail_log() : log when an indirect call target can't be resolved
 *   - ICALL trace ring buffer  : globals used by the RECOMP_ICALL macro
 *
 * The recomp pipeline generates an auto-dispatch table (recomp_lookup) that
 * resolves most function addresses. recomp_lookup_manual() is called FIRST,
 * giving you a chance to override any function with a custom implementation.
 *
 * Common reasons to add manual overrides:
 *   - Trace a function to understand call flow (wrap the generated version)
 *   - Fix a function the lifter translated incorrectly
 *   - Stub out a function that crashes (return early, set eax to a safe value)
 *   - Redirect a function to a native implementation (e.g., skip CRT init)
 *   - Intercept D3D/audio calls for custom rendering or sound
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#ifndef UCRR_PLAY_ONLY
#define UCRR_PLAY_ONLY 0
#endif
#include "scenery_submission_probe.h"
#if defined(UCRR_PARTICLE_RESTORATION)
#include "particle_restoration.h"
#endif
#if defined(UCRR_DECAL_RESTORATION)
#include "decal_restoration.h"
#endif
#if defined(UCRR_GLOW_REMOVAL)
#include "glow_removal.h"
#endif
#if defined(UCRR_DECAL_REMOVAL)
#include "decal_removal.h"
#endif
#if defined(UCRR_SPECULAR_REMOVAL)
#include "specular_removal.h"
extern void uc_specular_material(void);
#endif
#if defined(UCRR_LIGHT_REMOVAL)
#include "light_removal.h"
#endif
#if defined(UCRR_SHADOW_REMOVAL)
#include "shadow_removal.h"
extern void sub_000AF0F0(void);
extern void sub_000B3680(void);
/* Preserve the original leaf's read/return and upper EAX. The exact caller
 * consumes only AL to choose its existing empty-caster-tree path. */
static void uc_shadow_casts(void) {
    const uint32_t object = g_ecx;
    const uint32_t caller = g_esp >= 0x10000u && (uint64_t)g_esp + 4u <= g_xbox_total_ram ? MEM32(g_esp) : 0;
    sub_000AF0F0();
    SET_LO8(g_eax, uc_shadow_removal_filter((uint8_t)g_eax, object, caller));
}
/* Coverage counts actual caster submissions in the original shadow route.
 * It does not replace or skip the original caster routine. */
static void uc_shadow_caster(void) {
#if !UCRR_PLAY_ONLY
    const uint32_t object = g_ecx;
    const uint32_t caller = g_esp >= 0x10000u && (uint64_t)g_esp + 4u <= g_xbox_total_ram ? MEM32(g_esp) : 0;
    uc_shadow_removal_caster(object, caller);
#endif
    sub_000B3680();
}
#endif
#if defined(UCRR_FEATURE_REMOVAL)
#include "feature_removal.h"
#endif
#include "nv2a_pb_live.h"
#include "nv2a_pgraph_d3d11.h"
#include "xinput_xbox.h"
#include "display_interval_policy.h"
#include "gpu_fence_probe.h"
#include "gameplay_event_probe.h"
#include "hostage_trace.h"
#include "mission_transition_callbacks.h"
void sub_00161DA0(void){uc_hostage_transition_original();}
void sub_001AEE70(void){uc_mission_detach_original();}
#include "sound_request_probe.h"
#include "sound_stage_probe.h"
#include "sound_child_probe.h"
#include "sound_voice_probe.h"
extern void sub_002519C0_gen(void);
void sub_002519C0(void){sound_voice_status_probe(sub_002519C0_gen);}
extern void sub_00240D80_gen(void);
extern void sub_00245340_gen(void);
extern void sub_002406E0_gen(void);
extern void sub_0023FA30_gen(void);
extern void sub_00243970_gen(void);
void sub_00240D80(void){sound_voice_probe(0,sub_00240D80_gen);}
void sub_00245340(void){sound_voice_probe(1,sub_00245340_gen);}
void sub_002406E0(void){sound_voice_probe(2,sub_002406E0_gen);}
void sub_0023FA30(void){sound_voice_probe(3,sub_0023FA30_gen);}
void sub_00243970(void){sound_voice_probe(4,sub_00243970_gen);}
extern void sub_00249280_gen(void);
extern void sub_0024A1A0_gen(void);
extern void sub_00249CC0_gen(void);
extern void sub_0023FEA0_gen(void);
extern void sub_00244AB0_gen(void);
extern void sub_00241610_gen(void);
void sub_00249280(void){sound_child_probe(0,sub_00249280_gen);}
void sub_0024A1A0(void){sound_child_probe(1,sub_0024A1A0_gen);}
void sub_00249CC0(void){sound_child_probe(2,sub_00249CC0_gen);}
void sub_0023FEA0(void){sound_child_probe(3,sub_0023FEA0_gen);}
void sub_00244AB0(void){sound_child_probe(4,sub_00244AB0_gen);}
void sub_00241610(void){sound_child_probe(5,sub_00241610_gen);}
extern void sub_00216BD0_gen(void);
void sub_00216BD0(void){sound_weapon_probe();}
extern void sub_00246BB0_gen(void);
extern void sub_002468E0_gen(void);
extern void sub_002466A0_gen(void);
extern void sub_0024C650_gen(void);
extern void sub_00245AD0_gen(void);
extern void sub_00246600_gen(void);
extern void sub_00245B80_gen(void);
extern void sub_0024C800_gen(void);
void sub_00246BB0(void){sound_stage_probe(0,sub_00246BB0_gen);}
void sub_002468E0(void){sound_stage_probe(1,sub_002468E0_gen);}
void sub_002466A0(void){sound_stage_probe(2,sub_002466A0_gen);}
void sub_0024C650(void){sound_stage_probe(3,sub_0024C650_gen);}
void sub_00245AD0(void){sound_stage_probe(4,sub_00245AD0_gen);}
void sub_00246600(void){sound_stage_probe(5,sub_00246600_gen);}
void sub_00245B80(void){sound_stage_probe(6,sub_00245B80_gen);}
void sub_0024C800(void){sound_stage_probe(7,sub_0024C800_gen);}
void sub_00216020(void){sound_request_probe(0);}
void sub_00216C60(void){sound_request_probe(1);}
void sub_00216D30(void){sound_dispatch_probe();}
extern void sub_00216D30_gen(void);
extern void sub_00216020_gen(void);
extern void sub_00216C60_gen(void);
#include "pc_mouse_camera.h"
#include "door_activate_original.h"
void sub_001AFF10(void){ucrr_door_activate_original();}
/* Explicit wrapper declaration keeps manual-exclusion discovery intact. */
extern void sub_000BEF80_gen(void);
void sub_000BEF80(void){
    uint32_t object=g_ecx,caller=MEM32(g_esp);
    sub_000BEF80_gen();
    if(caller==0x000D30C3u)ucrr_pc_apply_look(object);
}
/* Explicit definitions/externs keep --exclude-manual wrapper discovery intact. */
extern void sub_001F7A70_gen(void);
extern void sub_00207D70_gen(void);
extern void sub_001F90A0_gen(void);
void sub_001F7A70(void){gameplay_property_probe();}
void sub_00207D70(void){gameplay_lookup_probe();}
void sub_001F90A0(void){hostage_script_trace();}
extern void sub_0020A230_gen(void);
void sub_0020A230(void){hostage_binding_trace();}

/* Game input-manager action query.  Keep the original binding implementation
 * and overlay host input at the point where it returns an action strength.
 * The XPP library in this title otherwise talks only to Xbox USB hardware. */
extern void sub_000567F0_gen(void);
extern void ucrr_host_begin_frontend_input(void);
extern void ucrr_host_end_frontend_input(void);
extern int ucrr_host_take_menu_press(uint32_t action);
extern int ucrr_host_pause_button(void);

/* The two original callers of this device-presence adapter only test nonzero
 * before populating logical controller mappings (0005AA00). Keep the native
 * result, and expose the same host-backed port used by our action adapter.
 * Do not substitute 1 in 0022E570: its other callers may need a device pointer. */
extern void sub_0022C2C0_gen(void);
void sub_0022C2C0(void)
{
    uint32_t port = MEM32(g_esp + 4u);
    static unsigned samples;
    sub_0022C2C0_gen();
    if (!g_eax && port < XBOX_MAX_CONTROLLERS && xbox_InputIsConnected(port)) {
        g_eax = 1;
        if (samples < 8) {
            ++samples;
            fprintf(stderr,"[INPUT-PRESENCE] host port=%u connected\n",port);
        }
    }
}

static float ucrr_host_stick_value(int value)
{
    /* Preserve direction; -32768 is the asymmetric native endpoint. */
    return value < 0 ? value / 32768.0f : value / 32767.0f;
}

static float ucrr_host_stick_axis(int x, int y, unsigned component, unsigned deadzone)
{
    /* Host-controller policy, not a claim about the original Xbox response
     * curve: circular XInput dead zone, then continuous range rescaling. */
    float a = ucrr_host_stick_value(x), b = ucrr_host_stick_value(y);
    float length = sqrtf(a*a + b*b), threshold = deadzone / 32767.0f;
    if (length <= threshold) return 0;
    float strength = ((length > 1 ? 1 : length) - threshold) / (1 - threshold);
    return (component ? b : a) / length * strength;
}

static float ucrr_host_pad_value(const XBOX_GAMEPAD *pad, uint32_t action)
{
    switch (action) {
    case 0x12: return ucrr_host_stick_axis(pad->sThumbLX, pad->sThumbLY, 0, 7849);
    case 0x13: return ucrr_host_stick_axis(pad->sThumbLX, pad->sThumbLY, 1, 7849);
    case 0x14: return ucrr_host_stick_axis(pad->sThumbRX, pad->sThumbRY, 0, 8689);
    case 0x15: return ucrr_host_stick_axis(pad->sThumbRX, pad->sThumbRY, 1, 8689);
    case 0x16: return pad->bAnalogButtons[XBOX_BUTTON_A] / 255.0f;
    case 0x17: return pad->bAnalogButtons[XBOX_BUTTON_B] / 255.0f;
    case 0x18: return pad->bAnalogButtons[XBOX_BUTTON_X] / 255.0f;
    case 0x19: return pad->bAnalogButtons[XBOX_BUTTON_Y] / 255.0f;
    /* Original World Input descriptors order White then Black (4FCD70 +
     * 8/9*16), opposite to XBOX_GAMEPAD's Black then White analog indices. */
    case 0x1A: return pad->bAnalogButtons[XBOX_BUTTON_WHITE] / 255.0f;
    case 0x1B: return pad->bAnalogButtons[XBOX_BUTTON_BLACK] / 255.0f;
    case 0x1C: return (pad->wButtons & XBOX_GAMEPAD_BACK) ? 1.0f : 0.0f;
    case 0x1D: return (pad->wButtons & XBOX_GAMEPAD_START) ? 1.0f : 0.0f;
    case 0x1E: return (pad->wButtons & XBOX_GAMEPAD_DPAD_UP) ? 1.0f : 0.0f;
    case 0x1F: return (pad->wButtons & XBOX_GAMEPAD_DPAD_DOWN) ? 1.0f : 0.0f;
    case 0x20: return (pad->wButtons & XBOX_GAMEPAD_DPAD_LEFT) ? 1.0f : 0.0f;
    case 0x21: return (pad->wButtons & XBOX_GAMEPAD_DPAD_RIGHT) ? 1.0f : 0.0f;
    case 0x22: return pad->bAnalogButtons[XBOX_BUTTON_LTRIGGER] / 255.0f;
    case 0x23: return pad->bAnalogButtons[XBOX_BUTTON_RTRIGGER] / 255.0f;
    case 0x24: return (pad->wButtons & XBOX_GAMEPAD_LEFT_THUMB) ? 1.0f : 0.0f;
    case 0x25: return (pad->wButtons & XBOX_GAMEPAD_RIGHT_THUMB) ? 1.0f : 0.0f;
    default: return 0.0f;
    }
}

static XBOX_INPUT_STATE host_update_pad[XBOX_MAX_CONTROLLERS];
static unsigned host_update_valid[XBOX_MAX_CONTROLLERS];
static DWORD host_update_thread[XBOX_MAX_CONTROLLERS];
static HWND host_update_focus[XBOX_MAX_CONTROLLERS];
static BOOL host_update_blocked[XBOX_MAX_CONTROLLERS];
static int ucrr_shared_input_enabled(void)
{
    static int enabled=-1;
    if(enabled<0){const char*s=getenv("UCRR_SHARED_INPUT_UPDATE");enabled=s&&!strcmp(s,"1");}
    return enabled;
}
static float ucrr_host_action_value(uint32_t player, uint32_t action)
{
    XBOX_INPUT_STATE state;
    if(player<XBOX_MAX_CONTROLLERS&&action>=0x12&&action<=0x15&&ucrr_shared_input_enabled()&&host_update_valid[player]){
        if(host_update_thread[player]==GetCurrentThreadId()&&host_update_focus[player]==GetForegroundWindow()&&host_update_blocked[player]==xbox_InputIsBlocked())
            return ucrr_host_pad_value(&host_update_pad[player].Gamepad,action);
        host_update_valid[player]=0;
    }
    if (player >= XBOX_MAX_CONTROLLERS ||
        xbox_InputGetState(player, &state) != ERROR_SUCCESS) return 0;
    return ucrr_host_pad_value(&state.Gamepad, action);
}

static float host_buttons[XBOX_MAX_CONTROLLERS][16];
static float host_previous_buttons[XBOX_MAX_CONTROLLERS][16];
static unsigned host_button_valid[XBOX_MAX_CONTROLLERS];

/* 0021F750 publishes one controller update through 00208880. Sample once
 * there, never consume an edge in an action query: multiple readers and the
 * original mode-2 chord query must see the same press in this update. */
static void ucrr_host_publish_buttons(uint32_t port)
{
    extern void xbox_MouseOrdersPublish(void);
    XBOX_INPUT_STATE state = {0};
    if (port >= XBOX_MAX_CONTROLLERS) return;
    if(port==0)xbox_MouseOrdersPublish();
    int connected = xbox_InputGetState(port, &state) == ERROR_SUCCESS;
    if(port==0&&connected&&ucrr_host_pause_button())state.Gamepad.wButtons|=XBOX_GAMEPAD_START;
    if(ucrr_shared_input_enabled()){
        if(!connected)memset(&state,0,sizeof(state));
        host_update_pad[port]=state;host_update_thread[port]=GetCurrentThreadId();
        host_update_focus[port]=GetForegroundWindow();host_update_blocked[port]=xbox_InputIsBlocked();
        host_update_valid[port]=1;
    }
    for (unsigned i = 0; i < 16; ++i) {
        host_previous_buttons[port][i] = connected ? host_buttons[port][i] : 0;
        host_buttons[port][i] = connected ? ucrr_host_pad_value(&state.Gamepad, i + 0x16) : 0;
    }
    host_button_valid[port] = 1;
#if UCRR_PLAY_ONLY
    return;
#endif
    static int trace = -1;
    static unsigned samples;
    if (trace < 0) trace = getenv("UCRR_INPUT_TRACE") != NULL && getenv("UCRR_QUIET_PLAY") == NULL;
    if (trace && samples < 64) {
        unsigned held = 0, pressed = 0;
        for (unsigned i = 0; i < 16; ++i) {
            if (host_buttons[port][i] > 0) held |= 1u << i;
            if (host_buttons[port][i] > 0 && host_previous_buttons[port][i] == 0) pressed |= 1u << i;
        }
        if (samples < 4 || pressed) {
            ++samples;
            fprintf(stderr, "[INPUT-PUBLISH] port=%u connected=%d held=%04X pressed=%04X sample=%u\n",
                    port, connected, held, pressed, samples);
        }
    }
}

static float ucrr_host_physical_query(uint32_t player, uint32_t action, uint32_t mode)
{
    if (player >= XBOX_MAX_CONTROLLERS || mode > 4) return 0;
    if (action >= 0x12 && action <= 0x15)
        return mode == 3 || mode == 4 ? ucrr_host_action_value(player, action) : 0;
    if (action < 0x16 || action > 0x25 || mode == 2) return 0;
    if (!host_button_valid[player]) return mode == 3 ? ucrr_host_action_value(player, action) : 0;
    unsigned i = action - 0x16;
    float now = host_buttons[player][i], previous = host_previous_buttons[player][i];
    /* Digital host policy: any nonzero strength is down. Mode 0 is held,
     * mode 1 is rising edge; mode 2 remains the original chord implementation. */
    if (mode == 0) return now > 0;
    if (mode == 1) return now > 0 && previous == 0;
    return mode == 4 ? previous : now;
}

/* 00055BF0 resolves logical actions 0..17 using the selected set of 24
 * {physical,logical} bindings. Follow that table rather than hardcoding a
 * control preset. Mode 2 is handled by the original recursive chord query. */
static float ucrr_host_bound_value(uint32_t manager, uint32_t player,
                                  uint32_t action, uint32_t mode)
{
    if (player >= XBOX_MAX_CONTROLLERS) return 0;
    if (action >= 0x12) {
        return ucrr_host_physical_query(player, action, mode);
    }
    if (mode == 2 || mode > 4) return 0;
    if (manager < 0x10000 || manager >= g_xbox_total_ram ||
        0x958u > g_xbox_total_ram - manager) return 0;
    uint32_t set = MEM32(manager + 0x948 + player * 4);
    uint64_t base = (uint64_t)manager + 0x958 + (uint64_t)set * 24 * 8;
    if (base >= g_xbox_total_ram || 24u * 8 > g_xbox_total_ram - base) return 0;
    float result = 0;
    for (unsigned i = 0; i < 24; ++i) {
        uint32_t entry = (uint32_t)base + i * 8;
        if (MEM32(entry + 4) != action) continue;
        uint32_t physical = MEM32(entry);
        float value = ucrr_host_physical_query(player, physical, mode);
        if (fabsf(value) > fabsf(result)) result = value;
    }
    return result;
}

void sub_000567F0(void)
{
    uint32_t manager = g_ecx;
    uint32_t player = MEM32(g_esp + 4u);
    uint32_t action = MEM32(g_esp + 8u);
    uint32_t query_mode = MEM32(g_esp + 12u);
    uint32_t caller = MEM32(g_esp);
    float host_value;
    double guest_value;
    static int input_trace = -1;
    static unsigned queries, active_samples;

    sub_000567F0_gen();
    guest_value = g_fp_stack[g_fp_top];
    host_value = ucrr_host_bound_value(manager, player, action, query_mode);
    /* Share real discrete press edges across frontend consumers. The host
     * retains Start's existing behavior outside that scope; other modes and
     * players retain their existing behavior. */
    if (player == 0u && query_mode == 1u &&
        ucrr_host_take_menu_press(action)) host_value = 1.0f;
    /* The original continuous-axis path compares absolute strengths and
     * retains the selected value's sign (00056BE3 and 00056C76). */
    if (((query_mode == 3u || query_mode == 4u) &&
         fabsf(host_value) > fabs(guest_value)) ||
        ((query_mode != 3u && query_mode != 4u) && host_value > guest_value)) {
        g_fp_stack[g_fp_top] = host_value;
    }
#if UCRR_PLAY_ONLY
    return;
#endif
    /* Opt-in diagnosis only: do not synthesize presses or alter query modes.
     * Bounded startup/active samples plus a sparse liveness heartbeat. */
    if (input_trace < 0) input_trace = getenv("UCRR_INPUT_VERBOSE_TRACE") != NULL;
    if (input_trace) {
        ++queries;
        if (queries <= 32u || (queries % 65536u) == 0u ||
            (host_value != 0.0f && active_samples < 512u)) {
            if (host_value != 0.0f) ++active_samples;
            fprintf(stderr, "[INPUT-ACTION] ms=%llu query=%u caller=%08X player=%u action=%X mode=%u guest=%.6g host=%.6g result=%.6g active-samples=%u\n",
                    (unsigned long long)GetTickCount64(), queries, caller,
                    player, action, query_mode, guest_value, (double)host_value,
                    g_fp_stack[g_fp_top], active_samples);
        }
    }
}

/* ── Selected generated-function tracing ───────────────────── */

static const uint32_t frontend_sites[] = {
        0x00272010u, 0x00272870u, 0x00263400u, 0x002641C0u,
        0x00018CE0u, 0x00065170u, 0x00271730u, 0x00271AF0u, 0x003F7031u
};
static unsigned frontend_counts[sizeof(frontend_sites) / sizeof(frontend_sites[0])];
static int frontend_enabled = -1;
static unsigned menu_actions;

/* Read-only resource diagnostics. The resolver's RET probe runs before its
 * ret 4, so its original name argument is still at esp+4. No shared nesting
 * stack is needed (resource loads can recurse or switch guest fibers). */
static unsigned resource_entries, resource_returns, resource_misses, geometry_probes, geometry_faults;
static unsigned material_entries, material_returns, material_misses;
static int resource_trace_enabled(void)
{
    if (frontend_enabled < 0)
        frontend_enabled = getenv("UCRR_FRONTEND_TRACE") != NULL;
    return frontend_enabled;
}

static int resource_readable(uint32_t address, uint32_t bytes)
{
    return address >= 0x10000u && address < 0x04000000u &&
           bytes <= 0x04000000u - address;
}

#include "pc_button_prompts.h"

static void resource_name(uint32_t address, char out[192])
{
    unsigned i;
    for (i = 0; i < 191; ++i) {
        unsigned char c;
        if (!resource_readable(address, i + 1)) break;
        c = MEM8(address + i);
        if (!c) break;
        out[i] = c >= 32 && c < 127 ? (char)c : '?';
    }
    out[i] = 0;
}

static void resource_trace(uint32_t va, int returning)
{
    char name[192];
    if (!resource_trace_enabled() || !resource_readable(g_esp, 8)) return;
    if (va == 0x0022B280u) {
        unsigned count;
        int material = MEM32(g_esp) == 0x000A6B57u;
        /* Earlier unrelated menu misses must not exhaust mission-material
         * diagnostics before the geometry constructor is reached. */
        if (returning) {
            count = material ? (g_eax ? ++material_returns : ++material_misses)
                             : (g_eax ? ++resource_returns : ++resource_misses);
            if (count > (g_eax ? 32u : 128u)) return;
        } else if ((material ? ++material_entries : ++resource_entries) > 256) return;
        resource_name(MEM32(g_esp + 4), name);
        fprintf(stderr, "[RESOURCE-%s] esp=%08X caller=%08X name-ptr=%08X name=\"%s\" eax=%08X manager=%08X context=%08X\n",
                returning ? (g_eax ? "FOUND" : "MISS") : "ENTER",
                g_esp, MEM32(g_esp), MEM32(g_esp + 4), name, g_eax,
                returning ? 0 : g_ecx,
                !returning && resource_readable(g_ecx, 16) ? MEM32(g_ecx + 12) : 0);
    } else if (!returning) {
        uint32_t owner = MEM32(g_esp + 4);
        uint32_t count, table, records, record_count;
        if (!resource_readable(owner, 0x34) || !resource_readable(g_ecx, 0x28)) return;
        count = MEM32(owner); table = MEM32(owner + 0x30);
        records = MEM32(g_ecx + 0x24); record_count = MEM32(g_ecx + 0xC);
        if (geometry_probes++ < 64)
        fprintf(stderr, "[GEOMETRY-RESOURCES] esp=%08X caller=%08X object=%08X owner=%08X count=%u table=%08X\n",
                g_esp, MEM32(g_esp), g_ecx, owner, count, table);
        /* Mirror only the records that the original function dereferences.
         * Bound inspection independently from untrusted guest counts. */
        for (uint32_t i = 0; i < record_count && i < 4096 && geometry_faults < 64; ++i) {
            uint32_t index, object = 0;
            uint64_t slot;
            if (!resource_readable(records, i * 0x21u + 8)) break;
            if (MEM16(records + i * 0x21u + 6) != 0xFFFFu) continue;
            index = MEM16(records + i * 0x21u + 4);
            slot = (uint64_t)table + index * 4u;
            if (slot <= 0xFFFFFFFFu && resource_readable((uint32_t)slot, 4))
                object = MEM32((uint32_t)slot);
            if (index >= count || !resource_readable(object, 4)) {
                ++geometry_faults;
                fprintf(stderr, "[GEOMETRY-BAD-RESOURCE] owner=%08X geometry=%08X record=%u index=%u count=%u table=%08X resource=%08X\n",
                        owner, g_ecx, i, index, count, table, object);
                break;
            }
        }
    }
}

static int frontend_site_index(uint32_t va)
{
    for (unsigned i = 0; i < sizeof(frontend_sites) / sizeof(frontend_sites[0]); ++i)
        if (frontend_sites[i] == va) return (int)i;
    return -1;
}

static void frontend_transition_probe(uint32_t object, const char *phase)
{
    static uint32_t previous[7];
    static unsigned samples;
    uint32_t state[7];
    if (!resource_trace_enabled() || samples >= 128u ||
        !resource_readable(object, 0x25)) return;
    state[0] = object;
    state[1] = MEM32(object + 4);
    state[2] = MEM32(object + 8);
    state[3] = MEM32(object + 12);
    state[4] = MEM8(object + 0x14);
    state[5] = resource_readable(state[1], 4) ? MEM32(state[1]) : 0;
    state[6] = resource_readable(state[3], 4) ? MEM32(state[3]) : 0;
    if (memcmp(previous, state, sizeof(state)) == 0) return;
    memcpy(previous, state, sizeof(state));
    ++samples;
    fprintf(stderr, "[FRONTEND-TRANSITION] ms=%llu phase=%s object=%08X screen=%08X kind=%u pending=%08X waiting=%u vtable=%08X pending-vtable=%08X sample=%u\n",
            (unsigned long long)GetTickCount64(), phase, state[0], state[1],
            state[2], state[3], state[4], state[5], state[6], samples);
}

#include "animation_blend_probe.h"
#include "menu_pointer_guest.h"

void recomp_trace_enter(const char *name, uint32_t va)
{
#if UCRR_PLAY_ONLY
    /* These generated trace sites also own input delivery. */
    if (va == 0x00208880u) ucrr_host_publish_buttons(MEM32(g_esp + 4));
    if (va == 0x00272870u) {menu_pointer_begin(g_ecx);ucrr_host_begin_frontend_input();}
    return;
#else

    if(va==0x0009D9B0u){
        static int enabled=-1;static unsigned samples;
        if(enabled<0)enabled=getenv("UCRR_RETICLE_TRACE")!=NULL;
        if(!enabled||samples>=4||!resource_readable(g_esp,36))return;
        uint32_t a=MEM32(g_esp+4),b=MEM32(g_esp+8);
        if(!resource_readable(a,8)||!resource_readable(b,8))return;
        float x=MEMF(a),y=MEMF(a+4);
        if((x!=47.5f&&x!=287.5f)||y!=207.5f)return;
        ++samples;
        fprintf(stderr,"[RETICLE-QUEUE] sample=%u caller=%08X object=%08X xy=%g,%g end=%g,%g esp=%08X\n",samples,MEM32(g_esp),g_ecx,x,y,MEMF(b),MEMF(b+4),g_esp);
        for(unsigned i=0;i<9;++i)fprintf(stderr,"[RETICLE-QUEUE-STACK] sample=%u word=%u value=%08X\n",samples,i,MEM32(g_esp+i*4));
        if(samples==1 && resource_readable(g_esp,1024))
            for(unsigned i=0;i<256;++i)fprintf(stderr,"[RETICLE-CALL-STACK] offset=%X value=%08X\n",i*4,MEM32(g_esp+i*4));
        /* 7AFE0 retains its sprite object in ESI and descriptor+10h in EDI.
         * Its original return address is 54h above this call's entry stack. */
        if(MEM32(g_esp)==0x0007B149u && resource_readable(g_esp,88) &&
           g_edi>=16 && resource_readable(g_edi-16,0x44) && resource_readable(g_esi,0x20)){
            fprintf(stderr,"[RETICLE-SPRITE] caller=%08X object=%08X descriptor=%08X origin=%g,%g extent=%g,%g uvscale=%g,%g\n",
                MEM32(g_esp+84),g_esi,g_edi-16,MEMF(g_edi-16),MEMF(g_edi-12),
                MEMF(g_esi+8),MEMF(g_esi+12),MEMF(g_esi+24),MEMF(g_esi+28));
        }
        return;
    }
    if(va==0x003EF600u || va==0x003EF5C0u){
        static int enabled=-1;static unsigned samples;
        if(enabled<0)enabled=getenv("UCRR_RETICLE_TRACE")!=NULL;
        if(!enabled||samples>=4||!resource_readable(g_esp,128))return;
        uint32_t slot=MEM32(g_esp+4);
        float x=MEMF(g_esp+8),y=MEMF(g_esp+12);
        if((slot!=0&&slot!=UINT32_MAX)||(x!=47.5f&&x!=287.5f)||(y!=207.5f&&y!=271.5f))return;
        ++samples;
        fprintf(stderr,"[RETICLE-PRODUCER] sample=%u va=%08X caller=%08X esp=%08X ebp=%08X esi=%08X edi=%08X x=%g y=%g\n",samples,va,MEM32(g_esp),g_esp,g_ebp,g_esi,g_edi,x,y);
        for(unsigned i=0;i<32;++i)fprintf(stderr,"[RETICLE-PRODUCER-STACK] sample=%u word=%u value=%08X\n",samples,i,MEM32(g_esp+i*4));
        return;
    }
    if (va == 0x00208880u) {
        ucrr_host_publish_buttons(MEM32(g_esp + 4));
        return;
    }
    if(va==0x002B40C0u || va==0x002D6802u){
        static int enabled=-1;static unsigned inspections[2],normal[2],negative[2];
        unsigned kind=va==0x002D6802u;
        if(enabled<0)enabled=getenv("UCRR_CONSTANT_SOURCE_TRACE")!=NULL;
        if(!enabled || inspections[kind]>=131072 || (normal[kind]>=4&&negative[kind]>=16))return;
        ++inspections[kind];
        if(!resource_readable(g_esp,44))return;
        double time=kind?g_fp_stack[(g_fp_top+1)&7]:MEMF(g_esp+12);
        int bad=!isfinite(time)||time<0;
        if(bad){if(negative[kind]>=16)return;++negative[kind];}
        else {if(normal[kind]>=4)return;++normal[kind];}
        fprintf(stderr,"[ANIMATION-TIME-PRODUCER] va=%08X inspection=%u caller=%08X object=%08X parent=%08X time=%.17g divisor=%.17g bad=%d\n",
            va,inspections[kind],MEM32(g_esp),g_ecx,g_esi,time,kind?g_fp_stack[g_fp_top]:0.0,bad);
        for(unsigned i=0;i<11;++i)fprintf(stderr,"[ANIMATION-TIME-PRODUCER-STACK] va=%08X i=%u bits=%08X\n",va,i,MEM32(g_esp+i*4));
        uint32_t object=kind?g_esi:g_ecx;
        if(resource_readable(object,32))for(unsigned i=0;i<8;++i)
            fprintf(stderr,"[ANIMATION-TIME-PRODUCER-OBJECT] va=%08X i=%u bits=%08X\n",va,i,MEM32(object+i*4));
        return;
    }
    if(va==0x001F1A40u || va==0x001F3BA0u){
        static int enabled=-1;static unsigned inspections[2],finite_samples[2],bad_samples[2];
        if(enabled<0)enabled=getenv("UCRR_CONSTANT_SOURCE_TRACE")!=NULL;
        unsigned kind=va==0x001F3BA0u;
        if(!enabled || inspections[kind]>=131072 || (finite_samples[kind]>=4&&bad_samples[kind]>=16))return;
        ++inspections[kind];
        if(!resource_readable(g_esp,28)||!resource_readable(g_ecx,0x44))return;
        uint32_t args[7],fields[3];int bad=0;
        for(unsigned i=0;i<7;++i)args[i]=MEM32(g_esp+i*4);
        for(unsigned i=0;i<3;++i)fields[i]=MEM32(g_ecx+0x20+i*4);
        /* Only floating-point arguments/state are classified. Pointer high
         * bits must not consume the nonfinite sample budget. */
        for(unsigned i=kind?2:3;i<=(kind?3:4);++i)
            if((args[i]&0x7F800000u)==0x7F800000u)bad=1;
        if(!kind)for(unsigned i=0;i<3;++i)if((fields[i]&0x7F800000u)==0x7F800000u)bad=1;
        int outside_unit=kind && (((args[2]&0x80000000u)&&(args[2]&0x7FFFFFFFu)) || ((args[2]&0x7FFFFFFFu)>0x3F800000u));
        if(bad||outside_unit){if(bad_samples[kind]>=16)return;++bad_samples[kind];}
        else{if(finite_samples[kind]>=4)return;++finite_samples[kind];}
        fprintf(stderr,"[ANIMATION-WEIGHT] va=%08X inspection=%u object=%08X bad=%d outside-unit=%d args=%08X,%08X,%08X,%08X,%08X,%08X,%08X fields20=%08X,%08X,%08X\n",
            va,inspections[kind],g_ecx,bad,outside_unit,args[0],args[1],args[2],args[3],args[4],args[5],args[6],fields[0],fields[1],fields[2]);
        if(kind && outside_unit && resource_readable(g_esi,0x38)){
            fprintf(stderr,"[ANIMATION-TIME-CALLER] record=%08X\n",g_esi);
            for(unsigned i=0;i<14;++i)fprintf(stderr,"[ANIMATION-TIME-CALLER-WORD] i=%u bits=%08X\n",i,MEM32(g_esi+i*4));
        }
        if(kind && outside_unit){
            uint32_t count=MEM32(g_ecx+0x1C),table=fields[0];
            if(count && count<=65536 && resource_readable(table,count*4))
                fprintf(stderr,"[ANIMATION-TIME-TABLE] count=%u table=%08X first=%08X last=%08X\n",count,table,MEM32(table),MEM32(table+(count-1)*4));
        }
        if(!kind && (args[0]==0x001F3D92u || args[0]==0x001F3F2Cu || args[0]==0x001F3F6Fu || args[0]==0x001F3F99u)){
            uint64_t caller_stack=(uint64_t)g_esp+20;
            if(caller_stack<=0xFFFFFFFFu && resource_readable((uint32_t)caller_stack,0xB0)){
                uint32_t s=(uint32_t)caller_stack;
                fprintf(stderr,"[ANIMATION-WEIGHT-FRAME] asset=%08X fraction=%08X index=%08X time=%08X weight=%08X scale=%08X sampler-caller=%08X\n",g_edi,MEM32(s+0x28),MEM32(s+0x3C),MEM32(s+0x9C),MEM32(s+0xA0),MEM32(s+0xA4),MEM32(s+0x94));
            }
        }
        return;
    }
    if(va==0x001F0E50u){animation_blend_probe(0);return;}
    if(va==0x002D3F00u){
        /* Original quaternion/translation -> matrix entry. Read-only source
         * provenance probe; no invalid-value sanitization or guest writes. */
        static int enabled=-1;static unsigned inspections,finite_samples,bad_samples;
        if(enabled<0)enabled=getenv("UCRR_CONSTANT_SOURCE_TRACE")!=NULL;
        if(!enabled || inspections>=65536 || (finite_samples>=4&&bad_samples>=16))return;
        ++inspections;
        if(!resource_readable(g_esp,12))return;
        uint32_t quaternion=MEM32(g_esp+4),translation=MEM32(g_esp+8),words[7];
        if(!resource_readable(quaternion,16)||!resource_readable(translation,12))return;
        int bad=0;
        for(unsigned i=0;i<7;++i){words[i]=MEM32(i<4?quaternion+i*4:translation+(i-4)*4);
            if((words[i]&0x7F800000u)==0x7F800000u)bad=1;}
        if(bad){if(bad_samples>=16)return;++bad_samples;}
        else {if(finite_samples>=4)return;++finite_samples;}
        fprintf(stderr,"[POSE-MATRIX-INPUT] inspection=%u caller=%08X destination=%08X quaternion=%08X translation=%08X bad=%d words=%08X,%08X,%08X,%08X,%08X,%08X,%08X\n",
            inspections,MEM32(g_esp),g_ecx,quaternion,translation,bad,words[0],words[1],words[2],words[3],words[4],words[5],words[6]);
        return;
    }
    if(va==0x003E39F0u){
        static int enabled=-1;static unsigned samples;
        if(enabled<0)enabled=getenv("UCRR_MESH_TRACE")!=NULL;
        if(enabled && samples<32 && resource_readable(g_esp,12)){
            ++samples;uint32_t caller=MEM32(g_esp),stage=MEM32(g_esp+4),palette=MEM32(g_esp+8);
            uint32_t common=0,data=0;int readable=palette && resource_readable(palette,12);
            if(readable){common=MEM32(palette);data=MEM32(palette+4);}
            fprintf(stderr,"[PALETTE-BIND-ENTRY] sample=%u caller=%08X stage=%u resource=%08X readable=%d common=%08X data=%08X\n",samples,caller,stage,palette,readable,common,data);
        }
        return;
    }
    if(va==0x003E6A00u){
        /* Original entry contract: ECX is the raw constant slot, EDX the
         * source, [ESP+4] the DWORD count. Read-only, bounded low-palette probe.
         * Do not interpret this diagnostic as resource ownership validation. */
        static int enabled=-1;static unsigned samples,bad_samples,inspections;
        if(enabled<0)enabled=getenv("UCRR_CONSTANT_SOURCE_TRACE")!=NULL;
        if(enabled && inspections<65536 && (samples<64 || bad_samples<8) && g_ecx<96 && g_esp>=0x10000u &&
           (uint64_t)g_esp+8<=g_xbox_total_ram){
            uint32_t count=MEM32(g_esp+4),source=g_edx;
            if(count && count<=768 && source>=0x10000u &&
               (uint64_t)source+(uint64_t)count*4<=g_xbox_total_ram){
                ++inspections;
                uint32_t first=MEM32(source);
                int bad=(first&0x7F800000u)==0x7F800000u;
                /* Reserve evidence for later invalid first words; the known
                 * witness has NaN in every lane. Other-lane-only NaNs are not
                 * covered by this sparse probe. */
                if(samples>=64 && (!bad || bad_samples>=8))return;
                if(bad)++bad_samples;
                ++samples;
                fprintf(stderr,"[CONSTANT-SOURCE] sample=%u slot=%u count=%u source=%08X caller=%08X bad-first=%d inspection=%u\n",samples,g_ecx,count,source,MEM32(g_esp),bad,inspections);
                for(unsigned i=0;i<count;++i)
                    fprintf(stderr,"[CONSTANT-SOURCE-WORD] i=%u bits=%08X\n",i,i?MEM32(source+i*4):first);
                if(bad && MEM32(g_esp)==0x000BABE4u && g_ebx>=0x10000u &&
                   (uint64_t)g_ebx+0x78<=g_xbox_total_ram){
                    uint32_t base=MEM32(g_ebx+0x74),model=MEM32(g_ebx+0x68);
                    if(source>=base && (source-base)%48==0 && model>=0x10000u &&
                       (uint64_t)model+0x48<=g_xbox_total_ram){
                        uint32_t bone=(source-base)/48,n=MEM32(model+0xC);
                        if(bone<n && n<=4096){
                            uint64_t matrices[3]={
                                (uint64_t)MEM32(model+0x40)+bone*0xA0ull+0x60,
                                (uint64_t)MEM32(g_ebx+0x70)+bone*64ull,
                                (uint64_t)MEM32(g_ebx+0x6C)+bone*64ull};
                            fprintf(stderr,"[BONE-SOURCE] object=%08X model=%08X bone=%u count=%u scale=%08X palette=%08X\n",g_ebx,model,bone,n,MEM32(model+0x44),base);
                            for(unsigned m=0;m<3;++m){
                                if(matrices[m]<0x10000u || matrices[m]+64>g_xbox_total_ram){fprintf(stderr,"[BONE-SOURCE-RANGE] kind=%u invalid=%llX\n",m,(unsigned long long)matrices[m]);continue;}
                                for(unsigned i=0;i<16;++i)fprintf(stderr,"[BONE-SOURCE-MATRIX] kind=%u address=%08X i=%u bits=%08X\n",m,(uint32_t)matrices[m],i,MEM32((uint32_t)matrices[m]+i*4));
                            }
                        }
                    }
                }
            }
        }
        return;
    }
    /* Semantic input boundary, deliberately independent of trace enablement.
     * Both the input gatherer and screen query the same action this tick. */
    if (va == 0x00272870u) {menu_pointer_begin(g_ecx);ucrr_host_begin_frontend_input();}
    if (va == 0x00272870u) frontend_transition_probe(g_ecx, "enter");
    if (va == 0x0022B280u || va == 0x000A60E0u) {
        resource_trace(va, 0);
        return;
    }
    for (unsigned i = 0; i < sizeof(frontend_sites) / sizeof(frontend_sites[0]); ++i) {
        if (va != frontend_sites[i]) continue;
        if (frontend_enabled < 0)
            frontend_enabled = getenv("UCRR_FRONTEND_TRACE") != NULL;
        if (!frontend_enabled) return;
        unsigned count = ++frontend_counts[i];
        if (count <= 4 || count % 600 == 0) {
            fprintf(stderr, "[FRONTEND] va=%08X calls=%u this=%08X ret=%08X args=%08X,%08X,%08X\n",
                    va, count, g_ecx, MEM32(g_esp), MEM32(g_esp + 4),
                    MEM32(g_esp + 8), MEM32(g_esp + 12));
            /* Do not dereference an unvalidated object just to log it. */
            if (g_ecx >= 0x10000u && g_ecx <= 0x03FFFFE0u &&
                (va == 0x00263400u || va == 0x00272870u)) {
                fprintf(stderr, "[FRONTEND-STATE] va=%08X object=%08X fields=%08X,%08X,%08X,%08X selected-arg=%u\n",
                        va, g_ecx, MEM32(g_ecx), MEM32(g_ecx + 4),
                        MEM32(g_ecx + 8), MEM32(g_ecx + 12), MEM8(g_esp + 8));
            }
        }
        return;
    }
    if (va == 0x00083B5Du) {
        return;
    }
    fprintf(stderr,
        "[RECOMP] enter %s (0x%08X) esp=%08X ret=%08X ebp=%08X seh=%08X "
        "args=%08X,%08X,%08X,%08X\n",
        name, va, g_esp, MEM32(g_esp), g_ebp, g_seh_ebp,
        MEM32(g_esp + 4), MEM32(g_esp + 8),
        MEM32(g_esp + 12), MEM32(g_esp + 16));
    if (va == 0x002E0AC0u) {
        fprintf(stderr,
            "[RECOMP] init-list head=%08X nodes=%08X/%08X/%08X/%08X/%08X "
            "results=%08X/%08X/%08X/%08X/%08X\n",
            MEM32(0x51E7A4),
            MEM32(0x505638), MEM32(0x5056A4), MEM32(0x5095EC),
            MEM32(0x5095F8), MEM32(0x509604),
            MEM32(0x51E78C), MEM32(0x51E818), MEM32(0x520200),
            MEM32(0x520238), MEM32(0x52023C));
    }
#endif
}

void recomp_trace_exit(const char *name, uint32_t va)
{
#if UCRR_PLAY_ONLY
    if (va == 0x00272870u) {menu_pointer_end();ucrr_host_end_frontend_input();}
    return;
#else

    if(va==0x003EF600u || va==0x003EF5C0u || va==0x0009D9B0u)return;
    if (va == 0x00208880u) return;
    if(va==0x001F0E50u){animation_blend_probe(1);return;}
    if(va==0x003E6A00u || va==0x003E39F0u || va==0x002D3F00u || va==0x001F1A40u || va==0x001F3BA0u || va==0x002B40C0u || va==0x002D6802u)return;
    if (va == 0x00272870u) {menu_pointer_end();ucrr_host_end_frontend_input();}
    if (va == 0x0022B280u || va == 0x000A60E0u) {
        resource_trace(va, 1);
        return;
    }
    int index = frontend_site_index(va);
    if (index >= 0) {
        unsigned count = frontend_counts[index];
        if (frontend_enabled == 1 && va == 0x00263400u && g_eax && menu_actions < 64) {
            ++menu_actions;
            fprintf(stderr, "[MENU-ACTION] callback=%08X esp=%08X\n", g_eax, g_esp);
        }
        if (frontend_enabled == 1 && count && (count <= 4 || count % 600 == 0)) {
            fprintf(stderr, "[FRONTEND-RETURN] va=%08X calls=%u eax=%08X esp=%08X\n",
                    va, count, g_eax, g_esp);
        }
        return;
    }
    if (va == 0x00083B5Du) {
        return;
    }
    fprintf(stderr,
        "[RECOMP] exit  %s (0x%08X) eax=%08X esp=%08X ebp=%08X seh=%08X\n",
        name, va, g_eax, g_esp, g_ebp, g_seh_ebp);
#endif
}

void recomp_trace_esp(const char *name, const char *tag)
{
#if UCRR_PLAY_ONLY
    return;
#else

    if (!strcmp(name,"sub_00272870") && !strcmp(tag,"transition-ready")) {
        static int enabled=-1; static unsigned samples;
        if(enabled<0)enabled=getenv("UCRR_AUDIO_WAIT_TRACE")!=NULL;
        if(enabled && samples<128 && resource_readable(g_esi,0x25) && resource_readable(g_esp,0x25) &&
           (g_eax || MEM8(g_esi+0x14))) {
            uint32_t screen=MEM32(g_esi+4),vt=resource_readable(screen,4)?MEM32(screen):0;
            fprintf(stderr,"[XMV-READY] tick_ms=%llu manager=%08X screen=%08X update=%08X callback=%08X pending=%08X waiting=%u ready=%u time=%g pending_time=%g dt=%g\n",
                (unsigned long long)GetTickCount64(),g_esi,screen,resource_readable(vt,8)?MEM32(vt+4):0,
                g_eax,MEM32(g_esi+12),MEM8(g_esi+0x14),MEM8(g_esp+11),MEMF(g_esi+0x1C),MEMF(g_esi+0x20),
                resource_readable(g_esp,0x68)?MEMF(g_esp+0x64):0);
            ++samples;
        }
        return;
    }
    /* Same suppression, but the sampled hot producer need not walk the list.
     * No change to transition probes or entry/exit input lifetime hooks. */
    if(strcmp(name,"sub_001F0E50")==0)return;
    if(strcmp(name,"sub_003EF600")==0||strcmp(name,"sub_003EF5C0")==0||strcmp(name,"sub_0009D9B0")==0)return;
    if (strcmp(name, "sub_00208880") == 0) return;
    if(strcmp(name,"sub_003E6A00")==0 || strcmp(name,"sub_003E39F0")==0 || strcmp(name,"sub_002D3F00")==0 || strcmp(name,"sub_001F0E50")==0 || strcmp(name,"sub_001F1A40")==0 || strcmp(name,"sub_001F3BA0")==0 || strcmp(name,"sub_002B40C0")==0 || strcmp(name,"sub_002D6802")==0)return;
    /* ESI still owns the controller at both generated pop-EDI sites. */
    if (strcmp(name, "sub_00272870") == 0 && strcmp(tag, "pop edi") == 0)
        frontend_transition_probe(g_esi, "leave");
    if (strcmp(name, "sub_0022B280") == 0 || strcmp(name, "sub_000A60E0") == 0)
        return;
    /* --trace-functions also inserts per-call/pop probes. Suppress those for
     * frontend sites: one glyph can otherwise produce many lines per frame,
     * even with entry sampling or the diagnostic disabled. */
    if (strncmp(name, "sub_", 4) == 0 && strlen(name) == 12 &&
        frontend_site_index((uint32_t)strtoul(name + 4, NULL, 16)) >= 0) {
        return;
    }
    if (strcmp(name, "sub_00083B5D") == 0) {
        return;
    }
    fprintf(stderr,
        "[RECOMP] esp   %s %s: eax=%08X esp=%08X esi=%08X edi=%08X ebx=%08X\n",
        name, tag, g_eax, g_esp, g_esi, g_edi, g_ebx);
#endif
}

void recomp_smartptr_trace(uint32_t original_arg, uint32_t adjusted,
                           uint32_t vtable, uint32_t target)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    fprintf(stderr,
        "[SMARTPTR] arg=%08X base=%08X vtable=%08X target=%08X "
        "esp=%08X ret=%08X ecx=%08X\n",
        original_arg, adjusted, vtable, target,
        g_esp, MEM32(g_esp), g_ecx);
}

void recomp_dsound_trace(const char *stage, uint32_t value, uint32_t object)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    fprintf(stderr, "[DSINIT] %-18s value=%08X object=%08X esp=%08X\n",
        stage, value, object, g_esp);
}

void recomp_stream_copy_trace(const char *stage, uint32_t object,
                              uint32_t destination, uint32_t requested,
                              uint32_t available, uint32_t result)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static unsigned emitted;
    static unsigned calls;
    uint32_t caller = 0;
    uint32_t source = 0;

    ++calls;
    if (strcmp(stage, "entry") == 0) {
        /* sub_0008E030 has saved ebx/ebp/esi when this probe runs. */
        caller = MEM32(g_esp + 12);
        source = MEM32(object + 8);
    }

    if (emitted >= 128 && (calls % 1024) != 0) {
        return;
    }
    ++emitted;
    fprintf(stderr,
        "[STREAMCOPY] #%u %-7s obj=%08X src=%08X dst=%08X "
        "requested=%u/%08X available=%u/%08X result=%08X caller=%08X\n",
        calls, stage, object, source, destination,
        requested, requested, available, available, result, caller);
}

void recomp_loader_count_trace(const char *stage, uint32_t stream,
                               uint32_t count, uint32_t remaining)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    uint32_t adapter = MEM32(stream + 8);
    uint32_t buffer = adapter ? MEM32(adapter) : 0;
    uint32_t base = buffer ? MEM32(buffer) : 0;
    uint32_t end = buffer ? MEM32(buffer + 0xC) : 0;
    uint32_t cursor = buffer ? MEM32(buffer + 0x10) : 0;

    fprintf(stderr,
        "[LOADCOUNT] %-12s stream=%08X count=%u/%08X remaining=%u/%08X "
        "adapter=%08X buffer=%08X base=%08X cursor=%08X end=%08X esp=%08X\n",
        stage, stream, count, count, remaining, remaining,
        adapter, buffer, base, cursor, end, g_esp);

    if (strcmp(stage, "outer-records") == 0 && cursor >= 36) {
        uint32_t start = cursor - 36;
        unsigned row;
        for (row = 0; row < 8; ++row) {
            uint32_t address = start + row * 16;
            unsigned column;
            fprintf(stderr, "[LOADBYTES] %08X  ", address);
            for (column = 0; column < 16; ++column) {
                fprintf(stderr, "%02X ", MEM8(address + column));
            }
            fprintf(stderr, " ");
            for (column = 0; column < 16; ++column) {
                unsigned char ch = MEM8(address + column);
                fputc((ch >= 32 && ch <= 126) ? ch : '.', stderr);
            }
            fputc('\n', stderr);
        }
    }
}

void recomp_loader_fields_trace(uint32_t stream, uint32_t total,
                                uint32_t first, uint32_t second,
                                uint32_t third, uint32_t fourth)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    uint32_t adapter = MEM32(stream + 8);
    uint32_t buffer = adapter ? MEM32(adapter) : 0;
    uint32_t end = buffer ? MEM32(buffer + 0xC) : 0;
    uint32_t cursor = buffer ? MEM32(buffer + 0x10) : 0;

    fprintf(stderr,
        "[LOADFIELDS] stream=%08X counts=%u/%u/%u/%u total=%u "
        "remaining=%u cursor=%08X end=%08X esp=%08X\n",
        stream, first, second, third, fourth, total,
        end - cursor, cursor, end, g_esp);
}

void recomp_resource_lookup_trace(const char *stage, uint32_t path,
                                  uint32_t target, uint32_t value)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    char text[160];
    unsigned i = 0;

    if (path) {
        while (i + 1 < sizeof(text)) {
            unsigned char ch = MEM8(path + i);
            text[i++] = (char)ch;
            if (ch == 0) {
                break;
            }
        }
    }
    if (i == 0 || text[i - 1] != 0) {
        text[i < sizeof(text) ? i : sizeof(text) - 1] = 0;
    }
    if (strcmp(stage, "index-insert") == 0 &&
        strncmp(text, "99-01.", 6) != 0) {
        return;
    }
    fprintf(stderr,
        "[RESOURCE] %-12s path=%08X \"%s\" target=%08X value=%08X "
        "esp=%08X\n",
        stage, path, path ? text : "<null>", target, value, g_esp);
}

static unsigned recomp_copy_guest_sso(char *output, unsigned output_size,
                                      uint32_t object, uint32_t storage_offset,
                                      uint32_t length_offset,
                                      uint32_t capacity_offset)
{
    uint32_t length = MEM32(object + length_offset);
    uint32_t capacity = MEM32(object + capacity_offset);
    uint32_t source = object + storage_offset;
    unsigned copied;

    if (length >= output_size || capacity > 0x00100000u) {
        snprintf(output, output_size, "<invalid:%u/%u>", length, capacity);
        return 0;
    }
    if (capacity >= 16) {
        source = MEM32(source);
    }
    copied = length < output_size - 1 ? length : output_size - 1;
    for (unsigned i = 0; i < copied; ++i) {
        unsigned char ch = MEM8(source + i);
        output[i] = (ch >= 32 && ch <= 126) ? (char)ch : '.';
    }
    output[copied] = 0;
    return copied;
}

void recomp_tree_lookup_trace(const char *stage, uint32_t query,
                              uint32_t node)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    char query_text[96];
    char node_text[96];

    recomp_copy_guest_sso(query_text, sizeof(query_text), query,
                          4, 0x14, 0x18);
    if (strncmp(query_text, "99-01.", 6) != 0) {
        return;
    }
    recomp_copy_guest_sso(node_text, sizeof(node_text), node,
                          0x10, 0x20, 0x24);
    fprintf(stderr,
        "[TREELOOKUP] %-12s query=%08X \"%s\" node=%08X \"%s\" "
        "payload_len=%u payload_cap=%u aux=%u esp=%08X\n",
        stage, query, query_text, node, node_text,
        MEM32(node + 0x3C), MEM32(node + 0x40), MEM32(node + 0x44), g_esp);
}

void recomp_tree_compare_trace(uint32_t query, uint32_t node,
                               int32_t result)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    char query_text[96];
    char node_text[96];

    recomp_copy_guest_sso(query_text, sizeof(query_text), query,
                          4, 0x14, 0x18);
    if (strncmp(query_text, "99-01.", 6) != 0) {
        return;
    }
    recomp_copy_guest_sso(node_text, sizeof(node_text), node,
                          0x10, 0x20, 0x24);
    fprintf(stderr,
        "[TREECMP] query=\"%s\" node=%08X \"%s\" result=%d "
        "left=%08X right=%08X parent=%08X esp=%08X\n",
        query_text, node, node_text, result,
        MEM32(node), MEM32(node + 8), MEM32(node + 4), g_esp);
}

void recomp_archive_index_trace(const char *stage, uint32_t path,
                                uint32_t tree, uint32_t count,
                                uint32_t name, uint32_t value)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    char path_text[128];
    char name_text[128];

    if (name) {
        unsigned i;
        for (i = 0; i < 6; ++i) {
            name_text[i] = (char)MEM8(name + i);
        }
        name_text[6] = 0;
        if (strcmp(name_text, "99-01.") != 0) {
            return;
        }
    }
    for (unsigned i = 0; i + 1 < sizeof(path_text); ++i) {
        unsigned char ch = path ? MEM8(path + i) : 0;
        path_text[i] = (char)ch;
        if (ch == 0) {
            break;
        }
        path_text[i + 1] = 0;
    }
    if (name) {
        unsigned i = 0;
        while (i + 1 < sizeof(name_text)) {
            unsigned char ch = MEM8(name + i);
            name_text[i++] = (char)ch;
            if (ch == 0) {
                break;
            }
        }
        name_text[sizeof(name_text) - 1] = 0;
    } else {
        strcpy_s(name_text, sizeof(name_text), "<none>");
    }
    fprintf(stderr,
        "[ARCHIVEINDEX] %-6s path=\"%s\" tree=%08X count=%u "
        "name=\"%s\" value=%08X esp=%08X\n",
        stage, path_text, tree, count, name_text, value, g_esp);
}

void recomp_tree_insert_trace(const char *stage, uint32_t tree,
                              uint32_t item, uint32_t first,
                              uint32_t second)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static unsigned emitted;
    char item_text[96];
    int interesting;

    recomp_copy_guest_sso(item_text, sizeof(item_text), item,
                          4, 0x14, 0x18);
    interesting = strncmp(item_text, "99-01.", 6) == 0;
    if (!interesting && emitted >= 20) {
        return;
    }
    ++emitted;
    fprintf(stderr,
        "[TREEINSERT] %-9s tree=%08X size=%u item=%08X \"%s\" "
        "first=%08X second=%08X esp=%08X\n",
        stage, tree, MEM32(tree + 8), item, item_text,
        first, second, g_esp);
}

void recomp_tree_loop_trace(uint32_t tree, uint32_t key, uint32_t node,
                            uint32_t parent, uint32_t steps)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    uint32_t header = tree ? MEM32(tree + 4) : 0;
    fprintf(stderr,
        "[TREELOOP] step=%u tree=%08X header=%08X keyptr=%08X key=%08X "
        "node=%08X parent=%08X left=%08X right=%08X nodekey=%08X nil=%u "
        "esp=%08X\n",
        steps, tree, header, key, key ? MEM32(key) : 0, node, parent,
        node ? MEM32(node) : 0, node ? MEM32(node + 8) : 0,
        node ? MEM32(node + 0xC) : 0, node ? MEM8(node + 0x15) : 0,
        g_esp);
}

void recomp_archive_valid_trace(uint32_t path, uint32_t index,
                                uint32_t reader, uint32_t entry, uint32_t vtable,
                                uint32_t target, uint32_t result)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static unsigned emitted;
    char path_text[96];
    unsigned i = 0;
    uint32_t source = 0;

    while (i + 1 < sizeof(path_text)) {
        unsigned char ch = MEM8(path + i);
        path_text[i++] = (char)ch;
        if (ch == 0) {
            break;
        }
    }
    path_text[sizeof(path_text) - 1] = 0;
    if (strncmp(path_text, "99-01.", 6) != 0 || emitted >= 16) {
        return;
    }
    ++emitted;
    if (reader != 0 && MEM32(reader + 0xD4) != 0) {
        source = MEM32(reader + 0xD4) + index * 0x18;
    }
    fprintf(stderr,
        "[ARCHIVEVALID] path=\"%s\" index=%u entry=%08X vtable=%08X "
        "target=%08X result=%08X source=%08X "
        "src=%08X/%08X/%08X/%08X/%02X/%02X/%08X "
        "dst=%08X/%08X/%08X/%08X/%02X/%02X/%08X esp=%08X\n",
        path_text, index, entry, vtable, target, result, source,
        source ? MEM32(source) : 0, source ? MEM32(source + 4) : 0,
        source ? MEM32(source + 8) : 0, source ? MEM32(source + 0xC) : 0,
        source ? MEM8(source + 0x10) : 0, source ? MEM8(source + 0x11) : 0,
        source ? MEM32(source + 0x14) : 0,
        entry ? MEM32(entry) : 0, entry ? MEM32(entry + 4) : 0,
        entry ? MEM32(entry + 8) : 0, entry ? MEM32(entry + 0xC) : 0,
        entry ? MEM8(entry + 0x10) : 0, entry ? MEM8(entry + 0x11) : 0,
        entry ? MEM32(entry + 0x14) : 0, g_esp);
}

void recomp_archive_fetch_trace(const char *stage, uint32_t path,
                                uint32_t index, uint32_t object,
                                uint32_t vtable, uint32_t target,
                                uint32_t result)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static unsigned emitted;
    char path_text[96];
    unsigned i = 0;

    while (i + 1 < sizeof(path_text)) {
        unsigned char ch = MEM8(path + i);
        path_text[i++] = (char)ch;
        if (ch == 0) {
            break;
        }
    }
    path_text[sizeof(path_text) - 1] = 0;
    if (strncmp(path_text, "99-01.", 6) != 0 || emitted >= 24) {
        return;
    }
    ++emitted;
    fprintf(stderr,
        "[ARCHIVEFETCH] %-4s path=\"%s\" index=%u object=%08X "
        "vtable=%08X target=%08X result=%08X esp=%08X\n",
        stage, path_text, index, object, vtable, target, result, g_esp);
}

void recomp_archive_name_trace(const char *stage, uint32_t query,
                               uint32_t candidate, int32_t result,
                               int32_t index, int32_t cached)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    char query_text[96];
    char candidate_text[96];
    unsigned i;

    for (i = 0; i + 1 < sizeof(query_text); ++i) {
        unsigned char ch = query ? MEM8(query + i) : 0;
        query_text[i] = (char)ch;
        if (ch == 0) {
            break;
        }
    }
    query_text[sizeof(query_text) - 1] = 0;
    for (i = 0; i + 1 < sizeof(candidate_text); ++i) {
        unsigned char ch = candidate ? MEM8(candidate + i) : 0;
        candidate_text[i] = (char)ch;
        if (ch == 0) {
            break;
        }
    }
    candidate_text[sizeof(candidate_text) - 1] = 0;
    if (strncmp(query_text, "99-01.", 6) != 0) {
        return;
    }
    fprintf(stderr,
        "[ARCHIVENAME] %-12s query=\"%s\" candidate=\"%s\" "
        "result=%d index=%d cached=%d esp=%08X\n",
        stage, query_text, candidate_text, result, index, cached, g_esp);
}

void recomp_object_fields_trace(const char *stage, uint32_t object)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    uint32_t p908 = object ? MEM32(object + 0x908) : 0;
    uint32_t p90c = object ? MEM32(object + 0x90C) : 0;
    uint32_t p910 = object ? MEM32(object + 0x910) : 0;
    uint32_t p914 = object ? MEM32(object + 0x914) : 0;
    uint32_t p918 = object ? MEM32(object + 0x918) : 0;
    uint32_t p91c = object ? MEM32(object + 0x91C) : 0;
    uint32_t p928 = object ? MEM32(object + 0x928) : 0;
    uint32_t p92c = object ? MEM32(object + 0x92C) : 0;
    uint32_t p930 = object ? MEM32(object + 0x930) : 0;
    uint32_t p934 = object ? MEM32(object + 0x934) : 0;
    uint32_t p938 = object ? MEM32(object + 0x938) : 0;

    fprintf(stderr,
        "[OBJECT] %-14s object=%08X vtable=%08X "
        "908=%08X 90C=%08X 910=%08X 914=%08X 918=%08X 91C=%08X "
        "928=%08X 92C=%08X 930=%08X 934=%08X 938=%08X esp=%08X\n",
        stage, object, object ? MEM32(object) : 0,
        object ? MEM32(object + 0x908) : 0,
        object ? MEM32(object + 0x90C) : 0,
        object ? MEM32(object + 0x910) : 0,
        object ? MEM32(object + 0x914) : 0,
        object ? MEM32(object + 0x918) : 0,
        object ? MEM32(object + 0x91C) : 0,
        object ? MEM32(object + 0x928) : 0,
        object ? MEM32(object + 0x92C) : 0,
        object ? MEM32(object + 0x930) : 0,
        object ? MEM32(object + 0x934) : 0,
        object ? MEM32(object + 0x938) : 0, g_esp);
    fprintf(stderr,
        "[OBJECTVT] %-12s 908=%08X 90C=%08X 910=%08X 914=%08X "
        "918=%08X 91C=%08X 928=%08X 92C=%08X 930=%08X 934=%08X 938=%08X\n",
        stage, p908 ? MEM32(p908) : 0, p90c ? MEM32(p90c) : 0,
        p910 ? MEM32(p910) : 0, p914 ? MEM32(p914) : 0,
        p918 ? MEM32(p918) : 0, p91c ? MEM32(p91c) : 0,
        p928 ? MEM32(p928) : 0, p92c ? MEM32(p92c) : 0,
        p930 ? MEM32(p930) : 0, p934 ? MEM32(p934) : 0,
        p938 ? MEM32(p938) : 0);
    fprintf(stderr,
        "[OBJECTCALL] %-10s 908[0]=%08X 914[10]=%08X 90C[8]=%08X "
        "918[4]=%08X 910[4]=%08X 91C[10]=%08X 92C[4]=%08X 938[8]=%08X\n",
        stage,
        p908 ? MEM32(MEM32(p908)) : 0,
        p914 ? MEM32(MEM32(p914) + 0x10) : 0,
        p90c ? MEM32(MEM32(p90c) + 8) : 0,
        p918 ? MEM32(MEM32(p918) + 4) : 0,
        p910 ? MEM32(MEM32(p910) + 4) : 0,
        p91c ? MEM32(MEM32(p91c) + 0x10) : 0,
        p92c ? MEM32(MEM32(p92c) + 4) : 0,
        p938 ? MEM32(MEM32(p938) + 8) : 0);
}

void recomp_trace_mode_reject(
    uint32_t width, uint32_t height, uint32_t format, uint32_t flags,
    uint32_t requested_mask, uint32_t table, uint32_t table_entry,
    uint32_t entry_bits, uint32_t entry_width, uint32_t entry_height,
    uint32_t entry_extra)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    fprintf(stderr,
        "[D3D] mode reject: width=%u height=%u format=%08X flags=%08X "
        "requested_mask=%08X table=%08X table_entry=%08X "
        "entry=%08X/%04X/%04X/%08X\n",
        width, height, format, flags, requested_mask, table, table_entry,
        entry_bits, entry_width, entry_height, entry_extra);
}

void recomp_trace_av_selector(
    const char *stage, uint32_t capabilities, uint32_t index,
    uint32_t cursor, uint32_t selected)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    fprintf(stderr,
        "[D3D] AV selector %s: capabilities=%08X index=%u cursor=%08X selected=%08X\n",
        stage, capabilities, index, cursor, selected);
}

void recomp_ctor_cursor_trace(uint32_t target, uint32_t before, uint32_t after)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    if (before != after) {
        fprintf(stderr,
            "[RECOMP] CRT constructor 0x%08X changed ESI cursor %08X -> %08X\n",
            target, before, after);
    }
}

/* Short-lived stream diagnostics used while validating the dialogue-font
 * loader.  Keeping the memory dump here avoids teaching generated code about
 * stdio and makes each probe a single, side-effect-free host call. */
void recomp_stream_trace(const char *stage)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static unsigned trace_started;
    static unsigned trace_window;
    static unsigned emitted;
    uint32_t object = g_esi;
    uint32_t vtable;

    /* This loader can loop after corruption.  Capture one complete first-read
     * window only, otherwise diagnostics themselves produce gigabytes. */
    if (strcmp(stage, "17A76-result") == 0 && !trace_started) {
        trace_started = 1;
        trace_window = 1;
        object = g_eax;
    }
    if (!trace_window || emitted >= 200) {
        return;
    }
    ++emitted;
    vtable = MEM32(object);

    fprintf(stderr,
        "[STREAM] %-12s esp=%08X eax=%08X ecx=%08X edx=%08X "
        "esi=%08X edi=%08X ebx=%08X stack=%08X/%08X/%08X/%08X "
        "obj=%08X vt=%08X methods=%08X/%08X/%08X/%08X field4=%08X\n",
        stage, g_esp, g_eax, g_ecx, g_edx, g_esi, g_edi, g_ebx,
        MEM32(g_esp), MEM32(g_esp + 4), MEM32(g_esp + 8),
        MEM32(g_esp + 12), object, vtable, MEM32(vtable),
        MEM32(vtable + 4), MEM32(vtable + 8), MEM32(vtable + 12),
        MEM32(object + 4));
    fflush(stderr);

    if (strcmp(stage, "18EA9-read1") == 0) {
        trace_window = 0;
    }
}

void recomp_parser_trace(const char *stage)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    fprintf(stderr,
        "[PARSER] %-12s esp=%08X eax=%08X ecx=%08X edx=%08X "
        "ebx=%08X esi=%08X edi=%08X stk=%08X/%08X/%08X/%08X\n",
        stage, g_esp, g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi,
        MEM32(g_esp), MEM32(g_esp + 4), MEM32(g_esp + 8),
        MEM32(g_esp + 12));
    fflush(stderr);
}

/* ── ICALL trace ring buffer ───────────────────────────────── */

/*
 * These globals are written by the RECOMP_ICALL macro (defined in
 * recomp_types.h) every time an indirect call is dispatched. When a
 * crash occurs, the VEH handler or recomp_icall_fail_log() can dump
 * the last 16 call targets to help you trace what happened.
 *
 * If your recomp_types.h defines these as extern, they must be
 * defined here (or in xbox_memory_layout.c if you use that pattern).
 */
/* The trace globals and thread-local register state are owned by the
 * xboxrecomp runtime and declared by recomp_types.h. */

/* ── Manual function overrides ─────────────────────────────── */

/*
 * Return a function pointer to override the given Xbox VA, or NULL
 * to fall through to the auto-generated dispatch table.
 *
 * This is called on every indirect call (RECOMP_ICALL) and every
 * direct call through the dispatch table, so keep it fast. A chain
 * of if-statements on uint32_t compiles to a simple comparison
 * sequence; for large override tables, consider a sorted array
 * with binary search.
 *
 * Examples of common override patterns:
 *
 *   // Trace wrapper: log entry/exit around the generated function
 *   extern void sub_00012345(void);
 *   static void traced_sub_00012345(void) {
 *       fprintf(stderr, "[TRACE] sub_00012345 entered, eax=0x%08X\n", g_eax);
 *       sub_00012345();
 *       fprintf(stderr, "[TRACE] sub_00012345 returned, eax=0x%08X\n", g_eax);
 *   }
 *
 *   // Stub: skip a function entirely (return 0 in eax)
 *   static void stub_00067890(void) {
 *       g_eax = 0;
 *   }
 *
 *   // Fix: replace a broken lifted function with correct C
 *   static void fixed_sub_000ABCDE(void) {
 *       // Read arguments from stack/registers per calling convention
 *       uint32_t arg1 = g_ecx;
 *       uint32_t arg2 = MEM32(g_esp + 4);
 *       // ... correct implementation ...
 *       g_eax = result;
 *   }
 */
/* CRT strrchr (0x002D6750-0x002D677D). The original uses STD / REPNE
 * SCASB / CLD to search backward. The generic lifter currently ignores DF
 * and scans past the terminator instead, breaking resource type dispatch.
 * Keep this byte-derived routine indivisible until DF is modeled generally.
 * cdecl: pop only RET; EBX/ESI/EDI/EBP and the exception chain stay intact. */
void sub_002D6750(void)
{
    uint32_t start = MEM32(g_esp + 4);
    uint8_t needle = (uint8_t)MEM32(g_esp + 8);
    uint32_t end = start;
    uint32_t found = 0;
    while (MEM8(end) != 0) ++end;
    /* Include the terminator, as required for strrchr(s, '\0'). */
    g_ecx = end - start + 1;
    do {
        --g_ecx;
        if (MEM8(end) == needle) {
            found = end;
            break;
        }
        --end;
    } while (g_ecx != 0);
    g_eax = found;
    g_esp += 4;
}

/* std::_Tree iterator increment (0x0004FFE0-0x00050037).
 *
 * The function scanner mistakes a nearby jump table for a vtable and splits
 * this 55-byte helper at 0x50000, 0x50002, 0x50004, and several subsequent
 * instruction boundaries. Those artificial C function boundaries discard
 * the flags produced by TEST and can bypass the matching POP EBX/RET. Since
 * this helper is shared by dozens of resource containers, implement the
 * original red-black-tree successor operation as one indivisible routine. */
void sub_0004FFE0(void)
{
    uint32_t iterator = g_ecx;
    uint32_t current = MEM32(iterator);
    uint32_t next;

    /* The original returns immediately when passed the tree's head node. */
    if (MEM8(current + 0x11) != 0) {
        g_esp += 4;
        return;
    }

    next = MEM32(current + 8); /* right child */
    if (MEM8(next + 0x11) == 0) {
        /* The successor is the leftmost node in the right subtree. */
        do {
            current = next;
            next = MEM32(next); /* left child */
        } while (MEM8(next + 0x11) == 0);
        MEM32(iterator) = current;
        g_eax = next;
        g_edx = current;
    } else {
        /* Walk upward while leaving a right subtree. */
        next = MEM32(current + 4); /* parent */
        while (MEM8(next + 0x11) == 0 && current == MEM32(next + 8)) {
            current = next;
            next = MEM32(next + 4);
        }
        MEM32(iterator) = next;
        g_eax = next;
        g_edx = current;
    }

    g_esp += 4;
}

extern void sub_00235080(void);
extern void sub_00423AB5(void);
extern void sub_00423E5C(void);
extern void sub_004238B0(void);
extern void sub_004238D2(void);
extern void sub_004275A7(void);
extern void sub_0042760F(void);
extern void sub_004286C8(void);
extern void sub_004288C2(void);
extern void sub_0042D86E(void);
extern void sub_0042D9BF(void);
extern void sub_0042D9EC(void);
extern void sub_0042E7F6(void);

static void recomp_buffer_trace(const char *stage, uint32_t stream, uint32_t work_ebp)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static unsigned emitted;
    static unsigned active;
    static int enabled = -1;

    if (enabled < 0) {
        enabled = getenv("UCRR_BUFFER_TRACE") != NULL;
    }
    if (!enabled) {
        return;
    }

    if (strcmp(stage, "entry") == 0) {
        active = (MEM32(g_esp + 0x1C) == 0x4000);
    }
    if (!active || emitted >= 48) {
        return;
    }
    ++emitted;
    fprintf(stderr,
        "[BUFFER] %-10s esp=%08X eax=%08X ecx=%08X edx=%08X "
        "ebx=%08X esi=%08X edi=%08X wepb=%08X "
        "stk=%08X/%08X/%08X/%08X/%08X/%08X "
        "f=%08X/%08X/%08X/%08X/%02X\n",
        stage, g_esp, g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi,
        work_ebp, MEM32(g_esp), MEM32(g_esp + 4), MEM32(g_esp + 8),
        MEM32(g_esp + 12), MEM32(g_esp + 16), MEM32(g_esp + 20),
        MEM32(stream + 0x40), MEM32(stream + 0x4C),
        MEM32(stream + 0x50), MEM32(stream + 0x54), MEM8(stream + 0x58));
    fflush(stderr);
    if (strcmp(stage, "success") == 0 || strcmp(stage, "eof") == 0 ||
        strcmp(stage, "failure") == 0) {
        active = 0;
    }
}

/* Buffered stream read (0x00233CD0-0x00233D8F).
 *
 * The scanner incorrectly treats 0x00233D00 as a second function even though
 * it is the fallthrough half of this routine.  Its backward branch to
 * 0x00233CD8 is consequently emitted as a call to an unresolved stub, which
 * discards one stack word and abandons the refill loop.  Keep the original
 * control flow together here. */
void sub_00233CD0(void)
{
    uint32_t frame_ebp = g_seh_ebp;
    uint32_t stream;
    uint32_t total;

    PUSH32(g_esp, g_ebx);
    PUSH32(g_esp, frame_ebp);
    PUSH32(g_esp, g_esi);
    PUSH32(g_esp, g_edi);
    g_esi = g_ecx;
    g_ebx = 0;
    stream = g_esi;
    recomp_buffer_trace("entry", stream, frame_ebp);

refill:
    recomp_buffer_trace("refill", stream, frame_ebp);
    g_eax = MEM32(stream + 0x4C);
    if (g_eax == MEM32(stream + 0x50)) {
        if (MEM8(stream + 0x58) != 0) {
            goto eof;
        }

        g_ecx = MEM32(g_esp + 0x14);
        g_eax = 0;
        MEM32(stream + 0x4C) = 0;
        MEM32(stream + 0x50) = 0;
        g_eax = MEM32(stream + 0x54);
        g_edx = MEM32(g_ecx);

        {
            uint32_t saved_esp = g_esp;
            uint32_t target = MEM32(g_edx + 4);
            PUSH32(g_esp, g_eax);
            g_eax = MEM32(stream + 0x40);
            PUSH32(g_esp, g_eax);
            PUSH32(g_esp, 0x00233D04u);
            recomp_buffer_trace("pre-read", stream, frame_ebp);
            RECOMP_ICALL_SAFE(target, saved_esp);
            recomp_buffer_trace("post-read", stream, frame_ebp);
        }

        MEM8(stream + 0x58) = (g_eax != MEM32(stream + 0x54)) ? 1 : 0;
        MEM32(stream + 0x50) = g_eax;
    }

    g_edx = MEM32(g_esp + 0x18);
    g_edi = MEM32(g_esp + 0x1C);
    frame_ebp = MEM32(stream + 0x50);
    g_eax = g_ebx + g_edx;
    MEM32(stream + 0x0C) = g_eax;
    g_eax = MEM32(stream + 0x4C);
    g_edi -= g_ebx;
    MEM32(stream + 0x10) = g_edi;
    if ((int32_t)g_eax < (int32_t)frame_ebp) {
        frame_ebp -= g_eax;
    } else {
        frame_ebp = 0;
    }

    g_ecx = MEM32(stream + 0x40) + g_eax;
    MEM32(stream + 4) = frame_ebp;
    MEM32(stream) = g_ecx;
    PUSH32(g_esp, 0);
    PUSH32(g_esp, stream);
    PUSH32(g_esp, 0x00233D45u);
    recomp_buffer_trace("pre-parse", stream, frame_ebp);
    sub_00235080();
    recomp_buffer_trace("post-parse", stream, frame_ebp);
    g_esp += 8;

    if (g_eax != 0 && g_eax != 1) {
        goto failure;
    }

    g_edx = MEM32(stream + 4);
    g_ecx = MEM32(stream + 0x4C);
    frame_ebp -= g_edx;
    g_ecx += frame_ebp;
    g_edi -= MEM32(stream + 0x10);
    MEM32(stream + 0x4C) = g_ecx;
    total = MEM32(g_esp + 0x1C);
    g_ebx += g_edi;
    if (g_ebx != total && g_eax == 0) {
        goto refill;
    }

    recomp_buffer_trace("success", stream, frame_ebp);
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, frame_ebp);
    g_eax = g_ebx;
    POP32(g_esp, g_ebx);
    g_esp += 16;
    return;

eof:
    recomp_buffer_trace("eof", stream, frame_ebp);
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, frame_ebp);
    g_eax = 0;
    POP32(g_esp, g_ebx);
    g_esp += 16;
    return;

failure:
    recomp_buffer_trace("failure", stream, frame_ebp);
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, frame_ebp);
    g_eax = 0xFFFFFFFFu;
    POP32(g_esp, g_ebx);
    g_esp += 16;
}

/* RunPushBuffer patches a reusable buffer's return jump. Consume its previous
 * use before that patch, and its newly published use before returning to the
 * guest. Otherwise a later patch can skip intervening draws/reports. Recording
 * (device flag 4) is not execution and must not drain the recorded stream. */
extern void sub_003EF870_gen(void);
extern void sub_003E9380_gen(void);
extern void sub_003E9430_gen(void);
static void ucrr_drain_after_gpu_kick(unsigned reason)
{
    static int enabled=-1;if(enabled<0)enabled=getenv("UCRR_ORDERED_GPU")!=NULL;
    if(!enabled)return;
    uint32_t device=MEM32(0x003F4238u);
    if(!device||device>g_xbox_total_ram-0x1C24u||
       (MEM32(device+8)&4u)||MEM32(device+0x1C20)!=0xFD800000u)return;
    nv2a_pb_live_drain_reason((uint8_t *)(uintptr_t)g_xbox_mem_offset,
                       g_xbox_total_ram,0x003F4238u,reason);
}
/* Preserve the original flush/publication code, then consume its published
 * stream. Startup waits on DMA GET before it ever reads pattern progress. */
void sub_003E9380(void)
{
    /* The original optional idle-drain mode is a hard progress boundary, not
     * an ordinary asynchronous publication. Preserve it on either side. */
    int idle=MEM32(0x003F3D90u)!=0;
    sub_003E9380_gen();
    ucrr_drain_after_gpu_kick(idle||MEM32(0x003F3D90u)?NV_WAIT_KICK_IDLE:NV_WAIT_KICK);
}
void sub_003E9430(void)
{
    sub_003E9430_gen();
    ucrr_drain_after_gpu_kick(NV_WAIT_KICK_IDLE);
}
/* Guest MMIO progress polling advances submitted work but never presents.
 * Host decoder/D3D code does not execute guest functions or clobber registers. */
void xbox_NotifyGpuProgressRead(uint32_t address)
{
    static int enabled=-1;if(enabled<0)enabled=getenv("UCRR_ORDERED_GPU")!=NULL;
    if(!enabled||address!=0xFD400B10u)return;
    nv2a_pb_live_drain_reason((uint8_t *)(uintptr_t)g_xbox_mem_offset,
                       g_xbox_total_ram,0x003F4238u,NV_WAIT_MMIO);
}
/* The original fence wait polls guest RAM, not an MMIO register. Only finish
 * already-issued host queries here: do not decode unpublished work or present. */
void xbox_NotifyMemoryReadPoll(uint32_t address)
{
    ucrr_fence_poll();
    static int enabled=-1;if(enabled<0)enabled=getenv("UCRR_ORDERED_GPU")!=NULL;
    if(!enabled||g_xbox_total_ram<0x003F423Cu)return;
    uint32_t device=MEM32(0x003F4238u);
    if(!device||device>g_xbox_total_ram-0x34u)return;
    uint32_t semaphore=MEM32(device+0x30u);
    if((semaphore&3u)||semaphore>g_xbox_total_ram-4u||address!=semaphore)return;
    unsigned previous=pgraph_d3d11_query_scope(NV_WAIT_RAM);
    pgraph_d3d11_poll_reports();pgraph_d3d11_query_scope(previous);
}
/* Diagnostic wrapper: original guest code and register/stack effects retained.
 * Wait residency begins at the first original RAM-loop poll, ends on return.
 * The tiny original epilogue is included; pre-wait command generation is not. */
extern void sub_003E9560_gen(void);
void sub_003E9560(void)
{
    if(!ucrr_fence_enabled()){sub_003E9560_gen();return;}
    UcrrFenceScope scope={0},*previous=ucrr_fence_active;
    if(previous)++ucrr_fence_nested;
    ucrr_fence_active=&scope;sub_003E9560_gen();
    ucrr_fence_finish(&scope);ucrr_fence_active=previous;
}
void sub_003EF870(void)
{
    uint32_t scenery_probe_resource = uc_scenery_probe_begin();
    uint32_t device = MEM32(0x003F4238u);
    int execute = device && device < g_xbox_total_ram - 12u &&
                  !(MEM32(device + 8u) & 4u);
    uint64_t pair = execute ? pgraph_d3d11_light_pair_begin(device,g_esp) : 0;
    if (execute) {
        if(pair)pgraph_d3d11_light_pair_half(pair,NV_LIGHT_PAIR_BEFORE);
        nv2a_pb_live_drain_reason((uint8_t *)(uintptr_t)g_xbox_mem_offset,
                           g_xbox_total_ram, 0x003F4238u,NV_WAIT_REUSE_BEFORE);
        if(pair)pgraph_d3d11_light_pair_half(pair,NV_LIGHT_PAIR_GUEST);
    }
    sub_003EF870_gen();
    uc_scenery_probe_end(scenery_probe_resource);
    if (execute) {
        if(pair)pgraph_d3d11_light_pair_half(pair,NV_LIGHT_PAIR_AFTER);
        nv2a_pb_live_drain_reason((uint8_t *)(uintptr_t)g_xbox_mem_offset,
                           g_xbox_total_ram, 0x003F4238u,NV_WAIT_REUSE_AFTER);
        if(pair)pgraph_d3d11_light_pair_end(pair);
    }
}

/* Original visibility polling can precede the next vertical blank. Consume
 * submitted GPU commands here without presenting or manufacturing a result. */
extern void sub_003E2C30_gen(void);
extern void ucrr_start_mission_watchdog(void);
void sub_003E2C30(void)
{
    ucrr_start_mission_watchdog();
#if !UCRR_LEAN_RUNTIME
    static unsigned samples;
    static uint32_t last_index = UINT32_MAX;
    static ULONGLONG next_sample;
    static unsigned periodic_samples;
#endif
    uint32_t index = MEM32(g_esp + 4);
    static int demand_enabled=-1;
    if(demand_enabled<0){const char*v=getenv("UCRR_CPU_BUNDLE_VISIBILITY");demand_enabled=v&&!strcmp(v,"1");}
    NvVisibilityDemand demand={0x003F4238u,index};
    const NvVisibilityDemand*previous_demand=NULL;
    if(demand_enabled)previous_demand=pgraph_d3d11_visibility_scope(&demand);
    nv2a_pb_live_drain_reason((uint8_t *)(uintptr_t)g_xbox_mem_offset,
                       g_xbox_total_ram, 0x003F4238u,NV_WAIT_VISIBILITY);
    if(demand_enabled)pgraph_d3d11_visibility_scope(previous_demand);
#if !UCRR_LEAN_RUNTIME
    ULONGLONG now = GetTickCount64();
    int periodic = now >= next_sample && periodic_samples < 12;
    if (index < 65536 && (periodic ||
        (samples < 128 && (samples < 8 || index != last_index)))) {
        if (periodic) ++periodic_samples;
        last_index = index;
        next_sample = now + 10000;
        uint32_t device = MEM32(0x003F4238u);
        if (device < g_xbox_total_ram - 0xBD4u) {
            uint32_t page = MEM32(device + 0x7D4u + (index >> 8) * 4u);
            uint64_t slot = (uint64_t)page + (index & 255u) * 16u;
            if (slot + 16u <= g_xbox_total_ram) {
                fprintf(stderr,"[VISIBILITY-WAIT] index=%u slot=%08X words=%08X,%08X,%08X,%08X\n",
                    index,(uint32_t)slot,MEM32(slot),MEM32(slot+4),MEM32(slot+8),MEM32(slot+12));
                  ++samples;
            }
        }
    }
#endif
    sub_003E2C30_gen();
}

/* The setter also serves unrelated callback objects. Default remains retail;
 * optional Target60 changes only the verified gameplay display request. */
extern void sub_0008BD50(void);
static void trace_display_interval_setter(void)
{
    static int enabled=-1,target60=-1;static unsigned samples,enhancements;
    if(enabled<0)enabled=getenv("UCRR_TIMING_TRACE")!=NULL;
    if(target60<0)target60=getenv("UCRR_TARGET_60")!=NULL;
    uint32_t arg_slot=0,requested=0,effective=0;
    if(target60&&resource_readable(0x519F70u,4)&&
       resource_readable(g_ecx,32)&&resource_readable(g_esp,8)){
        requested=MEM32(g_esp+4);
        effective=ucrr_display_interval(target60,MEM32(g_esp),g_ecx,
            MEM32(0x519F70u),MEM32(g_ecx),requested);
        if(effective!=requested){
            arg_slot=g_esp+4;
            if(enhancements++<4)fprintf(stderr,"[TARGET-60-ENHANCEMENT] caller=%08X object=%08X requested=%u effective=%u; original timers and vblank counters retained\n",MEM32(g_esp),g_ecx,requested,effective);
        }
    }
    if(enabled&&samples<16&&resource_readable(0x519F70u,4)&&
       g_ecx==MEM32(0x519F70u)&&resource_readable(g_ecx,32)&&
       resource_readable(g_esp,8)){
        ++samples;
        fprintf(stderr,"[DISPLAY-INTERVAL-SET] sample=%u caller=%08X object=%08X vtable=%08X previous=%u requested=%u\n",
            samples,MEM32(g_esp),g_ecx,MEM32(g_ecx),MEM32(g_ecx+24),MEM32(g_esp+4));
    }
    if(arg_slot)MEM32(arg_slot)=effective;
    sub_0008BD50();
    /* Restore consumed argument storage; the original setter owns object+18,
     * EAX and RET4 effects. Never manufacture extra vblank callbacks. */
    if(arg_slot)MEM32(arg_slot)=requested;
}

/* D3D BlockUntilVerticalBlank (0x003E2CC0-0x003E2CE4).
 *
 * The Xbox GPU interrupt signals the device's vertical-blank event and invokes
 * the callback registered at device + 0x1DB8.  The static XDK D3D code still
 * owns that callback state, while the host renderer does not drive the Xbox
 * interrupt handler.  Consequently the event stays signalled but the XDK's
 * vertical-blank counter at device + 0x1DE8 and the title callback never
 * advance.  Complete one vertical blank synchronously whenever the title
 * blocks for it. */
/* The original synchronous campaign loader draws two complete batches without
 * BlockUntilVerticalBlank. Present them at its completed-frame boundary, before
 * it releases the loading archive. Do not fabricate vblank ticks or callbacks. */
void ucrr_loading_present(void)
{
    nv2a_pb_live_frame((uint8_t *)(uintptr_t)g_xbox_mem_offset,
                       g_xbox_total_ram, 0x003F4238u);
}

void sub_003E2CC0(void)
{
    uint32_t device = MEM32(0x003F4238u);
    uint32_t callback = device ? MEM32(device + 0x1DB8u) : 0;

    /* Present every completed original-XDK command batch on the host device.
     * This runs on the game thread, so D3D11 context use and the Win32 message
     * pump remain single-threaded. */
    nv2a_pb_live_frame((uint8_t *)(uintptr_t)g_xbox_mem_offset,
                       g_xbox_total_ram, 0x003F4238u);
#if defined(UCRR_LIGHT_REMOVAL)
    /* Settings are published only after a completed frame, on the owner
     * thread. All selected lights in the next frame use the same choice. */
    uc_light_removal_apply_pending();
#endif
    ucrr_fence_report();

    if (device) {
        MEM32(device + 0x1DC0u) = 0;
        MEM32(device + 0x1DE8u) = MEM32(device + 0x1DE8u) + 1u;
    }

    if (callback) {
        PUSH32(g_esp, 0x003E2CE3u);
        RECOMP_ICALL(callback);
    }

    g_eax = 0; /* STATUS_SUCCESS from KeWaitForSingleObject. */
    g_esp += 4;
}

/* DirectSound APU command submission (0x00428059-0x004281CE).
 *
 * The last write rings an APU command mailbox and the Xbox CPU waits for the
 * sound processor to clear it.  Until the native APU is wired to the title's
 * internal command block, consume the command synchronously after preserving
 * all of the setup and buffer-copy work performed by the original routine. */
#define eax g_eax
#define ecx g_ecx
#define edx g_edx
#define ebx g_ebx
#define esp g_esp
#define esi g_esi
#define edi g_edi
void sub_00428059(void)
{
    uint32_t ebp = g_ebp;
    int _flags = 0;
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_flags; (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp;
    esp -= 0x1C;
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = ecx;
    eax = MEM32(edi + 8);
    eax = MEM32(eax + 0x10);
    ebx = MEM32(eax);
    eax = MEM32(ebx + 0x80C) + MEM32(ebx + 0x804);
    ecx = MEM32(edi + 0x20);
    esi = ebx + eax * 4 + 0x818;
    eax = (MEM32(esi) - 1) << 5;
    eax += 0x28;
    _fa = ecx; _fb = ecx;
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;
    MEM32(ebp - 20) = edi;
    MEM32(ebp - 16) = eax;
    if (TEST_Z(_fa, _fb)) goto loc_004280A2;

    PUSH32(esp, ecx);
    g_ebp = ebp;
    PUSH32(esp, 0x0042809Bu);
    sub_0042760F();
    MEM32(edi + 0x20) = 0;
    eax = MEM32(ebp - 16);

loc_004280A2:
    PUSH32(esp, 1);
    PUSH32(esp, eax);
    PUSH32(esp, 0x626F5344);
    g_ebp = ebp;
    PUSH32(esp, 0x004280AFu);
    sub_004275A7();
    _fa = eax; _fb = eax;
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;
    MEM32(edi + 0x20) = eax;
    if (TEST_NZ(_fa, _fb)) goto loc_004280C0;

    eax = 0x8007000Eu;
    goto loc_004281C7;

loc_004280C0:
    PUSH32(esp, MEM32(esi + 4));
    ecx = MEM32(edi + 8);
    g_ebp = ebp;
    PUSH32(esp, 0x004280CBu);
    sub_0042D86E();
    _fa = eax; _fb = eax;
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;
    if (TEST_S(_fas, _fbs)) goto loc_004281C7;

    eax = ebx + 0x818;
    MEM32(ebp - 4) = eax;
    eax = MEM32(ebp - 16) + esi;
    MEM32(ebp - 8) = eax;
    eax = ebp - 28;
    PUSH32(esp, eax);
    g_ebp = ebp;
    PUSH32(esp, 0x004280EDu);
    sub_0042D9BF();

    eax = MEM32(esi);
    PUSH32(esp, 0);
    PUSH32(esp, MEM32(ebp - 8));
    eax <<= 3;
    PUSH32(esp, eax);
    PUSH32(esp, MEM32(ebp - 8));
    eax = ebp - 28;
    PUSH32(esp, eax);
    g_ebp = ebp;
    PUSH32(esp, 0x00428104u);
    sub_0042D9EC();
    _fa = eax; _fb = eax;
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;
    if (CMP_LE(_fas & _fbs, 0)) goto loc_00428114;
    eax = (eax & 0xFFFF) | 0x80070000u;
    _fa = eax; _fb = eax;
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;

loc_00428114:
    if (TEST_S(_fas, _fbs)) goto loc_004281C7;
    MEM32(ebp - 12) = 0;
    _fa = MEM32(esi); _fb = 0;
    _fas = (int32_t)_fa; _fbs = 0;
    if (CMP_BE(_fa, _fb)) goto loc_004281A2;

    eax = MEM32(ebp - 8);
    MEM32(ebp - 8) = eax;
    edi = esi + 8;

loc_0042812C:
    PUSH32(esp, 0);
    PUSH32(esp, MEM32(ebp - 4));
    PUSH32(esp, MEM32(edi + 4));
    PUSH32(esp, MEM32(ebp - 4));
    PUSH32(esp, MEM32(ebp - 8));
    g_ebp = ebp;
    PUSH32(esp, 0x0042813Fu);
    sub_0042D9EC();
    _fa = eax; _fb = eax;
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;
    if (CMP_LE(_fas & _fbs, 0)) goto loc_0042814F;
    eax = (eax & 0xFFFF) | 0x80070000u;
    _fa = eax; _fb = eax;
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;

loc_0042814F:
    if (TEST_S(_fas, _fbs)) goto loc_004281C7;
    eax = MEM32(edi + 4);
    MEM32(ebp - 4) += eax;
    eax = MEM32(ebp - 20);
    ecx = MEM32(MEM32(eax + 0xC) + 4);
    MEM32(edi + 0x10) += 0xFE836000u;
    MEM32(ebp - 8) += 8;
    ecx -= 0x17C6818;
    MEM32(edi) += ecx;
    ecx = 0xFFA0BE7Au - MEM32(ebx + 0x804);
    edi += 0x20;
    ecx <<= 2;
    MEM32(edi - 24) += ecx;
    eax = MEM32(MEM32(eax + 8) + 0x14) - 0xC000;
    MEM32(edi - 8) += eax;
    MEM32(ebp - 12) += 1;
    eax = MEM32(ebp - 12);
    _fa = eax; _fb = MEM32(esi);
    _fas = (int32_t)_fa; _fbs = (int32_t)_fb;
    if (CMP_B(_fa, _fb)) goto loc_0042812C;
    edi = MEM32(ebp - 20);

loc_004281A2:
    ecx = MEM32(ebp - 16);
    edi = MEM32(edi + 0x20);
    eax = ecx;
    ecx >>= 2;
    memcpy((void *)XBOX_PTR(edi), (void *)XBOX_PTR(esi), ecx * 4);
    esi += ecx * 4; edi += ecx * 4; ecx = 0;
    ecx = eax;
    PUSH32(esp, 3);
    POP32(esp, eax);
    ecx &= eax;
    ebx += 0x810;
    memcpy((void *)XBOX_PTR(edi), (void *)XBOX_PTR(esi), ecx);
    esi += ecx; edi += ecx; ecx = 0;
    MEM32(ebx) = eax;
    MEM32(ebx) = 0; /* APU consumed the command. */

loc_004281C0:
    _fa = MEM32(ebx); _fb = 0;
    _fas = (int32_t)_fa; _fbs = 0;
    if (CMP_NE(_fa, _fb)) goto loc_004281C0;
    eax = 0;

loc_004281C7:
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    g_ebp = ebp;
    esp += 8;
}
#undef eax
#undef ecx
#undef edx
#undef ebx
#undef esp
#undef esi
#undef edi

/* DirectSound AC97 bus-master reset (0x0042E9EC-0x0042EA5E).
 *
 * Bit 1 of an AC97 bus-master control register is a self-clearing reset bit.
 * The Xbox writes it and polls until the controller clears it.  Our device
 * aperture is currently backed by ordinary memory, so it needs the hardware
 * side effect here.  The generated translation also hoists the register read
 * out of the polling loop, which would make the loop permanent even with an
 * asynchronous device model.  Keep the rest of the original routine intact
 * while completing the reset synchronously. */
void sub_0042E9EC(void)
{
    uint32_t frame_ebp;
    uint32_t controller_offset;
    uint32_t reset_register;

    PUSH32(g_esp, g_ebp);
    frame_ebp = g_esp;
    g_ebp = frame_ebp;
    PUSH32(g_esp, g_ecx);
    PUSH32(g_esp, g_ecx);
    MEM32(frame_ebp - 4) = 0;
    PUSH32(g_esp, g_esi);
    g_esi = g_ecx;

    g_ecx = frame_ebp - 8;
    g_ebp = frame_ebp;
    PUSH32(g_esp, 0x0042EA00u);
    sub_004238B0();

    controller_offset = MEM32(MEM32(g_esi) * 4u + 0x00441C38u);
    reset_register = controller_offset + 0xFEC0010Bu;
    MEM8(reset_register) = 2;
    MEM8(reset_register) = 0; /* AC97 hardware clears the reset bit. */

    g_eax = controller_offset;
    g_ecx = MEM32(g_esi + 0x1C);
    MEM32(controller_offset + 0xFEC00100u) = g_ecx;
    if (MEM32(g_esi) == 1) {
        g_eax = MEM32(g_esi + 0x28);
        MEM32(0xFEC0017Cu) = g_eax;
    }

    g_eax = MEM8(g_esi + 0x24);
    PUSH32(g_esp, 1);
    PUSH32(g_esp, 1);
    g_ecx = g_esi;
    PUSH32(g_esp, g_eax);
    g_eax = MEM8(g_esi + 0x25);
    PUSH32(g_esp, g_eax);
    g_ebp = frame_ebp;
    PUSH32(g_esp, 0x0042EA53u);
    sub_0042E7F6();

    g_ecx = frame_ebp - 8;
    g_ebp = frame_ebp;
    PUSH32(g_esp, 0x0042EA5Bu);
    sub_004238D2();

    POP32(g_esp, g_esi);
    g_esp = frame_ebp;
    POP32(g_esp, g_ebp);
    g_esp += 4;
}

/* Service missed voice-idle interrupts at the existing guest-thread service
 * point.  004288C2 is the original interrupt consumer: it checks the hardware
 * PERSIST format bit, final channel, device suspension and list membership before
 * calling 0042AD16. Never clear pending flags or manufacture notifier success.
 * Restrict polling to pending stops or real OFF events for normal0103 voices;
 * the original consumer expects an idle interrupt, which itself supplies the
 * inactive-state guarantee. Our polling substitute must check actual ACTIVE
 * at voice+0x54 bit21, not confuse PERSIST at voice+4 bit23 with playback. */
static void service_pending_voice_retirements(uint32_t device)
{
    static int enabled = -1;
    if (enabled < 0) enabled = getenv("UCRR_VOICE_RETIRE_SERVICE") != NULL;
    if (!enabled || !device || g_xbox_total_ram < 0x8000u ||
        device > g_xbox_total_ram - 0x4DCu || MEM32(device + 0x84u)) return;
    uint32_t voice_ram = MEM32(0x00442430u);
    if (!voice_ram || voice_ram > g_xbox_total_ram - 0x8000u) return;
    for (uint32_t handle = 0; handle < 256; ++handle) {
        uint32_t voice = MEM32(device + 0x88u + handle * 4u);
        if (!voice || voice > g_xbox_total_ram - 0x80u ||
            MEM32(voice + 8u) != device ||
            ((MEM16(voice + 0x12u) & 0x8001u) != 0x8001u &&
             MEM16(voice + 0x12u) != 0x0103u) ||
            MEM8(voice + 0x64u) == 0 || MEM8(voice + 0x64u) > 3 ||
            MEM8(voice + 0x65u) >= 3 ||
            MEM16(voice + 0x0Au + MEM8(voice + 0x64u) * 2u) != handle ||
            (MEM32(voice_ram + handle * 0x80u + 4u) & 0x00800000u) ||
            (MEM32(voice_ram + handle * 0x80u + 0x54u) & 0x00200000u) ||
            MEM32(voice + 0x4Cu) == voice + 0x4Cu) continue;
        /* Normal completion needs an actual OFF event, not merely inactive
         * RAM before a queued start. ON invalidates any prior OFF event. The
         * original pending-stop path remains available without a fresh event. */
        extern int ucrr_take_voice_idle(unsigned);
        int completed=ucrr_take_voice_idle(handle);
        if(MEM16(voice+0x12u)==0x0103u && !completed)continue;
        g_ecx = device;
        PUSH32(g_esp, handle);
        PUSH32(g_esp, 0x0042508Cu);
        sub_004288C2();
    }
}

/* Observe every GetNextFrame (003F7348) return, not just packet-wait polls.
 * Observe decoded timestamps independently of texture pixel hashes: static
 * pictures can legitimately hash identically while frames keep advancing. */
static void trace_xmv_delivery(uint32_t movie, uint32_t status, uint64_t elapsed)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static __declspec(thread) struct {
        uint32_t movie, timestamp, polls, changes, reports, delivered, waiting;
        uint64_t report_at, change_at, last_poll, max_poll, max_change, max_call;
    } t;
    static int enabled = -1;
    if (enabled < 0) enabled = !UCRR_PLAY_ONLY && getenv("UCRR_AUDIO_WAIT_TRACE") != NULL;
    if (!enabled || t.reports >= 128 || g_xbox_total_ram < 0x3F423Cu ||
        !movie || movie > g_xbox_total_ram - 0x160u) return;
    uint64_t now = GetTickCount64();
    uint32_t stamp = MEM32(movie + 0xE4u);
    if (t.movie != movie) {
        t.movie = movie; t.timestamp = stamp;
        t.report_at = t.change_at = t.last_poll = now;
        t.polls = t.changes = t.delivered = t.waiting = 0;
        t.max_poll = t.max_change = t.max_call = 0;
    }
    if (now - t.last_poll > t.max_poll) t.max_poll = now - t.last_poll;
    t.last_poll = now; ++t.polls;
    t.delivered += status == 1; t.waiting += status == 0;
    if (elapsed > t.max_call) t.max_call = elapsed;
    if (stamp != t.timestamp) {
        if (now - t.change_at > t.max_change) t.max_change = now - t.change_at;
        t.change_at = now; t.timestamp = stamp; ++t.changes;
    }
    if (now - t.report_at < 1000) return;
    uint32_t device = MEM32(0x003F4238u);
    uint32_t vblank = device && device <= g_xbox_total_ram - 0x1DECu
        ? MEM32(device + 0x1DE8u) : 0;
    fprintf(stderr, "[XMV-DELIVERY] tick_ms=%llu movie=%08X polls=%u advances=%u max_poll_ms=%llu max_advance_ms=%llu unchanged_ms=%llu stamp=%u pending=%u frames_left=%u ready=%u vblank=%u start_vblank=%u rate=%u delivered=%u waiting=%u max_call_ms=%llu report=%u/128\n",
        (unsigned long long)now, movie, t.polls, t.changes,
        (unsigned long long)t.max_poll, (unsigned long long)t.max_change,
        (unsigned long long)(now-t.change_at), stamp, MEM32(movie+0xF8u),
        MEM32(movie+0x3Cu), MEM32(movie+0xE8u), vblank,
        MEM32(movie+0xB8u), MEM32(movie+0xBCu), t.delivered, t.waiting,
        (unsigned long long)t.max_call, ++t.reports);
    t.report_at = now; t.polls = t.changes = t.delivered = t.waiting = 0;
    t.max_poll = t.max_change = t.max_call = 0;
}

extern void sub_003F7348_gen(void);
static uint64_t xmv_stage_begin(void)
{
#if UCRR_PLAY_ONLY
    return 0;
#endif

    static int enabled = -1;
    if (enabled < 0) enabled = !UCRR_PLAY_ONLY && getenv("UCRR_AUDIO_WAIT_TRACE") != NULL;
    LARGE_INTEGER now;
    if (!enabled || !QueryPerformanceCounter(&now)) return 0;
    return (uint64_t)now.QuadPart;
}
static void xmv_stage_end(const char *stage, uint64_t start)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    static __declspec(thread) unsigned reports[10], calls[10], slow_reports[10];
    LARGE_INTEGER now, frequency;
    unsigned kind = !strcmp(stage,"decode") ? 3 : !strcmp(stage,"open") ? 1 : !strcmp(stage,"initialize") ? 2 :
        !strcmp(stage,"title-cleanup") ? 4 : !strcmp(stage,"resource-release") ? 5 : !strcmp(stage,"title-open") ? 6 :
        !strcmp(stage,"next-frame") ? 7 : !strcmp(stage,"title-update") ? 8 : !strcmp(stage,"title-render") ? 9 : 0;
    if (!start) return;
    ++calls[kind];
    if (reports[kind] >= 128 && slow_reports[kind] >= 64) return;
    QueryPerformanceCounter(&now); QueryPerformanceFrequency(&frequency);
    double ms = 1000.0 * ((uint64_t)now.QuadPart-start) / frequency.QuadPart;
    /* Reserve rare-stall coverage after ordinary first-event logs fill up. */
    if (ms >= 20.0 && slow_reports[kind] < 64)
        fprintf(stderr,"[XMV-SLOW] stage=%s call=%u elapsed_ms=%.3f tick_ms=%llu report=%u/64\n",
            stage,calls[kind],ms,(unsigned long long)GetTickCount64(),++slow_reports[kind]);
    if (reports[kind] >= 128) return;
    if (kind >= 7 && ms < 20.0) return;
    if (kind == 3 && ms < 5.0 && calls[kind] > 10) return;
    fprintf(stderr,"[XMV-STAGE] stage=%s call=%u elapsed_ms=%.3f tick_ms=%llu report=%u/128\n",
        stage,calls[kind],ms,(unsigned long long)GetTickCount64(),++reports[kind]);
}
extern void sub_003F7031_gen(void);
extern void sub_00087420_gen(void);
void sub_00087420(void)
{
    uint64_t start=xmv_stage_begin();
    static __declspec(thread) unsigned origins;
    if (start && origins < 64 && g_esp <= g_xbox_total_ram - 16) {
        fprintf(stderr,"[XMV-CLEANUP-ORIGIN] tick_ms=%llu stack=%08X,%08X,%08X,%08X\n",
            (unsigned long long)GetTickCount64(),MEM32(g_esp),MEM32(g_esp+4),MEM32(g_esp+8),MEM32(g_esp+12));
        ++origins;
    }
    sub_00087420_gen();
    xmv_stage_end("title-cleanup",start);
}
extern void sub_003E55B0_gen(void);
void sub_003E55B0(void)
{
    uint64_t start=xmv_stage_begin();
    sub_003E55B0_gen();
    xmv_stage_end("resource-release",start);
}
extern void sub_00087960_gen(void);
void sub_00087960(void)
{
    uint64_t start=xmv_stage_begin();
    sub_00087960_gen();
    xmv_stage_end("title-open",start);
}
void sub_003F7031(void)
{
    uint64_t start=xmv_stage_begin();
    sub_003F7031_gen();
    xmv_stage_end("destroy",start);
}
extern void sub_003F6AED_gen(void);
void sub_003F6AED(void)
{
    uint64_t start=xmv_stage_begin();
    sub_003F6AED_gen();
    xmv_stage_end("open",start);
}
extern void sub_003F6D1A_gen(void);
void sub_003F6D1A(void)
{
    uint64_t start=xmv_stage_begin();
    sub_003F6D1A_gen();
    xmv_stage_end("initialize",start);
}
extern void sub_003F9F49_gen(void);
extern void sub_00087710_gen(void);
void sub_00087710(void)
{
    uint64_t start=xmv_stage_begin();
    sub_00087710_gen();
    xmv_stage_end("title-update",start);
}
extern void sub_00087810_gen(void);
void sub_00087810(void)
{
    uint64_t start=xmv_stage_begin();
    sub_00087810_gen();
    xmv_stage_end("title-render",start);
}
void sub_003F9F49(void)
{
    uint64_t start=xmv_stage_begin();
    sub_003F9F49_gen();
    xmv_stage_end("decode",start);
}
void sub_003F7348(void)
{
    static int enabled = -1;
    if (enabled < 0) enabled = !UCRR_PLAY_ONLY && getenv("UCRR_AUDIO_WAIT_TRACE") != NULL;
    if (!enabled) { sub_003F7348_gen(); return; }
    uint32_t movie = MEM32(g_esp + 4u), output = MEM32(g_esp + 12u);
    uint64_t stage_start = xmv_stage_begin();
    uint64_t start = GetTickCount64();
    sub_003F7348_gen();
    uint64_t elapsed = GetTickCount64() - start;
    xmv_stage_end("next-frame",stage_start);
    if ((int32_t)g_eax >= 0 && output && output <= g_xbox_total_ram - 4u)
        trace_xmv_delivery(movie, MEM32(output), elapsed);
}

/* DirectSoundDoWork (0x00425074-0x0042509D).
 *
 * The host kernel accepts the title's KeConnectInterrupt request, but it does
 * not deliver MCPX APU interrupts.  The original interrupt handler owns the
 * stream scan that observes completed notifier slots and invokes packet
 * callbacks.  Run that scan synchronously from DirectSoundDoWork, the XDK's
 * safe game-thread service point, before draining the original deferred-work
 * callbacks so newly completed packets can be serviced in this same poll. */
void sub_00425074(void)
{
    static uint32_t serviced_notifier_generation;
#if !UCRR_LEAN_RUNTIME
    /* Diagnostic state is per calling host thread, independent of guest state.
     * Millisecond host ticks line up with sparse APU lifecycle snapshots. */
    static __declspec(thread) struct {
        uint64_t last_entry, report_at, max_gap, max_call;
        uint32_t polls, scans, drains, catchups, reports;
    } service_trace;
    uint64_t trace_entry = service_trace.reports < 128 ? GetTickCount64() : 0;
    if (service_trace.reports < 128) {
        if (service_trace.last_entry && trace_entry-service_trace.last_entry > service_trace.max_gap)
            service_trace.max_gap = trace_entry-service_trace.last_entry;
        service_trace.last_entry = trace_entry;
        if (!service_trace.report_at) service_trace.report_at = trace_entry;
        ++service_trace.polls;
    }
#endif
    uint32_t do_notify;
    uint32_t sound_device;

    PUSH32(g_esp, g_esi);
    PUSH32(g_esp, 0x0042507Au);
    sub_00423AB5();

    do_notify = (uint32_t)(uint8_t)g_eax;
    /* A real completion can arrive during the scan or deferred callbacks.
     * Catch up once before returning to potentially expensive scene work.
     * Bound this to two passes; never synthesize a completion or spin until
     * an asynchronously changing generation settles. */
    for (unsigned service_pass=0; service_pass<2; ++service_pass) {
    sound_device = MEM32(0x00442320u);
    if (sound_device != 0) {
        uint32_t apu_device = MEM32(sound_device + 0x0Cu);
        uint32_t notifier_generation =
            xbox_GetMcpxApuNotifierGeneration();

        /* Voice-idle and SSL packet completion are distinct events. An OFF
         * can leave an already-successful SSL notifier unchanged, so its
         * generation cannot gate idle retirement. */
        if (apu_device != 0) service_pending_voice_retirements(apu_device);

        /* On Xbox, sub_004286C8 is reached from the MCPX APU DPC only after
         * hardware raises a completion interrupt.  Calling the full scan on
         * every DirectSoundDoWork poll is especially expensive for XMV, which
         * polls this entry point while waiting for each movie frame.  The
         * runtime's narrow APU model publishes a generation whenever it turns
         * an SSL notifier from 0x80 (in progress) into 1 (success), so service
         * the original routine once per observed completion batch. */
        /* Normal idle retirement queues device+4A0 work even if its already-
         * successful SSL notifier did not change generation. Drain that real
         * work too; otherwise software PLAYING stays stale after unlink. */
        uint32_t idle_head=(apu_device && apu_device<=g_xbox_total_ram-0x4A8u)
            ? MEM32(apu_device+0x4A0u):0;
        if (apu_device != 0 &&
            (notifier_generation != serviced_notifier_generation ||
             (idle_head && idle_head!=apu_device+0x4A0u))) {
            serviced_notifier_generation = notifier_generation;
            g_ecx = apu_device;
            PUSH32(g_esp, 0x0042508Cu);
            sub_004286C8();
#if !UCRR_LEAN_RUNTIME
            if (service_trace.reports < 128) {
                ++service_trace.scans;
                service_trace.catchups += service_pass != 0;
            }
#endif
        }
    }

    /* Original deferred callback drain (00423E5C -> 00428A1B) must see the
     * completions just harvested above. Re-read device ownership after scan. */
    sound_device = MEM32(0x00442320u);
    if (sound_device != 0) {
        PUSH32(g_esp, sound_device);
        PUSH32(g_esp, 0x0042508Cu);
        sub_00423E5C();
#if !UCRR_LEAN_RUNTIME
        if (service_trace.reports < 128) ++service_trace.drains;
#endif
    }

    sound_device = MEM32(0x00442320u);
    if (!sound_device || !MEM32(sound_device + 0x0Cu) ||
        xbox_GetMcpxApuNotifierGeneration() == serviced_notifier_generation)
        break;
    }

    POP32(g_esp, g_esi);
    if (do_notify != 0) {
        uint32_t saved_esp = g_esp;
        uint32_t target = MEM32(0x0044B3ECu);
        PUSH32(g_esp, 0x00441CA4u);
        PUSH32(g_esp, 0x0042509Cu);
        RECOMP_ICALL_SAFE(target, saved_esp);
    }

    g_esp += 4;
#if !UCRR_LEAN_RUNTIME
    if (service_trace.reports < 128) {
        uint64_t trace_end = GetTickCount64();
        if (trace_end-trace_entry > service_trace.max_call) service_trace.max_call = trace_end-trace_entry;
        if (trace_end-service_trace.report_at >= 5000) {
            fprintf(stderr, "[APU-SERVICE-HEALTH] tick_ms=%llu thread=%lu polls=%u scans=%u drains=%u catchups=%u max_entry_gap_ms=%llu max_call_ms=%llu serviced=%u published=%u report=%u/128\n",
                (unsigned long long)trace_end, (unsigned long)GetCurrentThreadId(),
                service_trace.polls, service_trace.scans, service_trace.drains, service_trace.catchups,
                (unsigned long long)service_trace.max_gap, (unsigned long long)service_trace.max_call,
                serviced_notifier_generation, xbox_GetMcpxApuNotifierGeneration(), service_trace.reports+1);
            ++service_trace.reports;
            service_trace.polls=service_trace.scans=service_trace.drains=service_trace.catchups=0;
            service_trace.max_gap=service_trace.max_call=0;
            service_trace.report_at=trace_end;
        }
    }
#endif
}

/* DirectSound's synchronous voice-command wait (0x0042B982).
 *
 * The original loop waits for the MCPX voice interrupt to call sub_0042AD16,
 * which retires the submitted method and clears flag 0x8000.  The standalone
 * runtime accepts KeConnectInterrupt but cannot preempt recompiled code with
 * that ISR.  Consequently the generated loop can only reach its emergency
 * timeout path after 0.5 seconds, often several times per movie frame.
 *
 * The APU command queue is intentionally modeled as always empty and no DSP
 * consumes these methods, so complete a synchronous command successfully at
 * the point where the title waits for it.  This preserves the game's own
 * completion bookkeeping instead of merely deleting the pending flag. */
void sub_0042B982(void)
{
    PUSH32(g_esp, g_esi);
    g_esi = g_ecx;

    if ((MEM8(g_esi + 0x12u) & 0x01u) != 0 &&
        (MEM16(g_esi + 0x12u) & 0x8000u) != 0) {
        g_ecx = g_esi;
        PUSH32(g_esp, 1u);
        PUSH32(g_esp, 0x0042B994u);
        sub_0042AD16();
    }

    POP32(g_esp, g_esi);
    g_esp += 4;
}

/* Alternate-entry deleting destructors used by front-end objects.
 *
 * The original image places each of these entry points inside or immediately
 * after another routine.  CFG ownership therefore folded their instructions
 * into a neighboring generated function.  Two addresses were emitted as
 * empty fallback stubs and two were absent from the dispatch table entirely.
 * Besides skipping object teardown, the fallback stubs implemented RET rather
 * than RET 4, leaking one guest argument on every indirect call.  Front-end
 * transitions exercise these paths heavily, so that four-byte drift quickly
 * corrupts later virtual-call targets.
 *
 * The two folded bodies below reuse their owning generated functions after
 * cancelling the owner's this-pointer adjustment. */
void sub_0029E6D0(void)
{
    g_ecx += 0x10u;
    sub_0029E6C0();
}

void sub_002FD600(void)
{
    g_ecx += 0x0Cu;
    sub_002FD5F0();
}

/* Deleting destructor at 0x00069570.  This entry follows seven alignment
 * INT3 instructions inside the incorrectly merged sub_00069500 range, so it
 * must be reproduced directly instead of entering that generated owner. */
void sub_00069570(void)
{
    PUSH32(g_esp, g_esi);
    g_esi = g_ecx;

    PUSH32(g_esp, 0x00069578u);
    sub_00069590();

    if ((MEM8(g_esp + 8u) & 1u) != 0) {
        PUSH32(g_esp, g_esi);
        PUSH32(g_esp, 0x00069585u);
        sub_002D5CCD();
        g_esp += 4u;
    }

    g_eax = g_esi;
    POP32(g_esp, g_esi);
    g_esp += 8u;
}

/* Deleting destructor at 0x00208C90.  Like the entry above, this short real
 * routine was missed as a standalone CFG function. */
void sub_00208C90(void)
{
    PUSH32(g_esp, g_esi);
    g_esi = g_ecx;

    PUSH32(g_esp, 0x00208C98u);
    sub_00208CB0();

    if ((MEM8(g_esp + 8u) & 1u) != 0) {
        PUSH32(g_esp, g_esi);
        PUSH32(g_esp, 0x00208CA5u);
        sub_002D5CCD();
        g_esp += 4u;
    }

    g_eax = g_esi;
    POP32(g_esp, g_esi);
    g_esp += 8u;
}

/* Xbox CRT strchr optimized-body alternate entry (0x002D65B6).
 *
 * The public CRT entry at 0x002D65B0 loads the search byte into AL and falls
 * directly into this body.  strstr also tail-jumps here after loading AL, so
 * 0x002D65B6 must remain callable even though CFG ownership correctly folds
 * the same instructions into 0x002D65B0.  A generated unresolved stub left AL
 * unchanged; its caller interpreted that byte as a pointer and attempted an
 * almost-4-GiB memcpy while resolving the first mission's asset path. */
void sub_002D65B6(void)
{
    const uint8_t needle = (uint8_t)(g_eax & 0xFFu);
    uint32_t cursor = MEM32(g_esp + 4u);

    for (;;) {
        const uint8_t current = MEM8(cursor);
        if (current == needle) {
            g_eax = cursor;
            break;
        }
        if (current == 0) {
            g_eax = 0;
            break;
        }
        ++cursor;
    }

    /* RET consumes the inherited return address.  The cdecl caller owns the
     * string and character arguments that remain above it. */
    g_esp += 4;
}

/* Urban Chaos implements a small cooperative coroutine system with its own
 * setjmp/longjmp routines at 0x00221890/0x002218C0.  The original code swaps
 * the Xbox ESP and resumes at an address saved inside a 0x30-byte context.
 * Mechanically translated C calls cannot perform that non-local transfer: a
 * translated longjmp used to return to its C caller, which then popped the
 * restored Xbox stack a second time.  The first front-end coroutine therefore
 * loaded 0x00218CC9 (a return address) into ESI and crashed in 0x0021AB20.
 *
 * Keep the original cooperative semantics with host fibers.  Each game
 * context owns a host call stack, while the shared 64-KiB Xbox coroutine stack
 * is copied out on yield and restored on resume just as the original routine
 * copied it through context fields +0x20/+0x24.  The public resume/yield entry
 * points remain unchanged, so every menu, entity, and serialization callback
 * continues to use the game's existing control flow.
 */

typedef struct UCRR_COROUTINE_REGS {
    uint32_t eax, ecx, edx, esp;
    uint32_t ebx, esi, edi;
    uint32_t ebp, seh_ebp;
    uint32_t exception_list;
    RecompXmm xmm[8];
    uint64_t mmx[8];
    double fp_stack[8];
    int fp_top;
    uint16_t fp_control_word;
    int fp_cmp;
    int recomp_zf;
} UCRR_COROUTINE_REGS;

typedef struct UCRR_COROUTINE UCRR_COROUTINE;

typedef struct UCRR_COROUTINE_FRAME {
    void *parent_fiber;
    UCRR_COROUTINE *parent_coroutine;
    UCRR_COROUTINE_REGS parent_regs;
} UCRR_COROUTINE_FRAME;

struct UCRR_COROUTINE {
    uint32_t context_va;
    uint32_t guest_stack_base;
    void *fiber;
    uint8_t *stack_copy;
    uint32_t stack_offset;
    int stack_valid;
    int finished;
    int last_yield;
    UCRR_COROUTINE_REGS regs;
    UCRR_COROUTINE_FRAME *parent_frame;
    UCRR_COROUTINE *next;
};

static RECOMP_TLS void *g_ucrr_thread_fiber;
static RECOMP_TLS UCRR_COROUTINE *g_ucrr_coroutines;
static RECOMP_TLS UCRR_COROUTINE *g_ucrr_active_coroutine;
static RECOMP_TLS unsigned g_ucrr_coroutine_warnings;

static void ucrr_coroutine_regs_save(UCRR_COROUTINE_REGS *regs)
{
    regs->eax = g_eax;
    regs->ecx = g_ecx;
    regs->edx = g_edx;
    regs->esp = g_esp;
    regs->ebx = g_ebx;
    regs->esi = g_esi;
    regs->edi = g_edi;
    regs->ebp = g_ebp;
    regs->seh_ebp = g_seh_ebp;
    regs->exception_list = MEM32(0);
    regs->xmm[0] = g_xmm0;
    regs->xmm[1] = g_xmm1;
    regs->xmm[2] = g_xmm2;
    regs->xmm[3] = g_xmm3;
    regs->xmm[4] = g_xmm4;
    regs->xmm[5] = g_xmm5;
    regs->xmm[6] = g_xmm6;
    regs->xmm[7] = g_xmm7;
    memcpy(regs->mmx, g_mmx, sizeof(regs->mmx));
    memcpy(regs->fp_stack, g_fp_stack, sizeof(regs->fp_stack));
    regs->fp_top = g_fp_top;
    regs->fp_control_word = g_fp_control_word;
    regs->fp_cmp = g_fp_cmp;
    regs->recomp_zf = g_recomp_zf;
}

static void ucrr_coroutine_regs_load(const UCRR_COROUTINE_REGS *regs)
{
    g_eax = regs->eax;
    g_ecx = regs->ecx;
    g_edx = regs->edx;
    g_esp = regs->esp;
    g_ebx = regs->ebx;
    g_esi = regs->esi;
    g_edi = regs->edi;
    g_ebp = regs->ebp;
    g_seh_ebp = regs->seh_ebp;
    MEM32(0) = regs->exception_list;
    g_xmm0 = regs->xmm[0];
    g_xmm1 = regs->xmm[1];
    g_xmm2 = regs->xmm[2];
    g_xmm3 = regs->xmm[3];
    g_xmm4 = regs->xmm[4];
    g_xmm5 = regs->xmm[5];
    g_xmm6 = regs->xmm[6];
    g_xmm7 = regs->xmm[7];
    memcpy(g_mmx, regs->mmx, sizeof(regs->mmx));
    memcpy(g_fp_stack, regs->fp_stack, sizeof(regs->fp_stack));
    g_fp_top = regs->fp_top;
    g_fp_control_word = regs->fp_control_word;
    g_fp_cmp = regs->fp_cmp;
    g_recomp_zf = regs->recomp_zf;
}

static void ucrr_coroutine_stack_save(UCRR_COROUTINE *coroutine)
{
    if (coroutine && coroutine->stack_copy && coroutine->guest_stack_base) {
        uint32_t offset = g_esp - coroutine->guest_stack_base;
        if (offset > 0x10000u) {
            fprintf(stderr, "[UCRR-COROUTINE] Stack outside context %08X: %08X\n",
                    coroutine->context_va, g_esp);
            abort();
        }
        coroutine->stack_offset = offset;
        memcpy(coroutine->stack_copy,
               (const void *)XBOX_PTR(coroutine->guest_stack_base + offset),
               0x10000u - offset);
        coroutine->stack_valid = 1;
    }
}

static void ucrr_coroutine_stack_load(const UCRR_COROUTINE *coroutine)
{
    if (coroutine && coroutine->stack_copy && coroutine->guest_stack_base &&
        coroutine->stack_valid) {
        memcpy((void *)XBOX_PTR(coroutine->guest_stack_base + coroutine->stack_offset),
               coroutine->stack_copy, 0x10000u - coroutine->stack_offset);
    }
}

static UCRR_COROUTINE *ucrr_coroutine_find(uint32_t context_va)
{
    UCRR_COROUTINE *coroutine;
    for (coroutine = g_ucrr_coroutines; coroutine;
         coroutine = coroutine->next) {
        if (coroutine->context_va == context_va) {
            if (coroutine == g_ucrr_active_coroutine || coroutine->parent_frame) {
                fprintf(stderr, "[UCRR-COROUTINE] Destroying running context %08X\n",
                        context_va);
                abort();
            }
            return coroutine;
        }
    }
    return NULL;
}

static void *ucrr_coroutine_prepare_thread(void)
{
    if (!g_ucrr_thread_fiber) {
        g_ucrr_thread_fiber = ConvertThreadToFiber(NULL);
        if (!g_ucrr_thread_fiber && GetLastError() == ERROR_ALREADY_FIBER) {
            g_ucrr_thread_fiber = GetCurrentFiber();
        }
        if (!g_ucrr_thread_fiber && g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] ConvertThreadToFiber failed: %lu\n",
                    (unsigned long)GetLastError());
        }
    }
    return g_ucrr_thread_fiber;
}

static void ucrr_coroutine_switch_to_parent(UCRR_COROUTINE *coroutine,
                                             int value)
{
    UCRR_COROUTINE_FRAME *frame = coroutine->parent_frame;

    coroutine->last_yield = value;
    ucrr_coroutine_regs_save(&coroutine->regs);
    ucrr_coroutine_stack_save(coroutine);

    if (!frame || !frame->parent_fiber) {
        if (g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] Context %08X has no parent fiber\n",
                    coroutine->context_va);
        }
        return;
    }

    if (frame->parent_coroutine) {
        ucrr_coroutine_stack_load(frame->parent_coroutine);
    }
    ucrr_coroutine_regs_load(&frame->parent_regs);
    g_ucrr_active_coroutine = frame->parent_coroutine;
    SwitchToFiber(frame->parent_fiber);
}

static VOID CALLBACK ucrr_coroutine_entry(void *argument)
{
    UCRR_COROUTINE *coroutine = (UCRR_COROUTINE *)argument;
    uint32_t object;
    uint32_t vtable;
    uint32_t target;
    uint32_t saved_esp;
    uint32_t return_va;

    g_ucrr_active_coroutine = coroutine;
    object = MEM32(coroutine->context_va + 0x28u);
    if (!object) {
        if (g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] Context %08X has no callback object\n",
                    coroutine->context_va);
        }
    } else {
        vtable = MEM32(object);
        if (MEM8(coroutine->context_va + 0x2Cu)) {
            target = MEM32(vtable + 0x5Cu);
            return_va = 0x001FB826u;
        } else {
            target = MEM32(vtable + 0x58u);
            return_va = 0x001FB83Du;
        }

        MEM8(coroutine->context_va + 0x2Du) = 1;
        g_eax = object;
        g_ecx = object;
        g_edx = vtable;
        saved_esp = g_esp;
        PUSH32(g_esp, return_va);
        RECOMP_ICALL_SAFE(target, saved_esp);
    }

    coroutine->finished = 1;
    ucrr_coroutine_switch_to_parent(coroutine, 2);

    /* A completed coroutine is normally deleted by 0x001FBA40 without being
     * resumed.  Keep a defensive yield loop so an unexpected extra resume can
     * never return from the Win32 fiber entry point and terminate the thread. */
    for (;;) {
        ucrr_coroutine_switch_to_parent(coroutine, 2);
    }
}

static UCRR_COROUTINE *ucrr_coroutine_create(
    uint32_t context_va, const UCRR_COROUTINE_REGS *parent_regs)
{
    UCRR_COROUTINE *coroutine;

    if (!ucrr_coroutine_prepare_thread()) {
        return NULL;
    }

    coroutine = (UCRR_COROUTINE *)calloc(1, sizeof(*coroutine));
    if (!coroutine) {
        return NULL;
    }
    coroutine->stack_copy = (uint8_t *)malloc(0x10000u);
    if (!coroutine->stack_copy) {
        free(coroutine);
        return NULL;
    }

    coroutine->context_va = context_va;
    coroutine->guest_stack_base = MEM32(0x51C3DCu);
    coroutine->regs = *parent_regs;
    coroutine->regs.eax = context_va;
    coroutine->regs.ebx = context_va;
    coroutine->regs.ecx = context_va;
    if (coroutine->guest_stack_base) {
        coroutine->regs.esp = coroutine->guest_stack_base + 0xFFF4u;
    }

    coroutine->fiber = CreateFiber(0, ucrr_coroutine_entry, coroutine);
    if (!coroutine->fiber || !coroutine->guest_stack_base) {
        if (coroutine->fiber) {
            DeleteFiber(coroutine->fiber);
        }
        free(coroutine->stack_copy);
        free(coroutine);
        return NULL;
    }

    coroutine->next = g_ucrr_coroutines;
    g_ucrr_coroutines = coroutine;
    if(getenv("UCRR_COROUTINE_TRACE"))fprintf(stderr,
            "[UCRR-COROUTINE] Created context=%08X guest_stack=%08X\n",
            context_va, coroutine->guest_stack_base);
    return coroutine;
}

static void ucrr_coroutine_destroy(uint32_t context_va)
{
    UCRR_COROUTINE **link = &g_ucrr_coroutines;
    UCRR_COROUTINE *coroutine;

    while ((coroutine = *link) != NULL) {
        if (coroutine->context_va == context_va) {
            *link = coroutine->next;
            if (coroutine != g_ucrr_active_coroutine && coroutine->fiber) {
                DeleteFiber(coroutine->fiber);
            }
            free(coroutine->stack_copy);
            free(coroutine);
            return;
        }
        link = &coroutine->next;
    }
}

/* Resume one coroutine step.  This replaces the generated body at 0x001FB790
 * and returns AL=1 only after the callback has completed, matching the
 * original comparison of the outer longjmp result with 2. */
void sub_001FB790(void)
{
    const uint32_t context_va = g_ecx;
    UCRR_COROUTINE_FRAME frame;
    UCRR_COROUTINE *coroutine;

    memset(&frame, 0, sizeof(frame));
    ucrr_coroutine_regs_save(&frame.parent_regs);
    frame.parent_coroutine = g_ucrr_active_coroutine;
    frame.parent_fiber = ucrr_coroutine_prepare_thread()
        ? GetCurrentFiber() : NULL;

    coroutine = ucrr_coroutine_find(context_va);
    if (!coroutine) {
        coroutine = ucrr_coroutine_create(context_va, &frame.parent_regs);
    }

    if (!coroutine) {
        fprintf(stderr, "[UCRR-COROUTINE] Cannot allocate context %08X\n", context_va);
        abort(); /* Do not report a loading callback as successfully completed. */
    }
    if (coroutine->parent_frame || coroutine == g_ucrr_active_coroutine) {
        fprintf(stderr, "[UCRR-COROUTINE] Recursive resume of context %08X\n", context_va);
        abort();
    }
    if (coroutine->finished) {
        MEM8(context_va + 0x2Du) = 0;
        g_eax = 1;
        g_esp = frame.parent_regs.esp + 4u;
        return;
    }

    if (frame.parent_coroutine) {
        ucrr_coroutine_stack_save(frame.parent_coroutine);
    }
    ucrr_coroutine_stack_load(coroutine);
    coroutine->parent_frame = &frame;
    MEM8(context_va + 0x2Du) = 1;
    ucrr_coroutine_regs_load(&coroutine->regs);
    g_ucrr_active_coroutine = coroutine;
    SwitchToFiber(coroutine->fiber);

    /* The yielding fiber restored this snapshot before switching back.  Load
     * it again deliberately so the contract remains explicit and resilient to
     * future diagnostic code added between the switch and this point. */
    ucrr_coroutine_regs_load(&frame.parent_regs);
    g_ucrr_active_coroutine = frame.parent_coroutine;
    coroutine->parent_frame = NULL;
    MEM8(context_va + 0x2Du) = 0;
    g_eax = (coroutine->last_yield == 2) ? 1u : 0u;
    g_esp = frame.parent_regs.esp + 4u;
}

/* Yield from the active callback.  Advancing ESP before the host switch makes
 * the suspended guest state exactly the state following this function's RET;
 * when the fiber is resumed, returning from this C function continues at the
 * original caller without executing a longjmp call site twice. */
void sub_001FB930(void)
{
    uint32_t context_va = g_ecx ? MEM32(g_ecx) : 0;
    UCRR_COROUTINE *coroutine = g_ucrr_active_coroutine;

    if (!coroutine || coroutine->context_va != context_va) {
        if (g_ucrr_coroutine_warnings++ < 8u) {
            fprintf(stderr,
                    "[UCRR-COROUTINE] Yield outside active context: %08X\n",
                    context_va);
        }
        g_eax = 1;
        g_esp += 4u;
        return;
    }

    MEM8(context_va + 0x2Du) = 0;
    g_eax = 1;
    g_esp += 4u;
    ucrr_coroutine_switch_to_parent(coroutine, 1);
}

extern void sub_001FBA40_gen(void);

void sub_001FBA40(void)
{
    uint32_t context_va = g_ecx ? MEM32(g_ecx) : 0;
    if (context_va) {
        ucrr_coroutine_destroy(context_va);
    }
    sub_001FBA40_gen();
}

/* Resource/render callback at 0x000583B0.
 *
 * This is a genuine standalone function immediately after an INT3-padded
 * neighbor.  Exact seeding makes the disassembler recover all 33 original
 * instructions, but the recompiler's later CFG-ownership pass still folds the
 * entry into an earlier weak root and omits it from the generated dispatch
 * table.  Reproduce the recovered body here so indirect calls can resolve it.
 */
extern void sub_00058420(void);
extern void sub_002D57FB(void);

void sub_000583B0(void)
{
    uint32_t allocation;
    uint32_t constructor_arg;

    PUSH32(g_esp, 0xFFFFFFFFu);
    PUSH32(g_esp, 0x0038AD6Bu);
    PUSH32(g_esp, MEM32(0));
    MEM32(0) = g_esp;
    PUSH32(g_esp, g_ecx);

    PUSH32(g_esp, 0x30u);
    PUSH32(g_esp, 0x000583CDu);
    sub_002D57FB();
    g_esp += 4u;

    allocation = g_eax;
    MEM32(g_esp) = allocation;
    MEM32(g_esp + 0x0Cu) = 0;
    if (allocation != 0) {
        /* Capture this before PUSH32 changes ESP: the original computes the
         * argument at [esp + 0x14] + 4, then pushes it for the constructor. */
        constructor_arg = MEM32(g_esp + 0x14u) + 4u;
        PUSH32(g_esp, constructor_arg);
        g_ecx = allocation;
        PUSH32(g_esp, 0x000583EEu);
        sub_00058420();
    } else {
        g_eax = 0;
    }

    g_ecx = MEM32(g_eax + 4u);
    MEM32(g_esp + 0x0Cu) = 0xFFFFFFFFu;
    if (g_ecx == 0) {
        uint32_t saved_esp = g_esp;
        g_edx = MEM32(g_eax);
        PUSH32(g_esp, 1u);
        g_ecx = g_eax;
        PUSH32(g_esp, 0x0005840Au);
        RECOMP_ICALL_SAFE(MEM32(g_edx + 4u), saved_esp);
        g_eax = 0;
    }

    g_ecx = MEM32(g_esp + 4u);
    MEM32(0) = g_ecx;
    g_esp += 0x18u; /* add esp, 0x10; ret 4 */
}

int g_recomp_call_cache_enabled;
unsigned g_uc_scene_getter_mask;
extern void sub_000898A0(void),sub_000898C0(void),sub_0008BBD0(void);
void uc_scene_getter_init(void){
    const char* enabled=getenv("UCRR_SCENE_GETTER_CERTIFICATE");
    g_uc_scene_getter_mask=0;
    if(!enabled||strcmp(enabled,"1"))return;
    const uint32_t va[]={0x898a0,0x898c0,0x8bbd0};
    recomp_func_t expected[]={sub_000898A0,sub_000898C0,sub_0008BBD0};
    for(unsigned i=0;i<3;i++){
        recomp_func_t fn=recomp_lookup_manual(va[i]);
        if(!fn)fn=recomp_lookup(va[i]);
        if(fn==expected[i])g_uc_scene_getter_mask|=1u<<i;
    }
}
__declspec(thread) RecompCallCache g_recomp_call_cache;
recomp_func_t recomp_lookup_manual(uint32_t xbox_va)
{
    /* Sparse switch avoids a linear miss walk; override priority is unchanged. */
    switch (xbox_va) {
#if defined(UCRR_PARTICLE_RESTORATION)
    case 0x0009AD30u: return uc_particle_restoration_enabled() ? uc_particle_restoration_draw : (recomp_func_t)0;
#endif
#if defined(UCRR_DECAL_RESTORATION)
    case 0x0009C2C0u: return uc_decal_restoration_enabled() ? uc_decal_restoration_callback_wrapper : (recomp_func_t)0;
#endif
#if defined(UCRR_GLOW_REMOVAL)
    case 0x000A32D0u: return uc_glow_removal_enabled() ? uc_glow_stage : (recomp_func_t)0;
#endif
#if defined(UCRR_DECAL_REMOVAL)
    case 0x0008A090u: return uc_decal_removal_enabled() ? uc_decal_removal_visibility_wrapper : (recomp_func_t)0;
    case 0x0009C4A0u: return uc_decal_removal_enabled() ? uc_decal_removal_render_wrapper : (recomp_func_t)0;
#endif
#if defined(UCRR_SPECULAR_REMOVAL)
    case 0x000B3960u: return uc_specular_removal_enabled() ? uc_specular_material : (recomp_func_t)0;
#endif
#if defined(UCRR_LIGHT_REMOVAL)
    case 0x000B31C0u: return uc_light_removal_enabled() ? uc_light_removal_begin : (recomp_func_t)0;
#endif
#if defined(UCRR_SHADOW_REMOVAL)
    case 0x000AF0F0u: return uc_shadow_removal_enabled() ? uc_shadow_casts : (recomp_func_t)0;
    case 0x000B3680u: return uc_shadow_removal_enabled() ? uc_shadow_caster : (recomp_func_t)0;
#endif
#if defined(UCRR_FEATURE_REMOVAL)
    case 0x00113A20u: return uc_particle_registration_enabled() ? uc_sky_model_setup : (recomp_func_t)0;
    case 0x000AD930u: return uc_glass_probe_enabled() ? uc_glass_render_probe : (recomp_func_t)0;
    case 0x00089EB0u: return uc_particle_registration_enabled() ? uc_registration_00089EB0 : (recomp_func_t)0;
    case 0x00089F90u: return uc_particle_registration_enabled() ? uc_registration_00089F90 : (recomp_func_t)0;
    case 0x00089FF0u: return uc_particle_registration_enabled() ? uc_registration_00089FF0 : (recomp_func_t)0;
    case 0x0008A100u: return uc_particle_registration_enabled() ? uc_registration_0008A100 : (recomp_func_t)0;
    case 0x00239AB0u: return uc_feature_removal_enabled() ? uc_feature_particles : (recomp_func_t)0;
    case 0x0023A330u: return uc_feature_removal_enabled() ? uc_feature_particles_1 : (recomp_func_t)0;
    case 0x0023AD00u: return uc_feature_removal_enabled() ? uc_feature_particles_2 : (recomp_func_t)0;
    case 0x0023B570u: return uc_feature_removal_enabled() ? uc_feature_particles_3 : (recomp_func_t)0;
    case 0x0023BD50u: return uc_feature_removal_enabled() ? uc_feature_particles_4 : (recomp_func_t)0;
    case 0x0023D1B0u: return uc_feature_removal_enabled() ? uc_feature_particles_5 : (recomp_func_t)0;
    case 0x002B9BA0u: return uc_feature_removal_enabled() ? uc_feature_blob_shadow : (recomp_func_t)0;
#endif
    case 0x00161DA0u: return sub_00161DA0;
    case 0x001AEE70u: return sub_001AEE70;
    case 0x001AFF10u: return sub_001AFF10;
    case 0x0004FFE0u: return sub_0004FFE0;
    case 0x000583B0u: return sub_000583B0;
    case 0x000567F0u: return sub_000567F0;
    case 0x0022C2C0u: return sub_0022C2C0;
    case 0x00069570u: return sub_00069570;
    case 0x001FB790u: return sub_001FB790;
    case 0x001FB930u: return sub_001FB930;
    case 0x001FBA40u: return sub_001FBA40;
    case 0x00208C90u: return sub_00208C90;
    case 0x0029E6D0u: return sub_0029E6D0;
    case 0x002D65B6u: return sub_002D65B6;
    case 0x002D6750u: return sub_002D6750;
    case 0x002FD600u: return sub_002FD600;
    case 0x00233CD0u: return sub_00233CD0;
    case 0x003E2CC0u: return sub_003E2CC0;
    case 0x0008BD50u: return trace_display_interval_setter;
    case 0x003E2C30u: return sub_003E2C30;
    case 0x003EF870u: return sub_003EF870;
    case 0x003E9560u: return sub_003E9560;
    case 0x003E9380u: return sub_003E9380;
    case 0x003E9430u: return sub_003E9430;
    case 0x00425074u: return sub_00425074;
    case 0x0042B982u: return sub_0042B982;
    case 0x00428059u: return sub_00428059;
    case 0x0042E9ECu: return sub_0042E9EC;
    default: return (recomp_func_t)0;
    }
}

/* ── ICALL failure logging ─────────────────────────────────── */

/*
 * Called when RECOMP_ICALL cannot resolve a target address.
 * This usually means one of:
 *   - A vtable dispatch to an address not in the dispatch table
 *   - A function pointer loaded from uninitialized or corrupt memory
 *   - A kernel thunk address that the bridge doesn't handle
 *
 * During early bring-up you will see many of these. Most are harmless
 * (the ICALL macro pops the dummy return address and continues).
 * Focus on the ones that cause crashes or incorrect behavior.
 */
void recomp_icall_fail_log(uint32_t va)
{
#if UCRR_PLAY_ONLY
    return;
#endif

    typedef struct UCRR_ICALL_FAILURE {
        uint32_t target;
        uint32_t return_va;
    } UCRR_ICALL_FAILURE;
    static UCRR_ICALL_FAILURE seen[64];
    static unsigned seen_count;
    uint32_t return_va = g_esp ? MEM32(g_esp) : 0;

    /* A broken vtable can be reached every frame. Report each distinct target
     * and call site once so diagnostics remain useful instead of producing
     * millions of duplicate lines while the watchdog samples the loop. */
    for (unsigned i = 0; i < seen_count; ++i) {
        if (seen[i].target == va && seen[i].return_va == return_va) {
            return;
        }
    }
    if (seen_count >= sizeof(seen) / sizeof(seen[0])) {
        return;
    }
    seen[seen_count].target = va;
    seen[seen_count].return_va = return_va;
    ++seen_count;

    fprintf(stderr, "[ICALL] Failed to resolve VA 0x%08X (total calls: %llu)\n",
            va, (unsigned long long)g_icall_count);
    fprintf(stderr,
            "  State: esp=%08X eax=%08X ecx=%08X edx=%08X "
            "ebx=%08X esi=%08X edi=%08X ebp=%08X seh=%08X\n",
            g_esp, g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi,
            g_ebp, g_seh_ebp);
    fprintf(stderr,
            "  Stack: %08X %08X %08X %08X %08X %08X\n",
            MEM32(g_esp), MEM32(g_esp + 4), MEM32(g_esp + 8),
            MEM32(g_esp + 12), MEM32(g_esp + 16), MEM32(g_esp + 20));

    /* Dump last 16 call targets from the ring buffer */
    fprintf(stderr, "  Recent ICALL targets:\n");
    for (int i = 0; i < 16; i++) {
        int idx = (g_icall_trace_idx - 16 + i) & 15;
        if (g_icall_trace[idx])
            fprintf(stderr, "    [%2d] 0x%08X\n", i, g_icall_trace[idx]);
    }
    fflush(stderr);
}
