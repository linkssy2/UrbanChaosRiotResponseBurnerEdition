#pragma once
#include <cfloat>
#include <cstdint>
#include <list>
#include "../bullet_history.h"
// Actual3C8D0/3C9C0/3CA30 lifetime owner and2C3C0 shot record owner.
// Registry/controller/view/removal endpoints are live services, never guesses.
class NativeCharacterAIMaintenance {
public:
    struct Services {
        void *owner=nullptr;
        int (*actor)(void *,int32_t,uint8_t *found,uint32_t *status198,uint8_t *controller)=nullptr;
        int (*tasks_empty)(void *,int32_t,uint8_t *)=nullptr; //33AA0
        int (*unobserved)(void *,int32_t,uint8_t *)=nullptr; //32EA0
        int (*retire)(void *,int32_t)=nullptr; //20A140(1)
    };
    struct Entry {int32_t actor,frame;float idle=0,age=0;};
    using Shot=UCBulletHistoryEntry;
    struct ShotServices {
        void *owner=nullptr;
        int (*actor)(void *,int32_t,float position[3],float forward[3])=nullptr; //1 found,0 absent,-1 failure
        int (*relation)(void *,int32_t actor,int32_t shooter,uint8_t *handled,uint8_t *result)=nullptr; //event61
        int32_t first_player=-1;
    };
private:
    std::list<Entry> alive_,dead_;
    int32_t limit_=13;bool stepping_=false;
    UCBulletHistory history_{};
    int erase_dead(const Services &,int32_t);
public:
    NativeCharacterAIMaintenance(){uc_bullet_history_init(&history_);}
    int add(const Services &,int32_t);
    int step(const Services &,uint32_t frame,float previous_delta);
    void limit(int32_t n){if(n<=13)limit_=n;} //3C840 retains signed input.
    int record(int32_t actor,int32_t kind,const float start[3],const float end[3],int32_t first_player);
    int bullet_by(const ShotServices &,int32_t actor,bool check_friendly,uint32_t *) const; //2C880/2C450
    int finish(float previous_delta); //2C1A0 AFTER controller sweep.
    const std::list<Entry>&alive()const{return alive_;}
    const std::list<Entry>&dead()const{return dead_;}
    uint32_t shot_count()const{return history_.count;}
    const Shot*shots()const{return history_.entries;}
    float shot_time()const{return history_.player_shot_age;}
    UCBulletHistory&history(){return history_;} //same2C3C0 owner borrowed by3F6E0
};
