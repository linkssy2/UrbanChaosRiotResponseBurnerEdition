#ifndef UC_HYBRID_MODELS_GUEST_H
#define UC_HYBRID_MODELS_GUEST_H
#include "hybrid_lighting_guest.h"
#include "../native/model_render.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UcHybridModelVertex {
    float position[3],normal[3],uv[2];
    uint32_t basis[2],color;
    float basis5[3],basis6[3]; /* Source v5/v6, posed with the normal palette. */
} UcHybridModelVertex;
/* Unposed, unique referenced vertices; palette rows and byte weights retain the
 * exact source fetch representation. No decoded normal or CPU skinning here. */
typedef struct UcHybridModelIndexedVertex {
    float position[3];uint32_t normal;
    float uv[2];uint32_t basis[2],rows,weights;
} UcHybridModelIndexedVertex;
typedef struct UcHybridModelBatch {
    uint32_t receiver_va,resource_va,node_va,material_va,source_batch,partition;
    uint32_t flags,material_flags,material_mode,skinned;
    float world[16],projection[16],tint[4],specular_strength,material_scalar;
    float bounds[12]; /* Source local AABB at receiver+80, min/max/center/extent. */
    /* Original light material setup program, copied and bounded. Empty means
     * the direct 8EC6E draw route. Host must qualify every nonempty recipe. */
    uint32_t setup_count,setup_bytes;
    unsigned char setup[256];
    uint32_t texture_image_va,texture_header_va,texture_header[5];
    uint32_t texture_address,texture_filter,texture_control0;
    const UcHybridModelVertex *vertices;
    uint32_t vertex_count;
    const UcHybridModelIndexedVertex *indexed_vertices;
    const uint32_t *indices;uint32_t index_count,primitive;
    const UCModelRenderPalette *palette;
} UcHybridModelBatch;
typedef struct UcHybridModelLightView {
    float clip_rows[4][4],attenuation_rows[4][4],center[4],direction[4],color[4],scalar[4];
    float eye[4],specular[4];
    uint32_t kind,material_kind,two_sided,padding;
} UcHybridModelLightView;
int uc_hybrid_models_prepare(const UcHybridLightingScene *,const UcHybridLightingRecord *,
    const UcHybridModelBatch *,UcHybridModelLightView *);
int uc_hybrid_models_prepare_spot(const UcHybridLightingScene *,const UcHybridLightingRecord *,
    const UcHybridModelBatch *,UcHybridModelLightView *);
/* Borrowed for the synchronous callback only. The reference publishes posed
 * triangle corners; the indexed candidate publishes unique unposed vertices,
 * ordered list/strip indices and the current palette. World, projection and
 * material policy remain explicit. No guest pointers are retained by a host.
 * Whole-light replacement keeps its existing rejection policy; the new original
 * material hook instead returns rejection to that same original material draw. */
void uc_hybrid_models_light(const UcHybridLightingScene *,const UcHybridLightingRecord *,
    uint32_t iterator_va,uint32_t context_va);
void uc_hybrid_models_visitor_begin(void);
typedef struct UcHybridModelPass {
    uint32_t partition;uint16_t light_index;
    uint8_t filter_light,require_draw_flag,filter_material,light_material;
    /* Source material hook: exactly this batch, after original BABB0. Do not
     * replay palette updates or inspect unrelated batches in this mode. */
    uint32_t single_batch,batch_index,node_va;
    uint32_t parent_clip; /* Base hook uses the original uploaded clip rows. */
} UcHybridModelPass;
typedef int (*UcHybridModelBatchVisitor)(void *,const UcHybridModelBatch *);
/* Exact shared live geometry/palette frontier. Admission and source mode-specific
 * batch predicates belong to the caller; all palette updates precede material
 * partition selection. No visibility or light-membership inference is made. */
int uc_hybrid_models_geometry(const UcHybridLightingScene *,uint32_t receiver,
    const UcHybridModelPass *,UCModelRenderPalette *,UcHybridModelBatchVisitor,void *);
int uc_hybrid_models_batch(const UcHybridLightingScene *,const UcHybridLightingRecord *,
    uint32_t receiver,uint32_t node,uint32_t source_batch,uint32_t partition);
int uc_hybrid_models_original_batch(uint32_t light,uint32_t receiver,uint32_t node,
    uint32_t source_batch,uint32_t partition,uint32_t modifier);
int uc_hybrid_models_host_palette(UCModelRenderPalette *);
int uc_hybrid_models_host_draw(const UcHybridLightingScene *,const UcHybridLightingRecord *,const UcHybridModelBatch *);
/* Qualified static/posed base submission after the original material has bound state. */
void uc_hybrid_models_base_begin(uint32_t receiver,uint32_t node,uint32_t batch,uint32_t modifier);
void uc_hybrid_models_base_end(void);
int uc_hybrid_models_base_submit(uint32_t batch_va,uint32_t material_va,uint32_t pass);
int uc_hybrid_models_host_base(const UcHybridModelBatch *);
void uc_hybrid_models_read_counts(uint64_t *,unsigned);
#ifdef __cplusplus
}
#endif
#endif
