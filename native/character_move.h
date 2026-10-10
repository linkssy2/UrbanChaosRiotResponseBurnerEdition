#ifndef UC_CHARACTER_MOVE_H
#define UC_CHARACTER_MOVE_H
#include "character_loop.h"
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* MoveRunBehaviour163A30/163A60. The controller owns the real navigator;
 * this state owns only the authored node sequence and current phase. */
typedef struct UCCharacterMove {
    UCCharacterLoopController *controller;
    void *actor;
    float *world;
    const int32_t *nodes;
    int32_t count,next,index,phase;
    uint8_t active;
} UCCharacterMove;
typedef struct UCCharacterMoveServices {
    void *owner;
    void (*reset)(void *,UCCharacterLoopController *);
    /* Complete207B30 ->2084E0 ->2104A0. NULL is an actual missing node. */
    const float *(*resolve)(void *,int32_t);
    void (*target)(void *,UCCharacterLoopController *,const float[3]);
    /* Complete33020(controller,1,1,0,1,1), not a distance approximation. */
    int (*run)(void *,UCCharacterLoopController *);
    void (*transition)(void *,UCCharacterMove *,int32_t);
    const float *(*actor_world)(void *,void *);
} UCCharacterMoveServices;
/* Returns1 for invalid input before effects,0 after the complete scalar body.
 * Services must preserve valid state/borrows; no reentry or destruction.
 * Actor/body, navigator and behaviour graph are required external owners.
 * This alone does not implement NPC locomotion, AI or mission construction. */
int uc_character_move_enter(UCCharacterMove *,UCCharacterLoopController *,const UCCharacterMoveServices *);
int uc_character_move_update(UCCharacterMove *,const UCCharacterMoveServices *);
/* MoveWalk1633A0 shares MoveRun's enter163A30 and authored node ownership.
 * Unlike33020 it stops for blocked/arrived and passes navigator1E950(0) to
 * the real movement owner. Phase resets before the arrived stop callback. */
typedef struct UCCharacterWalkServices {
    void *owner;
    int32_t (*status)(void *);
    void (*waypoint)(void *,float[3]);
    int32_t (*near_target)(void *);
    void (*request)(void *,const float[3],int32_t mode,int32_t near_target);
    void (*stop)(void *);
    void (*clear_heading)(void *);
} UCCharacterWalkServices;
int uc_character_walk_update(UCCharacterMove *,const UCCharacterMoveServices *,const UCCharacterWalkServices *);
/* Complete33020 controller-to-navigator/animator dispatch. Position services
 * expose the current actor transform; navigator status must perform1EA00, not
 * return a guessed distance state. Both player lookups occur as in the source.
 * All services must complete their effects and preserve valid finite borrows.
 * This function does not replace the navigator or animation scheduler. */
typedef struct UCCharacterRunServices {
    void *owner;
    void (*goal)(void *,float[3]);
    const float *(*actor_position)(void *);
    int32_t (*player_id)(void *);
    const float *(*position)(void *,int32_t);
    void (*stop)(void *);
    int32_t (*status)(void *);
    int (*emote_if_blocked)(void *);
    int32_t (*blocker)(void *);
    void (*emote)(void *,int32_t,int32_t);
    void (*waypoint)(void *,float[3]);
    void (*request)(void *,const float[3],int32_t);
    void (*clear_heading)(void *);
} UCCharacterRunServices;
/* flags in original argument order: unused, clear heading, walk, emote,
 * avoid player. Byte truth values are retained. Returns1 on invalid services,
 * otherwise0 with the actual signed navigation status (including early3). */
int uc_character_run(const UCCharacterRunServices *,const uint8_t flags[5],int32_t *status);
typedef struct UCCharacterLocomotionPlan {
    uint32_t base_action,turn_action,backwards; /* turn12 means absent */
    float turn_scale;
} UCCharacterLocomotionPlan;
/*2AEE30 action selection, before real controller construction/sequence ownership.
 * Original indices: Run,Walk,Backwalk,TurnLeft90,TurnLeft180,TurnRight90,
 * TurnRight180,StartRun,PushOff90Left,PushOff180Left,PushOff90Right,
 * PushOff180Right. Finite coordinates/angles bounded to1e6; distinct XZ target
 * required. Heading override is RELATIVE to current heading. Transactional.
 * No straight-line motion, animation playback or navigation is performed here. */
