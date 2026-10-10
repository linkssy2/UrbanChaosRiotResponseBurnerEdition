#pragma once
#include "mission.h"
class NativeCharacterScriptTasks;
// Actual329F0/32710 foreground script owner. Borrows the existing actorB0/B8/C4
// fields and controller task list; it does not start the world actor coroutine.
class NativeCharacterBehaviourScript {
    NativeMissionEntity *entity_=nullptr;
    const UCEntityAuthored *metadata_=nullptr;
    NativeMissionServices services_{};
    NativeCharacterScriptTasks *tasks_=nullptr;
public:
    int bind(NativeMissionEntity &,const UCEntityAuthored &,
        const NativeMissionServices &,NativeCharacterScriptTasks &);
    int request(int32_t type,int32_t index,bool grouped);
    int run(int32_t state,int32_t index,int32_t *new_state);
};
