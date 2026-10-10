#pragma once
#include "character_navigation.h"
#include "mission_world.h"

class NativeMissionCoverPoint;
// Actual node+38 lists: only constructed CoverPoints register. Authored inactive
// records are not cover; insertion order and first matching removal are retained.
class NativeCharacterCoverPoints {
    const UCLevelLogic &logic_;
    std::vector<std::vector<NativeMissionCoverPoint *>> nodes_;
public:
    explicit NativeCharacterCoverPoints(const UCLevelLogic &);
    int32_t add(NativeMissionCoverPoint &);
    void remove(NativeMissionCoverPoint &);
    NativeMissionCoverPoint *find(int32_t)const;
    const auto &at(uint32_t node)const{return nodes_.at(node);}
    const UCLevelLogic &logic()const{return logic_;}
};

// 1B8470/1B74C0 concrete base-only owner. The crouching domain includes actual
// initial969. Standing cover requires the real20FE90 side-ray adapter; unsupported
// cover-target events30/31 fail explicitly, never report usable cover.
class NativeMissionCoverPoint {
    NativeMissionWorld *world_=nullptr;NativeCharacterCoverPoints *points_=nullptr;
    UCEntitySubscriptions *subscriptions_=nullptr;
    int32_t node_=-1,occupant_=-1;float radius_=400,height_=100,fov_=45,cosine_=0,forward_[3]{};
    bool crouching_=true,left_=false,right_=false,disabled_=false;float danger_until_=0;
    int failure_=0;
public:
    NativeMissionEntity entity;
    NativeMissionCoverPoint();
    int construct(NativeMissionWorld &,uint32_t,NativeCharacterCoverPoints &);
    int update()const{return failure_;} int late()const{return failure_;}
    int close_detached();
    int property(const char *,int32_t *)const;
    int32_t node()const{return node_;}int32_t occupant()const{return occupant_;}
    int reserve(int32_t actor,int32_t sender); //real event32 through shared registry
};

//1BF90's concrete CopAttack recipe: NULL threats, radius[0,1000], cosine45,
//one result. Job initialization is deferred to the SAME1CC80 search manager.
//Live cover positions/occupants and trigger flags are read during each step.
class NativeCharacterCoverSearch {
    NativeCharacterCoverPoints &points_;NativeCharacterNavigationManager &manager_;
    int32_t actor_;
    std::array<float,3> origin_,direction_;
    struct Record {int32_t node,cover,parent;uint32_t state=0;float score=0;};
    std::vector<Record> records_;std::vector<int32_t> open_;
    NativeCharacterSearchManager::Job job_{};int status_=0;int32_t result_=-1;
    int32_t record(int32_t node,int32_t cover,int32_t parent);
    const float *position(int32_t)const;
    void enqueue(int32_t);void initialize();void step();
    bool admit(int32_t)const;bool terminal(int32_t)const;
public:
    NativeCharacterCoverSearch(NativeCharacterCoverPoints &,NativeCharacterNavigationManager &,
        int32_t actor,const float origin[3],const float direction[3]);
    ~NativeCharacterCoverSearch();
    NativeCharacterCoverSearch(const NativeCharacterCoverSearch&)=delete;
    NativeCharacterCoverSearch&operator=(const NativeCharacterCoverSearch&)=delete;
    int status()const{return status_;}int32_t result()const{return result_;}
};
