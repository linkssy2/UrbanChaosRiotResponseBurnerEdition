#ifndef UC_NATIVE_COLLISION_RIGID_UPDATE_H
#define UC_NATIVE_COLLISION_RIGID_UPDATE_H
#include "collision_phantom_world.h"
#include "collision_phantom_pairs.h"
typedef struct UCCollisionRigidUpdateMember {const void *world;const UCCollisionSAPHandle *handle;} UCCollisionRigidUpdateMember;
typedef struct UCCollisionRigidUpdateAgents {const UCCollisionPairAgent *data;uint32_t count;} UCCollisionRigidUpdateAgents;
typedef struct UCCollisionRigidUpdateHooks {
 UCCollisionStatus (*preflight)(void *,void *body,uint32_t mode,uint32_t child_mode);
 UCCollisionRigidUpdateMember (*member)(void *,void *body);
 const UCCollisionPhantomPairBody *(*selected)(void *,void *body);
 void (*query)(void *,void *body,UCCollisionSAPPairs *);
 const UCCollisionPhantomPairBody *(*other)(void *,const UCCollisionSAPHandle *);
 const void *(*filter)(void *);
 void (*phantom_refresh)(void *,const void *filter,const UCCollisionPhantomPairBody *phantom,const UCCollisionPhantomPairBody *rigid);
 void (*phantom_child)(void *,const void *phantom);
 UCCollisionRigidUpdateAgents (*agents)(void *,void *body);
 UCCollisionSAPPair (*agent_pair)(void *,const void *agent);
 void (*reconcile)(void *,UCCollisionSAPPairs *existing,UCCollisionSAPPairs *queried);
 int (*enabled)(void *,const void *filter,const void *selected,const void *other);
 uint16_t (*category)(void *,const void *collidable);
 uint8_t (*matrix)(void *,uint16_t selected,uint16_t other);
 void (*remove_agent)(void *,const void *agent);
 void (*dirty_island)(void *,void *body);
 void (*update_agent)(void *,const void *agent);
 void (*add_pairs)(void *,const void *captured_filter,const UCCollisionSAPPairs *);
 UCCollisionStatus (*enqueue)(void *,void *body,uint8_t mode,uint8_t child_mode);
 UCCollisionStatus (*drain)(void *);
} UCCollisionRigidUpdateHooks;
typedef struct UCCollisionRigidUpdate {
 const void *world;void *context;UCCollisionPhantomGates gates;
 UCCollisionRigidUpdateHooks hooks;UCCollisionSAPPairs queried,existing;uint8_t active;
} UCCollisionRigidUpdate;
/* Full2EEFF0 controller with explicit completing subsystem services. Immediate
 * admission requires registered class1 selected owner and its attached handle.
 * Primary busy alone defers opcode18 with BOTH mode LOW BYTES and a retained
 * body, then wraps pending_requests. Secondary phantom_busy is unused.
 * All immediate mode words are preserved. mode0 queries the SAME world SAP;
 * other modes skip discovery/reconciliation but still visit existing agents.
 * Query publishes complete ordered selected-first pairs. Each class2 nonnull
 * owner is refreshed as (phantom,rigid), child-called for child_mode!=0, then
 * removed by tail swap and index retry. Null phantom owners remain in output.
 * Existing agent pairs are captured in live-list order then actual3191A0
 * reconciles both arrays; its first remainder is never a destruction list.
 * The selected16-bit dispatch category is captured once. Each live agent gets
 * a fresh filter check and, if accepted, matrix(selectedCategory,otherCategory).
 * Rejected agents execute actual2FE190, then mark the body's CURRENT island
 * dirty, then retry the moved entry. Kept agents update only for child_mode==1.
 * Remaining pairs dispatch through actual319050 with ONE captured filter.
 * Category is source collidable+1A, not shape_type; matrix is actual+19D4.
 *
 * Every hook is required. Accessors/filter/enabled/category/matrix are pure;
 * selected metadata, identities and each captured entry remain valid/stable
 * through its immediate service. Agents getter reloads live storage/count.
 * Preflight admits bounded capacities, lifetime/reference headroom and complete
 * actual services BEFORE effects. query/reconcile and all effect hooks cannot
 * fail after admission; errors must never be silently swallowed. Counts fit
 * scratch capacities<=32768; query and agent arrays do not alias. No fabricated
 * query/contact/agent/island success, controller-local queue or filter owner.
 * Effects may publish a filter, change agent lists/island ownership, and request
 * deferred operations, but may not mutate scratch/gates/descriptor or invalidate
 * pending identities. Removal must actually compact the live agent owner.
 * No concurrent use; world/body/services survive the call and queued dispatch.
 * active is cleared before tail drain. applied=1 marks committed effects even
 * if drain then fails: retry drain only. Immediate tail drain tests primary
 * busy, pending and blocked; implements2ED490 counter reset through the hook.
 * Rejections before effects preserve applied. Init requires zeroed out; owns
 * only the two scratch allocations. Dispose only after all queued requests are
 * quiesced; queue body references do not retain this borrowed dispatcher. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_rigid_update_init(UCCollisionRigidUpdate *,const void *world,
 const UCCollisionPhantomGates *,const UCCollisionRigidUpdateHooks *,void *context,
 uint32_t pair_capacity,uint32_t agent_capacity);
UCCollisionStatus uc_collision_rigid_update_request(UCCollisionRigidUpdate *,void *body,
 uint32_t mode,uint32_t child_mode,int *applied);
UCCollisionStatus uc_collision_rigid_update_dispose(UCCollisionRigidUpdate *);
#ifdef __cplusplus
}
#endif
#endif
