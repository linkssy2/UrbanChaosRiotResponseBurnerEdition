#include "feedback_manager.h"
#include <algorithm>
#include <new>
#include <vector>
struct UCFeedbackManager {
    UCShotFeedback queue;
    void *world;
    uint32_t depth=0;
    UCFeedbackOutput output[4];
    std::vector<const UCFeedbackPositionObserver*> position;
    std::vector<const UCFeedbackPlayerObserver*> player;
    UCFeedbackObservers view{};
};
int uc_feedback_manager_create(void*w,UCFeedbackMemory memory,UCFeedbackManager**out){
    if(!out||!memory.allocate||!memory.release)return 1;
    auto*p=new(std::nothrow)UCFeedbackManager;if(!p)return 2;
    if(uc_shot_feedback_init(&p->queue,memory)!=UC_FEEDBACK_OK){delete p;return 2;}
    p->world=w;for(unsigned i=0;i<4;i++)p->output[i].amplitude=p->queue.output+i;
    *out=p;return 0;
}
UCShotFeedback*uc_feedback_manager_queue(UCFeedbackManager*p){return p?&p->queue:nullptr;}
void*uc_feedback_manager_world(const UCFeedbackManager*p){return p?p->world:nullptr;}
UCFeedbackOutput*uc_feedback_manager_output(UCFeedbackManager*p,uint32_t i){return p&&i<4?p->output+i:nullptr;}
template<class T>static int member(std::vector<const T*>&v,const T*o,int add,int*out){
    auto less=[](const T*a,const T*b){return uintptr_t(a)<uintptr_t(b);};
    auto it=std::lower_bound(v.begin(),v.end(),o,less);bool found=it!=v.end()&&*it==o;
    if(add&&!found){if(v.size()==65536)return 1;try{v.insert(it,o);}catch(...){return 2;}}
    if(!add&&found)v.erase(it);*out=add?!found:found;return 0;
}
int uc_feedback_manager_position(UCFeedbackManager*p,const UCFeedbackPositionObserver*o,int add,int*out){
    if(!p||!o||!out||!o->kind||!o->sample)return 1;return member(p->position,o,add,out);
}
int uc_feedback_manager_player(UCFeedbackManager*p,const UCFeedbackPlayerObserver*o,int add,int*out){
    if(!p||!o||!out||!o->kind||!o->player||!o->sample)return 1;return member(p->player,o,add,out);
}
const UCFeedbackObservers*uc_feedback_manager_observers(UCFeedbackManager*p){
    if(!p)return nullptr;p->view={p->position.data(),p->player.data(),uint32_t(p->position.size()),uint32_t(p->player.size())};return &p->view;
}
int uc_feedback_manager_enable(UCFeedbackManager*p,uint8_t enable){
    if(!p)return 1;uint32_t value=p->depth+(enable?UINT32_MAX:1u);p->depth=value&0x80000000u?0:value;return 0;
}
int uc_feedback_manager_reset(UCFeedbackManager*p){if(!p)return 1;p->depth=0;return 0;}
uint32_t uc_feedback_manager_depth(const UCFeedbackManager*p){return p?p->depth:0;}
int uc_feedback_manager_close(UCFeedbackManager*p,const UCFeedbackManagerClose*h){
    if(!p||!h||!h->preflight||!h->unregister_output||!h->release_output||!h->detach_position||!h->detach_player||h->preflight(h->owner))return 1;
    for(unsigned i=0;i<4;i++)h->unregister_output(h->owner,i,p->output+i);
    for(unsigned i=0;i<4;i++)h->release_output(h->owner,i,p->output+i);
    for(auto*o:p->position)h->detach_position(h->owner,o);p->position.clear();
    for(auto*o:p->player)h->detach_player(h->owner,o);p->player.clear();
    if(uc_shot_feedback_clear(&p->queue)!=UC_FEEDBACK_OK)return 2;
    delete p;return 0;
}