int uc_character_locomotion_plan(const float position[3],float heading,
    const float target[3],uint8_t override_heading,float relative_heading,
    int32_t mode,uint8_t near_target,UCCharacterLocomotionPlan *);
/*2AF210 locomotion wrapper decisions before child pose sampling. The turn
 * completion receiver supplies first_pending; mode is borrowed from the task.
 * Outputs represent original request9 heading and request2 steering. They must
 * be consumed by the body, not treated as a replacement navigation solution.
 * Finite coordinates/angles bounded to1e6; output and finished transactional. */
typedef struct UCCharacterLocomotionStep {
    float goal_heading,steering,steering_weight;
    uint8_t heading_emitted,steering_emitted;
} UCCharacterLocomotionStep;
int uc_character_locomotion_step(uint8_t *finished,uint8_t first_pending,
    const float position[3],float heading,const float target[3],uint8_t backwards,
    uint8_t override_heading,float relative_heading,int32_t original_mode,int32_t mode,
    float steering_weight,UCCharacterLocomotionStep *);
/*2AB7C0 composition after B92A0 supplies the actual sampled movement matrix.
 * Uses midpoint heading to rotate its translation, then adds body displacement
 * and the four-lane movement helper (XYZ plus heading). Does not scale local
 * translation twice or derive displacement from a difference of posed frames.
 * delta has already been applied by B92A0. Finite inputs/outputs required;
 * output arrays may alias inputs and are committed together on success. */
int uc_character_motion(const float movement[16],const float position[3],
    float heading,float rotation_scale,const float helper[4],const float displacement[3],
    float out_position[3],float *out_heading);
typedef struct UCCharacterHeading { float gain,max_acceleration,value,target,rate; } UCCharacterHeading;
/*21E10/21E60 navigation phases. Services execute the actual route search,
 * cursor and avoidance operations; returning a distance guess is not valid.
 * They may mutate waypoint/changed and navigation ownership. Timers commit
 * before services, exactly as the source; completed effects do not roll back. */
typedef struct UCCharacterNavigationServices {
    void *owner;
    uint8_t (*search)(void *,uint8_t continuing);
    uint8_t (*route)(void *,float waypoint[3],uint8_t *changed);
    uint8_t (*obstacles)(void *,float waypoint[3],uint8_t *changed);
    uint8_t (*agents)(void *,float waypoint[3],uint8_t *changed);
    void (*scripted_obstacle)(void *,const float position[3],const float waypoint[3]);
} UCCharacterNavigationServices;
int uc_character_navigation_search(float *elapsed,float delta,uint8_t continuing,
    const UCCharacterNavigationServices *);
int uc_character_navigation_active(float *elapsed,float delta,const float position[3],
    float waypoint[3],const UCCharacterNavigationServices *);
/*1F020/1F080 recovery decisions; restart requires completing1EAD0 in the owner
 * before the next phase. Blocked uses the optional blocking agent's VELOCITY;
 * arrived uses horizontal distance to the goal. No arrival is created here. */
int uc_character_navigation_blocked(float *elapsed,float delta,float radius,
    const float *blocking_velocity,uint8_t *restart);
int uc_character_navigation_arrived(float *elapsed,float delta,float radius,
    const float position[3],const float goal[3],uint8_t *restart);
typedef struct UCCharacterNavigationInfluence {float position[3],radius,weight;} UCCharacterNavigationInfluence;
/*1DC70 point penalty and1DB40 edge cost with1DA20 endpoint averaging. Source
 * divides by ALL influences, including out-of-range ones. Distances retain
 * x87 host-double arithmetic; penalty accumulation is single precision. */
int uc_character_navigation_penalty(const float point[3],const UCCharacterNavigationInfluence *,uint32_t count,double *);
int uc_character_navigation_cost(const float *from,const float to[3],const UCCharacterNavigationInfluence *,uint32_t count,double *);
typedef struct UCCharacterSearchRecord {int32_t state,parent;float cost,heuristic,total;} UCCharacterSearchRecord;
typedef struct UCCharacterSearchServices {
    void *owner;
    int32_t (*neighbor)(void *,int32_t current,int32_t ordinal);
    uint8_t (*admit)(void *,int32_t);
    double (*cost)(void *,int32_t);
    double (*heuristic)(void *,int32_t);
    void (*enqueue)(void *,int32_t);
} UCCharacterSearchServices;
/* Complete19360 neighbor relaxation. neighbor may append into the fixed-capacity
 * record storage and increase *count; all other services preserve that storage.
 * enqueue must implement19290, including equal-score ordering and reopen.
 * continue_neighbors=0 is the real end-of-neighbors signal, not search failure.
 * Admission/cost/query effects are committed; invalid service outputs return1. */
