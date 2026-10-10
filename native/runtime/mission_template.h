#pragma once
#include "mission_world.h"
extern "C" {
#include "../throwable_template.h"
#include "../molotov_template_property.h"
}
/* Actual source F8FA0/F8E50 Molotov template allocation, with the native
 * allocation owned by the containing factory. No actor/aux/coroutine exists.
 * Enclosing owner stays alive until world.close has detached it; close_detached
 * releases its actual subscription/name resources, then caller deletes owner. */
class NativeMolotovTemplate {
    UCEntityEventTarget identity_{};
    uint32_t flags_=0,secondary_=0;
    int32_t parent_=-1,id_=-1,source_=-1;
    float transform_[16]{};
    const void *definition_=nullptr;
    void *model_=nullptr,*helper_=nullptr;
    UCEntityFreshView view_{{&identity_,&flags_,&parent_},transform_,&source_,&id_,
        &secondary_,&definition_,&model_,&helper_,nullptr,nullptr};
    UCThrowableTemplate object_{};
    static int idle(void*p){return uc_throwable_template_idle(&((NativeMolotovTemplate*)p)->object_);}
    static int close(void*p){return uc_throwable_template_close_detached(&((NativeMolotovTemplate*)p)->object_);}
public:
    NativeMolotovTemplate()=default;
    NativeMolotovTemplate(const NativeMolotovTemplate&)=delete;
    NativeMolotovTemplate&operator=(const NativeMolotovTemplate&)=delete;
    /* Required event receiver is source F8210, or an explicit failing frontier.
     * It must never return successful completion for an unimplemented throw. */
    int construct(NativeMissionWorld &,uint32_t source,void *receive_owner,
        int (*receive)(void *,const void *,uint32_t *));
    //1D3840 ->20A230/207CD0: unplaced dynamic template, parent is the root
    //behavior's live registry ID. The factory retains this object until close.
    int construct_dynamic(NativeMissionWorld &,int32_t parent,void *receive_owner,
        int (*receive)(void *,const void *,uint32_t *));
    const UCEntityFreshView *view()const{return &view_;}
    int property(const char *name,UCMolotovTemplateProperty &out)const{return uc_molotov_template_property(name,&out);}
};
