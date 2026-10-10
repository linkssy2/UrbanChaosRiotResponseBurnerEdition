#pragma once
extern "C" {
#include "../character_move.h"
#include "../level_asset.h"
#include "../collision_asset_query.h"
#include "../collision_body.h"
#include "../collision_result.h"
#include "../character_retarget.h"
#include "../character_collision.h"
}
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include <list>

//1CA10/1CE40/1CC80: shared search queue. Initialization is free; ten search
//operations are granted per tick, at most twenty jobs run concurrently. Equal
//priorities insert before existing jobs and each tick starts at the list head.
class NativeCharacterSearchManager {
public:
    struct Job {
        float priority=0;void *owner=nullptr;
        void (*initialize)(Job &)=nullptr;
        void (*step)(Job &)=nullptr;
        void (*cancel)(Job &)=nullptr;
        bool complete=false,scheduled=false;
    };
private:
    std::list<Job *> pending_,active_,completed_;
    static void insert(std::list<Job *> &,Job &);
public:
    NativeCharacterSearchManager()=default;
    NativeCharacterSearchManager(const NativeCharacterSearchManager &)=delete;
    NativeCharacterSearchManager &operator=(const NativeCharacterSearchManager &)=delete;
    void add(Job &);void remove(Job &);void update();
    size_t pending()const{return pending_.size();}
    size_t active()const{return active_.size();}
};

class NativeCharacterNavigationManager;

// PC collision adapter shared with movement: actual mesh and active box bodies.
// Original ray preparation/body filtering, geometric scenery triangles (no MOPP
// or mesh welding). Borrowed geometry/metadata must stay live during each query.
// The caller supplies actual body filters; an absent body is never synthesized.
struct NativeCharacterWorld {
    struct Box {
        const UCSolidBlockGeometry *geometry;UCCollisionBodyFilter filter;int32_t entity;
        UCCollisionResultOwner result_owner{};
        const void *actor=nullptr; //native property45 pointer, never a truncated address
    };
    struct Capsule {
        const UCCharacterCapsule *geometry;UCCollisionBodyFilter filter;int32_t entity;
        UCCollisionResultOwner result_owner{};
        const void *actor=nullptr;
    };
    const UCCollisionAsset *scenery=nullptr;
    UCCollisionResultOwner scenery_owner{};
    void *impulse_owner=nullptr;
    int (*apply_impulse)(void *,int32_t,const float momentum[3])=nullptr;
    std::vector<Box> boxes;
    std::vector<Capsule> capsules;
    static int ray(void *,const float from[3],const float to[3],uint32_t region,
        uint32_t shape_mask,uint32_t type_mask,uint8_t *hit,int32_t *entity);
    //Same geometric selection as ray, followed by source2A6FF0 projection.
    //Selected owner metadata must be supplied. PC actor pointers are returned
    //separately: result.property45 is zero rather than a truncated uint32 pointer.
    //Scenery entity=0/actor=nullptr; boxes retain actual entity/actor identities.
    //1 hit,0 miss,-1 invalid; preserves outputs on0/-1. Dynamic impulses use
    //the optional retained-body callback; property20 remains unsupported. No
    //MOPP/terminal material filtering, welding or world mutation are added.
    int query(const float from[3],const float to[3],uint32_t type,uint32_t region,
        uint32_t mask,float impulse,UCCollisionResult &,int32_t &entity,const void **actor=nullptr)const;
};

//1F230/1F320. Queries use the actual class properties and template registry;
//open completes21F70's dynamically resolved event with bytes{1,0} and the real
//actor sender. cast_template completes45490, not an unrestricted scenery ray.
struct NativeCharacterDoors {
    void *owner=nullptr;
    uint8_t (*property)(void *,int32_t,const char *,uint8_t)=nullptr;
    const float *(*position)(void *,int32_t)=nullptr;
    void (*open)(void *,int32_t target,int32_t sender)=nullptr;
    int32_t (*simple_door)(void *)=nullptr; //0 means absent template
    int32_t (*cast_template)(void *,const float from[3],const float direction[3],int32_t)=nullptr;
    uint8_t check(int32_t target,int32_t sender,const float actor_position[3])const;
    void ahead(int32_t sender,const float from[3],const float waypoint[3])const;
};

