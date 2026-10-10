#include "audio_material_route.h"
#include <string.h>
#include <math.h>
static const char *const names[]={"Default","Concrete","Metal","Wood","ConcreteSoft",
    "Brick","Tile","ExplodingPipe","Glass","Flesh","WeaponButt","Carpet","Cloth",
    "Water","Flesh","Flesh","Flesh","Flesh","Flesh","Snow","Smoke","Slush",
    "Electrical","ThinMetal","Leather","HollowMetal","Paper","CSGas","Blood",
    "Grenade","WoodenHoles","Mud","Grass","Dirt","EnemyShield","Bell"};
const char *uc_audio_material_name(uint32_t material){return material<36?names[material]:NULL;}
UCAudioStatus uc_audio_world_material_request(const void *bank,const float point[3],
    uint32_t material,const char *prefix,uint32_t flags,const UCAudioWorldMaterialServices *h){
    const char *name=uc_audio_material_name(material);char text[260];size_t n=0;
    if(!point||!h||!h->source||!h->play)return UC_AUDIO_ARGUMENT;
    if(!name)return UC_AUDIO_UNSUPPORTED;
    for(unsigned k=0;k<3;k++)if(!isfinite(point[k]))return UC_AUDIO_ARGUMENT;
    if(prefix){while(n<sizeof text&&prefix[n])++n;
        if(n==sizeof text||strlen(name)>=sizeof text-n||strlen(names[0])>=sizeof text-n)return UC_AUDIO_FORMAT;
        memcpy(text,prefix,n);}
    memcpy(text+n,name,strlen(name)+1);
    void *source=h->source(h->context,point,1.f);if(!source)return UC_AUDIO_OK;
    if(h->play(h->context,source,bank,text,flags))return UC_AUDIO_OK;
    memcpy(text+n,names[0],strlen(names[0])+1);
    if(!h->play(h->context,source,bank,text,flags)&&prefix)h->play(h->context,source,bank,name,flags);
    return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_material_request(UCAudioMaterialActor *actor,const void *bank,
    uint32_t material,const char *prefix,char *output,size_t capacity,const UCAudioMaterialServices *h){
    const char *name=uc_audio_material_name(material);size_t length=0;
    if(!actor||!output||!h||!h->preflight||!h->create||!h->play||!capacity||capacity>4096)
        return UC_AUDIO_ARGUMENT;
    if(!name)return UC_AUDIO_UNSUPPORTED;
    if(!prefix)prefix="";
    while(length<capacity&&prefix[length])++length;
    if(length==capacity||strlen(name)>=capacity-length||strlen(names[0])>=capacity-length)
        return UC_AUDIO_FORMAT;
    if(!h->preflight(h->context,actor,bank))return UC_AUDIO_UNSUPPORTED;
    for(unsigned attempt=0;attempt<2;attempt++){
        memcpy(output,prefix,length);memcpy(output+length,name,strlen(name)+1);
        if(bank){
            if(!actor->controller){actor->controller=h->create(h->context,actor);if(!actor->controller)return UC_AUDIO_ARGUMENT;}
            if(h->play(h->context,actor->controller,bank,output)&&!attempt)break;
        }
        name=names[0];
    }
    return UC_AUDIO_OK;
}
