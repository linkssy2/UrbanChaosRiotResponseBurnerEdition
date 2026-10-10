#include "molotov_template_property.h"
#include <string.h>
int uc_molotov_template_property(const char *name,UCMolotovTemplateProperty *out){
    static const char *const keys[]={"model","particle sys 1","particle sys 2","particle sys 3"};
    static const char *const values[]={"Assets_Characters\\CHA_BurnerBlazer\\CHA_BurnerBlazer_Molotov_Skin.AMF",
        "MolotovTrail1.simpleparticlesystem","MolotovTrail2.simpleparticlesystem","MolotovTrail3.simpleparticlesystem"};
    if(!name||!out)return 0;
    for(unsigned i=0;i<4;i++)if(!strcmp(name,keys[i])){out->type=4;out->string=values[i];return 1;}
    if(!strcmp(name,"weapon time")){out->type=2;out->number=20;return 1;}
    return 0;
}
