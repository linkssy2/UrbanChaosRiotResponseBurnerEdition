#ifndef UC_NATIVE_COLLISION_ASSET_H
#define UC_NATIVE_COLLISION_ASSET_H
#include <stddef.h>
#include <stdint.h>

typedef struct UCCollisionTriangle {
    uint32_t vertex[3], material_index, material, subpart, local_triangle;
} UCCollisionTriangle;
typedef struct UCCollisionSubpart {
    uint32_t first_vertex, vertex_count, first_triangle, triangle_count;
} UCCollisionSubpart;
typedef struct UCCollisionAsset {
    float (*vertices)[3];
    UCCollisionTriangle *triangles;
    UCCollisionSubpart *subparts;
    uint32_t *sector_object_counts;
    uint8_t *mopp_bytes;
    uint32_t vertex_count, triangle_count, subpart_count, sector_count, mopp_size;
    uint32_t subpart_index_bits, shape_user_data, mesh_user_data;
    float scaling[4], radius, mopp_offset[4];
    uint8_t disable_welding;
    /* Native conservative bounds over32 consecutive triangles. Loaded mesh
     * vertices/indices are immutable; query order and exact tests stay intact. */
    double (*query_bounds)[6];
    float query_scaling[4];
} UCCollisionAsset;
typedef enum UCCollisionStatus {
    UC_COLLISION_OK, UC_COLLISION_ARGUMENT, UC_COLLISION_FORMAT,
    UC_COLLISION_UNSUPPORTED, UC_COLLISION_LIMIT, UC_COLLISION_MEMORY
} UCCollisionStatus;

/* Serialized prop collision shapes from the SAME2282C0 archive handler.
 * family selects its original object list (0/1/2); metadata preserves each
 * record's36/12/6-byte suffix without inventing gameplay meaning. Transform,
 * radius and dimensions are the packfile values BEFORE2A7780/2A3E50 runtime
 * conversion. kind1=box,2=sphere,3=capsule; dimensions are halfExtents/vertices.
 * This is asset ownership, not rigid-body construction or a physics solver. */
typedef struct UCModelCollisionShape {
    uint32_t group,object,family,kind,root_user_data,child_user_data;
    float transform[16],rotation[4],radius,dimensions[8];
    uint8_t metadata[36];
    uint32_t metadata_size;
    uint32_t root_kind; /* Original serialized ROOT: box7,sphere4,capsule8,transform25. */
} UCModelCollisionShape;
typedef struct UCModelCollisionAsset {
    UCModelCollisionShape *shapes;
    uint32_t *group_object_counts;
    uint32_t shape_count,group_count;
} UCModelCollisionAsset;

/* Reads the decompressed .geohk member. Owns exact original mesh triangle order,
 * material identifiers, scale/radius and MOPP bytes; does not invent colliders
 * from rendering meshes or implement movement/contact queries. Source arrays
 * are float3, uint16 triangle indices and uint16 material indices in this slice.
 * Packfile v2, 32-bit little-endian layout only. Unsupported layouts fail.
 * Output must be zero-initialized. Failure leaves it empty; input is borrowed
 * only during the call. Max 256 MiB input/output, 1M vertices / 2M triangles. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_load(const void *data, size_t size, UCCollisionAsset *out);
void uc_collision_free(UCCollisionAsset *asset);
/* .mdlhk: is_textured=0; .mtxthk:1. Zeroed output, exact EOF, bounded owned
 * shapes; unsupported graph/class/prefix fails transactionally. */
UCCollisionStatus uc_model_collision_load(const void *data,size_t size,int is_textured,UCModelCollisionAsset *out);
void uc_model_collision_free(UCModelCollisionAsset *asset);
const char *uc_collision_status(UCCollisionStatus status);
#ifdef __cplusplus
}
#endif
#endif
