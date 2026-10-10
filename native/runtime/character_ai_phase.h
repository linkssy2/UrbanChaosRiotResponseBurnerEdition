#pragma once
#include <cstddef>
#include <cstdint>
#include <list>
extern "C" {
#include "../mission_flags.h"
}
/* Actual2FC30/2F4E0 registration order and2F270 controller sweep. This is
 * the controller phase only;2F5A0 perception/coordination owners are external.
 * Registration/removal during a sweep is explicitly unsupported. */
class NativeCharacterAIPhase {
    std::list<int32_t> ids_;
    std::list<int32_t>::iterator cursor_=ids_.end();
    UCLevelBytes remap_{};uint8_t *flags_;size_t flag_count_;bool sweeping_=false;
    int population();
public:
    struct Services {
        void *owner=nullptr;
        //Resolve the actual actor/controller, run35ED0, then read flags4.
        int (*update)(void *,int32_t,bool,uint8_t *found,uint32_t *flags_after)=nullptr;
    };
    NativeCharacterAIPhase(UCLevelBytes remap,uint8_t *flags,size_t count):remap_(remap),flags_(flags),flag_count_(count){}
    NativeCharacterAIPhase(const NativeCharacterAIPhase&)=delete;
    NativeCharacterAIPhase&operator=(const NativeCharacterAIPhase&)=delete;
    int add(int32_t actor);
    int remove(int32_t actor);
    int tick(const Services &);
    size_t size()const{return ids_.size();}
    //2F570 reads the same newest-first collection used by the controller sweep.
    int at(size_t index,int32_t &out)const;
    int32_t cursor()const{return cursor_==ids_.end()?-1:*cursor_;}
};