int uc_character_search_expand(UCCharacterSearchRecord *,uint32_t *count,uint32_t capacity,
    int32_t current,int32_t ordinal,uint8_t revisit,const UCCharacterSearchServices *,uint8_t *continue_neighbors);
/*19290 list insertion. Includes the original equal-score duplicate behavior:
 * an old entry is removed only when found AFTER the insertion point. */
int uc_character_search_enqueue(UCCharacterSearchRecord *,uint32_t record_count,int32_t id,
    int32_t *open,uint32_t *open_count,uint32_t capacity);
/*2BAFE0 body orientation controller. Source acceleration clamp is per update,
 * not multiplied by delta. Finite forward time, nonnegative gain/acceleration,
 * angles bounded to1e6; publishes the whole state only on success. */
int uc_character_heading(UCCharacterHeading *,float delta);
/*2AE6E0 procedural final approach. StandIdle playback and emitted request4/5
 * consumers are owned externally; these results must be applied, not discarded.
 * inverse_world is the actual2D4360 result. callback_pending represents a real
 * borrowed completion receiver; completed requires delivery before sampling.
 * Finite forward time, nonnegative rate/phase, weight[0,1]. Transactional. */
typedef struct UCCharacterNear {
    float target[3],direction[3],rate,phase,initial_offset[3],initial_turn;
    uint8_t initialized,swap_feet,small_bob,callback_pending;
} UCCharacterNear;
typedef struct UCCharacterNearOutput {
    float helper[4],feet[2][3],foot_yaw[2],bob[3],weight;
    uint8_t helper_emitted,completed;
} UCCharacterNearOutput;
int uc_character_near(UCCharacterNear *,const float position[3],float heading,
    const float inverse_world[16],float delta,float weight,UCCharacterNearOutput *);
/* Original20FFF0 performs ordered left/right/center214230 rays, not a capsule
 * sweep. The callback must execute that world query with shape mask0x20 and
 * type mask0x2380, region and final argument0. Nonzero means query failure;
 * hit may have entity0 (static world still blocks). A failure commits prior
 * read-only queries but leaves caller outputs untouched. */
typedef int (*UCCharacterClearanceRay)(void *,const float from[3],const float to[3],
    uint32_t region,uint32_t shape_mask,uint32_t type_mask,uint8_t *hit,int32_t *entity);
/*1EBC0 agent prediction. Render centimetres, original velocity units. Caller
 * supplies a nonzero horizontal segment and positive finite fraction; absent self
 * velocity means no registered agent and misses.1 hit updates fraction,0 miss,
 * -1 unsupported/invalid leaves fraction unchanged. No agent-list owner here. */
typedef struct UCCharacterAgent {float radius,height,position[3],velocity[3];} UCCharacterAgent;
int uc_character_agent_cast(float radius,float height,const float *self_velocity,
    const float from[3],const float to[3],const UCCharacterAgent *,float *fraction);
typedef struct UCCharacterAgentEntry {int32_t entity;UCCharacterAgent body;} UCCharacterAgentEntry;
typedef struct UCCharacterAgentHit {float position[3];int32_t entity;} UCCharacterAgentHit;
/*1F5B0 ordered live-agent scan. Borrowed stable list in original order. A NULL
 * result selects first-hit Boolean mode; otherwise chooses the last narrowing
 * hit and writes its projected waypoint. Short/absent-self early misses leave
 * result untouched; ordinary misses write end/entity0. -1 is query failure. */
int uc_character_agents_cast(float radius,float height,float horizon,const float *velocity,
    int32_t self,int32_t ignore,const UCCharacterAgentEntry *,uint32_t count,
    const float from[3],const float to[3],UCCharacterAgentHit *);
/*23C00 obstacle edge directions, then24070 radius expansion. One point has
 * four diagonal directions; otherwise two directions per vertex. Stable
 * finite polygon, no zero horizontal edges, count1..4096. Output disjoint;
 * capacity in float3 rows.0 success,1 rejected/allocation failure. The original
 * acos boundary is host double, as in the other native pose math primitives. */
