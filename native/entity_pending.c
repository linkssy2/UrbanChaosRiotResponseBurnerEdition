#include "entity_pending.h"
int uc_entity_pending_drain(UCEntityList*p,void(*destroy)(void*,void*),void*owner){
    if(!p||!destroy)return 1;
    const UCEntityListNode*n=uc_entity_list_view(p).head;
    while(n){
        void*target=n->target;n=n->next;
        uc_entity_list_remove_first(p,target);
        if(target)destroy(owner,target);
    }
    return 0;
}
