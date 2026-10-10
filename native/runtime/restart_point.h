#pragma once
#include "mission_world.h"
#include <map>
extern "C" {
#include "../checkpoint.h"
}
/* Required real provider registration, player/objective queries and save owners.
 * operations.event must route to this SAME world's event(source,group).
 * Registration stores the generation-bearing entity ID, not this object's
 * address. No save/session defaults or successful missing service fallbacks. */
struct NativeRestartPointServices {
    UCCheckpointOperation operations{};
    int (*register_point)(void *,int32_t number,int32_t id)=nullptr;
    int (*close_helper)(void *,void *)=nullptr;
};
class NativeRestartPoint {
    NativeMissionWorld *world_=nullptr;
    NativeRestartPointServices services_{};
    NativeCoroutine primary_;
    UCEntitySubscriptions *subscriptions_=nullptr;
    UCCheckpoint checkpoint_{};
    uint8_t pending_=0,parked_=0;
    int failure_=0;
    static void run(void *,NativeCoroutine &);
    static int event(void *,uint32_t,uint32_t);
    static int poll(void *,uint32_t);
public:
    NativeMissionEntity entity;
    int construct(NativeMissionWorld &,uint32_t,const NativeRestartPointServices &);
    int update();
    int late();
    int close_detached();
    const UCCheckpoint &state()const{return checkpoint_;}
    uint8_t pending()const{return pending_;}
    int failure()const{return failure_;}
};
/*66020/660A0/66180/663F0: signed checkpoint numbers, first registration wins.
 * IDs are resolved through the same live world on every query. A stale ID is
 * an error, not an absent checkpoint. Session mode is the actual session+74;
 * nullptr means no session manager. This owns no player, save or flag snapshot. */
class NativeCheckpointManager {
    std::map<int32_t,int32_t> points_;
public:
    int32_t selected=-1;
    void register_point(int32_t number,int32_t id){points_.emplace(number,id);}
    int32_t minimum()const{return points_.empty()?-1:points_.begin()->first;}
    int32_t current(const uint32_t *session_mode)const{return session_mode&&!*session_mode&&selected==-1?minimum():selected;}
    const NativeRestartPoint *find(NativeMissionWorld &,int32_t number)const;
    bool ready(NativeMissionWorld &,const uint32_t *session_mode)const;
    bool auto_fade(NativeMissionWorld &,const uint32_t *session_mode)const;
    bool transform(NativeMissionWorld &,const uint32_t *session_mode,float out[16])const;
};
