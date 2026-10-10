#ifndef UC_POINT_LIGHT_DRAW_H
#define UC_POINT_LIGHT_DRAW_H
#include "model_light_membership.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the actual model membership, visibility subject(+B4), and asset+4C.
 * One stable descriptor per model. No second scene/model graph is constructed. */
typedef struct UCPointLightDrawModel {
    const UCModelLightMembership *membership;
    const void *visibility_subject;
    uint8_t asset_visible;
} UCPointLightDrawModel;
/* Caller owns this temporary unique pointer-ordered light set. Storage and
 * borrowed identities remain stable/alive until the complete pass finishes. */
typedef struct UCPointLightSelection {
    const UCModelMembershipLight **items;
    uint32_t count,capacity;
} UCPointLightSelection;
/* Actual B9F40. check_visibility consumes its LOW BYTE. Required visibility
 * service returns the actual graphics+C8/+C result; no default visible stub.
 * Existing set sorted/unique; all inputs immutable and disjoint, no reentry.
 * Capacity preflight covers even a model subsequently rejected by visibility.
 * Returns0 native rejection without effects,1 completed (possibly unchanged). */
int uc_point_light_collect(UCPointLightSelection *,const UCPointLightDrawModel *,
    uint32_t check_visibility,void *context,int (*visible)(void *,const void *));
/* Fields actually consumed from each original 20-byte draw record. Retain
 * original order and duplicates. partition is an opaque native identity. */
typedef struct UCPointLightDrawRecord {
    const UCPointLightDrawModel *model;
    const void *partition;
    uint16_t flags;
} UCPointLightDrawRecord;
typedef struct UCPointLightDrawCursor {
    const UCPointLightDrawRecord *records;
    uint32_t count,current;
    uint16_t mask,expected;
} UCPointLightDrawCursor;
/* Full96860 reset/filter semantics; count is the native end sentinel. */
int uc_point_light_draw_reset(UCPointLightDrawCursor *);
typedef struct UCPointLightDrawPass {
    uint32_t mode;
    const void *partition;
    const UCModelMembershipLight *light;
} UCPointLightDrawPass;
typedef struct UCPointLightDrawHooks {
    void (*manager_begin)(void *);
    void (*render_state)(void *,uint32_t address,uint32_t value);
    int (*light_begin)(void *,const UCModelMembershipLight *); /* LOW BYTE ready. */
    void (*receiver)(void *,const UCPointLightDrawModel *,const UCPointLightDrawPass *);
    void (*light_end)(void *,const UCModelMembershipLight *);
    void (*manager_end)(void *);
} UCPointLightDrawHooks;
/* Full98DC0 control for passes where no shadow-caster branch can execute:
 * shadow_request LOW BYTE is0, or suppress_shadows!=0, or every light shadows0.
 * Ready setup/epilogue B31C0/B29A0, manager A2B20/A2BF0 and actual receiver
 * rendering are required completing effects. No default success or fake draw.
 * Native selection replaces original tree allocations/unsigned address order.
 * All descriptors/memberships/records/state/flags/hooks must remain immutable
 * through callbacks; pass/cursor/counter only this call may write. Stable
 * already-preflighted geometry/services and non-reentrancy are caller duties.
 * These explicit services may publish their OWN GPU/provider/light fields.
 * Whole-call admission precedes effects; callbacks cannot fail. No rollback or
 * full-world culling/record construction claim. disabled suppresses diffuse
 * draws but preserves ready/end/counter effects. Counter adds modulo2^32.
 * Returns0 native rejection,1 completed. An initially ended cursor does nothing
 * and does not inspect later arguments, just as original98DC0. */
int uc_point_light_draw(UCPointLightDrawCursor *,const UCPointLightSelection *,
    UCPointLightDrawPass *,uint32_t *counter,uint32_t shadow_request,
    uint8_t suppress_shadows,uint8_t disabled,const UCPointLightDrawHooks *,void *);
#ifdef __cplusplus
}
#endif
#endif
