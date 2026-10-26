#pragma once
#include <cstdint>

namespace trial_coop {
constexpr int PhysicalPort(int logical,bool active){
    return logical<0||logical>=4?-1:active?(logical==1?0:-1):logical;
}
struct InputMapping {
    uint32_t owner=0;
    uint8_t saved[7]{};
    void Update(uint32_t current,uint8_t* record,bool active){
        if(!record||current!=owner)owner=0;
        if(!record)return;
        if(active){
            if(!owner){
                owner=current;
                for(unsigned i=0;i<2;++i){saved[i]=record[i];saved[2+i]=record[4+i];saved[4+i]=record[8+i];}
                saved[6]=record[12];
            }
            for(unsigned i=0;i<2;++i)record[i]=record[4+i]=uint8_t(i);
            record[12]=1;
        }else if(owner){
            // A new original mapping owns its changes; do not overwrite them on exit.
            if(record[0]==0&&record[1]==1&&record[4]==0&&record[5]==1&&record[12]==1){
                for(unsigned i=0;i<2;++i){record[i]=saved[i];record[4+i]=saved[2+i];record[8+i]=saved[4+i];}
                record[12]=saved[6];
            }
            owner=0;
        }
    }
};
}
