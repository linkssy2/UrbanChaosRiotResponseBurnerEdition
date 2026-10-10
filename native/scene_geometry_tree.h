#ifndef UC_SCENE_GEOMETRY_TREE_H
#define UC_SCENE_GEOMETRY_TREE_H
#include "scene_group_collect.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneGeometryNode {
    UCSceneGroupTreeNode link;
    uint32_t key;
    uint8_t color; /* original0 red /1 black */
} UCSceneGeometryNode;
typedef struct UCSceneGeometryMemory {
    void *owner;
    UCSceneGeometryNode *(*allocate)(void *);
    void (*release)(void *,UCSceneGeometryNode *);
} UCSceneGeometryMemory;
typedef struct UCSceneGeometryTree {
    UCSceneGroupTreeNode *header;
    uint32_t count;
    UCSceneGeometryMemory memory;
    uint8_t active;
} UCSceneGeometryTree;
/* The real primary group+84 unsigned-key tree, not the ordinary model LIST.
 * begin owns a heap header; set is95550/95490/95240, including red/black shape,
 * allocation-before-count publication and replacement of existing payloads.
 * find is95070/1F9400 for PRESENT keys only. A missing key is rejected
 * with unchanged output: original95070 reads the uninitialized header payload. Borrow header directly as UCSceneGroup.tree for
 * existing95160 collection; no second tree or flattened record list exists.
 * clear is the full-range8ABF0/8A1E0 branch; end additionally releases header
 * with the source8B970 field publication order. Borrowed targets are NEVER
 * destroyed. Individual erase/nonempty whole-group destruction is not owned.
 * Begin requires inactive/NULL header/count0, initially zero or after end.
 * Its fields/nodes remain read-only to callers; output pointers must not alias
 * tree, node or service storage.
 * Completing allocator/free services may inspect header/count and affect
 * external state, but must not mutate/reenter this tree. During clear the
 * source graph retains dangling links until final reset: release must not
 * traverse those links. All borrowed collectors must finish before clear/end.
 * Successful source allocation domain, bounded4096 live distinct keys.
 * Native allocation failure returns2 before tree publication; this does not
 * claim source NULL allocation is safe. Status1 invalid/pre-effect,0 complete.
 * Header key/target are deterministic native extras not observed in source. */
int uc_scene_geometry_tree_begin(UCSceneGeometryTree *,const UCSceneGeometryMemory *);
int uc_scene_geometry_tree_set(UCSceneGeometryTree *,uint32_t key,void *target);
int uc_scene_geometry_tree_find(const UCSceneGeometryTree *,uint32_t key,void **target);
int uc_scene_geometry_tree_clear(UCSceneGeometryTree *);
int uc_scene_geometry_tree_end(UCSceneGeometryTree *);
#ifdef __cplusplus
}
#endif
#endif
