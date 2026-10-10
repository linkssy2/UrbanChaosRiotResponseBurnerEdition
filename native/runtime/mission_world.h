#pragma once
#include "mission.h"
#include <memory>
extern "C" {
#include "../entity_names.h"
#include "../entity_records.h"
#include "../entity_listener.h"
#include "../entity_retirement.h"
#include "../mission_trigger_actor.h"
}
class NativeMissionWorld;
/* One real entity binding; fields remain owned by its concrete enclosing class.
 * Base-only classes may have actor==NULL and no script controller.
 * Bind before its constructor registers, so SAME-registry queries resolve it.
 * close runs only after actual slot/live/record detach; it must release real
 * class resources/allocation. No arbitrary class or fake endpoint is supplied. */
struct NativeMissionBinding {
    const UCEntityFreshView *view=nullptr;
    void *owner=nullptr;
    int (*step)(void *)=nullptr;
    int (*late)(void *)=nullptr;
    int (*close_detached)(void *)=nullptr;
    NativeMissionController *script=nullptr;
    /* Optional actual class virtual24 and deleting virtual20. A class without
     * these endpoints remains unsupported for gameplay removal. destroy must
     * release the enclosing owner only after its real suffix; no close alias. */
    int (*remove)(void *)=nullptr;
    int (*destroy)(void *)=nullptr;
};
struct NativeMissionWorldServices {
    void *owner=nullptr;
    void (*profile_step)(void *,uint32_t,double)=nullptr; // Optional bounded diagnostic; absent in ordinary play.
    /* Whole known-class construction + actual20A1D0 model publication using
     * this world's current authored context. Must bind a real entity before
     * registration. Nonzero preserves committed effects and stops the world.
     * A source NULL allocation can return0 with no binding; slot stays inactive.
     * Unsupported classes must return nonzero, never manufacture a marker. */
    int (*construct)(void *,NativeMissionWorld &,uint32_t source_index)=nullptr;
    int (*decode_extra)(void *,UCMissionEvent *,const UCLevelEntity *,int32_t *)=nullptr;
    int (*helper_tick)(void *,void *,float)=nullptr;
    int (*trigger_body)(void *,const UCEntityEventTarget *,UCMissionTriggerBody *)=nullptr;
    int (*character_query)(void *,int32_t)=nullptr;
    int (*character_status)(void *,int32_t,uint32_t *)=nullptr;
    //5D154/55610: frame already advanced; world+14 still contains prior delta.
    //Actual search/navigation/AI owners run here, before208F80 publishes dt.
    int (*pre_world)(void *,float previous_delta)=nullptr;
};
/* Aggregate authoritative level slots, registry, names, creation fields and
 * priority records. Borrows level/logic, actual mission flags and CRT stream.
 * Native ScriptControllers are concrete; other classes require their real
 * factory. Ordinary slots use2109C0/210A40, custom slots213720. No all-active
 * loadout or unconditional prop population. Updates reuse qualified208F80
 * active passes plus operate2058/action2064 queues. Generic trigger211B80,
 * maintenance and world tail remain external.16 sends retail eventBF using the
 * real sender ID/position; deferred delivery resolves both source slots, uses
 * zero position and expires after31 ticks or immediately when sender is gone.
 * Priority1 creation during step may prepend the same nonempty bucket or insert
 * after that current entity/a real parent newly inserted during the same step.
 * Other relative parents and any creation during late are rejected explicitly.
 * Calls stay on the creating thread. Level and borrowed buffers outlive close. */
class NativeMissionWorld {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeMissionWorld();~NativeMissionWorld();
    NativeMissionWorld(const NativeMissionWorld&)=delete;
    NativeMissionWorld&operator=(const NativeMissionWorld&)=delete;
    int load(const UCLevel &,UCLevelLogic &,uint8_t *flags,size_t flag_count,
             uint32_t *crt_seed,const NativeMissionWorldServices &);
    int start_automatic();
    //20990E: call once when the actual initial activation pass finishes.
    //5D154 advances the same world+0C counter before each simulation tick.
    int complete_startup();
    uint32_t frame()const;
    const char *failure_phase()const;
    int ensure(uint32_t,int32_t *id=nullptr);
    int ensure_trigger(uint32_t trigger_ordinal,int32_t *id=nullptr);
    int event(uint32_t source_index,uint32_t group,uint8_t *accepted);
    int tick(float delta);
    int close(); //explicit native shutdown; not generic authored destruction
    int bind(uint32_t,const NativeMissionBinding &);
    /* Actual dynamic factories have no authored level slot. Bind their real
     * enclosing owner before shared registration; source_count/slots/metadata
     * stay unchanged. At most1024 dynamic bindings, same live registry bound.
     * Returned binding index is an owner handle, never a source entity index.
     * Failed construction retains binding for close; no retry or marker entity. */
    int bind_dynamic(const NativeMissionBinding &,uint32_t *binding_index=nullptr);
    //208110 over both real record lists. During a step, only a child just
    //registered by this step may move; existing traversal owners stay put.
    int set_priority(int32_t id,int32_t priority);
    /* Returned view expires when another dynamic binding grows the collection;
     * the bound entity/view itself retains its enclosing owner's lifetime. */
    const NativeMissionBinding *binding(uint32_t)const;
    const UCEntityFreshContext &context()const;
    const UCEntityAuthored **active_descriptor();
    const UCEntityAuthored &metadata(uint32_t)const;
    const uint8_t **property_cursor(uint32_t);
    UCLevelEntitySlot &slot(uint32_t);
    const UCLevelEntitySlots &slots()const;
    UCEntityEventRegistry &registry();
    UCEntityNames *names();
    UCEntityListenerList *listeners();
    const UCEntityRetirement &retirement()const;
    const NativeMissionServices &script_services()const;
    const float &time()const; //world+8,209540 zero then208F80 addss before queued work
    const float &unscaled_delta()const; //world+18 published by208F80.
    uint32_t source_count()const;
    uint32_t failure_source()const;
    uint32_t failure_opcode()const;
    const char *failure_class()const;
    int failure()const;
};