int uc_character_obstacle_directions(const float (*points)[3],uint32_t count,
    float (*out)[3],uint32_t capacity);
int uc_character_obstacle_expand(const float (*points)[3],uint32_t count,
    const float (*directions)[3],float radius,float (*out)[3],uint32_t capacity);
/* 1FC00 with the 23C00/24070 polygon prepared for this navigator's radius.
 * Returns 1 hit, 0 miss, -1 invalid; fraction changes only on a nearer hit. */
int uc_character_obstacle_cast(float height,const float position[3],
    const float from[3],const float to[3],const float center[3],float half_height,
    float bounding_radius,uint8_t allow_inside,const float (*expanded)[3],
    uint32_t count,float *fraction);
typedef struct UCCharacterObstacle {
    const float (*expanded)[3];uint32_t count;
    float center[3],half_height,bounding_radius;uint8_t allow_inside;
} UCCharacterObstacle;
typedef struct UCCharacterObstacleHit {float position[3];int32_t index;} UCCharacterObstacleHit;
/*200D0 ordered scan; index is the borrowed obstacle ordinal, -1 on miss.
 * NULL output selects Boolean mode and stops at the first hit. */
int uc_character_obstacles_cast(float height,const float position[3],
    const float from[3],const float to[3],const UCCharacterObstacle *,uint32_t count,
    UCCharacterObstacleHit *);
/*201D0 creates both walks around the polygon expanded by radius+margin.
 * Output paths include intersections; Y is from.y+50. No segment-t clamp.
 * Each output requires count+2 rows.1 found,0 no detour,-1 invalid. */
int uc_character_obstacle_detours(const float from[3],const float to[3],
    const float (*expanded)[3],uint32_t count,float (*forward)[3],float (*reverse)[3],
    uint32_t capacity,uint32_t sizes[2]);
typedef struct UCCharacterAgentSteering {float waypoint[3];int32_t blocker,status;uint8_t changed;} UCCharacterAgentSteering;
/*216F0: candidate callback implements215E0 (1 clear,0 blocked,-1 failure).
 * Live agents and callback context must remain stable through the operation. */
typedef int (*UCCharacterCandidateClear)(void *,const float point[3],int32_t ignored_agent);
int uc_character_agent_steer(UCCharacterAgentSteering *,float radius,float height,float horizon,
    const float *velocity,int32_t self,const float position[3],const float goal[3],
    const UCCharacterAgentEntry *,uint32_t count,UCCharacterCandidateClear,void *owner);
int uc_character_clearance(const float from[3],const float to[3],float radius,
    uint32_t region,uint8_t extend_center,UCCharacterClearanceRay,void *owner,
    uint8_t *clear,int32_t *blocking_entity);
/*1F3E0 world blockage, including the scripted-obstacle exception. The predicate
 * executes1F230 for the actual hit entity, never for static entity0. The ray
 * owner completes214230; clear paths and negligible segments skip the predicate.
 * Query failure leaves blocked unchanged; earlier queries are read-only effects. */
typedef uint8_t (*UCCharacterScriptedObstacle)(void *,int32_t entity);
int uc_character_world_blocked(const float from[3],const float to[3],float radius,
    uint32_t region,uint8_t extend_center,uint8_t block_scripted,
    UCCharacterClearanceRay,UCCharacterScriptedObstacle,void *owner,uint8_t *blocked);
/*215E0 checks both legs of a detour against scenery and expanded obstacles,
 * and the first leg against agents. destination is the CURRENT route point
 * returned by1D080, not the route tail returned by1D3D0. */
typedef struct UCCharacterCandidateQuery {
    float radius,height,horizon;const float *position,*velocity,*destination;
    int32_t self;uint32_t region;
    const UCCharacterObstacle *obstacles;uint32_t obstacle_count;
    const UCCharacterAgentEntry *agents;uint32_t agent_count;
    UCCharacterClearanceRay ray;UCCharacterScriptedObstacle scripted;void *owner;
} UCCharacterCandidateQuery;
int uc_character_candidate_clear(void *query,const float point[3],int32_t ignored_agent);
/*20970 shortcut /20A00 direct check. both use extend0, block_scripted1.
 * mode1 uses20FA0's direct check, including the vertical
 * exclusion. Returns1 clear,0 blocked,-1 query/input failure. */
