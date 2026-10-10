#pragma once
#include "mission.h"
#include <memory>
class NativeMissionWorld;
extern "C" {
#include "../solid_block.h"
}
/* Known authored SolidCollisionBlock. Reuses the existing enclosing actor and
 * script owner; its body is a real stationary PC-query allocation. This is an
 * explicit PC collision adaptation, not Havok island/solver/notification parity.
 * The shared mission world owns registration, slots and post-constructor flags. */
class NativeSolidBlock {
    struct Body {UCSolidBlockGeometry geometry{};bool active=false;};
    std::unique_ptr<Body> body_;
    UCSolidBlock state_{};
    const UCEntityFreshContext *context_=nullptr;
    const UCEntityAuthored *const *active_=nullptr;
    const float *radius_=nullptr;
    int failure_=0;
    NativeMissionWorld *world_=nullptr;
    static int base(void *);
    static int prepare(void *,UCSolidBlock *);
    static int add(void *,void *);
    static int remove(void *,void *);
    static int suffix(void *);
public:
    NativeMissionEntity entity;
    NativeSolidBlock()=default;
    NativeSolidBlock(const NativeSolidBlock&)=delete;
    NativeSolidBlock&operator=(const NativeSolidBlock&)=delete;
    /* Bind entity.view in SAME registry before calling. Actual class, absent
     * asset/model and stable borrowed metadata/context/radius required. radius
     * is read when state0 creates its body (source505AC0 initially .05 metres).
     *0 complete,1 pure rejection,2 committed failure. No rollback/retry on2. */
    int construct(const UCEntityFreshContext &,const UCEntityAuthored *const *active,
        const UCEntityAuthored &,const NativeMissionServices &,const float *radius,NativeMissionWorld *world=nullptr);
    int retire();
    int update();
    int late();
    /* Native shutdown AFTER shared records/live/slot detach. Completes real
     * command/coroutine, PC body and aux lifetimes; no generic destructor claim. */
    int close_detached();
    const UCSolidBlockGeometry *geometry()const{return body_&&body_->active?&body_->geometry:nullptr;}
    const UCSolidBlock &state()const{return state_;}
    int failure()const{return failure_;}
};
