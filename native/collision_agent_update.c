#include "collision_agent_update.h"
UCCollisionStatus uc_collision_agent_update(UCCollisionAgent *agent,uint32_t size,
    const UCCollisionAgentUpdateCallback *callbacks,uint32_t count,void *context,
    const void *input) {
    if(!agent||size<sizeof(*agent))return UC_COLLISION_ARGUMENT;
    if(agent->tag!=2&&agent->tag!=4&&agent->tag!=6)return UC_COLLISION_OK;
    if(!input||count>256||(count&&!callbacks))return UC_COLLISION_ARGUMENT;
    if(agent->type>=count)return UC_COLLISION_UNSUPPORTED;
    UCCollisionAgentUpdateCallback callback=callbacks[agent->type];
    if(!callback)return UC_COLLISION_OK;
    uint32_t offset=(uint32_t)sizeof(*agent)+(agent->tag==4?16u:0u);
    if(size<offset)return UC_COLLISION_ARGUMENT;
    callback(context,agent,(uint8_t*)agent+offset,agent->side[0],agent->side[1],input);
    return UC_COLLISION_OK;
}