//21180 obstacle-path owner. Geometry is supplied by the actual obstacle owner,
// separately expanded for collision radius and radius+detour margin. No body
// registration or obstacle is inferred from a visible model. Inputs stay live.
class NativeCharacterDetour {
public:
    using Point=std::array<float,3>;
    struct Obstacle {const void *identity;UCCharacterObstacle collision;const float (*detour)[3];uint32_t detour_count;};
private:
    std::vector<Point> paths_[2];uint32_t cursor_=0,attempts_=0;
    const void *blocking_=nullptr;int selected_=-1;bool forward_=false;
    static int path_clearance(void *,const float *,const float *,float,uint32_t,uint8_t *);
    static bool world_blocked(const UCCharacterCandidateQuery &,const float *,const float *,bool);
    static bool path_clear(const UCCharacterCandidateQuery &,const std::vector<Point> &);
    void reset();
public:
    //0 temporarily blocked,1 continue. Throws on unsupported/invalid queries.
    int step(UCCharacterAgentSteering &,const UCCharacterCandidateQuery &,const float goal[3],
        float margin,const Obstacle *,uint32_t count);
    bool active()const{return blocking_!=nullptr;}
    const void *identity()const{return blocking_;}
};

// Authored-node route search (18FC0/1E0C0 family), not the navigator around it.
// Caller supplies nodes resolved by213510 and retains the live trigger flags.
// A step expands one original directed-edge state. Scheduling, world clearance,
// obstacle avoidance and mission movement remain with the enclosing navigator.
class NativeCharacterRoute {
    const UCLevelLogic &logic_;
    std::vector<UCCharacterNavigationInfluence> influences_;
    struct Edge {int32_t from,to;};
    std::vector<Edge> edges_;
    std::vector<UCCharacterSearchRecord> records_;
    std::unordered_map<uint64_t,int32_t> edge_ids_;
    std::vector<int32_t> open_;
    std::array<float,3> from_,to_;
    uint32_t count_=0;int32_t goal_;int status_=1;
    int32_t edge(int32_t,int32_t,int32_t);
    void enqueue(int32_t);
    double heuristic(int32_t)const;
    void finish(int32_t);
public:
    struct Point {std::array<float,3> position;int32_t node;};
    std::vector<Point> path;
    NativeCharacterRoute(const UCLevelLogic &,
        int32_t start,int32_t goal,const float from[3],const float to[3],
        const UCCharacterNavigationInfluence *influences=nullptr,uint32_t influence_count=0);
    void step();
    int status()const{return status_;} //1 initial,2 searching,3 failed,4 complete
};

