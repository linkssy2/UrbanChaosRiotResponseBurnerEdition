#pragma once
#include "coroutine.h"
extern "C" {
#include "../entity_authored.h"
#include "../entity_stream.h"
#include "../mission_controller.h"
#include "../mission_schedule.h"
#include "../mission_command.h"
#include "../level_entity_slots.h"
#include "../wave_controller.h"
}
struct NativeMissionServices {
    uint8_t *flags=nullptr;
    size_t flag_count=0;
    uint32_t flag_limit=0,*crt_seed=nullptr;
    const float *delta=nullptr;
    void *owner=nullptr;
    int (*ensure)(void *,uint32_t)=nullptr;
    /* Required only for commands beyond the concrete scalar decoder. It must
     * perform the actual handler and update the SAME event cursor/slots. No
     * unknown-command advance or successful ignored effect is provided. This initial
     * extension admits immediate handlers only: C0 must remain NULL, no yield
     * or separately allocated command owner. Stateful handlers need their actual
     * destructor/lifetime integration before admission. */
    int (*decode_extra)(void *,UCMissionEvent *,const UCLevelEntity *,int32_t *)=nullptr;
    const UCLevelEntitySlots *slots=nullptr;
    // Actual resolved Character virtual8("is character") and +198 state.
    int (*character_query)(void *,int32_t)=nullptr;
    int (*character_status)(void *,int32_t,uint32_t *)=nullptr;
    const UCEntityEventRegistry *registry=nullptr;
    UCLevelLogic *logic=nullptr;
    int (*entity_action)(void *,uint32_t,uint32_t)=nullptr;
    int (*operate)(void *,uint32_t source,int32_t sender,const float *position,uint32_t target)=nullptr;
};
/* Actual ScriptController120B90/120C60 two-continuation connector; no global
 * entity registry or duplicate actor/script fields. The enclosing entity owns
 * FreshView and level metadata/conditions; this owns D0 and the lazy C8 holder.
 * NativeMissionController cannot move. All calls run on its creating thread. */
class NativeMissionController {
    const UCEntityFreshView *view_=nullptr;
    const UCEntityAuthored *metadata_=nullptr;
    NativeMissionServices services_{};
    UCMissionEvent event_{};
    UCMissionSchedule schedule_{};
    NativeCoroutine primary_,secondary_;
    uint32_t property_type_=0,group_=UINT32_MAX;
    int failure_=0;
    bool controller_=false;
    bool wave_=false;
    uint8_t spawn_finished_=0;
    uint32_t end_group_=UINT32_MAX;
    UCWaveMembers members_{};
    static int decode(void *,int32_t *);
    static int yield(void *);
    static int ensure(void *,uint32_t);
public:
    NativeMissionController()=default;
    NativeMissionController(const NativeMissionController&)=delete;
    NativeMissionController&operator=(const NativeMissionController&)=delete;
    /* Already constructed actual authored actor. ScriptController reads the
     * real two-word EnvScript property at current cursor; generic actors use
     * attach_actor and only their existing C8 suffix. Asset/model-free admitted
     * ScriptController domain matches all53 Westside/Harbor authored records. */
    int attach_controller(const UCEntityFreshView &,const UCEntityAuthored &,
        const uint8_t **property_cursor,const NativeMissionServices &);
    int attach_actor(const UCEntityFreshView &,const UCEntityAuthored &,
        const NativeMissionServices &);
    // F0400/F0570/F06D0: currently admits the authored zero-exclusion-list
    // domain. Spawn decoder22/61 tracks real members; unsupported commands fail.
    int attach_wave(const UCEntityFreshView &,const UCEntityAuthored &,
        const uint8_t **property_cursor,const NativeMissionServices &);
    int update();
    int event(uint32_t request,uint8_t grouped,uint8_t *accepted);
    /* Detach-only native shutdown after external registry/update removal.
     * Releases current concrete scalar command then native continuations;
     * does not pretend to implement generic authored entity destruction. */
    int close_detached();
    int failure()const{return failure_;}
    UCMissionEvent &fields(){return event_;}
    uint32_t group()const{return group_;}
};
/* One real enclosing ScriptController allocation. Source-unwritten fields
 * have explicit zero-initialized native provenance until the constructor. */
struct NativeMissionEntity {
    UCEntityEventTarget identity{};
    uint32_t flags=0,secondary=0;
    int32_t parent=-1,id=-1,source=-1;
    float transform[16]{};
    const void *definition=nullptr;
    void *model=nullptr,*helper=nullptr;
    UCEntityFreshAux *aux=nullptr;
    UCEntityFreshActor actor{};
    UCEntityFreshView view{{&identity,&flags,&parent},transform,&source,&id,
        &secondary,&definition,&model,&helper,&aux,&actor};
    NativeMissionController script;
    NativeMissionEntity()=default;
    NativeMissionEntity(const NativeMissionEntity&)=delete;
    NativeMissionEntity&operator=(const NativeMissionEntity&)=delete;
};
/* Known ScriptController factory and210910 slot publication. allocate returns
 * a real object already visible to the SAME registry's target resolver. Empty
 * source allocation returnsNULL and leaves slot inactive, resets cursor. No
 * unknown class or fake model loading: authored asset must be absent. All
 * metadata/slot/cursor owners remain stable through completing callbacks.
 * output receives allocated owner even on committed construction failure.
 * caller owns real retirement/removal before close_detached/aux destruction.
 *0 source completed,1 unsupported/invalid before effects,2 committed failure. */
int native_mission_create(const UCEntityFreshContext &,
    const UCEntityAuthored **active,const UCEntityAuthored &metadata,
    const uint8_t **property_cursor,const UCLevelEntitySlot &slot,
    const NativeMissionServices &,void *allocation_owner,
    NativeMissionEntity *(*allocate)(void *),NativeMissionEntity **output);
