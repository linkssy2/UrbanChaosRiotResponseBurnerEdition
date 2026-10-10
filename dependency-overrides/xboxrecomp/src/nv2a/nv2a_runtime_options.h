#ifndef NV2A_RUNTIME_OPTIONS_H
#define NV2A_RUNTIME_OPTIONS_H
#include <stdlib.h>
/* Renderer-owner-thread launch options. Preserve getenv presence semantics
 * (including empty strings and "0"). Like the existing GPU-route flags, these
 * are immutable after first use; changing options requires a new process. */
#define NV_OPTION_FUNCTION(name, key) \
    static int nv_option_##name(void) { \
        static int value = -1; \
        if (value < 0) value = getenv(key) != NULL; \
        return value; \
    }
#if UCRR_PLAY_ONLY
#define NV_DIAGNOSTIC_OPTION_FUNCTION(name, key) static int nv_option_##name(void) {return 0;}
#else
#define NV_DIAGNOSTIC_OPTION_FUNCTION NV_OPTION_FUNCTION
#endif
NV_OPTION_FUNCTION(depth_reinterpret, "UCRR_DEPTH_REINTERPRET")
NV_OPTION_FUNCTION(stencil_state, "UCRR_STENCIL_STATE")
NV_OPTION_FUNCTION(multi_textures, "UCRR_MULTI_TEXTURES")
NV_OPTION_FUNCTION(lighting_textures, "UCRR_LIGHTING_TEXTURES")
NV_OPTION_FUNCTION(inline_arrays, "UCRR_INLINE_ARRAYS")
NV_DIAGNOSTIC_OPTION_FUNCTION(pb_trace, "UCRR_PB_TRACE")
NV_DIAGNOSTIC_OPTION_FUNCTION(effect_trace, "UCRR_EFFECT_TRACE")
NV_DIAGNOSTIC_OPTION_FUNCTION(display_trace, "UCRR_DISPLAY_TRACE")
NV_DIAGNOSTIC_OPTION_FUNCTION(surface_trace, "UCRR_SURFACE_TRACE")
NV_DIAGNOSTIC_OPTION_FUNCTION(surface_dump, "UCRR_SURFACE_DUMP_DIR")
NV_DIAGNOSTIC_OPTION_FUNCTION(texture_cache_trace, "UCRR_TEXTURE_CACHE_TRACE")
NV_DIAGNOSTIC_OPTION_FUNCTION(watchdog_mission, "UCRR_WATCHDOG_MISSION")
NV_DIAGNOSTIC_OPTION_FUNCTION(timing_trace, "UCRR_TIMING_TRACE")
NV_OPTION_FUNCTION(vertex_fetch_plan, "UCRR_VERTEX_FETCH_PLAN")
#undef NV_DIAGNOSTIC_OPTION_FUNCTION
#undef NV_OPTION_FUNCTION
#endif
