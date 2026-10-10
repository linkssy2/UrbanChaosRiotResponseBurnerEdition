#include "basic_prop.h"
#include <stddef.h>
int uc_basic_prop_parameters(const UCLevelEntity*d,const uint8_t**cursor,UCBasicPropParameters*out){
    if(!d||!cursor||!out)return 1;
    static const uint8_t types[]={3,2,3,3,2,2,2,3};
    static const size_t offsets[]={offsetof(UCBasicPropParameters,body),offsetof(UCBasicPropParameters,model_value),offsetof(UCBasicPropParameters,visible_thermal),offsetof(UCBasicPropParameters,visible_normal),offsetof(UCBasicPropParameters,rotation_z),offsetof(UCBasicPropParameters,rotation_y),offsetof(UCBasicPropParameters,rotation_x),offsetof(UCBasicPropParameters,difficulty_variant)};
    const uint8_t*p=*cursor;UCBasicPropParameters v={0};
    for(unsigned i=0;i<sizeof types;i++){
        void*f=(uint8_t*)&v+offsets[i];int found;
        if(uc_entity_stream_get(d,&p,types[i],f,&found)||!found)return 1;
        if(types[i]==3)*(uint32_t*)f=(*(uint32_t*)f&255)!=0;
    }
    *out=v;*cursor=p;return 0;
}
