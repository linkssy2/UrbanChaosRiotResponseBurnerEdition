#include "mission_flags.h"
#include <string.h>
#include <limits.h>
static int target(UCLevelBytes map,size_t count,int32_t number,int32_t*out){
    *out=-1;
    if(map.size%8||(map.size&&!map.data)||map.size/8>INT32_MAX)return 1;
    int32_t rows=(int32_t)(map.size/8);if(number>rows)return 0;
    for(int32_t i=0;i<rows;i++){
        int32_t pair[2];memcpy(pair,map.data+(size_t)i*8,8);
        if(pair[0]==number){if(pair[1]>=0&&(size_t)pair[1]>=count)return 1;*out=pair[1];break;}
    }
    return 0;
}
int uc_mission_flag_set(UCLevelBytes map,uint8_t*flags,size_t count,int32_t number,uint8_t value){
    int32_t index;if((count&&!flags)||value>1||target(map,count,number,&index))return 1;
    if(index>=0)flags[index]=value;return 0;
}
int uc_mission_equipment_flags(const UCLevel*l,uint8_t*flags,size_t count,int32_t item){
    int32_t pistol,taser;
    if(!l||count!=l->field||(count&&!flags)||target(l->remap,count,13,&pistol)||target(l->remap,count,14,&taser))return 1;
    if(pistol>=0)flags[pistol]=(uint8_t)(item==2||item==16||item==7||item==23);
    if(taser>=0)flags[taser]=(uint8_t)(item==1||item==15);
    return 0;
}
