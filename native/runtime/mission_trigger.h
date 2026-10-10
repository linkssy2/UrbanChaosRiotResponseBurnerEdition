#pragma once
#include "mission.h"
extern "C" {
#include "../mission_trigger_actor.h"
}
/* Direct211820 custom-volume class specialization. Same enclosing actor,
 * registration/level slot and metadata; no registered TriggerRegion substitute.
 * Caller owns the active descriptor/context and all borrowed owners. */
class NativeMissionTrigger {
    const UCEntityFreshContext *context_=nullptr;
    const UCEntityFreshView *view_=nullptr;
    UCLevelTrigger *trigger_=nullptr;
    UCEntityAuthored metadata_{};
    const UCEntityAuthored **active_=nullptr;
    const uint8_t *property_cursor_=nullptr;
    UCMissionTriggerActor actor_{};
    NativeMissionController mission_;
    UCMissionTriggerWorld world_{};
    uint32_t *seed_=nullptr;
    int failure_=0;
    static int base(void *);
    static int suffix(void *);
public:
    NativeMissionTrigger()=default;
    NativeMissionTrigger(const NativeMissionTrigger&)=delete;
    NativeMissionTrigger&operator=(const NativeMissionTrigger&)=delete;
    /* Exact source custom slot gate:40 set,8/10 clear. Caller preadmits capacity
     * in the SAME registration/list/record owners. FreshView starts auxNULL.
     * Construction copies source-unwritten custom metadata basis as retained by
     * the native loader (deterministically zero);2112D0 publishes real position.
     * No geometry/rendering meaning is assigned to that unspecified basis.
     * Full slot/global startup iteration remains the caller's responsibility. */
    int construct(const UCEntityFreshContext &,const UCEntityAuthored **active,
        const UCEntityFreshView &,
        UCLevelTrigger &,int32_t source_index,UCLevelEntitySlot &,
        UCMissionCondition *,uint32_t condition_count,
        const UCMissionTriggerWorld &,const NativeMissionServices &);
    int update();
    /* Explicit native shutdown after caller removes entity/update registration.
     * Disposes this owned row allocation and concrete script continuation only;
     * enclosing FreshView/aux/entity allocation remain the caller's ownership.
     * This is not a claim of full218B30/EB4D0 game shutdown equivalence. */
    int close_detached();
    const UCMissionTriggerActor &state()const{return actor_;}
    int failure()const{return failure_;}
};