int uc_character_route_clear(const UCCharacterCandidateQuery *,const float point[3],float margin,uint8_t direct);
/* Same20A00, explicit second trailing argument.32D60(point,0) uses0. */
int uc_character_route_clear_options(const UCCharacterCandidateQuery *,const float point[3],float margin,uint8_t direct,uint8_t block_scripted);
typedef struct UCCharacterRouteProgress {
    float anchor[3],elapsed;uint32_t cursor;int32_t status;
} UCCharacterRouteProgress;
/*20FA0 on a retained base route. tags come from original1CFC0 (node.field10==2).
 * Status2 continues,1 requires the owner to execute1EAD0,4 requires clearing
 * the owned paths. changed/status transitions require resetting detour/agent
 * owners exactly as20FA0; this function does not own them. Query failures may
 * follow timer/query effects.0 success,1 invalid,2 failed query. */
int uc_character_route_update(UCCharacterRouteProgress *,const float (*points)[3],
    const uint8_t *tags,uint32_t count,float margin,float delta,
    const UCCharacterCandidateQuery *,uint8_t *changed,float waypoint[3]);
/*20AE0 completed-search path after job removal/copy/destruction. Repeated
 * shortcuts stop at the first tagged point or blocked segment. The route is
 * borrowed after its real owner retained it; no scheduler job is synthesized. */
int uc_character_route_adopt(UCCharacterRouteProgress *,const float (*points)[3],
    const uint8_t *tags,uint32_t count,const UCCharacterCandidateQuery *);
/*213510 selects the nearest enabled node within2000cm whose original three
 * rays clear. Stable source order breaks equal-distance ties. Tests start at
 * position+10Y, radius40, without extending center. -1 is a completed miss.
 * Reads the same live trigger records changed by mission commands52/53. */
int uc_character_navigation_endpoint(const UCLevelLogic *,
    const float position[3],uint32_t region,UCCharacterClearanceRay,void *owner,int32_t *node);
/*1D3F0 path cursor, with the actual ordered route already owned by navigation.
 * Source arrival uses separate horizontal/vertical tolerances; intermediate
 * points beyond40cm require clearance to the NEXT point before advancing.
 * The callback must complete20FFF0, including its world query contract above.
 * Read-only query failures leave cursor/output unchanged. No path search or
 * obstacle avoidance is invented. Points remain borrowed and finite. */
typedef int (*UCCharacterPathClearance)(void *,const float[3],const float[3],float,uint32_t,uint8_t *);
int uc_character_path_advance(const float (*points)[3],uint32_t count,uint32_t *next,
    const float position[3],float horizontal,float vertical,float radius,uint32_t region,
    UCCharacterPathClearance,void *owner,uint8_t *changed,uint8_t *active,float waypoint[3]);
typedef struct UCCharacterClipRequest { float weight,previous,phase,rate,root_scale; } UCCharacterClipRequest;
/*2BB2A0 result exposed by the body helper A5BF0. This scales root yaw,
 * independently of the request rate used by1F3BA0 for root sampling. Requests
 * are in original head-to-tail order; has_root comes from2B5200. Up to32
 * finite requests, weights[0,1]; empty input returns zero. */
int uc_character_clip_rotation(const UCCharacterClipRequest *,const uint8_t *has_root,
    uint32_t count,float *scale);
/*2B40C0 leaf emission. Clip/sample identity remains with the caller; an emitted
 * request corresponds to the actual2B41E0 list node, not an elapsed-time guess.
 * Incoming phase is capped to the source's0.999989986 value. Five graph weights
 * are represented at the boundary, although this leaf consumes only weight0.
 * delta/rate positive, phase[0,1], previous[0,1), weights[0,1], finite scale.
 * No emission leaves request unchanged; all outputs publish after validation. */
int uc_character_clip_request(float *previous,uint8_t has_root,float clip_frequency,
    float delta,float phase,const float weights[5],float root_scale,
    float output_weights[5],UCCharacterClipRequest *,uint8_t *emitted);
