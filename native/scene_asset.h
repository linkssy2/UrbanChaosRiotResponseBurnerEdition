#ifndef UC_NATIVE_SCENE_ASSET_H
#define UC_NATIVE_SCENE_ASSET_H
#include <stddef.h>
#include <stdint.h>

/* Owned static GEO meshes from original assets, independent of any Xbox runtime.
 * Positions are world-space for this admitted static layout (original identity
 * instance/palette). Source packed channels are retained without guessing their
 * decoding. UVs and position bytes are unchanged. */
typedef struct UCSceneVertex {
    float position[3], uv[2];
    uint32_t source_color, packed_normal;
    float uv1[2];
    uint32_t packed_basis[2];
} UCSceneVertex;
typedef struct UCSceneBatch {
    uint32_t first_index, index_count, material_index, sector_index, node_flags;
    uint16_t light_index; /* 0xffff = base pass; otherwise original light pass. */
    uint8_t flags[2]; /* Exact source descriptor bytes 15 and 16. */
    float local_to_world[16];
} UCSceneBatch;
typedef struct UCScene {
    UCSceneVertex *vertices;
    uint32_t *indices; /* Triangle list; strip winding and degenerates retained. */
    UCSceneBatch *batches;
    char **material_names; /* Exact archive byte names with appended NUL. */
    uint32_t vertex_count, index_count, batch_count, material_count;
    uint32_t model_count, sector_count, light_count, room_count, portal_count;
    size_t geometry_end; /* Remaining room/portal structure is validated, not used. */
    float bounds_min[3], bounds_max[3];
} UCScene;
typedef enum UCSceneStatus {
    UC_SCENE_OK, UC_SCENE_ARGUMENT, UC_SCENE_FORMAT, UC_SCENE_UNSUPPORTED,
    UC_SCENE_LIMIT, UC_SCENE_MEMORY
} UCSceneStatus;

/* Input is the decompressed embedded .geo entry, not its outer archive. Output
 * must be zero-initialized. Success owns all memory; failure leaves it empty.
 * Scope: one-group/one-node static AMF models with one identity palette matrix,
 * ordinary float vertices and no children/skin/shared streams/aux resource.
 * Auxiliary 20-byte descriptor arrays and visibility records are span-validated
 * but not interpreted or exposed. Other model layouts return UNSUPPORTED.
 * 256 MiB input and total native allocations, <=1M vertices / <=8M indices. */
UCSceneStatus uc_scene_load(const void *data, size_t size, UCScene *out);
void uc_scene_free(UCScene *scene);
const char *uc_scene_status(UCSceneStatus status);
#endif
