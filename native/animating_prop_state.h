#ifndef UC_ANIMATING_PROP_STATE_H
#define UC_ANIMATING_PROP_STATE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAnimatingPropCondition {int32_t count;const int32_t *flags;} UCAnimatingPropCondition;
typedef struct UCAnimatingPropState {
    int32_t current,desired;
    uint8_t finished,return_to_idle,loop[9],interrupt[9],restart_audio,stop_previous;
    int32_t start_event[9],finish_event[9];
    UCAnimatingPropCondition conditions[9];
    uint32_t *entity_flags;const uint8_t *disabled;
    void *const *model;void *bank;const char *sound_name;float volume;
} UCAnimatingPropState;
typedef struct UCAnimatingPropStateServices {
    void *owner;
    unsigned (*done)(void *);unsigned (*has_animation)(void *,int32_t);
    int32_t (*model_groups)(void *,void *model);
    int (*apply)(void *,int32_t);
    int (*event)(void *,int32_t,uint8_t grouped);
    int (*stop)(void *,void *bank,const char *);
    int (*play)(void *,void *bank,const char *);
    int (*parameter)(void *,void *bank,const char *,float);
    /* Exact1EB930..1EB96A completion, required only when the real model has
     * that group. NULL returns2 before this missing branch, not success. */
    int (*group)(void *,int32_t);
} UCAnimatingPropStateServices;
/* Whole1EB690 plus1EB760 for actual early-finish2F9=0 and reset-model2F8=0.
 * Current/desired0..8 (actual assets have no Setup), stable borrowed pointers,
 * <=64 indexes per condition, flags supplied from the SAME level owner.
 * Source callbacks are completing effects, including action sound and2175A0
 * event dispatch. They may mutate represented state/live field contents, but
 * keep valid indexes, strings, aliases and metadata until return. No reentry.
 * Model/disabled/flags are actual shared slots, not copied entity state.
 * Missing animation still retains source current publication. Group callbacks
 * may be omitted only when actual group_count<=selected index. No generic
 * physics/body, bank/helper construction or actor scheduling is supplied.
 * Returns0 completed,1 invalid before effects,2 incomplete committed prefix.
 */
int uc_animating_prop_state_update(UCAnimatingPropState *,const uint8_t *flags,
    size_t flag_count,const UCAnimatingPropStateServices *);
/* Whole1EB690 only. Publishes desired even while action effects are pending. */
int uc_animating_prop_select(UCAnimatingPropState *,const uint8_t *,size_t);
const char *uc_animating_prop_action(int32_t);
#ifdef __cplusplus
}
#endif
#endif
