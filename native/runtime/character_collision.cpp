#include "character_collision.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {void require(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}}
NativeCharacterCollision::NativeCharacterCollision(const UCModelCollisionAsset&a,uint32_t bones,uint32_t group,int32_t entity,const void*actor)
    :group_(group),entity_(entity),actor_(actor){
    require(a.group_count==1&&a.group_object_counts&&a.group_object_counts[0]==1&&a.shape_count&&a.shapes&&bones&&entity>0&&actor&&group<=0xFFFF,"NPC authored collision owner");
    parts_.reserve(a.shape_count);
    for(uint32_t i=0;i<a.shape_count;i++){
        const auto&s=a.shapes[i];Part p{};p.source=s;
        require(!s.group&&!s.object&&s.family==1&&s.kind==3&&s.root_kind==8&&s.metadata_size==12&&std::isfinite(s.radius)&&s.radius>0,"NPC authored capsule family");
        std::memcpy(&p.bone,s.metadata+6,2);std::memcpy(&p.material,s.metadata+8,4);
        require(p.bone<bones,"NPC collision bone index");parts_.push_back(p);
    }
}
void NativeCharacterCollision::update(const UCModel&m,const UCModelRender&pose,const float*world){
    require(world&&m.bones&&pose.model==&m&&pose.ready&&pose.world&&pose.world_valid,"NPC collision pose owner");
    for(unsigned i=0;i<16;i++)require(std::isfinite(world[i]),"NPC collision actor transform");
    for(auto&p:parts_){
        require(p.bone<m.bone_count&&pose.world_valid[p.bone],"NPC collision posed bone");float local[16];
        require(!uc_character_collision_pose(m.bones[p.bone].bind_correction,pose.world[p.bone],local),"NPC collision normalized pose");
        //29F570 composes the retained pose with actor world, then converts its
        //rotation/translation to physics axes. Equivalently, convert the authored
        //physics endpoint to render XYZ, apply those matrices, then convert back.
        for(unsigned end=0;end<2;end++){
            const float*source=p.source.dimensions+end*4;float render[3],transformed[3];
            float x=source[0]*100.f,y=source[2]*100.f,z=source[1]*100.f;
            for(unsigned i=0;i<3;i++)render[i]=(x*local[i]+y*local[4+i])+(z*local[8+i]+local[12+i]);
            for(unsigned i=0;i<3;i++)transformed[i]=(render[0]*world[i]+render[1]*world[4+i])+(render[2]*world[8+i]+world[12+i]);
            float*out=end?p.world.b:p.world.a;out[0]=transformed[0]*.01f;out[1]=transformed[2]*.01f;out[2]=transformed[1]*.01f;
            for(unsigned i=0;i<3;i++)require(std::isfinite(out[i]),"NPC collision endpoint");
        }
        p.world.radius=p.source.radius;
    }
    posed_=true;
}
void NativeCharacterCollision::append(NativeCharacterWorld&w)const{
    require(posed_,"NPC collision unpublished pose");
    for(const auto&p:parts_){
        //2A7AD0 overwrites ROOT userdata from metadata+8.2B4F90/21B2E0
        //select mask8,class9,actor group. Family1 motion6 makes2F1C40 byte92=1.
        //Property45 remains a real native actor pointer, returned separately.
        w.capsules.push_back({&p.world,{8,p.material,0x24800000u|group_,1,0,0,0},entity_,{&p,1,1,0,0},actor_});
    }
}
