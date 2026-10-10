#include "character_script_tasks.h"
#include <cmath>
#include <stdexcept>
void NativeCharacterScriptTasks::clear(){
    for(auto at=tasks_.begin();at!=tasks_.end();){if(&*at==executing_){at->state=3;++at;}else at=tasks_.erase(at);}
}
void NativeCharacterScriptTasks::request_script(int32_t index,bool persistent){clear();tasks_.push_front({true,1,index,persistent,0});}
void NativeCharacterScriptTasks::wait(float seconds){
    if(!std::isfinite(seconds))throw std::runtime_error("Character script wait duration");tasks_.push_front({false,1,-1,false,seconds});
}
bool NativeCharacterScriptTasks::busy()const{for(const auto&t:tasks_)if(t.run)return true;return false;}
int NativeCharacterScriptTasks::update(uint32_t flags,bool expensive,float delta,const Services&h){
    if(executing_||!std::isfinite(delta)||delta<0||!h.foreground_run||!h.idle)return 1;
    if(tasks_.empty())return 0;
    auto at=tasks_.begin();
    if((flags&1)&&!at->run){ //343E0, with executing14 already resetNULL
        for(auto i=tasks_.begin();i!=tasks_.end();)if(!i->run||!i->persistent)i=tasks_.erase(i);else ++i;return 0;
    }
    executing_=&*at;
    struct Reset {Task *&p;~Reset(){p=nullptr;}}reset{executing_};
    int32_t result;
    if(at->run){
        if(expensive){int32_t next=at->state;if(h.foreground_run(h.owner,at->state,at->index,&next))return 2;at->state=next;}
        result=at->state;
    }else{
        if(h.idle(h.owner))return 2;
        //Source compares the still-live x87 subtraction before float32 rounding.
        double remaining=double(at->remaining)-delta;at->remaining=float(remaining);
        if(remaining<0){at->state=3;result=3;}else result=2;
    }
    if(result==3){executing_=nullptr;if(flags&1)clear();else tasks_.erase(at);}
    return 0;
}
