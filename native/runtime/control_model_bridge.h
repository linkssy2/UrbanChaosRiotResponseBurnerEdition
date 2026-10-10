#ifndef UC_NATIVE_CONTROL_MODEL_BRIDGE_H
#define UC_NATIVE_CONTROL_MODEL_BRIDGE_H
#include "control_particle_bridge.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNativeControlModels UCNativeControlModels;
typedef struct UCNativeControlModelVertex {
    float position[3],normal[3],uv[2];
    uint32_t basis[2],color;
    float basis5[3],basis6[3]; /* Posed, unnormalized source tangent attributes. */
} UCNativeControlModelVertex;
/* Unique source vertices. Position is the original FLOAT3 or decoded SHORT3;
 * normal and tangent words retain authored11/11/10 quantization. Rows packs
 * three absolute raw-palette row indices in bytes0..2, weights in bytes0..1. */
typedef struct UCNativeControlModelIndexedVertex {
    float position[3];uint32_t normal;
    float uv[2];uint32_t basis[2],rows,weights;
} UCNativeControlModelIndexedVertex;

typedef struct UCNativeControlModelView {
    float clip_rows[4][4],attenuation_rows[4][4],center[4],direction[4],color[4],scalar[4],eye[4],specular[4];
    uint32_t kind,material_kind,two_sided,padding; /* 0 directional, 1 point, 2 spotlight; +3 adds source specular alpha. */
} UCNativeControlModelView;
/* Already-posed local triangles, source-qualified material and light constants.
 * No topology, guest memory, camera, palette or command interpreter is owned
 * here. Parent additive blend, depth, stencil and raster state are explicit.
 * Full D3D11.1 context state is restored. 0 rejects before Draw; -1 is fatal
 * and must never cause replay. Uses the particle bridge status values. */
UCNativeControlModels *uc_native_control_models_create(ID3D11Device *,ID3D11DeviceContext *,
    const wchar_t *shader_directory,char *error,size_t capacity);
int uc_native_control_models_draw(UCNativeControlModels *,const UCNativeControlModelVertex *,uint32_t count,
    const UCNativeControlModelView *,const UCNativeControlParticleDraw *,ID3D11RasterizerState *,char *error,size_t capacity);
/* Exact original GPU viewport contract, before the host composes a PC clip
 * matrix. screen is logical width,height,NV097_CLIP_MAX,unused. */
typedef struct UCNativeControlModelSourceClip {
    float rows[4][4],scale[4],bias[4],screen[4];
} UCNativeControlModelSourceClip;
/* Source GPU-first policy for ordinary directional/point kinds0/1/3/4 only.
 * Reproduces RCC, screen mapping and invalid-vertex zero/sentinel semantics.
 * Referenced palette rows and compact spans remain CPU validated; no CPU pose.
 * Spotlight/masked callers retain the conservative API below. */
int uc_native_control_models_draw_indexed_source(UCNativeControlModels *,
    const UCNativeControlModelIndexedVertex *,uint32_t vertex_count,
    const uint32_t *indices,uint32_t index_count,uint32_t primitive,
    const float palette[192][4],uint32_t skinned,const UCNativeControlModelSourceClip *,
    const UCNativeControlModelView *,const UCNativeControlParticleDraw *,
    ID3D11RasterizerState *,char *error,size_t capacity);
/* Optional GPU-pose path; original triangle list5 or strip6 index order,
 * including strip degenerates. Borrowed arrays remain valid only for this call.
 * CPU performs position-only clip admission once per unique vertex; normal and
 * tangent posing moves to GPU. Palette has192float4 rows; only referenced rows
 * must be initialized/finite. Rejection/fatal and parent-state rules as above. */
int uc_native_control_models_draw_indexed(UCNativeControlModels *,
    const UCNativeControlModelIndexedVertex *,uint32_t vertex_count,
    const uint32_t *indices,uint32_t index_count,uint32_t primitive,
    const float palette[192][4],uint32_t skinned,
    const UCNativeControlModelView *,const UCNativeControlParticleDraw *,
    ID3D11RasterizerState *,char *error,size_t capacity);
/* Qualified posed/static texture * quantized vertex-color base draw. The caller
 * retains original material setup, supplies raw source clip/tint and parent states.
 * Keep viewport/RCC operation order so D24 matches the preceding source draws.
 * Status 0 rejects before submission; -1 forbids original replay. */
int uc_native_control_models_base(UCNativeControlModels *,const UCNativeControlModelVertex *,uint32_t,
    const UCNativeControlModelSourceClip *,const float tint[4],const float exposure[4],const float alpha[2],const float constant[4],const UCNativeControlParticleDraw *,
    ID3D11RasterizerState *,char *,size_t);
/* Same base shader and admission, with ordered triangle-list indices. The
 * source strip expansion retains degenerates; no content survives this call. */
int uc_native_control_models_base_indexed(UCNativeControlModels *,const UCNativeControlModelVertex *,uint32_t,
    const uint32_t *,uint32_t,const UCNativeControlModelSourceClip *,const float tint[4],const float exposure[4],const float alpha[2],const float constant[4],
    const UCNativeControlParticleDraw *,ID3D11RasterizerState *,char *,size_t);
void uc_native_control_models_destroy(UCNativeControlModels *);
#ifdef __cplusplus
}
#endif
#endif
