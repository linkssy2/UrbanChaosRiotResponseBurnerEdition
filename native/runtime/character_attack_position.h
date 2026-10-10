#pragma once
#include "character_navigation.h"

//44C5B4/19CA0: retained attack-position sampler and its actual1E0C0 search job.
//Search initialization/expansion belongs to the shared1CC80 manager. request()
//samples only the caller's original iteration count; it never runs route jobs.
class NativeCharacterAttackPosition {
public:
    struct Services {
        void *owner=nullptr;
        int32_t (*target)(void *)=nullptr; //33B40, may be called twice on a change
        const float *(*position)(void *,int32_t)=nullptr; //1F8FC0 actor+40, NULL missing
        const float *(*defend_position)(void *,int32_t)=nullptr; //actual source reference
        double (*random)(void *,float,float)=nullptr; //shared2D3780
        //214230: -1 error,0 miss,1 hit. Preserve actual type/group/mask/impulse.
        int (*ray)(void *,const float[3],const float[3],uint32_t,uint32_t,uint32_t,float,float[3])=nullptr;
    };
private:
    struct State {
        int32_t actor=0,target=-1,defend=-1;
        uint32_t region=0;bool initialized=false,defend_valid=false;
        float position[3]{},target_position[3]{},distance=0,direction[3]{};
        float defend_position[3]{},defend_min=0,defend_max=0;
        float defend_target_direction[3]{},defend_target_distance=0;
        float defend_actor_direction[3]{},defend_actor_distance=0,defend_heading=0;
        int32_t mode=0;float ground_depth=0,range_min=300,range_max=700,min_square=250000;
        int32_t status=0;float chosen[3]{},heading=0,anchor[3]{},candidate[3]{};
        int32_t attempts=0,rejected=0;
    } state_;
    const UCLevelLogic &logic_;
    NativeCharacterNavigationManager &navigation_;Services services_;
    NativeCharacterSearchManager::Job job_{};
    std::unique_ptr<NativeCharacterRoute> route_;
    float route_from_[3]{},route_to_[3]{};int route_status_=0;bool route_owned_=false;
    bool refresh();void refresh_defend();bool allowed(const float *)const;
    bool clear_segment(const float *)const;int ray(const float *,const float *,uint32_t,float *)const;
    void sample();void cancel_route();void begin_route();void initialize_route();void step_route();
public:
    NativeCharacterAttackPosition(const UCLevelLogic &,
        NativeCharacterNavigationManager &,Services,int32_t actor,uint16_t region,float ground_depth);
    ~NativeCharacterAttackPosition();
    NativeCharacterAttackPosition(const NativeCharacterAttackPosition &)=delete;
    NativeCharacterAttackPosition &operator=(const NativeCharacterAttackPosition &)=delete;
    void ranges(float minimum,float maximum); //19EF0
    void reset(); //19D90; losing the target does NOT discard an existing job
    void clear(); //19F10; retains rejection counter and any existing job
    void request(int iterations); //1A400, not a boolean force flag
    int status()const{return state_.status;} //899F0
    const float *point()const{return state_.status==3?state_.chosen:state_.position;} //1ABC0
    //The enclosing behaviour writes these original fields when authored.
    void defend(int32_t source,float minimum,float maximum);
    void mode(int value){state_.mode=value;}
};
