#pragma once
#include <cstdint>
#include <list>

//Controller+10 task list, distinct from the behavior's actual script runner.
//32890/32A70 prepend Run/Wait tasks; clear34450 preserves an executing node,
//marking state3. Services may prepend/clear tasks synchronously, but not update
//recursively or destroy this owner. No fixed per-frame task drain is invented.
class NativeCharacterScriptTasks {
    struct Task {bool run;int32_t state=1,index=-1;bool persistent=false;float remaining=0;};
    std::list<Task> tasks_;Task *executing_=nullptr;
public:
    struct Services {
        void *owner=nullptr;
        //36210→329C0: actual CURRENT foreground; missing foreground yields3.
        //Return0 after writing source32710 state; nonzero is committed failure.
        int (*foreground_run)(void *,int32_t state,int32_t index,int32_t *next)=nullptr;
        //3CD50: real body idle at current body position, mode0/near0/callbackNULL.
        int (*idle)(void *)=nullptr;
    };
    NativeCharacterScriptTasks()=default;
    NativeCharacterScriptTasks(const NativeCharacterScriptTasks&)=delete;
    NativeCharacterScriptTasks&operator=(const NativeCharacterScriptTasks&)=delete;
    void clear();
    void request_script(int32_t index,bool persistent);
    void wait(float seconds);
    bool empty()const{return tasks_.empty();}
    bool busy()const; //343B0 any Run, even behind a Wait
    //35F68..3600C. flags1 Wait pruning343E0 retains only persistent Runs;
    //flags0 removes the ORIGINAL front node on result3, even after prepending.
    //0 completed,1 invalid before effects,2 service failed after effects.
    int update(uint32_t flags,bool expensive,float delta,const Services &);
};
