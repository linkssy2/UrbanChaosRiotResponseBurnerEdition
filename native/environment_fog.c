#include "environment_fog.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

UCFogStatus uc_environment_fog_properties(const UCLevel *level, uint32_t entity,
    UCFogProperties *out) {
    if (!level || !out || entity >= level->entity_count || !level->entities)
        return UC_FOG_ARGUMENT;
    const UCLevelEntity *e = &level->entities[entity];
    if (!e->class_name || strcmp(e->class_name, "FogController")) return UC_FOG_ARGUMENT;
    if (e->properties.size != 32 || !e->properties.data) return UC_FOG_FORMAT;
    UCFogProperties p;
    memcpy(&p.enabled, e->properties.data, 4);
    memcpy(&p.start, e->properties.data + 4, 28);
    *out = p;
    return UC_FOG_OK;
}

static uint32_t boundary(float z) {
    /* A6FFC/A7020: signed _ftol followed by UNSIGNED min/max. */
    uint32_t n = (uint32_t)(int64_t)((double)z * 65536.0 - 1.0);
    return n > 65535 ? 65535 : n < 1 ? 1 : n;
}

UCFogStatus uc_environment_depth_lookup(float start,float end,float maximum,
    float m22,float m32,uint8_t out[65536]) {
    if(!out)return UC_FOG_ARGUMENT;
    if (fegetround() != FE_TONEAREST || (_mm_getcsr() & 0x6000)) return UC_FOG_UNSUPPORTED;
    if (!isfinite(start) || !isfinite(end) || !isfinite(maximum) ||
        !isfinite(m22) || !isfinite(m32) || maximum < 0 || maximum > 1 ||
        m22 <= 0 || m32 >= 0) return UC_FOG_ARGUMENT;
    start = fabsf(m32) > start ? fabsf(m32) + 1.0f : start;
    if (!isfinite(start) || start<=0 || end <= start) return UC_FOG_ARGUMENT;
    float z0 = (float)((double)m32 / (double)start + (double)m22);
    float z1 = m32 / end;
    z1 = z1 + m22;
    /* Bound conversions and admit the perspective interval used by this pass. */
    if (!isfinite(z0) || !isfinite(z1) || z0 < 0 || z1 > 1 || z1 < z0)
        return UC_FOG_UNSUPPORTED;
    uint32_t first = boundary(z0), last = boundary(z1);
    if (last < first) return UC_FOG_UNSUPPORTED;
    memset(out, 0, first);
    memset(out + last, (uint8_t)((double)maximum * 255.0), 65536 - last);
    if (first < last) {
        float step = (float)(((double)z1 - (double)z0) / (double)(last - first));
        float scale = maximum / (end - start);
        scale = scale * 256.0f;
        float z = z0;
        for (uint32_t i = first; i < last; ++i) {
            float v = z - m22;
            v = m32 / v;
            v = v - z0; /* Real A7126 subtraction; do not simplify it away. */
            v = v - start;
            v = v * scale;
            out[i] = !(v < 255) ? 255 : v <= 0 ? 0 : (uint8_t)v;
            z = step + z;
        }
    }
    return UC_FOG_OK;
}
UCFogStatus uc_environment_fog_build(const UCFogProperties *p, float m22,
    float m32, float exposure, UCFogTable *out) {
    if (!p || !out || p->enabled > 1) return UC_FOG_ARGUMENT;
    if (!p->enabled) return UC_FOG_DISABLED;
    if(!isfinite(exposure)||exposure<0||p->start<=0||p->end<=p->start)return UC_FOG_ARGUMENT;
    if(p->distance_to_full!=0)return UC_FOG_UNSUPPORTED;
    for(unsigned k=0;k<3;k++)if(!isfinite(p->color[k])||p->color[k]<0||p->color[k]>1)return UC_FOG_ARGUMENT;
    UCFogTable result;
    UCFogStatus status=uc_environment_depth_lookup(p->start,p->end,p->maximum,m22,m32,result.alpha);
    if(status!=UC_FOG_OK)return status;
    uint32_t bytes[4];
    for (unsigned k = 0; k < 3; ++k) bytes[k] = (uint32_t)(p->color[k] * 255.0f);
    bytes[3] = 255;
    result.authored_argb = bytes[3] << 24 | bytes[0] << 16 | bytes[1] << 8 | bytes[2];
    for (unsigned k = 0; k < 4; ++k) {
        float v = (float)bytes[k] * (1.0f / 255.0f);
        v = v * exposure;
        if (v > 1) v = 1;
        if (v < 0) v = 0;
        v = v * 255.0f;
        /* 3EEEEE CVTSS2SI is round-to-nearest, unlike 87286 CVTTSS2SI. */
        bytes[k] = (uint32_t)_mm_cvtss_si32(_mm_set_ss(v));
        result.rgba[k] = (float)bytes[k] * (1.0f / 255.0f);
    }
    result.constant_argb = bytes[3] << 24 | bytes[0] << 16 | bytes[1] << 8 | bytes[2];
    *out = result;
    return UC_FOG_OK;
}
