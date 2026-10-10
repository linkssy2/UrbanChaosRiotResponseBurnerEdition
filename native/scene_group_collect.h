#ifndef UC_SCENE_GROUP_COLLECT_H
#define UC_SCENE_GROUP_COLLECT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneGroupTreeNode {
    struct UCSceneGroupTreeNode *left,*parent,*right;
    void *target;
    uint8_t nil;
} UCSceneGroupTreeNode;
typedef struct UCSceneGroupListNode {
    struct UCSceneGroupListNode *next,*previous;
    void *target;
} UCSceneGroupListNode;
typedef struct UCSceneGroup {
    UCSceneGroupTreeNode *tree;
    UCSceneGroupListNode *list;
} UCSceneGroup;
/* Actual95160/951D0, using the original1F9320 successor. These are borrowed
 * current group+88/+74 graph fields, not flattened copies. Source tree NULL
 * payloads are skipped; list payloads must be nonnull. Header is a nil tree
 * node, with left=min/right=max/parent=root; empty links point to itself.
 * Valid acyclic trees/lists, at most4096 nodes each, and successful completing
 * callbacks are the admitted domain. All pointers refer to accessible objects.
 * Callback receives actual virtual offset40 or44 and both unchanged arguments.
 * It may change later payloads, or replace a graph with a valid bounded graph,
 * provided the current node/links remain alive through AFTER-callback advance,
 * successor traversal terminates at the CURRENT header, and remaining list
 * payloads stay nonnull. No concurrent/reentrant collection of the same group.
 * Allocation, tree/list insertion/removal, model visibility/culling and actual
 * record publication belong to their concrete owners; dispatch must implement
 * those effects, never a claimed-success observer. No palette setup is added.
 * Returns0 completed,1 invalid initial graph/arguments before callback effects.
 * The validator is pure. Callback mutations must preserve the domain; there is
 * no rollback or late validation of a completing callback's effects. */
typedef void (*UCSceneGroupCollect)(void *owner,void *target,uint32_t slot,
                                   void *collector,void *manager);
int uc_scene_group_validate(const UCSceneGroup *);
int uc_scene_group_collect(UCSceneGroup *,uint32_t slot,void *collector,
                           void *manager,UCSceneGroupCollect,void *owner);
#ifdef __cplusplus
}
#endif
#endif
