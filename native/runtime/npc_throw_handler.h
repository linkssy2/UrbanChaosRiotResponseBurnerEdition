#pragma once
#include <cstdint>
#include <deque>
struct NativeNpcThrownView {float position[3]{},fuse=-1;bool bounced=false,exploded=false;};
void native_npc_throw_matrix(const float local[16],const float world[16],float out[16]); //2E5D0
const char *native_npc_throw_bank(int32_t weapon); //442D0 +B0 after static initialization.
struct NativeNpcThrowServices {
    void*owner=nullptr;
    int(*projectile)(void*,int32_t,NativeNpcThrownView*)=nullptr; //1 found,0 missing,-1 failure
    int(*position)(void*,int32_t,bool target_point,float[3])=nullptr; //optional target property with world-position fallback
    int(*incapacitated)(void*,int32_t)=nullptr;
    int(*can_throw)(void*,int32_t actor,int32_t target,float speed,int32_t selection)=nullptr;
    int32_t(*restock)(void*,int32_t actor,int32_t weapon,bool secondary)=nullptr;
    int(*ignite)(void*,int32_t,float)=nullptr;
    int(*set_fuse)(void*,int32_t,float)=nullptr;
    int(*drop)(void*,int32_t)=nullptr;
    int(*remove)(void*,int32_t)=nullptr;
    int(*sound)(void*,int32_t,const char*)=nullptr;
    //2DE80 trajectory and DB1D0 submission, after the handler resolves target.
    int(*launch)(void*,int32_t projectile,const float point[3],
        uint32_t options,float speed,int32_t selection)=nullptr;
};
class NativeNpcThrowHandler {
public:
    struct Record {int32_t id=-1,target=-1,state=1;};
private:
    NativeNpcThrowServices services_{};
    bool lookup(int32_t,NativeNpcThrownView&)const;
    bool update_record(Record&);
    void restock(bool);void ignite(bool);void release();
public:
    int32_t actor_id=-1,primary=-1,weapon=-1,state=-1,next=-1,selection=-1,target_id=-1,secondary=-1;
    uint8_t enabled=1,flags[7]{},ignite_requested=0,fixed_target=0,drop_requested=0,target_requested=0,launch_requested=0;
    uint32_t options=0;float point[3]{},speed=0,delay=0,elapsed=0;
    std::deque<Record> records;
    NativeNpcThrowHandler(int32_t actor,int32_t kind,const NativeNpcThrowServices&);
    bool no_primary()const{return primary==-1;}
    bool ready()const;float fuse()const;bool can_throw(int32_t,float)const;
    void prime(){drop_requested=1;} //2D790
    void target(int32_t,float,float,uint32_t ignored=0);
    void throw_target(int32_t id,float velocity,uint32_t bits){target_id=id;speed=velocity;options=bits;launch_requested=1;}
    void throw_point(const float*,float,uint32_t);
    void pause_fuses(double delta);
    void update();void close();void drop(); //2EB60; also DeadBehaviour1B5460 virtual8.
};