typedef struct UCCharacterClipLoop { float phase,leaf_phase; } UCCharacterClipLoop;
typedef struct UCCharacterClipCompletion { int32_t cycles;float total;uint8_t crossed; } UCCharacterClipCompletion;
/*2B1EB0 ->2B40C0 loop graph. Optional completion records source cycle-count,
 * accumulated normalized time and strict unwrapped_phase>1 crossing. Existing
 * crossed stays latched. Forward finite time with unwrapped phase<2^31;
 * integer cycle accumulation wraps like source ADD. No event dispatch or
 * controller-list/clip-cache lifetime is invented by this scalar operation. */
int uc_character_clip_loop(UCCharacterClipLoop *,UCCharacterClipCompletion *,
    uint8_t has_root,float frequency,float delta,const float weights[5],
    float output_weights[5],UCCharacterClipRequest *,uint8_t *emitted);
typedef struct UCCharacterClipSequence { float elapsed,leaf_phase[2];uint8_t first_pending; } UCCharacterClipSequence;
/*2B3530 turn/start -> loop graph, including cosine fade2B3890/2D5140.
 * Leaf0 is first, leaf1 loops. Source evaluates leaf1 BEFORE leaf0 and prepends
 * emitted requests; the owner must retain this order and all clip/cache owners.
 * first_finished reports the original callback boundary, which the real owner
 * must deliver before publishing requests. Completion is optional. Positive
 * frequencies/delta, nonnegative finite fade-out/elapsed; same leaf domain.
 * Pure scalar transaction only; outputs disjoint, no callback reentry modeled. */
int uc_character_clip_sequence(UCCharacterClipSequence *,UCCharacterClipCompletion *,
    const uint8_t has_root[2],const float frequencies[2],float first_fade_out,
    float first_root_scale,float delta,const float weights[5],float output_weights[5],
    UCCharacterClipRequest requests[2],uint8_t emitted[2],uint8_t *first_finished);
/*2B8DB0 task scheduler. Graphs/callbacks are real external owners, not animation
 * names. The transition/fade/combine services transfer their graph ownership
 * exactly as their original constructors do. Services may mutate this scheduler
 * but must preserve its lifetime and valid remaining borrows. No rollback of
 * completed callbacks or destruction. This is not a body or task factory. */
typedef struct UCCharacterSchedule {
    void *fading,*current,*pending,*pending_callback,*stop_callback;
    uint8_t interrupt;
} UCCharacterSchedule;
typedef struct UCCharacterScheduleServices {
    void *owner;
    uint8_t (*finished)(void *,void *);
    void *(*interruptor)(void *,void *);
    void (*interrupt)(void *,void *,uint8_t);
    void (*destroy)(void *,void *);
    void *(*transition)(void *,void *pending,void *current,void *callback);
    void *(*fade)(void *,void *);
    void *(*combine)(void *,void *,void *);
    void (*notify)(void *,void *);
    /* All five output weights finite; original graph/list effects completed. */
    void (*sample)(void *,void *,float,const float weights[5],float output[5]);
    void *(*wrap)(void *,void *); /*2B9000 interruptible owner for an unowned leaf*/
    void (*cancel)(void *,void *); /*callback virtual8, distinct from notify4*/
} UCCharacterScheduleServices;
int uc_character_schedule_update(UCCharacterSchedule *,const UCCharacterScheduleServices *,
    float delta,const float weights[5],float output[5]);
int uc_character_schedule_queue(UCCharacterSchedule *,const UCCharacterScheduleServices *,
    void *graph,void *callback,uint8_t interrupt);
int uc_character_schedule_stop(UCCharacterSchedule *,const UCCharacterScheduleServices *,void *callback);
typedef struct UCCharacterGraphFade { void *current;float duration,elapsed; } UCCharacterGraphFade;
typedef struct UCCharacterGraphTransition {
    void *current,*previous,*callback;float duration,elapsed;
} UCCharacterGraphTransition;
/*2B2CD0/2B2F00 graph owners. Duration may be zero with positive delta. Fade
 * removal/callback happens strictly AFTER the endpoint, matching the source.
 * Services retain the same mutation/lifetime contract as the scheduler. */
int uc_character_graph_fade(UCCharacterGraphFade *,const UCCharacterScheduleServices *,
    float delta,const float weights[5],float output[5]);
int uc_character_graph_transition(UCCharacterGraphTransition *,const UCCharacterScheduleServices *,
    float delta,const float weights[5],float output[5]);
#ifdef __cplusplus
}
#endif
#endif