//344C0 navigator ownership for a stopped controller's new MoveRun goal.
// Borrows the actual level/flags; the caller schedules search_step through its
// search manager, then update through its navigator manager. No guessed time
// budget or mission registration. MoveRun resets before start; FollowEntity
// uses21D20/20C50 retarget, preserving/splicing the actual retained route.
class NativeCharacterNavigator {
    friend class NativeCharacterNavigationManager;
    const UCLevelLogic &logic_;
    std::unique_ptr<NativeCharacterRoute> search_;
    std::vector<std::array<float,3>> points_;std::vector<uint8_t> tags_;
    std::vector<int32_t> nodes_;
    std::array<float,3> goal_{},search_from_{},waypoint_{};
    UCCharacterRouteProgress progress_{};NativeCharacterDetour detour_;
    struct Influence {int32_t entity;float radius,weight;};
    std::vector<Influence> influences_;
    std::vector<UCCharacterNavigationInfluence> search_influences_;
    float timers_[4]{};int32_t blocker_=0;uint32_t restarts_=0;
    int job_=0; //0 absent,1 awaiting manager initialization,2 running,3 failed,4 complete
    NativeCharacterNavigationManager *manager_=nullptr;
    void clear_paths();void restart();
    static void validate(const UCCharacterCandidateQuery &);
public:
    NativeCharacterNavigator(const UCLevelLogic &);
    ~NativeCharacterNavigator();
    NativeCharacterNavigator(const NativeCharacterNavigator &)=delete;
    NativeCharacterNavigator &operator=(const NativeCharacterNavigator &)=delete;
    void stop();
    void clear_influences(){influences_.clear();} //2012C0, separate from path cancellation.
    void influence(int32_t entity,float radius,float weight){influences_.insert(influences_.begin(),{entity,radius,weight});}
    void start(const float goal[3],const UCCharacterCandidateQuery &);
    void retarget(const float goal[3],const UCCharacterCandidateQuery &);
    void search_step(const UCCharacterCandidateQuery &);
    void update(float delta,const UCCharacterCandidateQuery &,
        const NativeCharacterDetour::Obstacle *,uint32_t count,
        void (*scripted_ray)(void *,const float *,const float *));
    int status()const{return progress_.status;}
    bool near_goal()const; //1E950: goal versus current waypoint, not actor position
    int32_t blocker()const{return blocker_;}
    uint32_t restarts()const{return restarts_;}
    bool searching()const{return job_!=0;}
    bool blocked()const{return blocker_!=0||detour_.active();} //1EA90: either retained obstacle owner.
    const float *goal(const float position[3])const{return status()?goal_.data():position;}
    const float *waypoint(const float position[3])const{return status()<2?position:waypoint_.data();}
};

//22CA0/227F0/24920: borrowed live actors, ordered obstacle/agent lists and the
//real search manager. Call update before character controllers/animation, as
//55610 does. Caller completes24630 obstacle refresh through refresh_obstacles;
//rendered meshes are never silently promoted into navigation obstacles.
class NativeCharacterNavigationManager {
    friend class NativeCharacterNavigator;
public:
    struct Binding {
        int32_t entity=0;NativeCharacterNavigator *navigator=nullptr;
        const float *position=nullptr;uint32_t region=0;
        UCCharacterClearanceRay ray=nullptr;UCCharacterScriptedObstacle scripted=nullptr;
        void (*scripted_ray)(void *,const float *,const float *)=nullptr;
        void *owner=nullptr;
        float radius=40,height=200;
    };
private:
    struct Client {Binding binding;NativeCharacterNavigationManager *manager;NativeCharacterSearchManager::Job search;};
    NativeCharacterSearchManager searches_;
    std::vector<std::unique_ptr<Client>> clients_;
    std::vector<UCCharacterAgentEntry> agents_;
    std::vector<const NativeCharacterDetour::Obstacle *> obstacle_owners_;
    std::vector<NativeCharacterDetour::Obstacle> obstacles_;
    std::vector<UCCharacterObstacle> collision_;
    Client &client(const NativeCharacterNavigator &);
    void enqueue(NativeCharacterNavigator &);void cancel(NativeCharacterNavigator &);
    void snapshot_obstacles();
public:
    ~NativeCharacterNavigationManager();
    void attach(const Binding &);void detach(int32_t entity);
    void add_agent(int32_t entity,const float *position,float radius,float height);
    void add_obstacle(const NativeCharacterDetour::Obstacle &);
    void remove_obstacle(const void *identity);
    UCCharacterCandidateQuery query(int32_t entity)const;
    void update(float delta,void (*refresh_obstacles)(void *)=nullptr,void *owner=nullptr);
    NativeCharacterSearchManager &search_manager(){return searches_;}
    size_t size()const{return clients_.size();}
};
