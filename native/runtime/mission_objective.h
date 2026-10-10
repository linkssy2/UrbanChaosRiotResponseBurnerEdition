#pragma once
#include "mission_world.h"
#include <map>
#include <vector>

// World+924 status records: 60480 inserts two false bytes and reports whether
// the record already existed. Completion60350 requires that existing record.
// Current/saved bytes remain distinct; native saves export the captured byte.
class NativeObjectiveSavedStatus {
    std::map<int32_t,std::pair<bool,bool>> records_;
public:
    bool ensure(int32_t source);
    bool completed(int32_t source)const;
    int complete(int32_t source);
    void capture(); //60410 current completion byte -> saved byte
    void restore(); //603A0 saved byte -> current completion byte
    std::vector<int32_t> saved_completed()const;
    void load_saved(const std::vector<int32_t> &); // Before the world's first update.
    size_t size()const{return records_.size();}
};
struct NativeObjectiveServices {
    void *owner=nullptr;
    // Actual current provider4A840 virtual2C (checkpoint ready).
    int (*player_ready)(void *,bool *)=nullptr;
    //73060(type,title,text,0,-1). Must enqueue the actual HUD notification.
    int (*message)(void *,int32_t type,const char *title,const char *text)=nullptr;
};
// Actual E87E0/E8A10/E8A50 owner, including the permanently parked coroutine
// after completion/failure. Flags/timer remain live while suspended. Caller
// binds this entity before construction and detaches it before close.
class NativeMissionObjective {
    NativeMissionWorld *world_=nullptr;
    NativeObjectiveSavedStatus *saved_=nullptr;
    NativeObjectiveServices services_{};
    NativeCoroutine coroutine_;
    std::vector<int32_t> activate_,fail_;
    const char *name_="",*secondary_="";
    float timer_=0;
    int32_t status_=0;
    bool primary_=false,done_=false,secondary_shown_=false,named_=false;
    int failure_=0;
    static void run(void *,NativeCoroutine &);
    bool message(int32_t,const char *,const char *);
public:
    NativeMissionEntity entity;
    int construct(NativeMissionWorld &,uint32_t source,NativeObjectiveSavedStatus &,const NativeObjectiveServices &);
    int update();int late();int close_detached();
    int32_t status()const{return status_;}
    bool primary()const{return primary_;}
    bool done()const{return done_;}
    bool secondary_shown()const{return secondary_shown_;}
    float timer()const{return timer_;}
    const char *name()const{return name_;}
    const char *secondary()const{return secondary_;}
    int failure()const{return failure_;}
};
//136E97..136EF2: every registered OBJECTIVES member must remain a live
//Objective. A stale/cross-owner name is an invariant failure, not permission
//to capture a checkpoint. Only authored status2 blocks an otherwise valid set.
int native_objectives_block_checkpoint(NativeMissionWorld &,uint8_t *blocked);
