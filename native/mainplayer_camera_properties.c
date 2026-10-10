#include "mainplayer_camera_properties.h"
int uc_mainplayer_camera_properties(UCEntityProperties *p,UCEntityProperty *scratch,float fov){
    if(!p||!scratch)return 1;
    const float one=1,from[3]={0,170,0},to[3]={0,0,10000};const int32_t target=0;
    const struct{uint32_t type;const char *name;const void *value;} entries[]={
        {2,"fovCurrent",&fov},{2,"fovDamper",&one},{4,"fromOffset",from},
        {2,"fromDamper",&one},{4,"toOffset",to},{2,"toDamper",&one},
        {1,"TargetEntity",&target}};
    uc_entity_properties_begin(p,-1);
    for(unsigned i=0;i<7;i++)
        uc_entity_properties_append(p,scratch,entries[i].type,entries[i].name,entries[i].value);
    p->placement_mode=1;return 0;
}
