#include "character_ai_phase.h"
#include <algorithm>
#include <new>
#include <iterator>
int NativeCharacterAIPhase::at(size_t index,int32_t&out)const{
    if(index>=ids_.size())return 1;auto i=ids_.begin();std::advance(i,index);out=*i;return 0;
}
int NativeCharacterAIPhase::population(){
    //2F0A0 deliberately has no threshold case for populations16 and above.
    for(int32_t i=18;i<=21;i++)if(uc_mission_flag_set(remap_,flags_,flag_count_,i,0))return 1;
    if(ids_.size()>=12&&ids_.size()<=15)
        for(int32_t i=18+int32_t(ids_.size()-12);i>=18;i--)
            if(uc_mission_flag_set(remap_,flags_,flag_count_,i,1))return 1;
    return 0;
}
int NativeCharacterAIPhase::add(int32_t id){
    if(id<0||sweeping_)return 1;
    if(std::find(ids_.begin(),ids_.end(),id)==ids_.end()){
        try{ids_.push_front(id);}catch(const std::bad_alloc&){return 1;}
    }
    cursor_=ids_.begin();return population();
}
int NativeCharacterAIPhase::remove(int32_t id){
    if(id<0||sweeping_)return 1;
    auto i=std::find(ids_.begin(),ids_.end(),id);if(i!=ids_.end())ids_.erase(i);
    cursor_=ids_.begin();return population();
}
int NativeCharacterAIPhase::tick(const Services&s){
    if(sweeping_||!s.update)return 1;if(ids_.empty())return 0;
    struct Scope{bool &busy;Scope(bool&b):busy(b){busy=true;}~Scope(){busy=false;}}scope(sweeping_);
    if(cursor_==ids_.end())cursor_=ids_.begin();const int32_t start=*cursor_;
    unsigned expensive=0;bool wrapped=false;
    auto advance=[&](auto&i){if(++i==ids_.end())i=ids_.begin();};
    while(!wrapped&&expensive<2){
        uint8_t found=0;uint32_t flags=0;if(s.update(s.owner,*cursor_,true,&found,&flags)||found>1)return 2;
        if(found&&!(flags&1))++expensive;
        advance(cursor_);wrapped=*cursor_==start;
    }
    if(!wrapped){auto i=cursor_;do{uint8_t found=0;uint32_t flags=0;
        if(s.update(s.owner,*i,false,&found,&flags)||found>1)return 2;advance(i);
    }while(*i!=start);}
    return 0;
}
