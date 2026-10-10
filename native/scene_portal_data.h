#ifndef UC_SCENE_PORTAL_DATA_H
#define UC_SCENE_PORTAL_DATA_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCScenePortalData {
    uint32_t group;
    int32_t vertex_count;
    const float (*vertices)[3];
    float normal[3];
} UCScenePortalData;
typedef struct UCScenePortalMemory {
    void *owner;
    void *(*allocate)(void *,size_t);
    void (*release)(void *,void *);
} UCScenePortalMemory;
/* Actual B6710 normal and B6880/B6910 directed vertex containment/grouping.
 * These operate on temporary portal records constructed by B70C0; they do not
 * implement persistent B69C0 room/portal topology or camera visibility.
 * Normal uses vertices0,1,2; it does not average or reverse winding. Contains
 * requires EVERY first vertex to have ANY second vertex strictly closer than
 * source float0.05. This is directed and permits reused matching vertices.
 * Group compares in source index order, skipping previously marked rows, and
 * replaces each matching later row.group with the current unmarked row INDEX.
 * Untouched groups retain their original values. No sorting/dedup deletion.
 * RN, normal denormals, finite accessible immutable coordinates, vertex counts
 * <=4096 (signed nonpositive means empty), <=4096 portals. Normal additionally
 * requires >=3 vertices and a finite nondegenerate computed cross product.
 * Completing memory callbacks cannot mutate/reenter the input arrays. Group
 * allocates exactly count scratch bytes, including size0, and releases once;
 * NULL allocation is permitted only for count0. Native positive-size failure
 * returns2 before group writes, not an original OOM/exception claim.
 * Output/allocator storage must not alias input fields/coordinates. Status0
 * complete,1 invalid before effects,2 scratch allocation failed. */
int uc_scene_portal_normal(UCScenePortalData *);
int uc_scene_portal_contains(const UCScenePortalData *,const UCScenePortalData *,uint8_t *);
int uc_scene_portal_group(UCScenePortalData *,uint32_t count,const UCScenePortalMemory *);
#ifdef __cplusplus
}
#endif
#endif
