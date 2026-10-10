#pragma once
#include "heartbeat_widget.h"
#include "mission_world.h"
extern "C" {
#include "../heartbeat_monitors.h"
}
class NativeHeartbeatMonitors {
    NativeHeartbeatWidget widgets_[4];UCHeartbeatMonitors state_{};
    NativeMissionWorld*world_=nullptr;UCGameRandom*random_=nullptr;
    void(*sound_)(void*)=nullptr;void*sound_owner_=nullptr;
    int(*message_)(void*,const char*)=nullptr;void*message_owner_=nullptr;
    int(*order_)(void*,int32_t,int32_t*,uint8_t*)=nullptr;void*order_owner_=nullptr;
    bool order_available(int32_t)const;
    UCHeartbeatMonitorServices services();UCHudHeartbeatActor actor(int32_t)const;
public:
    void load(const std::filesystem::path&,NativeRenderer&,NativeDialogueText&,const UCArchive*,NativeMissionWorld&,UCGameRandom&,void(*sound)(void*),void*);
    bool ready()const{return world_!=nullptr;}
    int32_t references(int32_t id)const{for(const auto&s:state_.slots)if(s.actor==id)return s.references;return 0;}
    int show(int32_t,const char*);int remove(int32_t);void step(float);
    int join(int32_t,const char*,const char*,const char*);
    void message_provider(void*owner,int(*callback)(void*,const char*)){message_owner_=owner;message_=callback;}
    // Original71750 and7202B query the live actor property, never infer an order.
    void order_provider(void*owner,int(*callback)(void*,int32_t,int32_t*,uint8_t*)){order_owner_=owner;order_=callback;}
    int action_feedback(ID3D11ShaderResourceView*);
    //Actual717B0 adds (0,30), then eased slide*30 and one30px row perwidget.
    void draw(NativeRenderer&,float parent_x,float parent_y,const float color[4]);
};
