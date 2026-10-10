#ifndef UC_SCENE_MODEL_REGISTRATION_H
#define UC_SCENE_MODEL_REGISTRATION_H
#include "scene_group_collect.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneModelNode {
    UCSceneGroupListNode link;
    uint8_t owned; /* source list payload's low byte at node+C */
} UCSceneModelNode;
typedef struct UCSceneModelMemory {
    void *owner;
    UCSceneModelNode *(*allocate)(void *owner);
    void (*release)(void *owner,UCSceneModelNode *);
} UCSceneModelMemory;
typedef struct UCSceneModelGroup {
    UCSceneGroup graph;
    UCSceneGroupTreeNode empty_tree;
    uint32_t count;
    UCSceneModelMemory memory;
    uint8_t active;
} UCSceneModelGroup;
/* Concrete50920 empty circular list,958B0/11E4F0 append and95090 remove ALL
 * pointer matches. The group owns its heap list nodes; payloads are borrowed.
 * Its tree remains the actual empty tree; no map insertion/ordering is implied.
 * Borrow &group.graph for95160 collection: the list is the SAME live storage.
 * Source count limit is wider; native domain limits live nodes to4096.
 * Completing allocator/free hooks may inspect publication and affect external
 * owners, but may not mutate this group or free borrowed targets. Allocation
 * must succeed in the original admitted domain. Outputs and service storage
 * must not alias the group or other live inputs. Native allocation failure is
 * status2 with no new list publication, not an emulated source NULL outcome.
 * Begin requires a zero-initialized object. End requires an empty active list;
 * target ownership/destructors and nonempty8B970 teardown are not claimed.
 * Status0 success,1 invalid/pre-effect,2 allocation failure. NULL/duplicate
 * payloads are legal storage; NULL list payloads cannot be collected by95160. */
int uc_scene_model_group_begin(UCSceneModelGroup *,const UCSceneModelMemory *);
int uc_scene_model_group_append(UCSceneModelGroup *,void *target,uint8_t owned);
int uc_scene_model_group_remove(UCSceneModelGroup *,const void *target,uint32_t *removed);
int uc_scene_model_group_end(UCSceneModelGroup *);

typedef struct UCSceneModelCreateHooks {
    int (*preflight)(void *owner,const void *resource,void *initialization,const char *name);
    void *(*create)(void *owner,void *factory,const void *resource);
    void (*initialize)(void *owner,void *model,void *initialization);
    UCSceneModelGroup *(*current_primary)(void *owner);
    void (*name)(void *owner,void *model,const char *name);
} UCSceneModelCreateHooks;
/* Whole9EE00 ordering. factory aliases graphics+60 and is captured once after
 * pure preflight. Actual91200 create, modelvirtual4 initialization andvirtual1C
 * naming must complete through concrete services. Primary group is obtained
 * CURRENT after initialization, as graphics+DC. Real append uses owned0.
 * NULL create is a genuine completed source outcome, with no later callbacks.
 * NULL name uses the actual44D9B8 empty-string value. An explicit name must stay
 * alive until callback; service mutations retain model and factory lifetimes.
 * preflight guarantees capacity and completing services before any creation;
 * status2 records a violated completion/allocator contract after effects and
 * *model exposes the created borrowed object for caller cleanup, never retry.
 * No generic graphics factory/model constructor implementation is implied. */
int uc_scene_model_create(void *const *factory,const void *resource,
                          void *initialization,const char *name,
                          const UCSceneModelCreateHooks *,void *owner,void **model);
#ifdef __cplusplus
}
#endif
#endif
